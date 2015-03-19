// Copyright 2015 Johan Sköld. All rights reserved.
// License: http://www.opensource.org/licenses/BSD-2-Clause

#include <metassert.h>

void main() {
    auto a = rand();
    auto b = rand();
    METASSERT(a == b);
}