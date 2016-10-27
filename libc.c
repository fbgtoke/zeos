/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno;

void itoa(int a, char *b) {
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a) {
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

char* strcpy(char *strDest, const char *strSrc) {
    char *temp = strDest;
    while(*strDest++ = *strSrc++); // or while((*strDest++=*strSrc++) != '\0');
    return temp;
}

char* strcat(char *dest, const char *src) {
    int i,j;
    for (i = 0; dest[i] != '\0'; i++)
        ;
    for (j = 0; src[j] != '\0'; j++)
        dest[i+j] = src[j];
    dest[i+j] = '\0';
    return dest;
}

void perror() {
  char msg[128];
  char num[4];
  itoa(errno, num);

  strcpy(msg, "Error #");
  strcat(msg, num);
  strcat(msg, ". Please refer to the Linux manual for more information\n");
  write(1, msg, 128);
}

int write(int fd, char* buffer, int size) {
	int erno;

	asm(
		"int	$0x80\n\t"
		: "=a" (erno)
		: "a" (0x04), "b" (fd), "c" (buffer), "d" (size)
	);

	if (erno < 0) {
		errno = -erno;
		return -1;
	}

	return erno;
}

int gettime() {
	int erno;

	asm(
		"int	$0x80\n\t"
		: "=a" (erno)
		: "a" (0x0a)
	);

	return erno;
}

int getpid() {
	int PID;

	asm(
		"int	$0x80\n\t"
		: "=a" (PID)
		: "a" (20)
	);

	return PID;
}

int fork() {
	int erno;

	asm(
		"int	$0x80\n\t"
		: "=a" (erno)
		: "a" (2)
	);

	return erno;
}
