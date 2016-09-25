#include <asm.h>
#include <segment.h>
#include "entry.S"

/******************************/
/* Keyboard Interrupt Handler */
/******************************/

#ifndef HANDLER_KEYBOARD
#define HANDLER_KEYBOARD

ENTRY(handler_keyboard)
SAVE_ALL
EOI

call rsi_keyboard; // call service routine

RESTORE_ALL

iret;

#endif
