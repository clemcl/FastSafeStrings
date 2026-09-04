#include <iostream>
#include "fast_string.hpp"

int main()
{
    fast_string<256> name;
    fast_string<256> surname("Smith");

    name = "John";
    name += " ";
    name += surname;

    std::cout << name.c_str() << "\n";
    std::cout << "Length: " << name.length() << "\n";

    return 0;
}
