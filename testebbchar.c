/**
* @file   testebbchar.c
* @author Derek Molloy
* @date   7 April 2015
* @version 0.1
* @brief  A Linux user space program that communicates with the ebbchar.c LKM. It passes a
* string to the LKM and reads the response from the LKM. For this example to work the device
* must be called /dev/ebbchar.
* @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
*/
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM

int main(){
  int ret, fd;
  char stringToSend[BUFFER_LENGTH];
  printf("Starting device test code example...\n");
  fd = open("/dev/ebbchar", O_RDWR);             // Open the device with read/write access
  if (fd < 0){
      perror("Failed to open the device...");
      return errno;
  }

  	int color[3] ;
	char b[] = { 'N', 0, 0, 0, 0 };
	for (int i = 0 ; i < 3 ; i++)
		scanf("%x", &color[i]);                // Read in a string (with spaces)
	
	for (int i = 0 ; i < 3 ; i++)
		printf("%x\n", color[i]);   

  int j = 25;
  for (int i = 0; i<10; i++) {
    b[1] = i;
    b[2] = color[0];
    b[3] = color[1];
    b[4] = color[2];
    ret = write(fd, b, 5);
  }

  printf("End of the program\n");
  close(fd);
  return 0;
}
