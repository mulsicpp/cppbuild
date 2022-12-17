#include "math/my_math.h"
#include "math/random_useless.h"

auto lol() {
    struct A { int x; int y; };
    return A{1, 2};
}

int main(void) {
    add(1, 2);
    auto[x, y] = lol();
    return 0;
}