#include <iostream>

class foo {
public:
    foo();
    ~foo();
};

foo::foo() { puts("a:ctor:65535[class]"); }
foo::~foo()  { puts("a:dtor:65535[class]"); }

foo f;

__attribute__((constructor(101))) void fca101() { puts("a:ctor:101"); }
__attribute__((constructor(102))) void fca102() { puts("a:ctor:102"); }
__attribute__((constructor)) void fca() { puts("a:ctor:65535"); }

__attribute__((destructor(101))) void fda101() { puts("a:dtor:101"); }
__attribute__((destructor(102))) void fda102() { puts("a:dtor:102"); }
__attribute__((destructor)) void fda() { puts("a:dtor:65535"); }

int main(void) { puts("main"); return 0; }