#include "session.h"
#include "debug.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netinet/in.h>

static long frul_timestamp() {
  struct timespec ts = {};
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    return 0;
  return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

fsess *fsess_new() {
  fsess *session = malloc(sizeof(fsess));
  if (session)
    fsess_init(session);
  return session;
}

void fsess_free(fsess *session) {
  free(session);
}

int fsess_init(fsess *session) {
  assert(session);
  memset(session, 0, sizeof(fsess));
  session->state = FRUL_ESTABLISHED;
  session->mss = FRUL_DEFAULT_MSS;
  session->read_buffer_limit = FRUL_MAX_RECV_BUFFER;
  session->write_buffer_limit = FRUL_MAX_SEND_BUFFER;
  INIT_LIST_HEAD(&session->write_queue);
  INIT_LIST_HEAD(&session->read_queue);
  session->base_timestamp = frul_timestamp();
  session->send_next = 1;
  session->recv_next = 1;
  session->cwnd = 2 * session->mss;
  session->rwnd = session->read_buffer_limit;
  session->wnd = 2 * session->mss;
  session->errno = FRUL_E_SUCCESS;
  return 0;
}

static inline struct frul_hdr *frul_seg_hdr(struct frul_buf *buf) {
  return (struct frul_hdr *) ((char *) buf + offsetof(struct frul_buf, seg));
}

static frul_buf *frul_buf_new(size_t data_len) {
  if (data_len > UINT16_MAX) {
    return NULL;
  }
  size_t seg_size = FRUL_HDR_LEN + data_len;
  size_t buf_size = sizeof(struct frul_buf) + seg_size;
  struct frul_buf *buf = malloc(buf_size);
  if (buf) {
    memset(buf, 0 ,sizeof(buf_size));
    buf->seg_len = seg_size;
    struct frul_hdr *hdr = frul_seg_hdr(buf);
    hdr->ver = (char)FRUL_PROTO_VERSION;
    hdr->len = htons((uint16_t)data_len);
  }
  return buf;
}

static inline void frul_buf_free(struct frul_buf *buf) {
  free(buf);
}

struct frul_buf *fsess_mkbuf(fsess *session, const void *buffer, size_t n)
{
  assert(session);
  assert(n == 0 || buffer);
  if (n > session->mss) {
    session->errno = FRUL_E_WOULDFRAG;
    return NULL;
  }
  if (n + session->write_buffer_used > session->write_buffer_limit) {
    session->errno = FRUL_E_WOULDBLOCK;
    return NULL;
  }
  struct frul_buf *buf = frul_buf_new(n);
  if (!buf) {
    session->errno = FRUL_E_NOMEM;
    return NULL;
  }
  struct frul_hdr *hdr = frul_seg_hdr(buf);
  hdr->len = htons(n);
  if (n > 0)
    memcpy(hdr->data, buffer, n);
  return buf;
}

static ssize_t fsess_call_output(fsess *session, struct frul_hdr *seg) {
  assert(session);
  assert(session->output);
  assert(seg);
  if (seg->f_ack) {
    if (!seg->ack_seq)
      seg->ack_seq = htonl(session->recv_next);
    // update ts_echo?
  }
  seg->window = htonl((uint32_t)(session->recv_user + session->read_buffer_limit - session->recv_next));
  long now = (uint32_t)frul_timestamp();
  seg->ts = htonl((uint32_t)(now - session->base_timestamp)); // #FIXME: timestamp may wrap around
  D("fsess_call_output: seq=%u\n", htonl(seg->seq));
  return session->output(seg, ntohs(seg->len) + FRUL_HDR_LEN, session->userdata);
}

static ssize_t fsess_transmit_frulbuf(fsess *session, struct frul_buf *buf)
{
  ssize_t sent = 0, sent_per_time;
  do {
    sent_per_time = fsess_call_output(session, frul_seg_hdr(buf));
    sent += sent_per_time;
  } while (sent_per_time >= 0 && sent < buf->seg_len);
  if (sent_per_time < 0) {
        D("fsess_call_output failed, returned %zd\n", sent_per_time);
    return -1;
  }
  if (sent != buf->seg_len) {
        D("fsess_call_output failed, sent != seg_len (%zd, %zd)\n", sent, buf->seg_len);
    return -1;
  }
  return sent;
}

ssize_t fsess_ack(fsess *session, uint32_t ts_echo)
{
  ssize_t retval = -1;
  struct frul_buf *buf = fsess_mkbuf(session, NULL, 0);
  if (!buf)
    goto cleanup;
  struct frul_hdr *hdr = frul_seg_hdr(buf);
  hdr->f_ack = 1;
  hdr->seq = htonl(session->send_next);
  hdr->ts_echo = htonl(ts_echo);
  //hdr->ack_seq = htonl(session->recv_next);
  fsess_transmit_frulbuf(session, buf);
  cleanup:
  if (buf) {
    frul_buf_free(buf);
  }
  return retval;
}

ssize_t fsess_transmit(fsess *session)
{
  assert(session);
  if (session->wnd == 0) {
    D("receiver has no receive buffer, should be probed later\n");
    return 0;
  }
  size_t max_unacked = min(session->cwnd, session->wnd);
  size_t unacked = session->send_next - session->send_una;
  if (unacked >= max_unacked) {
    D("flow control applied: unacked >= max_unacked (%u/%u).\n", unacked, max_unacked);
    return 0;
  }
  ssize_t sent_total = 0, sent;
  while (session->send_head && session->send_head != &session->write_queue) {
    struct frul_buf *buf = list_entry(session->send_head, struct frul_buf, list);
    if (buf->seg_len > max_unacked) {
      D("buf->seg_len >= max_unacked: %u/%u\n", buf->seg_len, max_unacked);
      break;
    }
    sent = fsess_transmit_frulbuf(session, buf);
    if (sent < 0) {
      D("fsess_transmit_frulbuf failed\n");
      return -1;
    }
    sent_total += sent;
    session->send_head = buf->list.next;
  }
  return sent_total;
}

ssize_t fsess_send(fsess *session, const void *buffer, size_t n) {
  assert(session);
  assert(buffer);
  D("fsess_send called, size=%zu\n", n);
  if (session->state != FRUL_ESTABLISHED) {
    session->errno = FRUL_E_STATE;
    return -1;
  }
  if (n == 0)
    return 0; // do nothing
  int retval = 0;
  struct frul_buf *buf = fsess_mkbuf(session, buffer, n);
  if (!buf) {
    retval = -1;
    goto cleanup;
  }
  struct frul_hdr *hdr = frul_seg_hdr(buf);
  hdr->f_ack = 1;
  hdr->seq = htonl(session->send_next);
  list_add_tail(&buf->list, &session->write_queue);
  session->write_buffer_used += buf->seg_len;
  session->send_next += buf->seg_len;
  session->send_head = &buf->list;
  fsess_transmit(session);
  return buf->seg_len;
  cleanup:
  if (retval == -1) { // on error
    frul_buf_free(buf);
  }
  return retval;
}

struct frul_buf *frul_seg_parse(const char *buffer, size_t n) {
  struct frul_buf *dest;
  assert(buffer);
  if (n < FRUL_HDR_LEN) {
    D("frul_seg_parse error: segment length is shorter than header\n");
    return NULL;
  }
  // FIXME: unaligned access risk
  const struct frul_hdr *src = (struct frul_hdr *) buffer;

  if (src->ver != (char)FRUL_PROTO_VERSION) {
    // unsuported version
    D("frul_seg_parse error: segment version is unsupported (received: %u, expected: %u)\n", (char)src->ver, FRUL_PROTO_VERSION);
    return NULL;
  }
  uint16_t claimed_data_len = ntohs(src->len);
  if (claimed_data_len + FRUL_HDR_LEN < n) {
    // length mismatches
    D("frul_seg_parse error: segment is shorter than it claims (received: %u, expected: %u)\n", n, claimed_data_len + FRUL_HDR_LEN );
    return NULL;
  }
  dest = frul_buf_new(claimed_data_len);
  if (dest)
    memcpy(dest->seg, src, FRUL_HDR_LEN + claimed_data_len);
  else
        D("frul_buf_new error\n");
  return dest;
}

static void fsess_recycle_write_queue(fsess *session)
{
  assert(session);
  struct list_head *p, *q;
  list_for_each_safe(p, q, &session->write_queue) {
    frul_buf *buf = list_entry(p, frul_buf, list);
    struct frul_hdr *seg = frul_seg_hdr(buf);
    if (ntohl(seg->seq) + ntohs(seg->len) < session->send_una) {
      list_del(&buf->list);
      frul_buf_free(buf);
    }
  }
}


ssize_t fsess_input(fsess *session, const void *buffer, size_t n) {
  // parse
  assert(session);
  assert(buffer);
  bool queued = false;
  D("fsess_input: fsess_input called, size=%zu\n", n);
  struct frul_buf *buf = frul_seg_parse(buffer, n); // FIXME: memory leak
  if (!buf) {
    D("fsess_input: frul_seg_parse error\n");
    return -1;
  }
  ssize_t retval = -1;
  if (buf->seg_len + session->read_buffer_used > session->read_buffer_limit) {
    D("fsess_input: no enough recv buffer\n");
    goto cleanup;
  }

  struct frul_hdr *seg = frul_seg_hdr(buf);
  uint32_t seq = ntohl(seg->seq);
  uint32_t ack_seq = ntohl(seg->ack_seq);

  uint32_t recv_wnd_upper_bound = session->recv_user + (uint32_t)session->read_buffer_limit;
  if (seq >= recv_wnd_upper_bound) {
    // seq exceeds receive window
        D("seq exceeds recv wnd: %u\n", seq);
    goto cleanup;
  }
  if (seq < session->recv_next) {
    // received duplicated seq
        D("duplicated seq: %u\n", seq);
    goto cleanup;
  }
  if (seq > session->recv_next) {
    // reordered segment received
        D("reordered seq: %u\n", seq);
    goto cleanup;
  }
  if (seq > session->recv_last) {
    session->recv_last = seq;
  }

  // seq == session->recv_next

  if (ack_seq < session->send_una) {
    // duplicated ack
        D("duplicated ack: %u\n", ack_seq);
    // drop
    goto cleanup;
  }

  if (ack_seq > session->send_next) {
    // ack is bigger than send_next
        D("ack is bigger than send_next: %u / %u\n", ack_seq, session->send_next);
    // drop
    goto cleanup;
  }

  // sent segments acked
  session->send_una = ack_seq;
  // recycle acked sent segs

  // queue
  if (buf->seg_len > FRUL_HDR_LEN) {
    list_add_tail(&buf->list, &session->read_queue);
    session->read_buffer_used += buf->seg_len;
    session->recv_next += buf->seg_len;
    queued = true;
    // check if reordered segments and this segment become a sequence
    // advance recv_head
    session->recv_head = &buf->list;
    // send ack
    fsess_ack(session, ntohl(seg->ts_echo));
    fsess_recycle_write_queue(session);
  }

  // ack-clocking
  fsess_transmit(session);
  return buf->seg_len;
  cleanup:
  if (buf && !queued) {
    frul_buf_free(buf);
  }
  return retval;
}

ssize_t fsess_recv(fsess *session, void *buffer, size_t n) {
  assert(session);
  assert(buffer);
  D("fsess_recv called, size=%zu\n", n);
  if (session->recv_head && session->recv_head != &session->read_queue) {
    struct frul_buf *buf = list_entry(session->recv_head, struct frul_buf, list);
    struct frul_hdr *hdr = frul_seg_hdr(buf);
    size_t payload_len = ntohs(hdr->len);
    if (n < payload_len) {
      D("fsess_recv error: buffer is too short\n");
      return -1;
    }
    struct list_head *next = buf->list.next;
    list_del(&buf->list);
    session->recv_head = next;
    if (hdr->f_opt) {
      D("fsess_recv: OPT flag is set, but it is unsupported by current implementation\n");
    }
    memcpy(buffer, hdr->data, payload_len);
    session->recv_user = (uint32_t)(htonl(hdr->seq) + payload_len);
    frul_buf_free(buf);
    return payload_len;
  }
  D("fsess_recv: no data");
  return 0;
}

