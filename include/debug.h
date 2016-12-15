#ifndef _FRUL_DEBUG_H
#define _FRUL_DEBUG_H

#define debug_print(...) \
    do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while(0)

#define D debug_print

#endif
