/*
 * simple network simulator with loss and latency
 * Copyright (C) 2016 Yuxiang Zhu <vfreex+frul@gmail.com>
 * */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <time.h>

#include "common.h"
#include "list.h"
#include "debug.h"
#include "netsim.h"

#define FRUL_DEFAULT_MTU 1500
#define FRUL_DEFAULT_MAX_WMEM 10000 * 1500
#define FRUL_DEFAULT_MAX_RMEM 0


static void netsim_dev_init(struct netdev *dev, size_t wmem, size_t rmem, size_t mtu,
                            long bandwidth, int latency, int loss) {
  assert(dev != NULL);
  dev->write_maxmem = wmem;
  dev->read_maxmem = rmem;
  dev->mtu = 1500;
  dev->bandwidth = bandwidth;
  dev->latency = latency;
  dev->loss = loss;
  INIT_LIST_HEAD(&dev->write_queue);
  INIT_LIST_HEAD(&dev->read_queue);
  INIT_LIST_HEAD(&dev->on_the_fly);
}

struct netsim *netsim_create(long bandwidth, int latency, int loss) {
  assert(bandwidth > 0);
  assert(latency >= 0 && latency < 10000);
  assert(loss >= 0 && loss <= 100);

  struct netsim *p = malloc(sizeof(struct netsim));
  if (p) {
    memset(p, 0, sizeof(struct netsim));
    netsim_dev_init(p->peers, FRUL_DEFAULT_MAX_WMEM, FRUL_DEFAULT_MAX_RMEM, FRUL_DEFAULT_MTU, 50 * 1000000L, 200, 5);
    netsim_dev_init(p->peers + 1, FRUL_DEFAULT_MAX_WMEM, FRUL_DEFAULT_MAX_RMEM, FRUL_DEFAULT_MTU, 50 * 1000000L, 200, 5);
  }
  return p;
}

long netsim_timestamp() {
  struct timespec ts = {};
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    return 0;
  return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

ssize_t netsim_dev_write(struct netdev *dev, const char *buffer, size_t len) {
  if (len > dev->mtu) {
    eprintf("packet is too large (current is %zu, maximum is %zu)\n", len, dev->mtu);
    return -1;
  }
  spinlock_lock(&dev->write_queue_lock);
  size_t new_writemem = dev->write_mem + len;
  if (dev->write_maxmem && new_writemem > dev->write_maxmem) {
    spinlock_unlock(&dev->write_queue_lock);
    eprintf("maximum write_mem exceeded\n");
    return -1;
  }
  struct netpkt *pkt = malloc(sizeof(struct netpkt) + len);
  if (!pkt) {
    spinlock_unlock(&dev->write_queue_lock);
    eprintf("malloc error\n");
    return -1;
  }
  pkt->length = len;
  memcpy(pkt->data, buffer, len);
  list_add_tail(&pkt->list, &dev->write_queue);
  dev->write_mem = new_writemem;
  dev->write_queue_size++;
  spinlock_unlock(&dev->write_queue_lock);
  return 0;
}

int netsim_random(int min, int max) {
  return rand() % (max - min + 1) + min;
}

#define TOKEN_REFILL_INTERVAL 100 // in ms

static long netsim_transmit(struct netsim *sim, struct netdev *from, struct netdev *to, long cur) {
  // refill the token bucket
  long max_tokens = from->bandwidth / 8;
  long period_passed = cur - from->refill_timestamp / TOKEN_REFILL_INTERVAL + 1;
  if (period_passed < 1) {
    return from->refill_timestamp;
  }
  // FIXME: overflow risk
  long tokens_to_refill = max(max_tokens - from->tokens, max_tokens * TOKEN_REFILL_INTERVAL / 1000 * period_passed);
  from->tokens += tokens_to_refill;
  from->refill_timestamp = cur + TOKEN_REFILL_INTERVAL;

  struct list_head *p, *q;
  spinlock_lock(&from->write_queue_lock);
  list_for_each_safe(p, q, &from->write_queue) {
    struct netpkt *pkt = container_of(p, struct netpkt, list);

    if (from->tokens < pkt->length) {
      // no more tokens, wait for tokens to be refilled
      break;
    }

    from->tokens -= pkt->length; // reduce the token bucket
    list_del(p); // remove from write_queue
    --from->write_queue_size;
    from->write_mem -= pkt->length;
    from->tx_all += 1;

    if (netsim_random(0, 99) < from->loss) {
      // packet is lost
      free(pkt);
      continue;
    }

    pkt->timestamp = cur;

    // the packet is on the fly
    spinlock_lock(&from->on_the_fly_queue_lock);
    list_add_tail(p, &from->on_the_fly);
    spinlock_unlock(&from->on_the_fly_queue_lock);
  }
  spinlock_unlock(&from->write_queue_lock);

  spinlock_lock(&from->on_the_fly_queue_lock);
  list_for_each_safe(p, q, &from->on_the_fly) {
    struct netpkt *pkt = container_of(p, struct netpkt, list);
    if (cur < pkt->timestamp + from->latency) {
      // packet is on its way!
      break;
    }

    // move packet to read_queue of the receiver
    list_del(p);
    spinlock_lock(&to->read_queue_lock);
    to->rx_all++;
    if (to->read_maxmem && to->read_mem + pkt->length > to->read_maxmem) {
      // read_maxmem exceeded
      free(p); // packet is dropped
      to->rx_dropped++;
      continue;
    }
    to->read_mem += pkt->length;
    list_add_tail(p, &to->read_queue);
    ++to->read_queue_size;
    spinlock_unlock(&to->read_queue_lock);
  }
  spinlock_unlock(&from->on_the_fly_queue_lock);

}
long netsim_clock_update(struct netsim *sim, long current_timestamp) {
  // peer0 -> peer1
  netsim_transmit(sim, sim->peers, sim->peers + 1, current_timestamp);
  // peer1 -> peer0
  netsim_transmit(sim, sim->peers + 1, sim->peers, current_timestamp);
  return sim->peers[0].refill_timestamp;
}

int netsim_sleep(long ms) {
  struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = ms % 1000 * 1000000};
  return nanosleep(&ts, NULL);
}

ssize_t netsim_dev_read(struct netdev *dev, char *buffer, size_t len) {
  spinlock_lock(&dev->read_queue_lock);
  if (list_empty(&dev->read_queue)) {
    spinlock_unlock(&dev->read_queue_lock);
    eprintf("netsim_dev_read: read_queue is empty\n");
    return -1;
  }
  struct list_head *p = dev->read_queue.next;
  struct netpkt *pkt = container_of(p, struct netpkt, list);
  if (len < pkt->length) {
    spinlock_unlock(&dev->read_queue_lock);
    eprintf("netsim_dev_read: buffer is too small\n");
    return -1;
  }
  list_del(p);
  spinlock_unlock(&dev->read_queue_lock);
  memcpy(buffer, pkt->data, pkt->length);
  free(p);
  return pkt->length;
}
