#include "tscribe-ega-codebook.h"
#include "tscribe-bittwiddler.h"

#ifndef EXP_GOLOMB_K
/* default exponential golomb model */
#define EXP_GOLOMB_K 2
#endif

unsigned char codebook[256];
unsigned char reverse[128];

void
tscribe_ega_init() {
	unsigned i;

	for (i=0; i<128; i++) {
		codebook[i] = i;
		reverse[i] = i;	
	}
	for (; i<256; i++) {
		codebook[i] = i;
	}
}

/**
 * For EXP_GOLOMB_K = 2
 * binCode => code
 *   0 => 1 00 
 *   1 => 1 01
 *   2 => 1 10
 *   3 => 1 11
 * 
 *   4 => 0 1 000
 *   5 => 0 1 001
 *   6 => 0 1 010
 *   7 => 0 1 011
 *   8 => 0 1 100
 *   9 => 0 1 101
 *  10 => 0 1 110
 *  11 => 0 1 111
 *
 *  12 => 00 1 0000
 * ...
 *
 * 2^k number of codes can be optimized
 * 252 => 00000 00
 * ...
 * 255 => 00000 11
 */
inline unsigned char
tscribe_ega_encode(unsigned char clear, unsigned char *code) {
	/* the length of the returned code in bits */
	unsigned char length;
	/* the code in binary */
	unsigned char binCode = codebook[clear];
	unsigned char swapCode;
	unsigned char swapReverse;

	/* convert binCode to an exponential golomb code */
	*code = binCode + (1 << EXP_GOLOMB_K);
	length = 16 - clz(*code);
	length = length + (length - (EXP_GOLOMB_K + 1));

	//printf("clear=%u, binCode=%u, code=%u, length=%u\n", clear, binCode, *code, length);

	/* some optimization */
	//if (binCode >= 252) {
	//	*code = 255 - binCode;
	//	length = 7;
	//}

	/* swap binCode with binCode/2 */
	swapCode = binCode/2;
	swapReverse = reverse[swapCode];
	codebook[clear] = swapCode;
	codebook[swapReverse] = binCode;
	/* do not ever need to know the reverse of codes >=128 */
	if (binCode < 128) {
		reverse[binCode] = swapReverse;
	}
	reverse[swapCode] = clear;

	return length;
}
