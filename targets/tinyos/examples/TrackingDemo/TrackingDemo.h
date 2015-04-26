#ifndef TRACKING_DEMO_H
#define TRACKING_DEMO_H

typedef nx_struct demo_msg {
	nx_uint16_t nodeid;
	nx_uint16_t reading;
	nx_uint8_t state;
} demo_msg_t;

enum {
	AM_DEMO_MSG = 6
};

enum {
	NOT_DETECTED = 1,
	DETECTED = 2 
};

#endif

