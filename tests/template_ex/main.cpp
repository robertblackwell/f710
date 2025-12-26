#include "myclass.h"

int main() {
    MyClass<int> intInstance;
    intInstance.myMethod(); // Linker finds the pre-generated code

    // MyTemplate<char> charInstance; // ERROR: Linker error, char not explicitly instantiated

    return 0;
}
