#ifndef __BITTWIDDLER_H
#define __BITTWIDDLER_H

/* count leading zeros of a single byte */
static inline unsigned char clz8(unsigned char byte) {
	unsigned char count = 0;

	if (byte & 0xf0) {
		byte >>= 4;
	} else {
		count = 4;
	}

	if (byte & 0x0c) {
		byte >>= 2;
	} else {
		count += 2;
	}

	if (!(byte & 0x02)) {
		count++;
	}

	return count;
}

/* count leading zeros of a 16-bit word */
static inline unsigned char clz(unsigned word) {
	unsigned char count = 0;

	if (word & 0xff00) {
		word >>= 8;
	} else {
		count = 8;
	}

	count += clz8(word);

	return count;
}

#endif
