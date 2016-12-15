#include <stdio.h>
#include <stdlib.h>
#include "frul.h"

int main()
{
#if __BIG_ENDIAN
    puts("BIG ENDIAN");
#elif __LITTLE_ENDIAN
    puts("LITTLE ENDIAN");
#endif

    struct frulcb frul;
    if (frul_init(&frul) == -1) {
        fprintf(stderr, "frul_init failed");
        exit(1);
    }

    return 0;
}
