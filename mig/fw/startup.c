/* Minimal replacement for Xilinx's standalone BSP startup support, matching
 * exactly what crt0.o expects (checked via objdump -dr): _crtinit runs after
 * stack/SDA setup and is expected to init .bss then call main(), returning
 * its result; exit() and the exception/interrupt vector targets are never
 * actually invoked in this design (interrupts stay disabled, no faults
 * expected), so they're simple infinite loops. */

extern char __bss_start[];
extern char __bss_end[];

extern int main(void);

int _crtinit(void) {
    char *p = __bss_start;
    while (p < __bss_end) {
        *p++ = 0;
    }
    return main();
}

void exit(int status) {
    (void)status;
    while (1) {
    }
}

void _exception_handler(void) {
    while (1) {
    }
}

void _interrupt_handler(void) {
    while (1) {
    }
}

void _hw_exception_handler(void) {
    while (1) {
    }
}
