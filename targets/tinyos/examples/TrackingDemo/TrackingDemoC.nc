/**
 *
 */

#include <stdio.h>
#include <stdint.h>
#include "Timer.h"
#include "TrackingDemo.h"
 
module TrackingDemoC @safe() {
	uses {
		interface Leds;
		interface Boot;
		interface Receive;
		interface AMSend;
		interface Timer<TMilli> as MilliTimer;
		interface Read<uint16_t>;
		interface SplitControl as AMControl;
		interface Packet;
	}
}
implementation {
	#define LIGHT_THRESHOLD 32
	static void fatalProblem();
	static void reportProblem();
	uint16_t counter = 0;

	message_t packet;
	bool locked;
	uint8_t state = NOT_DETECTED;
  
	event void Boot.booted() {
		call AMControl.start();
	}

	event void AMControl.startDone(error_t err) {
		if (err == SUCCESS) {
			call MilliTimer.startPeriodic(1024);
		} else {
			call AMControl.start();
		}
	}

	event void AMControl.stopDone(error_t err) {
		// do nothing
	}
  
	event void MilliTimer.fired() {
		if (counter == 30) {
			call Leds.led2On();
			tscribe_flush_state();
		} 

		counter++;

		if (TOS_NODE_ID != 0) {
			if (call Read.read() != SUCCESS) {
				fatalProblem();
			}
		}
	}

	event void Read.readDone(error_t result, uint16_t data) {
		bool sendUpdate = FALSE;

		if (result != SUCCESS) {
			data = 0xffff;
			reportProblem();
		}

		printf("data %u\n", data);

		if (state == DETECTED && data > LIGHT_THRESHOLD) {
			state = NOT_DETECTED;
			sendUpdate = TRUE;
		} else if (state == NOT_DETECTED && data < LIGHT_THRESHOLD) {
			state = DETECTED;
			sendUpdate = TRUE;
		}

		if (!sendUpdate || locked) {
			return;
		} else {
			demo_msg_t* msg = (demo_msg_t*)call Packet.getPayload(&packet, sizeof(demo_msg_t));
			if (msg == NULL) {
				return;
			}

			msg->reading = data;
			msg->state = state;
			if (call AMSend.send(AM_BROADCAST_ADDR, &packet, sizeof(demo_msg_t)) == SUCCESS) {
				locked = TRUE;
			}
		}
	}

	event message_t* Receive.receive(message_t* bufPtr, void* payload, uint8_t len) {
		if (len != sizeof(demo_msg_t)) {
			return bufPtr;
		} else {
			demo_msg_t* msg = (demo_msg_t*)payload;
			__asm ("mov R4, R4");
			printf("Received msg from=%u reading=%u state=%u.\n", msg->nodeid, msg->reading, msg->state);
			if (msg->state == DETECTED) {
				call Leds.led1Off();
			} else {
				call Leds.led1On();
			}
		}
		return bufPtr;
	}

	event void AMSend.sendDone(message_t* bufPtr, error_t error) {
		if (&packet == bufPtr) {
			locked = FALSE;
		}
	}

	static void fatalProblem() {
		call Leds.led0On(); 
		call Leds.led1On();
		call Leds.led2On();
		call MilliTimer.stop();
	}

	static void reportProblem() {
		call Leds.led0On(); 
	}
}

