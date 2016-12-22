#ifndef _FRUL_H
#define _FRUL_H

#include <stdint.h>
#include <stdbool.h>
#include "endian.h"
#include "list.h"

typedef uint16_t __be16;
typedef uint32_t __be32;
typedef uint64_t __be64;

struct frul_hdr {
    char  ver;
    char reserved;
#if  __LITTLE_ENDIAN
    __be16 f_res1: 8,
           f_init: 1,
           f_ack: 1,
           f_conn: 1,
           f_shut: 1,
           f_comp: 1,
           f_opt: 1,
           f_res2: 2;
#elif __BIG_ENDIAN
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

enum frul_state {
    FRUL_CLOSED,
    FRUL_LISTEN,
    FRUL_INIT_SENT,
    FRUL_INIT_ACKED,
    FRUL_CONN_SENT,
    FRUL_CONN_RECV,
    FRUL_ESTABLISHED,
    FRUL_SHUT_SENT,
    FRUL_SHUT_RECV,
    FRUL_COMP_WAIT,
    FRUL_COMPLETING,
    FRUL_TIME_WAIT
};

enum frul_errno {
    FRUL_E_SUCCESS,
    FRUL_E_UNKNOWN,
    FRUL_E_STATE,
    FRUL_E_NOMEM,
    FRUL_E_RANGE,
    FRUL_E_UNSUPP,
    FRUL_E_NOTIMPL,
    FRUL_E_WOULDBLOCK,
    FRUL_E_IO,
    FRUL_E_WOULDFRAG
};

struct frulcb {
    enum frul_state state;
    enum frul_errno errno;
    size_t send_next;
    size_t recv_next;
    size_t cwnd;
    size_t rwnd;
    size_t send_capacity;
    size_t recv_capacity;
    size_t mss;
    void *userdata;
    ssize_t (*output)(const void *buffer, size_t n, void *userdata);

    size_t write_buffer_used;
    size_t write_buffer_limit;
    size_t read_buffer_used;
    size_t read_buffer_limit;
    struct list_head write_queue;
    struct list_head *send_head;
};

struct frul_buf {
    struct list_head list;
    size_t data_len;
    char data[];
};

#define RRUL_PROTO_VERSION 1
#define FRUL_DEFAULT_MSS 1400
#define FRUL_MAX_RECV_BUFFER 100000
#define FRUL_MAX_SEND_BUFFER 100000
#define FRUL_INITIAL_CWND 2

int frul_input(struct frulcb *frul, char *buffer, size_t n);

int frul_init(struct frulcb *frul);

int frul_listen(struct frulcb *frul, bool listen);

int frul_send(struct frulcb *frul, const char *buffer, size_t n);

int frul_recv(struct frulcb *frul, char *buffer, size_t n);

int frul_flush(struct frulcb *frul);

int frul_close(struct frulcb *frul);

#endif
