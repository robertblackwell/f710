#include "myclass.h"
#include <iostream>

// Definition of the method
template <typename T>
void MyClass<T>::myMethod() {
    std::cout << "myMethod called for type T in CPP file" << std::endl;
}

// Explicit instantiations for supported types
template class MyClass<int>;
template class MyClass<double>;
