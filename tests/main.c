#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>

#include "common.h"
#include "debug.h"
#include "netsim.h"

static void print_dev_stat(struct netdev *dev, const char *devname) {
  eprintf("dev %s\n", devname);
  eprintf("\twrite_mem=%zu/%zu\n", dev->write_mem, dev->write_maxmem);
  eprintf("\twrite_queue_size=%zu\n", dev->write_queue_size);
  eprintf("\tread_mem=%zu/%zu\n", dev->read_mem, dev->read_maxmem);
  eprintf("\tread_queue_size=%zu\n", dev->read_queue_size);
  eprintf("\ttx packets=%zu\n", dev->tx_all);
  eprintf("\ttx errors=%zu\n", dev->tx_dropped);
  eprintf("\trx packets=%zu\n", dev->rx_all);
  eprintf("\trx errors=%zu\n", dev->rx_dropped);
}

static void *netsim_worker_proc(void *para) {
  struct netsim *sim = para;
  eprintf("sim = %p\n", sim);
  int r;
  for (;;) {
    long ts = netsim_timestamp();
    eprintf("ts=%ld\n", ts);
    netsim_clock_update(sim, ts);
    r = netsim_sleep(100);
  }
  return NULL;
}

static void *netsim_write_proc(void *para) {
  struct netsim *sim = para;
  struct netdev *local = sim->peers, *remote = sim->peers + 1;
  size_t bytes_to_write = 100 * 1000 * 1000; // 100MB
  size_t buffer_len = 1500;
  char buffer[buffer_len];
  memset(buffer, 3, buffer_len);
  ssize_t r;
  while (bytes_to_write > 0) {
    size_t actual_write = min(buffer_len, bytes_to_write);
    r = netsim_dev_write(local, buffer, actual_write);
    if (r) {
      eprintf("netsim_dev_write error\n");
      break;
    }
    bytes_to_write -= actual_write;
    netsim_sleep(0);
    //netsim_clock_update(sim, netsim_timestamp());
  }
  print_dev_stat(local, "local");
  print_dev_stat(remote, "remote");
}

static void *netsim_read_proc(void *para) {
  size_t bytes_read = 0;
  struct netsim *sim = para;
  struct netdev *local = sim->peers, *remote = sim->peers + 1;
  int r;
  size_t buffer_size = 1500;
  char buffer[buffer_size];
  for (;;) {
    netsim_dev_read(sim->peers + 1, buffer, buffer_size);
    print_dev_stat(local, "local");
    print_dev_stat(remote, "remote");
  }
  return NULL;
}

int main() {
  struct netsim *sim = netsim_create(1000*1000*1000, 200, 0);
  if (!sim) {
    eprintf("netsim_create error \n");
    return 1;
  }
  struct netdev *local = sim->peers, *remote = sim->peers + 1;

  pthread_t netsim_worker, netsim_writer, netsim_reader;
  pthread_create(&netsim_worker, NULL, netsim_worker_proc, sim);
  pthread_create(&netsim_reader, NULL, netsim_read_proc, sim);
  pthread_create(&netsim_writer, NULL, netsim_write_proc, sim);


  pthread_join(netsim_writer, NULL);


  //pthread_join(netsim_worker, NULL);
  //pthread_join(netsim_reader, NULL);

  return 0;
}
