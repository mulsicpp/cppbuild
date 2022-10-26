#include "stdio.h"
#include "math/my_math.h"

int main(void) {
    printf("%i + %i = %i", 1, 4, add(1, 4));

    int a = 0;

    {
        int b = 3;
    }

    int c;

    printf("%i\n", c);

    return 0;
}