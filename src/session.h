#ifndef _FRUL_SESSION_H
#define _FRUL_SESSION_H

#include "constant.h"
#include "proto.h"
#include "list.h"

#include <stdbool.h>

enum frul_state {
  FRUL_CLOSED,
  FRUL_LISTEN,
  FRUL_INIT_SENT,
  FRUL_CONN_SENT,
  FRUL_ESTABLISHED,
  FRUL_SHUT_SENT,
  FRUL_LAST_ACK,
  FRUL_TIME_WAIT
};

typedef struct fsess {
  enum frul_errno errno;
  enum frul_state state;
  size_t mss;
  size_t cwnd; /* congestion window */
  size_t rwnd;
  size_t awnd; /* receiver’s advertised window */
  void *userdata;
  ssize_t (*output)(const void *buffer, size_t n, void *userdata);
  void (*recv_ready)(size_t n);

  size_t write_buffer_used;
  size_t write_buffer_limit;
  size_t read_buffer_used;
  size_t read_buffer_limit;
  struct list_head write_queue;
  struct list_head *send_head;
  size_t unacked;
  struct list_head read_queue;
  struct list_head *recv_head;

  long base_timestamp;
  long ack_timestamp;
  uint32_t send_next;
  uint32_t recv_next;
} fsess;

typedef struct frul_buf {
  struct list_head list;
  size_t seg_len;
  char seg[0];
} frul_buf;


int fsess_init(fsess *session);
fsess * fsess_new();
void fsess_free(fsess *session);
ssize_t fsess_input(fsess *session, const void *buffer, size_t n);
ssize_t fsess_send(fsess *session, const void *buffer, size_t n);
ssize_t fsess_recv(fsess *session, void *buffer, size_t n);

#endif //_FRUL_SESSION_H
