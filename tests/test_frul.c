//
// Created by yuxzhu on 1/11/17.
//

#include "test_frul.h"
#include "debug.h"
#include "frul.h"

#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct userdata {
  int clientsock, serversock;
  struct sockaddr_in from;
  struct sockaddr_in to;
};

ssize_t test_output(const void *buffer, size_t n, void *userdata) {
  struct userdata *data = userdata;
  ssize_t r = write(data->clientsock, buffer, n);
  return r;
}

static int test_frul_connect() {
  struct userdata mydata;
  memset(&mydata, 0, sizeof(struct userdata));
  mydata.from.sin_family = AF_INET;
  mydata.from.sin_port = htons(8000);
  mydata.to.sin_family = AF_INET;
  mydata.to.sin_port = htons(8001);
  inet_pton(AF_INET, "127.0.0.1", &mydata.from.sin_addr);
  inet_pton(AF_INET, "127.0.0.2", &mydata.to.sin_addr);

  int r;
  mydata.serversock = socket(AF_INET, SOCK_DGRAM, 0);
  if (mydata.serversock == -1) {
    eprintf("socket() failed.\n");
    return -1;
  }


  r = listen(mydata.serversock, 100);
  if (r == -1) {
    perror("listen() failed");
    return -1;
  }

  mydata.clientsock = socket(AF_INET, SOCK_DGRAM, 0);
  if (mydata.clientsock == -1) {
    eprintf("socket() failed.\n");
    return -1;
  }

  r = bind(mydata.clientsock, (struct sockaddr *) &mydata.from, sizeof(mydata.from));
  if (r == -1) {
    eprintf("bind error.\n");
    return -1;
  }
  r = connect(mydata.clientsock, (struct sockaddr *) &mydata.to, sizeof(mydata.to));
  if (r == -1) {
    eprintf("connect error.\n");
    return -1;
  }

  struct frulcb frul;
  if (frul_init(&frul) == -1) {
    eprintf("frul_init failed");
    return -1;
  }
  frul.userdata = &mydata;
  frul.output = test_output;

  frul_connect(&frul);

  frul_close(&frul);

}

int main() {
  test_frul_connect();
}