//
// Created by yuxzhu on 2/22/17.
//

#ifndef FRUL_PROTO_H
#define FRUL_PROTO_H
#include "utils.h"
#include <stdint.h>

#define FRUL_PROTO_VERSION (char)(0xDD)

struct frul_hdr {
  char ver;
#if  __LITTLE_ENDIAN
  char f_opt: 1,
      f_ack: 1,
      f_conn: 1,
      f_shut: 1,
      f_comp: 1,
      f_init: 1,
      f_res: 2;
#elif __BIG_ENDIAN
  char f_res: 2,
       f_init: 1,
       f_comp: 1,
       f_shut: 1,
       f_conn: 1,
       f_ack: 1,
       f_opt: 1;
#else
#error "Could not determine byte order."
#endif
  uint16_t len;
  uint32_t seq;
  uint32_t ack_seq;
  uint32_t window;
  uint32_t ts;
  uint32_t ts_echo;
  char data[0];
};

#define FRUL_HDR_LEN (sizeof(struct frul_hdr))

#endif //FRUL_PROTO_H
