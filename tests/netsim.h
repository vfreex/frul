#ifndef _FRUL_NETSIM_H
#define _FRUL_NETSIM_H

#include <stdint.h>
#include "list.h"
#include "lock.h"

struct netdev {
  ssize_t (*write)(struct netdev *dev, const char *buffer, size_t len);
  ssize_t (*read)(struct netdev *dev, char *buffer, size_t len);
  size_t write_maxmem;
  size_t write_mem;
  size_t write_queue_size;
  struct list_head write_queue;
  spinlock_t write_queue_lock;
  size_t read_maxmem;
  size_t read_mem;
  size_t read_queue_size;
  struct list_head read_queue;
  spinlock_t read_queue_lock;
  struct list_head on_the_fly;
  spinlock_t on_the_fly_queue_lock;
  size_t rx_all, rx_dropped;
  size_t tx_all, tx_dropped;
  size_t mtu;
  long bandwidth;
  int latency;
  int loss;
  int tokens;
  long refill_timestamp;
};

struct netpkt {
  struct list_head list;
  long timestamp;
  size_t length;
  char data[0];
};

struct netsim {
  struct netdev peers[2];
};

long netsim_timestamp();
int netsim_random(int min, int max);
int netsim_sleep(long ms);

struct netsim *netsim_create(long bandwidth, int latency, int loss);
ssize_t netsim_dev_write(struct netdev *dev, const char *buffer, size_t len);
ssize_t netsim_dev_read(struct netdev *dev, char *buffer, size_t len);
long netsim_clock_update(struct netsim *sim, long current_timestamp);


#endif
