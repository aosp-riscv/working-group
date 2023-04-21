#include <stdio.h>

__attribute__((constructor(101))) void fcb101() { puts("b:ctor:101"); }
__attribute__((constructor(102))) void fcb102() { puts("b:ctor:102"); }
__attribute__((constructor)) void fcb() { puts("b:ctor:65535"); }

__attribute__((destructor(101))) void fdb101() { puts("b:dtor:101"); }
__attribute__((destructor(102))) void fdb102() { puts("b:dtor:102"); }
__attribute__((destructor)) void fdb() { puts("b:dtor:65535"); }
