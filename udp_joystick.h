/* udp_joystick.h */

#ifndef __UDP_JOYSTICK_H__
#define __UDP_JOYSTICK_H__

#define UDP_JS_UDP_SERVER_IP "127.0.0.1"
#define UDP_JS_UDP_PORT         5001  /* The default UDP port number */
#define UDP_JS_UDP_CHAR_BUFSIZE 1024  /* Starting size of UDP data buffer*/

#define UDP_JS_DEVNAME "/dev/input/js0"    /* Joystick device */
#define UDP_JS_BUTTONS   12                /* Number of joystick buttons */
#define UDP_JS_DISPATCH  42

#define JS_EVENT_BUTTON         0x01    /* button pressed/released */
#define JS_EVENT_AXIS           0x02    /* joystick moved          */
#define JS_EVENT_INIT           0x80    /* initial state of device */

// Joystick Button Definitions
#define JS_B_SQ 0   /* Square Button */
#define JS_B_TR 1   /* Triangle Button */
#define JS_B_X  2   /* X Button */
#define JS_B_O  3   /* O Button */
#define JS_B_L1 4   /* L1 Button */
#define JS_B_R1 5   /* R1 Button */
#define JS_B_L2 6   /* L2 Button */
#define JS_B_R2 7   /* R2 Button */
#define JS_B_LJ 8   /* Left thumb joystick button */
#define JS_B_RJ 9   /* Right thumb joystick button */
#define JS_B_SE 10  /* Famicon 'select' button, the '-' button */
#define JS_B_ST 11  /* Famicon 'start' button, the '+' button */

/* The directional pad does not give 1 or 0 as a return value, but 32767.
   So, button presses are detected by being tested against this value*/
#define JS_CROSSVAL 32767     

// Should probably be defined elsewhere
#define SHRT_MIN -32768
#define SHRT_MAX 32767     

// Joystick event structure
typedef struct js_event {
  unsigned int  time;	// timestamp
  short         value;  // 
  unsigned char type;   // Event type
  unsigned char number; // Axis/button number
} js_event;

// Stores the joystick state
typedef struct js_state {
  unsigned int timestamp; 
  int initialized;
  short button[UDP_JS_BUTTONS];
  short cross_x;   // The directional pad
  short cross_y;
  short l_stick_x;
  short l_stick_y;
  short r_stick_x;
  short r_stick_y;
} js_state;


int js_state_as_str(js_state *jsst, char *buf);
int js_update_state(js_event *jse, js_state *jsst);
int js_create_udp_server(int portnum, int *sock);
int js_create_udp_server_nonblocking(int portnum, int *sock);
int js_read_udp_packet(int *sock, char *str_buf);
int js_read_udp_packet_nonblocking(int *sock, char *str_buf);
int js_str_to_state(char* str_buf, js_state *jsst);
double normadoublify (short val);

#endif  // __UDP_JOYSTICK_H__
