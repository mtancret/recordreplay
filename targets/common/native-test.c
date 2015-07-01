/**
 * Author(s): Matthew Tan Creti
 *            matthew.tancreti at gmail dot com
 */
#include <stdio.h>
#include "tscribe.h"
#include "tscribe-bit-pack.h"

#define BUF_SIZE (sizeof(unsigned int) * 512)
TSCRIBE_DECLARE_BUF(test_buf, BUF_SIZE);

void test_bit_pack_bytes(void) {
	TSCRIBE_INIT_BUF(test_buf, BUF_SIZE);

	TSCRIBE_PACK_BITS(test_buf, 0xff, 8);
	TSCRIBE_PACK_BITS(test_buf, 0xff, 8);
	TSCRIBE_PACK_BITS(test_buf, 0xff, 8);
	TSCRIBE_PACK_BITS(test_buf, 0xff, 8);

	unsigned int length = TSCRIBE_BUF_LENGTH(test_buf);

	if (length != 4) {
		printf("Test failed: test_bit_pack expected 4 found %u\n", TSCRIBE_BUF_LENGTH(test_buf));
	}
}

void test_bit_pack_wrap(void) {
	unsigned int i;

	TSCRIBE_INIT_BUF(test_buf, BUF_SIZE);

	TSCRIBE_BUF_FIRST_WORD(test_buf) += 10;
	TSCRIBE_BUF_NEXT_WORD(test_buf) += 10;

	unsigned int length = TSCRIBE_BUF_LENGTH(test_buf);
	if (length != 0) {
		printf("Test failed: test_bit_pack_warp expected 0 found %u\n", length);
	}

	for (i=0; i<BUF_SIZE-1; i++) {
		TSCRIBE_PACK_BITS(test_buf, 0xff, 8);
	}

	length = TSCRIBE_BUF_LENGTH(test_buf);
	if (length != BUF_SIZE-sizeof(unsigned int)) {
		printf("Test failed: test_bit_pack expected %u found %u\n", (unsigned int)(BUF_SIZE-sizeof(unsigned int)), length);
	}

	TSCRIBE_PACK_BITS(test_buf, 0xff, 8);

	length = TSCRIBE_BUF_LENGTH(test_buf);
	if (length != BUF_SIZE) {
		printf("Test failed: test_bit_pack expected %u found %u\n", (unsigned int)BUF_SIZE, length);
	}

	if (!TSCRIBE_BUF_FULL(test_buf)) {
		printf("Test failed: test_bit_pack expected full found not full\n");
	}	
}

unsigned char plain_text[] = {
 0,   0,    0,   0,  64,   0,   0,  64,   0,   0,
64,   0,   0,  64,   0,   0,  64,   0,   0,  64,
 0,   0,  64,   0,  18, 223,  91, 136, 103, 137,
 0,   0,   0,  64,   0,   0,  64,   0,   0,  64,
 0,   0,  64,   0,   0,  64,   0,   0,  64,   0,
 0,  64,   0,  18, 223,  91, 136, 103, 137,   0,
 0,   0,  64,   0,   0,  64,   0,   0,  64,   0,
 0,  64,   0,   0,  64,   0,   0,  64,   0,   0,
64,   0,  18, 223,  91, 136, 103, 137,   0,   0,
 0,  64,   0,   0,  64,   0,   0,  64,   0,   0,
64,   0,   0,  64,   0,   0,  64,   0,   0,  64,
 0,  18, 223,  91, 136, 103, 137,   0,   0, 106,
 0,  86,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,       
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
 1,   1,   1,   1,   1,   1};

void test_lzrw(void) {
	unsigned int i;
	unsigned char c1, c2, c3, c4;

	TSCRIBE_INIT_BUF(compressed_gen, BUF_SIZE);
	tscribe_lzrw_init();

	tscribe_lzrw_compress(plain_text, 128);

	for (i=0; i<BUF_SIZE; i+=4) {
		c1 = *(((char*)TSCRIBE_BUF_FIRST_WORD(compressed_gen)) + i);
		c2 = *(((char*)TSCRIBE_BUF_FIRST_WORD(compressed_gen)) + i + 1);
		c3 = *(((char*)TSCRIBE_BUF_FIRST_WORD(compressed_gen)) + i + 2);
		c4 = *(((char*)TSCRIBE_BUF_FIRST_WORD(compressed_gen)) + i + 3);
		printf("%u, %u, %u, %u ", c4, c3, c2, c1);
	}
}

int main() {
	test_bit_pack_bytes();
	test_bit_pack_wrap();
	test_lzrw();
	printf("Tests complete.\n");
	return 0;
}
