#include "tscribe-bittwiddler.h"
#include "tscribe-bit-pack.h"
#include "tscribe-ega-codebook.h"
 
#define NULL 0
#define LZRW_TABLE_SIZE 64

unsigned char* prev;
unsigned char prev_length;
unsigned char prev_prev_length;
unsigned char table[LZRW_TABLE_SIZE];

void
tscribe_lzrw_init () {
	unsigned i;

	tscribe_ega_init ();

	for (i=0; i<LZRW_TABLE_SIZE; i++) {
		table[i] = 0;
	}

	prev = NULL;
	prev_length = 0;	
	prev_prev_length = 0;
}

// for testing
unsigned
tscribe_lzrw_compress_test (unsigned char* in, unsigned in_length) {
	unsigned i;
	unsigned error = 0;

	for (i=0; i<in_length; i++) {
		error |= TSCRIBE_PACK_BITS(compressed_gen, in[i], 8);
	}

	return error;
}

unsigned code;
unsigned code_length;

//inline
unsigned
tscribe_lzrw_compress (unsigned char *in, unsigned in_length) {
	unsigned next_idx;
	unsigned next_match_idx;
	unsigned match_length;
	unsigned prev_start_match_idx;
	unsigned next_prev_match_idx;
	unsigned offset_bits;
	unsigned length_bits;
	unsigned hash;
	unsigned i;
	unsigned error = 0;

	// test code
	/*
	for (i=0; i<in_length; i++) {
		code_length = tscribe_ega_encode(in[i], &code);
		//TSCRIBE_PACK_BITS(compressed_gen, in[i], 8);
		//TSCRIBE_PACK_BITS(compressed_gen, code, 8);
		//TSCRIBE_PACK_BITS(compressed_gen, code_length, 8);

		TSCRIBE_PACK_BITS(compressed_gen, code, code_length);
	}
	return 0;
	*/

	offset_bits = 16 - clz((unsigned)prev_length + in_length - 1);
	length_bits = offset_bits;

	//printf("offset_bits %d length_bits %d\n", offset_bits, length_bits);

	/* update hash table references */
	for (i=0; i<LZRW_TABLE_SIZE; i++) {
		table[i] -= prev_prev_length;
	}

	// encode all of in[]
	next_idx = 0;
	while (next_idx < in_length) {
		//printf("next_idx=%d in=%d\n", next_idx, in[next_idx]);
		match_length = 0;
		if (next_idx < in_length - 2) {
			hash = (in[next_idx] + (in[next_idx] << 4) + in[next_idx+1] + in[next_idx+2]) % LZRW_TABLE_SIZE;
			prev_start_match_idx = table[hash];
			next_match_idx = next_idx;
			next_prev_match_idx = prev_start_match_idx;

			// find a possible match in the previous block
			while (next_match_idx < in_length &&
				next_prev_match_idx < next_idx + prev_length && (
					(
						next_prev_match_idx < prev_length &&
						in[next_match_idx] == prev[next_prev_match_idx]
					) || (
						next_prev_match_idx >= prev_length && 
						in[next_match_idx] == in[next_prev_match_idx - prev_length]
					)
				)
			) {
				next_match_idx++;
				next_prev_match_idx++;
			}

			match_length = next_match_idx - next_idx;
			table[hash] = next_idx + prev_length;
		}

		if (match_length < 3) {
			// No EGA
			code = in[next_idx];
			code_length = 8;
			//code_length = tscribe_ega_encode(in[next_idx], &code);

			error |= TSCRIBE_PACK_BITS(compressed_gen, code, code_length+1);

			if (error != 0) {
				return error;
			}

			next_idx++;
		} else {
			unsigned remaining = in_length - next_idx;
			if (remaining < in_length/2) {
				if (remaining > 1) {
					unsigned bits = 8 - clz8(remaining - 1);
					length_bits = bits;
				} else {
					length_bits = 0;
				}
			}

			//printf("Encoding reference with offset: %u length: %u offset: %u bits: %u\n", prev_start_match_idx, match_length, offset_bits, length_bits);

			error |= TSCRIBE_PACK_BITS(compressed_gen, 1, 1);
			error |= TSCRIBE_PACK_BITS(compressed_gen, prev_start_match_idx, offset_bits);
			// recorded length is one smaller than actual length 
			error |= TSCRIBE_PACK_BITS(compressed_gen, match_length - 1, length_bits);

			if (error != 0) {
				return error;
			}

			next_idx += match_length;
		}
	}

	//if (prev != NULL) {
	//	free_buffer (prev);
	//}

	prev = in;
	prev_prev_length = prev_length;
	prev_length = in_length;

	return error;
}

