/**
 * tinyPack 
 * Author(s): Matthew Tan Creti
 *
 * Copyright 2011 Matthew Tan Creti
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include "stdbool.h"

#include "tscribe-telos.h"
#include "tscribe-bit-pack.h"
#include "tscribe.h"

inline unsigned
tscribe_pack_bits(
	unsigned int* buf,
	unsigned int* terminate,
	unsigned int* first_word,
	unsigned int in,
	unsigned int num_bits,
	/* passed by reference, may be modified */
	unsigned int* next_bit,
	bool* full,
	unsigned int** next_word) {

	unsigned int remaining;
	unsigned int shift; 

	/* exit if full */
	if (*full) {
		return TSCRIBE_ERR_BIT_PACK_OVERFLOW;
	}

	remaining = *next_bit + 1;
	if (num_bits < remaining) {
		*next_bit -= num_bits;
		**next_word |= (in << (*next_bit + 1));
	} else {
		shift = num_bits - remaining;
		**next_word |= (in >> shift);
		*next_bit = sizeof(unsigned int)*8 - 1 - shift;

		*next_word += 1;
		if (*next_word >= terminate) {
			*next_word = buf;
		}

		/* check for overflow */
		if (*next_word == first_word) {
			*full = true;
			return TSCRIBE_ERR_BIT_PACK_OVERFLOW;
		}

		**next_word = ((*next_bit == sizeof(unsigned int)*8 - 1) ? 0 : (in << (*next_bit + 1)));
	}

	return 0;
}

inline unsigned
tscribe_buf_length (unsigned int* buf,
	unsigned int* next_word,
	unsigned int* first_word,
	unsigned int next_bit,
	unsigned int* buf_terminate,
	bool full) {

	unsigned length;
	unsigned int s;
	SPLHIGH(s);

	if ((next_word < first_word) || full) {
		length = ((buf_terminate - first_word) + (next_word - buf)) * sizeof(unsigned int);
	} else {
		length = (next_word - first_word) * sizeof(unsigned int);
	}

	//if (next_bit != 0) {
	//	length++;
	//}

	SPLX(s);

	return length;
}
