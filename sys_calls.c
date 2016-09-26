#include <io.h> //
#include <devices.h> // sys_write_console

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
	int fd_error = check_fd(fd, LECTURA);
	if (fd_error != 0)
		return fd_error;

	// Check buffer is not NULL pointer
	#define SYS_WRITE_NULL_POINTER -10;
	if (buffer == NULL)
		return SYS_WRITE_NULL_POINTER;

	// Check size is positive
	#define SYS_WRITE_INVALID_SIZE -11;
	if (size <= 0)
		return SYS_WRITE_INVALID_SIZE;

	// Print
	volatile int erno = sys_write_console(buffer, size);
	return erno;
}
