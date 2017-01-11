#include <debug.h>
#include <frul.h>

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <time.h>
#include <arpa/inet.h>

int frul_init(struct frulcb *frul) {
  memset(frul, 0, sizeof(struct frulcb));
  INIT_LIST_HEAD(&frul->write_queue);
  frul->state = FRUL_CLOSED;
  frul->mss = FRUL_DEFAULT_MSS;
  frul->read_buffer_limit = FRUL_MAX_RECV_BUFFER;
  frul->write_buffer_limit = FRUL_MAX_SEND_BUFFER;
  return 0;
}

int frul_listen(struct frulcb *frul, bool listen) {
  if (frul->state != FRUL_CLOSED && frul->state != FRUL_LISTEN) {
    frul->errno = FRUL_E_STATE;
    return -1;
  }
  frul->state = listen ? FRUL_LISTEN : FRUL_CLOSED;
  frul->errno = FRUL_E_SUCCESS;
  return 0;
}

static int frul_put_write_queue(struct frulcb *frul, struct frul_buf *buf) {
  size_t write_buffer_used = frul->write_buffer_used + buf->seg_len;
  if (write_buffer_used > frul->write_buffer_limit) {
    frul->errno = FRUL_E_WOULDBLOCK;
    return -1;
  }
  list_add_tail(&buf->list, &frul->write_queue);
  return 0;
}

static inline struct frul_hdr *frul_seg_hdr(struct frul_buf *buf) {
  return (struct frul_hdr *) ((char *) buf + offsetof(struct frul_buf, seg));
}

static inline struct frul_buf *frul_buf_new(size_t data_len) {
  size_t seg_size = sizeof(struct frul_hdr) + data_len;
  size_t buf_size = sizeof(struct frul_buf) + seg_size;
  struct frul_buf *buf = calloc(1, buf_size);
  if (buf)
    buf->seg_len = seg_size;
  return buf;
}

static inline void frul_buf_free(struct frul_buf *buf) {
  free(buf);
}

static long frul_timestamp() {
  struct timespec ts = {};
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    return 0;
  return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

int frul_flush(struct frulcb *frul) {
  ssize_t bytes_sent;
  struct list_head *p, *q;
  list_for_each_safe(p, q, &frul->write_queue) {
    struct frul_buf *buf = list_entry(p, struct frul_buf, list);
    struct frul_hdr *hdr = frul_seg_hdr(buf);
    hdr->ts = htons((uint16_t)frul_timestamp());
    hdr->seq = htonl(0x12345678);
    hdr->window = htonl((uint32_t)(frul->read_buffer_limit - frul->read_buffer_used));
    bytes_sent = frul->output(buf->seg, buf->seg_len, frul->userdata);
    if (bytes_sent < 0) {
      frul->errno = FRUL_E_IO;
      return -1;
    } else if (bytes_sent != buf->seg_len) {
      frul->errno = FRUL_E_WOULDFRAG;
      return -1;
    }
    list_del(&buf->list);
    frul_buf_free(buf);
  }
  return 0;
}

int frul_connect(struct frulcb *frul) {
  if (frul->state != FRUL_CLOSED) {
    frul->errno = FRUL_E_STATE;
    return -1;
  }
  int retval = 0;
  struct frul_buf *buf = frul_buf_new(0);
  if (!buf) {
    frul->errno = FRUL_E_NOMEM;
    retval = -1;
    goto cleanup;
  }
  struct frul_hdr *hdr = frul_seg_hdr(buf);
  hdr->ver = FRUL_PROTO_VERSION;
  hdr->f_init = 1;
  if (frul_put_write_queue(frul, buf) == -1) {
    retval = -1;
    goto cleanup;
  }
  if (frul_flush(frul) == -1) {
    retval = -1;
    goto cleanup;
  }
  frul->state = FRUL_INIT_SENT;
  cleanup:
  if (retval) { // on error
    frul_buf_free(buf);
  }
  return retval;
}

int frul_send(struct frulcb *frul, const char *buffer, size_t n) {
  if (frul->state == FRUL_CLOSED) {
  }
  return -1;
}

int frul_recv(struct frulcb *frul, char *buffer, size_t n) {
  return -1;
}

int frul_close(struct frulcb *frul) {

}
