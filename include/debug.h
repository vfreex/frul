#ifndef _FRUL_DEBUG_H
#define _FRUL_DEBUG_H

#define eprintf(...) \
    do { fprintf(stderr, __VA_ARGS__); } while(0)

#define debug_print(...) \
    do { if (DEBUG) eprintf(__VA_ARGS__); } while(0)

#define D debug_print


#endif
