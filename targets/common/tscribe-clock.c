/**
 * Author(s): Matthew Tan Creti
 *            matthew.tancreti at gmail dot com
 */
#include <msp430x16x.h>
#include "stdbool.h"

#include "tscribe-clock.h"

bool saved = false;
char saved_dcoctl;
char saved_bcsctl1; 

inline void
tscribe_max_dco(void)
{
	if (!saved) {
		saved_dcoctl = DCOCTL;
		saved_bcsctl1 = BCSCTL1;
	
		DCOCTL = DCO0 | DCO1 | DCO2;      // max DCO
		BCSCTL1 |= RSEL0 | RSEL1 | RSEL2; // max RSEL
	
		saved = true;
	}
}

inline void
tscribe_restore_dco(void)
{
	saved = false;

	DCOCTL = saved_dcoctl;
	BCSCTL1 = saved_bcsctl1;
}
