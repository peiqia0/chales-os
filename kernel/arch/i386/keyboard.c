#include <stdint.h>

#include <kernel/portio.h>
#include <kernel/keyboard.h>

static uint8_t key_states[128] = {0};

void keyboard_initialize(void)
{
}

uint8_t keyboard_read_scancode(void)
{
    return inport8(KEYBOARD_DATA_PORT);
}

uint8_t keyboard_is_key_pressed(uint8_t scancode)
{
    uint8_t key_code = scancode & 0x7F;
    uint8_t is_release = scancode & 0x80;

    if (is_release) {
        key_states[key_code] = 0;
        return 0;
    } else {
        key_states[key_code] = 1;
        return 1;
    }
}

// Simple scancode to ASCII mapping (US keyboard layout)
static char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' ', 0
};

// Function to convert scancode to ASCII character
char keyboard_scancode_to_ascii(uint8_t scancode)
{
    if (scancode < sizeof(scancode_to_ascii)) {
        return scancode_to_ascii[scancode];
    }
    return 0;
}

// Keyboard buffer implementation
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;
static int buffer_count = 0;

void keyboard_buffer_push(char c)
{
    if (buffer_count < KEYBOARD_BUFFER_SIZE) {
        keyboard_buffer[buffer_tail] = c;
        buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
        buffer_count++;
    }
}

char keyboard_buffer_pop(void)
{
    if (buffer_count > 0) {
        char c = keyboard_buffer[buffer_head];
        buffer_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
        buffer_count--;
        return c;
    }
    return 0;
}

int keyboard_buffer_has_data(void)
{
    return buffer_count > 0;
}