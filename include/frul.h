#ifndef _FRUL_H
#define _FRUL_H

#include <stdint.h>
#include "endian.h"
#include "list.h"

typedef uint16_t __be16;
typedef uint32_t __be32;
typedef uint64_t __be64;

struct frul_hdr {
    char  ver;
    char reserved;
#if  __BIG_ENDIAN
    __be16 f_res1: 8,
           f_init: 1,
           f_ack: 1,
           f_conn: 1,
           f_shut: 1,
           f_comp: 1,
           f_opt: 1,
           f_res2: 2;
#elif __LITTLE_ENDIAN
     __be16 f_res1: 8,
           f_res2: 2,
           f_init: 1,
           f_comp: 1,
           f_shut: 1,
           f_conn: 1,
           f_ack: 1,
           f_opt: 1;
#else
#error "Could not determine byte order."
#endif
    __be32 seq;
    __be32 ack_seq;
    __be32 window;
    __be16 ts;
    __be16 ts_echo;
    __be32 options[];
};

struct frulcb {
    int (*output)(const char *buffer, size_t n, void *userdata);
    size_t send_next;
    size_t recv_next;
    size_t cwnd;
    size_t rwnd;
    size_t send_capacity;
    size_t recv_capacity;

    struct list_head write_queue;
    struct list_head *send_head;
};

struct frul_buff {
    struct list_head list;
    size_t data_len;
    char data[];
};

int frul_input(struct frulcb *frul, char *buffer, size_t n);

int frul_init(struct frulcb *frul);

int frul_send(struct frulcb *frul, const char *buffer, size_t n);

int frul_recv(struct frulcb *frul, char *buffer, size_t n);

int frul_flush(struct frulcb *frul);

int frul_close(struct frulcb *frul);

#endif
