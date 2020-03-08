/* 
   udp_joystick.c     A simple program to send joystick state over UDP
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "udp_joystick.h"

// For packet transmission and debugging
int js_state_as_str(js_state *jsst, char *buf)
{
  int offset = 0;
  int i;
  
  // Build up a string safely, starting with the timestamp 
  offset += snprintf(buf + offset, UDP_JS_UDP_CHAR_BUFSIZE - offset,  
		     "%d %u ", UDP_JS_DISPATCH, jsst->timestamp);

  // Add the button states
  for (i=0; i < UDP_JS_BUTTONS; i++){
    offset += snprintf(buf + offset, UDP_JS_UDP_CHAR_BUFSIZE - offset,  
		       "%d ", jsst->button[i]);
  }

  // Add the joystick states
  offset += snprintf(buf + offset, UDP_JS_UDP_CHAR_BUFSIZE - offset,  
		     "%d %d %d %d %d %d ",
		     jsst->l_stick_x, jsst->l_stick_y,
		     jsst->r_stick_x, jsst->r_stick_y,
		     jsst->cross_x, jsst->cross_y);
  
  // If we reached the end of the buffer, give a debug msg
  if (offset >= UDP_JS_UDP_CHAR_BUFSIZE - 1) {
    printf("UDP_JS_UDP_CHAR_BUFFSIZE must be too damn small.\n");
    exit(-1);
  }

  return 0;
}


int js_update_state(js_event *jse, js_state *jsst)
{
  // If we are receiving non-synthetic events, we are already initialized
  if (!(jse->type & JS_EVENT_INIT)) {
    jsst->initialized = 1;
  }

  // Treat synthetic events generated at init as regular events
  jse->type &= ~JS_EVENT_INIT;  

  // First update the timestamp
  jsst->timestamp = jse->time;

  // Do different things depending on event type
  if (jse->type == JS_EVENT_AXIS) {
    switch (jse->number) 
      {
      case 0: jsst->l_stick_x = jse->value;  break;
      case 1: jsst->l_stick_y = jse->value;  break;
      case 2: jsst->r_stick_x = jse->value;  break;
      case 3: jsst->r_stick_y = jse->value;  break;
      case 4: jsst->cross_x = jse->value;    break;
      case 5: jsst->cross_y = jse->value;    break;
      default:     break;
      }

  } else if (jse->type == JS_EVENT_BUTTON) {
    if (jse->number >= UDP_JS_BUTTONS) {
      printf("There are more buttons than expected!\n");
      exit(-1);
    }
    jsst->button[jse->number] = jse->value;

  } else {
    printf ("Unknown event!\n");
    exit(-1);
  }

  return 0;
}

// Create a blocking server socket port
int js_create_udp_server(int portnum, int *sock) 
{
  struct sockaddr_in server_addr;
  
  if ((*sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Socket");
    exit(-1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portnum);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_addr.sin_zero),8);

  if (bind(*sock, (const struct sockaddr *)&server_addr,
	   sizeof(struct sockaddr)) == -1) {
      perror("Bind");
      exit(1);
  }
  return 0;
}

// Create a blocking server socket port
int js_create_udp_server_nonblocking(int portnum, int *sock) 
{
  struct sockaddr_in server_addr;
  int flags;
  
  if ((*sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Socket");
    exit(-1);
  }

  flags = fcntl(*sock, F_GETFL,0);
  if(flags == -1) {perror("FCNTL"); exit(-1);}
  fcntl(*sock, F_SETFL, flags | O_NONBLOCK);
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portnum);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_addr.sin_zero),8);

  if (bind(*sock, (const struct sockaddr *)&server_addr,
	   sizeof(struct sockaddr)) == -1) {
      perror("Bind");
      exit(1);
  }
  return 0;
}


// Reads a packet from the UDP server socket, blocking if empty
int js_read_udp_packet(int *sock, char *str_buf)
{
  ssize_t bytes_read;
  struct sockaddr_in client_addr;
  socklen_t len = sizeof(struct sockaddr);

  bytes_read = recvfrom(*sock, str_buf, UDP_JS_UDP_CHAR_BUFSIZE,
			0, (struct sockaddr *) &client_addr, &len); 

  if (bytes_read < 0) perror("recvfrom");

  str_buf[bytes_read] = '\0';
      
  return 0;
}

int js_read_udp_packet_nonblocking(int *sock, char *str_buf)
{
  ssize_t bytes_read;
  struct sockaddr_in client_addr;
  socklen_t len = sizeof(struct sockaddr);

  bytes_read = recvfrom(*sock, str_buf, UDP_JS_UDP_CHAR_BUFSIZE,
			0, (struct sockaddr *) &client_addr, &len); 
  
  //printf("bytes read = %d\n", bytes_read);

  if (bytes_read <= 0) return -1;

  str_buf[bytes_read] = '\0';
      
  return 0;
}



int js_str_to_state(char* str_buf, js_state *jsst)
{ 
  int rc, jscode;
  unsigned int ts;
  short b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11;
  short lsx, lsy, rsx, rsy, crx, cry;
  #ifdef _DEBUGGING_
  char dbg_buf[UDP_JS_UDP_CHAR_BUFSIZE];
  #endif

  rc = sscanf(str_buf,
	     "%d %u %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd",
	      &jscode, &ts, &b0, &b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8, &b9, &b10, &b11,
	     &lsx, &lsy, &rsx, &rsy, &crx, &cry);
  
  if (rc != 20) {
    printf("Wrong # of fields in js packet (19!=%d). Discarding...\n", rc);
    return -1;
  }

  if(jscode != UDP_JS_DISPATCH){
    printf("Somehow the dispatch to parse_joystick_str was incorrect?!?\n");
    return -1;
  }

  // Discard the packet if it's older than the most recent one
  if (ts < jsst->timestamp) {
    printf("Old js packet recieved, discarding...\n");
    return -1;
  }

  // Check that all fields are in proper ranges
  // Buttons are zero or one.
  // I'm assuming stick states will be properly handled by sscanf above
  if (0 <= b0 && b0 <= 1 &&
      0 <= b1 && b1 <= 1 &&
      0 <= b2 && b2 <= 1 &&
      0 <= b3 && b3 <= 1 &&
      0 <= b4 && b4 <= 1 &&
      0 <= b5 && b5 <= 1 &&
      0 <= b6 && b6 <= 1 &&
      0 <= b7 && b7 <= 1 &&
      0 <= b8 && b8 <= 1 &&
      0 <= b9 && b9 <= 1 &&
      0 <= b10 && b10 <= 1 &&
      0 <= b11 && b11 <= 1) {
    
    jsst->timestamp = ts;

    jsst->button[0] = b0;
    jsst->button[1] = b1;
    jsst->button[2] = b2;
    jsst->button[3] = b3;
    jsst->button[4] = b4;
    jsst->button[5] = b5;
    jsst->button[6] = b6;
    jsst->button[7] = b7;
    jsst->button[8] = b8;
    jsst->button[9] = b9;
    jsst->button[10] = b10;
    jsst->button[11] = b11;

    jsst->l_stick_x = lsx;
    jsst->l_stick_y = lsy;
    jsst->r_stick_x = rsx;
    jsst->r_stick_y = rsy;
    jsst->cross_x = crx;
    jsst->cross_y = cry;

    #ifdef _DEBUGGING_
    printf("Updating JS state @time %u...\n", ts);
    js_state_as_str((js_state *) jsst, dbg_buf);
    printf("Recv: '%s'\n", dbg_buf); fflush(stdin);
    #endif

    return 0;

  } else {

    printf("Invalid joystick state string. Discarding...\n");
    return -1;
  }  
}


// Converts a signed short integer into a normalized double 
// Returned double will be in range [-1<x<1], inclusive. 
double normadoublify (short val) 
{
  return -1.0 + 2.0 * (val - (double) SHRT_MIN) / 
    ((double) SHRT_MAX - (double) SHRT_MIN);
}


////////////////////////////////////////////////////////////////////////////////


#ifdef _UDP_JS_LOOPBACK_TEST_

int main()
{
  struct sockaddr_in server_addr;
  struct hostent *host;
  struct js_event jse;
  struct js_state jsst;
  struct js_state jsst_recv;
  char send_data[UDP_JS_UDP_CHAR_BUFSIZE];
  char recv_data[UDP_JS_UDP_CHAR_BUFSIZE];
  int jsfd; // Joy stick file descriptor
  int cli_sock; // Socket descriptor
  int serv_sock;
  int run = 1;

  printf("Opening the joystick device\n");
  if((jsfd = open(UDP_JS_DEVNAME, O_RDONLY))< 0) {
    perror("joystick");
    exit(1);
  }

  printf("Starting UDP server...\n");
  js_create_udp_server_nonblocking(UDP_JS_UDP_PORT, &serv_sock);

  printf("UDP server listening on port %d\n", UDP_JS_UDP_PORT);
  fflush(stdout);

  printf("Setting up the host object...\n");
  host = (struct hostent *) gethostbyname((char *)UDP_JS_UDP_SERVER_IP);

  printf("Creating a client socket file descriptor...\n");
  if ((cli_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  
  printf("Configuring the server information struct...\n");
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(UDP_JS_UDP_PORT);
  server_addr.sin_addr = *((struct in_addr *)host->h_addr);
  bzero(&(server_addr.sin_zero),8);

  // Mark joystick state as uninitialized
  jsst.initialized = 0;  

  // The main loop
  while (run) {

    // Read a joystick event
    if (read(jsfd, &jse, sizeof(jse)) != sizeof(jse)) {  
      printf("Joystick read error occurred.\n");
      exit(-1);
    }

    // Debugging information
    #ifdef _DEBUGGING_
    printf("Event: time %8u, value %8hd, type: %3u, axis/button: %u\n", 
	   jse.time, jse.value, jse.type, jse.number);    
    #endif

    // Update the state of the joystick
    if (js_update_state(&jse, &jsst) != 0) {  
      printf("Joystick update error ocurred.\n");
      exit(-1);
    }
    
    // Send out the state over UDP if initialization complete
    if (jsst.initialized == 1) {
      // Form a string sending the state of the joystick   
      js_state_as_str(&jsst, send_data);
      
      // Debugging information
      #ifdef _DEBUGGING_
      printf("Sent: '%s'\n", send_data);
      fflush(stdin);
      #endif

      // Send the packet over UDP to the server
      sendto(cli_sock, send_data, strlen(send_data), 0,
	     (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    }

    // Read the UDP back from the server
    if (js_read_udp_packet_nonblocking(&serv_sock, recv_data) != -1) {
      fflush(stdout);
      if (js_str_to_state(recv_data, &jsst_recv) != 0){
	printf("js_str_to_state wasn't happy...\n");
	fflush(stdout);
      }
      continue;
    }   
  }

  close(jsfd);

  return 0;
}

#endif 

#ifdef _UDP_JOYSTICK_

int main()
{
  struct sockaddr_in server_addr;
  struct hostent *host;
  struct js_event jse;
  struct js_state jsst;
  char send_data[UDP_JS_UDP_CHAR_BUFSIZE];
  int jsfd; // Joy stick file descriptor
  int sock; // Socket descriptor
  int run = 1;

  printf("Opening the joystick device\n");
  if((jsfd = open(UDP_JS_DEVNAME, O_RDONLY))< 0) {
    perror("joystick");
    exit(1);
  }

  printf("Setting up the host object...\n");
  host = (struct hostent *) gethostbyname((char *)UDP_JS_UDP_SERVER_IP);

  printf("Creating a client socket file descriptor...\n");
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  
  printf("Configuring the server information struct...\n");
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(5000);
  server_addr.sin_addr = *((struct in_addr *)host->h_addr);
  bzero(&(server_addr.sin_zero),8);

  // Mark joystick state as uninitialized
  jsst.initialized = 0;

  // The main loop
  while (run) {

    // Read a joystick event
    if (read(jsfd, &jse, sizeof(jse)) != sizeof(jse)) {  
      printf("Joystick read error occurred.\n");
      exit(-1);
    }

    // Debugging information
    #ifdef _DEBUGGING_
    printf("Event: time %8u, value %8hd, type: %3u, axis/button: %u\n", 
    jse.time, jse.value, jse.type, jse.number);    
    #endif

    // Update the state of the joystick
    if (js_update_state(&jse, &jsst) != 0) {  
      printf("Joystick update error ocurred.\n");
      exit(-1);
    }
    
    // Send out the state over UDP if initialization complete
    if (jsst.initialized == 1) {
      // Form a string sending the state of the joystick   
      js_state_as_str(&jsst, send_data);
      
      #ifdef _DEBUGGING_
      printf("Sent: '%s'\n", send_data);
      fflush(stdin);
      #endif

      // Send the packet over UDP to the server
      sendto(sock, send_data, strlen(send_data), 0,
	     (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    }
  }

  close(jsfd);

  return 0;
}

#endif
