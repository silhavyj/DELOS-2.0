#include <system.h>

extern "C" void _cpp_startup();
extern "C" void _cpp_shutdown();

extern int main();

extern "C" unsigned int __bss_start;
extern "C" unsigned int __bss_end;

void __crt0_init_bss() {
    unsigned int* begin = (unsigned int*)__bss_start;
    for (; begin < (unsigned int*)__bss_end; begin++)
        *begin = 0;
}

extern "C" void __crt0_run() {
    __crt0_init_bss();
    _cpp_startup();

    int exit_code = main();

    _cpp_shutdown();
    exit(exit_code);
}