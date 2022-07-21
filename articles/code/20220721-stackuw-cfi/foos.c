extern void unwind_by_backtrace() ;

void foo_3() {
    unwind_by_backtrace();
}

void foo_2() {
    foo_3();
}

void foo_1() {
    foo_2();
}

void foo_0() {
    foo_1();
}
