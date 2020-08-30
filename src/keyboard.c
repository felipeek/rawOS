#include "keyboard.h"
#include "interrupt.h"
#include "screen.h"
#include "asm/io.h"
#include "keyboard_scancode_set_1.h"

#define KEYBOARD_DATA_PORT 0x60                 // Read/Write port
#define KEYBOARD_STATUS_REGISTER_PORT 0x64      // Read port
#define KEYBOARD_COMMAND_REGISTER_PORT 0x64     // Write port

void keyboard_interrupt_handler(const Interrupt_Handler_Args* args) {
    // For now we interact with the keyboard device considering it is a PS/2 Keyboard, managed by the PS/2 Controller.
    // Nowadays they usually aren't PS/2, but for compatibility purposes, the mainboard supports USB Legacy Mode, meaning
    // that it emulates a PS/2 keyboard, even if it is a USB keyboard.
    // For the future, once we have a USB driver, we can disable the legacy mode and use the USB controller to interact with the keyboard.

    // When we receive an IRQ 1, we don't need to check the output buffer status in the status register
    // However, just for learning purposes, I'm confirming it is 1.
    u8 status = io_byte_in(KEYBOARD_STATUS_REGISTER_PORT);
    if (!(status & 0x01)) {
        screen_print("Error: Output buffer status is 0 after receiving the interrupt!");
    }

    // Get the scancode from the Keyboard device.
    u8 scan_code = io_byte_in(KEYBOARD_DATA_PORT);

    // When we receive an IRQ 1, we don't need to check the output buffer status in the status register
    // However, just for learning purposes, I'm confirming it is 0 after the scancode is read.
    status = io_byte_in(KEYBOARD_STATUS_REGISTER_PORT);
    if (status & 0x01) {
        screen_print("Error: Output buffer status is 1 after receiving the interrupt!");
    }
    status = io_byte_in(KEYBOARD_STATUS_REGISTER_PORT);

    switch (scan_code) {
        case KEYBOARD_A_PRESS: screen_print("A"); break;
        case KEYBOARD_B_PRESS: screen_print("B"); break;
        case KEYBOARD_C_PRESS: screen_print("C"); break;
        case KEYBOARD_D_PRESS: screen_print("D"); break;
        case KEYBOARD_E_PRESS: screen_print("E"); break;
        case KEYBOARD_F_PRESS: screen_print("F"); break;
        case KEYBOARD_G_PRESS: screen_print("G"); break;
        case KEYBOARD_H_PRESS: screen_print("H"); break;
        case KEYBOARD_I_PRESS: screen_print("I"); break;
        case KEYBOARD_J_PRESS: screen_print("J"); break;
        case KEYBOARD_K_PRESS: screen_print("K"); break;
        case KEYBOARD_L_PRESS: screen_print("L"); break;
        case KEYBOARD_M_PRESS: screen_print("M"); break;
        case KEYBOARD_N_PRESS: screen_print("N"); break;
        case KEYBOARD_O_PRESS: screen_print("O"); break;
        case KEYBOARD_P_PRESS: screen_print("P"); break;
        case KEYBOARD_Q_PRESS: screen_print("Q"); break;
        case KEYBOARD_R_PRESS: screen_print("R"); break;
        case KEYBOARD_S_PRESS: screen_print("S"); break;
        case KEYBOARD_T_PRESS: screen_print("T"); break;
        case KEYBOARD_U_PRESS: screen_print("U"); break;
        case KEYBOARD_V_PRESS: screen_print("V"); break;
        case KEYBOARD_X_PRESS: screen_print("X"); break;
        case KEYBOARD_W_PRESS: screen_print("W"); break;
        case KEYBOARD_Y_PRESS: screen_print("Y"); break;
        case KEYBOARD_Z_PRESS: screen_print("Z"); break;
        case KEYBOARD_ENTER_PRESS: screen_print("\n"); break;
    }
}

void keyboard_init() {
    interrupt_register_handler(keyboard_interrupt_handler, IRQ1);
}