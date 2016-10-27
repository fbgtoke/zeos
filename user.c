#include <libc.h>
#include <asm.h>

char buff[24];
int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{

  //runjp();

	fork();
  while(1) {
    pid = getpid();

		itoa(pid, buff);
		write(1, buff, 24);
  }
}
