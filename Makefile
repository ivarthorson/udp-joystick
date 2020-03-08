# Makefile for a UDP joystick program

CC = g++
LD = g++

CFLAGS = -Wall -D_DEBUGGING_
LFLAGS = -lm -lrt -lpthread

TARGET=udp_joystick
# OBJECT=

INCLUDE = /usr/X11R6/include
LIB=/usr/X11R6/lib

ALL : $(TARGET) $(OBJECT)

$(TARGET) : $(OBJECT)
	$(LD) -o $(TARGET) $(LFLAGS) $(OBJECT)

udp_joystick.o:	 Makefile udp_joystick.h
	$(CC) $(CFLAGS) -c udp_joystick.cpp

udp_joystick_test:	 Makefile udp_joystick.h
	$(CC) $(CFLAGS) $(LFLAGS) -D_UDP_JS_LOOPBACK_TEST_ udp_joystick.c -o udp_joystick_test

udp_joystick:	 Makefile udp_joystick.h udp_joystick_test
	$(CC) $(CFLAGS) $(LFLAGS) -D_UDP_JOYSTICK_ udp_joystick.c -o udp_joystick

.c.o :
	$(CC) $(CFLAGS) -c $*.c

.cpp.o :
	$(CC) $(CFLAGS) -c $*.cpp

clean:
	rm -f $(TARGET) $(OBJECT) 
