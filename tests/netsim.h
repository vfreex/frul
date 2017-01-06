#ifndef _FRUL_NETSIM_H
#define _FRUL_NETSIM_H

#include <stdint.h>
#include <list.h>

struct netdev {
    ssize_t (*write)(struct netdev *dev, const char *buffer, size_t len);
    ssize_t (*read)(struct netdev *dev, char *buffer, size_t len);
    int write_maxmem;
    int write_mem;
    int write_queue_size;
    struct list_head write_queue;
    int read_maxmem;
    int read_mem;
    int read_queue_size;
    struct list_head read_queue;
    struct list_head on_the_fly;
    int rx_all, rx_dropped;
    int tx_all, tx_dropped;
    int mtu;
    int bandwidth;
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


struct netsim *netsim_create(int bandwidth, int latency, int loss);

#endif
