#include "TrackingDemo.h"

configuration TrackingDemoAppC {
}
implementation {
	components MainC, TrackingDemoC as App, LedsC;
	components new DemoSensorC() as Sensor;
	components new AMSenderC(AM_DEMO_MSG);
	components new AMReceiverC(AM_DEMO_MSG);
	components new TimerMilliC();
	components ActiveMessageC;
	components SerialPrintfC;

	//MainC.SoftwareInit -> Sensor;

	App.Boot -> MainC.Boot;
	App.Read -> Sensor;
	App.Receive -> AMReceiverC;
	App.AMSend -> AMSenderC;
	App.AMControl -> ActiveMessageC;
	App.Leds -> LedsC;
	App.MilliTimer -> TimerMilliC;
	App.Packet -> AMSenderC;
}

