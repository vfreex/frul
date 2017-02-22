#ifndef _FRUL_UTILS_H
#define _FRUL_UTILS_H

/* max, min */
#ifndef max
#define max(a, b) \
    ( (a) > (b) ? (a) : (b) )
#endif

#ifndef min
#define min(a, b) \
    ( (a) < (b) ? (a) : (b) )
#endif

/* endian */
#ifndef __LITTLE_ENDIAN
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __LITTLE_ENDIAN 1234
#endif
#endif

#ifndef __BIG_ENDIAN
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __BIG_ENDIAN 4321
#endif
#endif

#if ! defined(__BIG_ENDIAN) && ! defined(__LITTLE_ENDIAN)
#error "Could not determine byte order"
#endif


#endif //FRUL_COMMON_H
