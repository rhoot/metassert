#include "../include/metassert.h"

void main() {
    auto a = rand();
    auto b = rand();
    METASSERT(a == b);
}