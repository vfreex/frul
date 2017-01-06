#include <stdio.h>

#include "debug.h"
#include "netsim.h"

int main()
{
    struct netsim *sim = netsim_create(1000000000, 200, 10);
    if (!sim) {
        eprintf("netsim_create error \n");
        return 1;
    }
    puts("Hello frul!");
    return 0;
}
