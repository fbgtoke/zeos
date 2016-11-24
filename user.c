#include <libc.h>

char buff[24];

int pid;

void hola() {

	write(1, "hola", 4);

	exit();
}

int __attribute__ ((__section__(".text.main"))) main(void) {

  runjp();  

  while(1) {
  }
}
