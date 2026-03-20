#include <stdio.h>
#include "faststr.h"

int main()
{
    DCL(name, 256);
    DCL(addr, 256);

    SET(name, "John");
    SET(addr, " Smith");

    CAT(name, addr);

    printf("Name: %s\n", name);
    printf("Length: %u\n", LEN(name));

    return 0;
}
