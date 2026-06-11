#include <stdint.h>

// =====================================================
// GPIO MMIO
// =====================================================
#define GPIO_BASE      0x10000000

#define GPIO_LEDR      (*(volatile uint32_t*)(GPIO_BASE + 0x00))
#define GPIO_HEX       (*(volatile uint32_t*)(GPIO_BASE + 0x04))
#define GPIO_SW        (*(volatile uint32_t*)(GPIO_BASE + 0x0C))

// =====================================================
// UART MMIO
// =====================================================
#define UART_BASE      0x20000000

#define UART_TX_DATA   (*(volatile uint32_t*)(UART_BASE + 0x00))
#define UART_RX_DATA   (*(volatile uint32_t*)(UART_BASE + 0x04))
#define UART_STATUS    (*(volatile uint32_t*)(UART_BASE + 0x08))

#define TX_FULL        (1 << 0)
#define RX_EMPTY       (1 << 1)
#define TX_READY       (1 << 2)
#define RX_IRQ         (1 << 3)

// =====================================================
// VGA MMIO
// =====================================================
#define VGA_BASE       0x30000000

#define VGA_CTRL       (*(volatile uint32_t*)(VGA_BASE + 0x4000))
#define VGA_STATUS     (*(volatile uint32_t*)(VGA_BASE + 0x4004))

#define VGA_COLS       80
#define VGA_ROWS       30
#define VGA_SIZE       (VGA_COLS * VGA_ROWS)

#define VGA_TEXT(i)    (*(volatile uint32_t*)(VGA_BASE + ((i) * 4)))

#define VGA_CELL(ch, color) \
    ((((uint32_t)(color)) << 8) | ((uint32_t)(ch) & 0xFF))

// =====================================================
// COLOR
// =====================================================
#define COLOR_WHITE     0x0F
#define COLOR_YELLOW    0x0E
#define COLOR_GREEN     0x0A
#define COLOR_CYAN      0x0B
#define COLOR_RED       0x0C
#define COLOR_MAGENTA   0x0D

// =====================================================
// UART FUNCTIONS
// =====================================================
static inline void uart_send(char c)
{
    while (!(UART_STATUS & TX_READY));
    UART_TX_DATA = (uint32_t)c;
}

void uart_print(const char *s)
{
    while (*s != 0)
    {
        uart_send(*s);
        s++;
    }
}

void uart_print_hex32(uint32_t v)
{
    const char hex[] = "0123456789ABCDEF";

    uart_print("0x");

    for (int i = 7; i >= 0; i--)
    {
        uint32_t nibble = (v >> (i * 4)) & 0xF;
        uart_send(hex[nibble]);
    }
}

// =====================================================
// VGA FUNCTIONS
// =====================================================
void vga_putc(uint32_t x, uint32_t y, char c, uint8_t color)
{
    uint32_t index;

    if (x >= VGA_COLS || y >= VGA_ROWS)
        return;

    index = y * VGA_COLS + x;
    VGA_TEXT(index) = VGA_CELL(c, color);
}

void vga_print(uint32_t x, uint32_t y, const char *s, uint8_t color)
{
    while (*s != 0)
    {
        if (x >= VGA_COLS)
            return;

        vga_putc(x, y, *s, color);
        x++;
        s++;
    }
}

void vga_clear_line(uint32_t y)
{
    for (uint32_t x = 0; x < VGA_COLS; x++)
    {
        vga_putc(x, y, ' ', COLOR_WHITE);
    }
}

void vga_clear_screen(void)
{
    for (uint32_t i = 0; i < VGA_SIZE; i++)
    {
        VGA_TEXT(i) = VGA_CELL(' ', COLOR_WHITE);
    }
}

void vga_print_buffer(uint32_t x, uint32_t y, volatile char *buf, uint32_t len, uint8_t color)
{
    for (uint32_t i = 0; i < len; i++)
    {
        if (x >= VGA_COLS)
            break;

        vga_putc(x, y, buf[i], color);
        x++;
    }
}

void vga_print_hex32(uint32_t x, uint32_t y, uint32_t v, uint8_t color)
{
    const char hex[] = "0123456789ABCDEF";

    vga_putc(x++, y, '0', color);
    vga_putc(x++, y, 'x', color);

    for (int i = 7; i >= 0; i--)
    {
        uint32_t nibble = (v >> (i * 4)) & 0xF;
        vga_putc(x++, y, hex[nibble], color);
    }
}

