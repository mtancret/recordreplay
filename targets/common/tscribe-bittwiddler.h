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
