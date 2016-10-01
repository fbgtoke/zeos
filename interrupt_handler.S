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

/******************************/
/* Clock Interrupt Handler    */
/******************************/

#ifndef HANDLER_CLOCK
#define HANDLER_CLOCK

ENTRY(handler_clock)
SAVE_ALL
EOI

call rsi_clock;

RESTORE_ALL

iret;

#endif

/******************************/
/* System Calls Handler       */
/******************************/

#ifndef HANDLER_SYSCALL
#define HANDLER_SYSCALL

ENTRY(system_call_handler)
SAVE_ALL

cmpl	$0, % eax 			// Is syscall number negative?
jl	err                             // If it is, jump to return an error
cmpl	$MAX_SYSCALL, %eax 		// Is syscall greater than MAX_SYSCALL (4)?
jg	err                             // If it is, jump to return an error
call	*sys_call_table(, %eax, 0x04)  	// Call the corresponding service routine
jmp	fin                             // Finish

err:
movl 	$-38, %eax	 		// Move to eax the ENOSYS error

fin: 	
movl 	%eax, 0x18(%esp) 	        // Change the eax value in the stack

RESTORE_ALL                         	// Restore the context
iret

#endif 
