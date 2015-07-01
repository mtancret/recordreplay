/**
 * Author(s): Matthew Tan Creti
 *            matthew.tancreti at gmail dot com
 */
configuration TscribeC { }

implementation {
	components TscribeP, new Msp430Spi0C();
	TscribeP.Resource -> Msp430Spi0C;
}

