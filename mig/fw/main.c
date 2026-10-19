/* K7-160T-DP MicroBlaze + MIG DDR3 bring-up test.
 * No Xilinx BSP: raw register access to axi_uartlite (PG142) and axi_gpio
 * (PG144), matching the address map from the Block Design (verify_mig.tcl /
 * add_gpio.tcl output). */

#define UART_BASE 0x40600000u
#define UART_TX_FIFO   (*(volatile unsigned int *)(UART_BASE + 0x0))
#define UART_STAT_REG  (*(volatile unsigned int *)(UART_BASE + 0x8))
#define UART_STAT_TX_FULL (1u << 3)

#define GPIO_BASE 0x40000000u
#define GPIO_DATA (*(volatile unsigned int *)(GPIO_BASE + 0x0))
#define GPIO_TRI  (*(volatile unsigned int *)(GPIO_BASE + 0x4))

/* Heartbeat LED (C21/D5, confirmed-good pin from earlier LED bring-up),
 * firmware-driven so we can prove CPU execution independent of both
 * UART and MIG calibration state. */
#define LED_BASE 0x40010000u
#define LED_DATA (*(volatile unsigned int *)(LED_BASE + 0x0))
#define LED_TRI  (*(volatile unsigned int *)(LED_BASE + 0x4))

#define DDR3_BASE 0x80000000u

static void uart_putc(char c) {
    while (UART_STAT_REG & UART_STAT_TX_FULL) {
    }
    UART_TX_FIFO = (unsigned int)(unsigned char)c;
}

static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

static void uart_put_hex32(unsigned int v) {
    static const char hex[] = "0123456789ABCDEF";
    char buf[9];
    int i;
    for (i = 7; i >= 0; i--) {
        buf[7 - i] = hex[(v >> (i * 4)) & 0xF];
    }
    buf[8] = 0;
    uart_puts(buf);
}

static void uart_put_dec(unsigned int v) {
    char buf[11];
    int i = 10;
    buf[10] = 0;
    if (v == 0) {
        uart_putc('0');
        return;
    }
    while (v > 0 && i > 0) {
        buf[--i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    uart_puts(&buf[i]);
}

static void delay(volatile unsigned int n) {
    while (n--) {
    }
}

int main(void) {
    /* Heartbeat: blink a few times before touching UART at all, so we can
     * confirm CPU execution reached main() even if UART turns out broken. */
    LED_TRI = 0x0u; /* output direction */
    for (int i = 0; i < 6; i++) {
        LED_DATA = (i & 1) ? 0x1u : 0x0u;
        delay(3000000);
    }

    uart_puts("\n=== K7-160T-DP MicroBlaze + MIG DDR3 test ===\n");

    GPIO_TRI = 0xFFFFFFFFu; /* input direction, in case IP default differs */

    uart_puts("Waiting for init_calib_complete");
    unsigned int waited = 0;
    while ((GPIO_DATA & 0x1u) == 0) {
        uart_putc('.');
        delay(2000000);
        waited++;
        if (waited > 60) {
            uart_puts("\nTIMEOUT: init_calib_complete never asserted.\n");
            uart_puts("GPIO_DATA = 0x");
            uart_put_hex32(GPIO_DATA);
            uart_puts("\n");
            while (1) {
            }
        }
    }
    uart_puts("\ninit_calib_complete = 1 -- MIG calibration OK!\n");

    volatile unsigned int *ddr = (volatile unsigned int *)DDR3_BASE;
    const unsigned int words = 4096; /* 16KB test region */
    unsigned int errors = 0;
    unsigned int first_err_addr = 0, first_err_exp = 0, first_err_got = 0;

    uart_puts("Writing test pattern (16KB)...\n");
    for (unsigned int i = 0; i < words; i++) {
        ddr[i] = 0xA5A50000u ^ (i * 0x1000193u);
    }

    uart_puts("Reading back...\n");
    for (unsigned int i = 0; i < words; i++) {
        unsigned int expected = 0xA5A50000u ^ (i * 0x1000193u);
        unsigned int actual = ddr[i];
        if (actual != expected) {
            if (errors == 0) {
                first_err_addr = DDR3_BASE + i * 4u;
                first_err_exp = expected;
                first_err_got = actual;
            }
            errors++;
        }
    }

    uart_puts("Memtest done. errors=");
    uart_put_dec(errors);
    uart_puts(" / ");
    uart_put_dec(words);
    uart_puts(" words\n");

    if (errors != 0) {
        uart_puts("first mismatch @0x");
        uart_put_hex32(first_err_addr);
        uart_puts(" expected 0x");
        uart_put_hex32(first_err_exp);
        uart_puts(" got 0x");
        uart_put_hex32(first_err_got);
        uart_puts("\n");
        uart_puts("MEMTEST FAIL\n");
    } else {
        uart_puts("MEMTEST PASS\n");
    }

    while (1) {
    }
    return 0;
}
