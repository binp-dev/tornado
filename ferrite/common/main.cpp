#include <iostream>
#include <cassert>

#include "_out.hpp"

int main() {
    assert(sizeof(IppVectorUint24) == 2);
    std::cout << "Test passed" << std::endl;
    return 0;
}
