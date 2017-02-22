#include "common.h"
#include "frul.h"
#include <stdio.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

int clientsock, serversock;

ssize_t client_output(const void *buffer, size_t n, void *userdata)
{
  return write(clientsock, buffer, n);
}

ssize_t server_output(const void *buffer, size_t n, void *userdata)
{
  return write(serversock, buffer, n);
}

int main()
{
  int r;
  clientsock = socket(AF_INET, SOCK_DGRAM, 0);
  if (clientsock == -1) {
    perror("socket");
    return -1;
  }
  serversock = socket(AF_INET, SOCK_DGRAM, 0);
  if (serversock == -1) {
    perror("socket");
    return -1;
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8888);
  r = inet_pton(AF_INET, "127.0.0.2", &server_addr.sin_addr);
  if (r == -1) {
    perror("inet_pton");
    return -1;
  }
  r = bind(serversock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
  if (r == -1) {
    perror("bind");
    return -1;
  }
  r = connect(clientsock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
  if (r == -1) {
    perror("connect");
    return -1;
  }
  fsess *client = fsess_new();
  client->output = client_output;
  fsess *server = fsess_new();
  server->output = server_output;

  fsess_send(client, "abcdefg", 8);

  char low_buffer[1500];
  ssize_t received = recvfrom(serversock, low_buffer, 1500, 0, NULL, 0);
  printf("server: low level - %zd bytes data received\n", received);
  fsess_input(server, low_buffer, received);

  char buffer[1500];
  received = fsess_recv(server, buffer, 1500);
  printf("server: %zd bytes received, data=%s\n",received, buffer);

  printf("hello\n");
  return 0;
}