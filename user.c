#include <libc.h>
#include <asm.h>

char buff[24];
int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{

  	runjp();
	//int f = fork();

	while(1) {
	  	//pid = getpid();
	  	//itoa(-1, buff);
	  	//write(1, buff, 24);
	}
}
