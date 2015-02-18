#ifndef _BIT_PACK_H
#define _BIT_PACK_H

#include "stdbool.h"

#define INT_BUF_SIZE 512
#define STATE_BUF_SIZE 512
#define COMPRESSED_GEN_BUF_SIZE 512
#define GEN_BUF_SIZE 128

unsigned int tscribe_buf_error;

inline unsigned tscribe_pack_bits(unsigned int *buf, unsigned int *terminate, unsigned int *first_word, unsigned int in, unsigned int num_bits, unsigned int *next_bit, bool *full, unsigned int **next_word);

inline unsigned tscribe_buf_length (unsigned int *buf, unsigned int *next_word, unsigned int *first_word, unsigned int next_bit, unsigned int *buf_terminate, bool full);

/* Buf points the the start of the buffer */
#define TSCRIBE_BUF(NAME) tscribe_ ## NAME ## _buf
/* Terminate points to one past the end of the buffer. */
#define TSCRIBE_BUF_TERMINATE(NAME) tscribe_ ## NAME ## _buf_end
/* First points to the first word stored in the buffer. */
#define TSCRIBE_BUF_FIRST_WORD(NAME) tscribe_ ## NAME ## _buf_first_word
/* Next points to the next word to be stored in the buffer. */
#define TSCRIBE_BUF_NEXT_WORD(NAME) tscribe_ ## NAME ## _buf_next_word
/* Next bit points to the next bit in next word to be stored in the buffer. */
#define TSCRIBE_BUF_NEXT_BIT(NAME) tscribe_ ## NAME ## _buf_next_bit
/* Full is a boolean value, true if buffer is full. */
#define TSCRIBE_BUF_FULL(NAME) tscribe_ ## NAME ## _buf_full

#define TSCRIBE_DECLARE_BUF(NAME, SIZE)                         \
   unsigned int TSCRIBE_BUF(NAME)[SIZE/sizeof(unsigned int)];   \
   unsigned int *TSCRIBE_BUF_TERMINATE(NAME);                   \
   unsigned int *TSCRIBE_BUF_FIRST_WORD(NAME);                  \
   unsigned int *TSCRIBE_BUF_NEXT_WORD(NAME);                   \
   unsigned int TSCRIBE_BUF_NEXT_BIT(NAME);                     \
   bool TSCRIBE_BUF_FULL(NAME)

TSCRIBE_DECLARE_BUF(int, INT_BUF_SIZE);
TSCRIBE_DECLARE_BUF(state, STATE_BUF_SIZE);
TSCRIBE_DECLARE_BUF(compressed_gen, COMPRESSED_GEN_BUF_SIZE);

#define TSCRIBE_INIT_BUF(NAME, SIZE)                                           \
   TSCRIBE_BUF_TERMINATE(NAME) = TSCRIBE_BUF(NAME) + SIZE/sizeof(unsigned int);\
   TSCRIBE_BUF_FIRST_WORD(NAME) = TSCRIBE_BUF(NAME);                           \
   TSCRIBE_BUF_NEXT_WORD(NAME) = TSCRIBE_BUF(NAME);                            \
   TSCRIBE_BUF_NEXT_BIT(NAME) = sizeof(unsigned int)*8 - 1;                    \
   TSCRIBE_BUF_FULL(NAME) = false

#define TSCRIBE_PACK_BITS(NAME, IN, NUMBITS) tscribe_pack_bits( \
   TSCRIBE_BUF(NAME),                                           \
   TSCRIBE_BUF_TERMINATE(NAME),                                 \
   TSCRIBE_BUF_FIRST_WORD(NAME),                                \
   IN,                                                          \
   NUMBITS,                                                     \
   &TSCRIBE_BUF_NEXT_BIT(NAME),                                 \
   &TSCRIBE_BUF_FULL(NAME),                                     \
   &TSCRIBE_BUF_NEXT_WORD(NAME))

#define TSCRIBE_BUF_LENGTH(NAME) tscribe_buf_length( \
		TSCRIBE_BUF(NAME), \
		TSCRIBE_BUF_NEXT_WORD(NAME), \
		TSCRIBE_BUF_FIRST_WORD(NAME), \
		TSCRIBE_BUF_NEXT_BIT(NAME), \
		TSCRIBE_BUF_TERMINATE(NAME), \
		TSCRIBE_BUF_FULL(NAME))

#define TSCRIBE_BUF_TO_FLASH(NAME, ERR) tscribe_buf_to_flash( \
   (unsigned char*)TSCRIBE_BUF(NAME),                         \
   (unsigned char*)TSCRIBE_BUF_TERMINATE(NAME),               \
   TSCRIBE_BUF_NEXT_BIT(NAME),                                \
   &ERR,                                                      \
   &TSCRIBE_BUF_FULL(NAME),                                   \
   (unsigned char**)&TSCRIBE_BUF_FIRST_WORD(NAME))

#define TSCRIBE_BUF_TO_FLASH_IMMEDIATE(NAME, ERR) tscribe_buf_to_flash_immediate( \
   (unsigned char*)TSCRIBE_BUF(NAME),                         \
   (unsigned char*)TSCRIBE_BUF_TERMINATE(NAME),               \
   TSCRIBE_BUF_NEXT_BIT(NAME),                                \
   &ERR,                                                      \
   &TSCRIBE_BUF_FULL(NAME),                                   \
   (unsigned char**)&TSCRIBE_BUF_FIRST_WORD(NAME))

#endif //_BIT_PACK_H
