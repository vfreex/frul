/*
 * simple network simulator with loss and latency
 * Copyright (C) 2016 Yuxiang Zhu <vfreex+frul@gmail.com>
 * */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

struct netdev {
    ssize_t (*write)(struct netdev *dev, const char *buffer, size_t len);
    ssize_t (*read)(struct netdev *dev, char *buffer, size_t len);
    int write_queue_capacity;
    int write_queue_size;
    struct list_head write_queue;
    int read_queue_capacity;
    int read_queue_size;
    struct list_head read_queue;
};

struct netsim {
    struct netdev peers[2];
    int pmtu;
    int loss;
    int bandwidth;
    int latency;
};

struct netsim *netsim_create(int bandwidth, int latency, int loss)
{
    struct netsim *p = malloc(sizeof(struct netsim));
    if (p) {
        memset(p, 0, sizeof(struct netsim));
    }
    return p;
}



