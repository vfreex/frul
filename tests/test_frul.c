//
// Created by yuxzhu on 1/11/17.
//

#include "test_frul.h"
#include "debug.h"
#include "frul.h"

#include <string.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct userdata {
  int clientsock, serversock;
  struct sockaddr_in from;
  struct sockaddr_in to;
  struct frulcb frul_client, frul_server;
};

ssize_t test_output(const void *buffer, size_t n, void *userdata) {
  struct userdata *data = userdata;
  ssize_t r = write(data->clientsock, buffer, n);
  return r;
}

ssize_t server_output(const void *buffer, size_t n, void *userdata) {
  struct userdata *data = userdata;
  ssize_t r = sendto(data->serversock, buffer, n, 0, (struct sockaddr *) &data->from, sizeof(struct sockaddr_in));
  return r;
}

static void *server_proc(void *userdata) {
  struct userdata *data = userdata;
  struct frul *frul = &data->frul_server;
  frul_listen(frul, true);
  char buffer[1500];
  struct sockaddr_in sender_addr;
  socklen_t sender_addr_len = sizeof(sender_addr);
  ssize_t
      bytes_received = recvfrom(data->serversock, buffer, 1500, 0, (struct sockaddr *) &sender_addr, &sender_addr_len);
  if (bytes_received == -1) {
    perror("recvfrom");
    return -1;
  }
  char sender_addr_str[100];

  eprintf("received packet from %s\n", inet_ntop(AF_INET, &sender_addr, sender_addr_str, sizeof(sender_addr_str)));
  int r = frul_input(frul, buffer, (size_t) bytes_received, &sender_addr, sender_addr_len);
  if (r == -1) {
    eprintf("frul_input error\n");
    return -1;
  }

  return 0;
}

static int test_frul_connect() {
  struct userdata mydata;
  memset(&mydata, 0, sizeof(struct userdata));
  mydata.from.sin_family = AF_INET;
  mydata.from.sin_port = htons(18000);
  mydata.to.sin_family = AF_INET;
  mydata.to.sin_port = htons(18001);
  inet_pton(AF_INET, "127.0.0.1", &mydata.from.sin_addr);
  inet_pton(AF_INET, "127.0.0.2", &mydata.to.sin_addr);

  int r;
  mydata.serversock = socket(AF_INET, SOCK_DGRAM, 0);
  if (mydata.serversock == -1) {
    eprintf("socket() failed.\n");
    return -1;
  }

  r = bind(mydata.serversock, (struct sockaddr *) &mydata.to, sizeof(mydata.to));
  if (r == -1) {
    perror("bind error");
    return -1;
  }

  frul_init(&mydata.frul_server);
  mydata.frul_server.output = server_output;
  mydata.frul_server.userdata = &mydata;

  pthread_t server_thread;
  pthread_create(&server_thread, NULL, server_proc, &mydata);

  sleep(1);

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

  pthread_join(server_thread, 0);

}

int main() {
  test_frul_connect();
}