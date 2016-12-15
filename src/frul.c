#include <string.h>

#include <frul.h>

int frul_init(struct frulcb *frul)
{
    memset(frul, 0, sizeof(struct frulcb));
    return -1;
}

int frul_send(struct frulcb *frul, const char *buffer, size_t n)
{
    return -1;
}

int frul_recv(struct frulcb *frul, char *buffer, size_t n)
{
    return -1;
}

int frul_flush(struct frulcb *frul)
{
    return -1;
}
