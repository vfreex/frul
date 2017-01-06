/*
 * simple network simulator with loss and latency
 * Copyright (C) 2016 Yuxiang Zhu <vfreex+frul@gmail.com>
 * */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <time.h>

#include "debug.h"
#include "netsim.h"

static void netsim_dev_init(struct netdev *dev)
{
    assert(dev != NULL);
    dev->read_queue_capacity = 1000;
    dev->write_queue_capacity = 1000;
    dev->mtu = 1500;
    INIT_LIST_HEAD(&dev->write_queue);
    INIT_LIST_HEAD(&dev->read_queue);
    INIT_LIST_HEAD(&dev->on_the_fly);
}

struct netsim *netsim_create(int bandwidth, int latency, int loss)
{
    assert(bandwidth > 0);
    assert(latency >= 0 && latency < 10000);
    assert(loss >= 0 && loss <= 100);

    struct netsim *p = malloc(sizeof(struct netsim));
    if (p) {
        memset(p, 0, sizeof(struct netsim));
        netsim_dev_init(p->peers);
        netsim_dev_init(p->peers + 1);
        p->pmtu = 1500;
        p->bandwidth = bandwidth;
        p->latency = latency;
        p->loss = loss;
    }
    return p;
}

static long netsim_timestamp()
{
    struct timespec ts = {};
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return 0;
    return ts.tv_sec * 1000L + ts.tv_nsec / 1e6;
}

ssize_t netsim_dev_write(struct netdev *dev, const char *buffer, size_t len)
{
    if (len > dev->mtu) {
        eprintf("packet is too large (current is %zu, maximum is %d)\n", len, dev->mtu);
        return -1;
    }
    if (dev->write_queue_size >= dev->write_queue_capacity) {
        eprintf("write_queue is full\n");
        return -1;
    }
    struct netpkt *pkt = malloc(sizeof(struct netpkt) + len);
    if (!pkt) {
        eprintf("malloc error.\n");
        return -1;
    }
    pkt->length = len;
    memcpy(pkt->data, buffer, len);
    list_add_tail(&pkt->list, &dev->write_queue);
    dev->write_queue_size++;
    return 0;
}

int netsim_random(int min, int max)
{
    return rand() % (max - min + 1) + min;
}

static void netsim_transmit(struct netsim *sim, struct netdev *from, struct netdev *to, long cur)
{
    struct netpkt *pkt, *temp;
    int *all = sim->stat_all;
    int *loss = sim->stat_dropped;
    list_for_each_entry_safe(pkt, temp, &from->write_queue, list) {
        if (pkt->timestamp + sim->latency < cur) {
            return;
        }
        list_del(&pkt->list);
        ++from->tx_all;
        ++*all;
        if (netsim_random(0, 99) < sim->loss) {
            // packet is loss
            free(pkt);
            ++*loss;
            continue;
        }
        to->rx_all++;
        if (to->read_queue_size >= to->read_queue_capacity) {
            // insufficient receive buffer
            free(pkt);
            to->rx_dropped++;
            continue;
        }
        list_add_tail(&pkt->list, &to->read_queue);
    }   
}

int netsim_clock_update(struct netsim *sim)
{
    long cur = netsim_timestamp();
    int pkts_per_sec = sim->bandwidth / sim->pmtu / 8;
    // peer0 -> peer1
    netsim_transmit(sim, sim->peers, sim->peers + 1, cur);
    // peer1 -> peer0
    netsim_transmit(sim, sim->peers + 1, sim->peers, cur);
}

