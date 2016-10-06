#include <io.h> //
#include <devices.h> // sys_write_console
#include <interrupt.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

/******************************************/
/* Service Routines for System Calls      */
/******************************************/

int check_fd(int fd, int permissions);

int sys_write( int fd, char* buffer, int size) {
	// fd: file descriptor. In this delivery it must always be 1.
	// buffer: pointer to the bytes.
	// size: number of bytes.
	// return ’ Negative number in case of error (specifying the kind of error) and the number of bytes written if OK.

	// Check file descriptor
	int fd_error = check_fd(fd, ESCRIPTURA);
	if (fd_error != 0)
		return fd_error;

	// Check buffer is not NULL pointer
	if (buffer == NULL)
		return -EFAULT;

	// Check size is positive
	if (size < 0)
		return -EINVAL;

	// Print

	// copy from user
	volatile int erno = sys_write_console(buffer, size);
	return erno;
}

int sys_getticks() {
	return zeos_ticks;
}
