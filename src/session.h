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
  size_t mss; // not including the furl header
  long base_timestamp;

  /* callbacks */
  void *userdata;
  ssize_t (*output)(const void *buffer, size_t n, void *userdata);
  void (*recv_ready)(size_t n);

  /* sender */
  size_t write_buffer_used;
  size_t write_buffer_limit;
  uint32_t send_next;
  uint32_t send_una;
  /* Size of sender's flow window, recorded from remote peer's advertised window.
   * Sender's flow windows is range [send_una, send_una + wnd).
   * */
  size_t wnd;
  size_t cwnd; /* congestion window */
  struct list_head write_queue;
  struct list_head *send_head;

  /* receiver */
  size_t read_buffer_used;
  size_t read_buffer_limit;
  size_t rwnd;
  struct list_head read_queue;
  struct list_head *recv_head;
  uint32_t recv_user;
  uint32_t recv_next;
  uint32_t recv_last;

  //size_t unacked;
  long ack_timestamp;
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
