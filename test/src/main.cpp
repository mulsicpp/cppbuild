#if defined(_WIN32)
#include "Windows.h"
#endif

#include <stdio.h>
#include "math/my_math.h"
#include "math/random_useless.h"

int main(void) {
    char buffer[1024];
    printf("Please input something: ");
    fgets(buffer, 1024, stdin);
    printf("Thanks\n");
    return 0;
}