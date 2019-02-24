/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Please Changeto Yourname (pcy2301)
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* micro36.ee.columbia.edu */
#define SERVER_HOST "128.59.148.182"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128
#define MAX_PER_ROW 64
#define INIT_ROW 21
#define INIT_COL 0
#define MAXROW 23

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);
void Custum_Initial();
void InitiateRow(int, int);

int main()
{
  int err, col, currentCol, currentRow;
  char dispCharacter;
  char[500] writeString;
  char *writeStringHead;
  char *writeHead;
  struct sockaddr_in serv_addr;
  int count=0;
  int i;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }
  /* initial the screen */
  Custum_Initial();
  /* Draw rows of asterisks across the top and bottom of the screen */
  for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 0, col);
    fbputchar('*',11,col);
    fbputchar('*', 23, col);
  }
  
  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  currentRow=INIT_ROW;
  currentCol=INIT_COL;
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
	      packet.keycode[1]);
      printf("%s\n", keystate);
      fbputs(keystate, 6, 0);
      writeStringHead = writeString;
      writeHead = writeString;
      if (packet.keycode[0]!=0){
         dispCharacter = keyValue(packet.keycode[0]);
         fbputchar(dispCharacter, currentRow, currentCol);
	 currentCol++;
	 count++;
	 writeString[count] = dispCharacter;
	 /* writeString++; */
      }

      if (currentCol > MAX_PER_ROW)
      {
	currentRow=currentRow+1;
        currentCol=INIT_COL;
      }

      /* scroll */
      if (count==BUFFER_SIZE){
	for (i=0;i<MAX_PER_ROW;i++)
	  writeStringHead++;
      }
      else if (count>BUFFER_SIZE && count % MAX_PER_ROW==0){
          InitiateRow(21, 22);
	  for(i=0;i<MAX_PER_ROW;i++){
	  writeStringHead++;
          fbputchar(*writeStringHead, 21, i);
          currentRow = 22;
	  }
      }
	    
      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	break;
      }
    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void Custum_Initial()
{
  int col, row;
  for (row = 0; row < 23; row++){
    for (col = 0 ; col < 64 ; col++) {
      fbputchar(' ', row, col);
    }
  }	
}

void InitiateRow(int min, int max){
  int col, row;
  for (row = min; row < max + 1; row++){
    for (col = 0 ; col < 64 ; col++) {
      fbputchar(' ', row, col);
    }
  }	
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  char tempBuf1[MAX_PER_ROW+1];
  char tempBuf2[MAX_PER_ROW+1];
  int n;
  int i;
  int tempRow=1;
  /* Read: Receive data */
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    if (n < MAX_PER_ROW || n==MAX_PER_ROW){  
      fbputs(recvBuf, tempRow, 0);
      tempRow++;
    }
    else{
      for (i=0;i<MAX_PER_ROW;i++){
        tempBuf1[i]=recvBuf[i];
      }
	tempBuf1[i]='\0';
      for (i=0;i < n-MAX_PER_ROW;i++){
        tempBuf2[i]=recvBuf[i];
      }
	tempBuf2[i]='\0';
      fbputs(tempBuf1, tempRow, 0);
      tempRow++;
      fbputs(tempBuf2, tempRow, 0);
      tempRow++;
    }	  
    /* Scroll up when it is full */
  }
  
  return NULL;
}

