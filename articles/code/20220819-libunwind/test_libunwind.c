extern void unwind_by_libunwind();

void foo_3() {
    unwind_by_libunwind();
}

void foo_2() {
    foo_3();
}

void foo_1() {
    foo_2();
}

void foo() {
    foo_1();
}

int main()
{
    foo();
    return 0;
}