// =====================================================
// GPIO FUNCTIONS
// =====================================================
void gpio_init_debug(void)
{
    GPIO_LEDR = 0x00000155;
    GPIO_HEX  = 0x00123456;
}

void gpio_update_debug(uint32_t enter_count)
{
    uint32_t sw;

    sw = GPIO_SW & 0x3FF;

    // LEDR realtime mirrors SW[9:0]
    GPIO_LEDR = sw;

    // HEX shows enter_count, low 24-bit
    GPIO_HEX = enter_count & 0x00FFFFFF;
}

// =====================================================
// MAIN
// =====================================================
int main(void)
{
    volatile char rx_buf[64];

    uint32_t idx = 0;
    uint32_t enter_count = 0;
    uint32_t sw_value = 0;

    for (uint32_t i = 0; i < 64; i++)
    {
        rx_buf[i] = 0;
    }

    // Enable VGA
    VGA_CTRL = 0x00000001;

    // GPIO boot test
    gpio_init_debug();

    vga_clear_screen();

    uart_print("RV32IM UART VGA GPIO TEST READY!!\n");
    uart_print("TYPE TEXT THEN PRESS ENTER\n");
    uart_print("GPIO TEST:\n");
    uart_print("- LEDR mirrors SW[9:0]\n");
    uart_print("- HEX shows ENTER count\n\n");

    vga_print(0, 0, "RV32IM UART -> VGA + GPIO TEST", COLOR_YELLOW);
    vga_print(0, 2, "RX FROM PC:", COLOR_CYAN);
    vga_print(0, 5, "CPU ECHO:", COLOR_GREEN);
    vga_print(0, 8, "GPIO SW:", COLOR_MAGENTA);
    vga_print(0, 9, "ENTER COUNT:", COLOR_MAGENTA);

    while (1)
    {
        // =================================================
        // GPIO realtime debug
        // =================================================
        gpio_update_debug(enter_count);

        sw_value = GPIO_SW & 0x3FF;

        vga_clear_line(8);
        vga_print(0, 8, "GPIO SW:", COLOR_MAGENTA);
        vga_print_hex32(10, 8, sw_value, COLOR_WHITE);

        vga_clear_line(9);
        vga_print(0, 9, "ENTER COUNT:", COLOR_MAGENTA);
        vga_print_hex32(13, 9, enter_count, COLOR_WHITE);

        // =================================================
        // UART receive
        // =================================================
        if (!(UART_STATUS & RX_EMPTY))
        {
            uint32_t data = UART_RX_DATA;
            char c = (char)(data & 0xFF);

            // =============================================
            // ENTER: end line
            // =============================================
            if (c == '\n' || c == '\r')
            {
                uart_send('\n');

                rx_buf[idx] = 0;

                enter_count++;

                uart_print("CPU ECHO: ");
                uart_print((const char *)rx_buf);
                uart_print(" | SW=");
                uart_print_hex32(sw_value);
                uart_print(" | ENTER=");
                uart_print_hex32(enter_count);
                uart_send('\n');
                uart_send('\n');

                vga_clear_line(3);
                vga_clear_line(6);

                vga_print_buffer(0, 3, rx_buf, idx, COLOR_WHITE);

                vga_print(0, 6, "CPU ECHO: ", COLOR_GREEN);
                vga_print_buffer(10, 6, rx_buf, idx, COLOR_WHITE);

                idx = 0;

                for (uint32_t i = 0; i < 64; i++)
                {
                    rx_buf[i] = 0;
                }
            }

            // =============================================
            // BACKSPACE
            // =============================================
            else if (c == 8 || c == 127)
            {
                if (idx > 0)
                {
                    idx--;

                    rx_buf[idx] = 0;

                    uart_send('\b');
                    uart_send(' ');
                    uart_send('\b');

                    vga_clear_line(3);
                    vga_print_buffer(0, 3, rx_buf, idx, COLOR_WHITE);
                }
            }

            // =============================================
            // NORMAL CHARACTER
            // =============================================
            else
            {
                if (idx < 63)
                {
                    rx_buf[idx] = c;
                    idx++;

                    uart_send(c);

                    vga_clear_line(3);
                    vga_print_buffer(0, 3, rx_buf, idx, COLOR_WHITE);
                }
                else
                {
                    uart_print("\nERROR: BUFFER FULL\n");

                    vga_clear_line(3);
                    vga_print(0, 3, "ERROR: BUFFER FULL", COLOR_RED);

                    idx = 0;

                    for (uint32_t i = 0; i < 64; i++)
                    {
                        rx_buf[i] = 0;
                    }
                }
            }
        }
    }

    return 0;
}