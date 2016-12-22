#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <debug.h>
#include <frul.h>


int frul_init(struct frulcb *frul)
{
    memset(frul, 0, sizeof(struct frulcb));
    INIT_LIST_HEAD(&frul->write_queue);
    frul->state = FRUL_CLOSED;
    frul->mss = FRUL_DEFAULT_MSS;
    frul->read_buffer_limit = FRUL_MAX_RECV_BUFFER;
    frul->write_buffer_limit = FRUL_MAX_SEND_BUFFER;
    return 0;
}

int frul_listen(struct frulcb *frul, bool listen)
{
    if (frul->state != FRUL_CLOSED && frul->state != FRUL_LISTEN) {
        frul->errno = FRUL_E_STATE;
        return -1;
    }
    frul->state = listen ? FRUL_LISTEN : FRUL_CLOSED;
    frul->errno = FRUL_E_SUCCESS;
    return 0;
}

static int frul_put_write_queue(struct frulcb *frul, struct frul_buf *buf)
{
    size_t write_buffer_used = frul->write_buffer_used + buf->data_len;
    if (write_buffer_used > frul->write_buffer_limit) {
        frul->errno = FRUL_E_WOULDBLOCK;
        return -1;
    }
    list_add_tail(&buf->list, &frul->write_queue);
    return 0;
}

static inline struct frul_hdr *frul_seg_hdr(struct frul_buf *buf)
{
    return (struct frul_hdr *)((char *)buf + offsetof(struct frul_buf, data));
}

static inline struct frul_buf *frul_buf_new(size_t data_len)
{
    size_t seg_size = sizeof(struct frul_buf) + sizeof(struct frul_hdr) + data_len;
    struct frul_buf *buf = calloc(seg_size, 1);
    if (buf)
        buf->data_len = data_len;
    return buf;
}

int frul_flush(struct frulcb *frul)
{
    struct frul_buf *buf, *tempbuf;
    ssize_t bytes_sent;
    list_for_each_entry_safe(buf, tempbuf, &buf->list, list) {
        bytes_sent = frul->output(buf->data, buf->data_len, frul->userdata);
        if (bytes_sent < 0) {
            frul->errno = FRUL_E_IO;
            return -1;
        }
        else if (bytes_sent != buf->data_len) {
            frul->errno = FRUL_E_WOULDFRAG;
            return -1;
        }
        list_del(&buf->list);
    }
    return 0;
}

int frul_connect(struct frulcb *frul)
{
    if (frul->state != FRUL_CLOSED) {
        frul->errno = FRUL_E_STATE;
        return -1;
    }
    struct frul_buf *buf = frul_buf_new(0);
    if (!buf) {
        frul->errno = FRUL_E_NOMEM;
        return -1;
    }
    struct frul_hdr *hdr = frul_seg_hdr(buf);
    hdr->ver = FRUL_PROTO_VERSION;
    hdr->f_init = 1;
    if (frul_put_write_queue(frul, buf) == -1) {
        return -1;
    }
    if (frul_flush(frul) == -1) {
        return -1;
    }
    frul->state = FRUL_INIT_SENT;
    return 0;
    
}

int frul_send(struct frulcb *frul, const char *buffer, size_t n)
{
    if (frul->state == FRUL_CLOSED) {
    }
    return -1;
}

int frul_recv(struct frulcb *frul, char *buffer, size_t n)
{
    return -1;
}

