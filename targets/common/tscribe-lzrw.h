#ifndef _LZRW_H
#define _LZRW_H

void tscribe_lzrw_init (bit_buffer_t *out);
unsigned tscribe_lzrw_compress (unsigned char *in, unsigned int in_length);

#endif //_LZRW_H
