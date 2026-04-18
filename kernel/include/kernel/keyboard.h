#ifndef _KERNEL_KEYBOARD_H
#define _KERNEL_KEYBOARD_H

#include <stdint.h>

// Keyboard I/O port
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// Keyboard scancodes for common keys
#define KEY_ESC 0x01
#define KEY_1 0x02
#define KEY_2 0x03
#define KEY_3 0x04
#define KEY_4 0x05
#define KEY_5 0x06
#define KEY_6 0x07
#define KEY_7 0x08
#define KEY_8 0x09
#define KEY_9 0x0A
#define KEY_0 0x0B
#define KEY_MINUS 0x0C
#define KEY_EQUAL 0x0D
#define KEY_BACKSPACE 0x0E
#define KEY_TAB 0x0F
#define KEY_Q 0x10
#define KEY_W 0x11
#define KEY_E 0x12
#define KEY_R 0x13
#define KEY_T 0x14
#define KEY_Y 0x15
#define KEY_U 0x16
#define KEY_I 0x17
#define KEY_O 0x18
#define KEY_P 0x19
#define KEY_LEFTBRACKET 0x1A
#define KEY_RIGHTBRACKET 0x1B
#define KEY_ENTER 0x1C
#define KEY_LEFTCTRL 0x1D
#define KEY_A 0x1E
#define KEY_S 0x1F
#define KEY_D 0x20
#define KEY_F 0x21
#define KEY_G 0x22
#define KEY_H 0x23
#define KEY_J 0x24
#define KEY_K 0x25
#define KEY_L 0x26
#define KEY_SEMICOLON 0x27
#define KEY_QUOTE 0x28
#define KEY_BACKTICK 0x29
#define KEY_LEFTSHIFT 0x2A
#define KEY_BACKSLASH 0x2B
#define KEY_Z 0x2C
#define KEY_X 0x2D
#define KEY_C 0x2E
#define KEY_V 0x2F
#define KEY_B 0x30
#define KEY_N 0x31
#define KEY_M 0x32
#define KEY_COMMA 0x33
#define KEY_PERIOD 0x34
#define KEY_SLASH 0x35
#define KEY_RIGHTSHIFT 0x36
#define KEY_KEYPADASTERISK 0x37
#define KEY_LEFTALT 0x38
#define KEY_SPACE 0x39
#define KEY_CAPSLOCK 0x3A

// Function to initialize keyboard
void keyboard_initialize(void);

// Function to read keyboard scancode
uint8_t keyboard_read_scancode(void);

// Function to check if key is pressed (not released)
uint8_t keyboard_is_key_pressed(uint8_t scancode);

// Function to convert scancode to ASCII character
char keyboard_scancode_to_ascii(uint8_t scancode);

// Function to get current state of a key
uint8_t keyboard_get_key_state(uint8_t scancode);

// Keyboard buffer functions
#define KEYBOARD_BUFFER_SIZE 256

void keyboard_buffer_push(char c);
char keyboard_buffer_pop(void);
int keyboard_buffer_has_data(void);

#endif