configuration TscribeC { }

implementation {
	components TscribeP, new Msp430Spi0C();
	TscribeP.Resource -> Msp430Spi0C;
}

