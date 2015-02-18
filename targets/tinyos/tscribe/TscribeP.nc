#include "tscribe.h"

module TscribeP {
	uses interface Resource;
}

implementation {
	void tscribe_request_bus (void) @C() @spontaneous() { call Resource.request(); }
	void tscribe_release_bus (void) @C() @spontaneous() { call Resource.release(); }
	event void Resource.granted () { tscribe_bus_granted(); }
}

