#include <libc.h>
#include <asm.h>

char buff[24];
int pid;

int __attribute__((noinline)) write (int fd, char* buffer, int size) {
	int erno;

	asm(
		"movl	$0x04, %%eax\n\t"
		"movl	%1, %%ebx\n\t"
		"movl	%2, %%ecx\n\t"
		"movl	%3, %%edx\n\t"
		"int	$0x80\n\t"
		"movl	%%eax, %0"
		: "=r" (erno)
		: "r" (fd), "r" (buffer), "r" (size)
	);

	return erno;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  while(1) {
    volatile int w = write(1, "HOLA", 4); // volatile para evitar que el compilador lo optimice
  }
}
