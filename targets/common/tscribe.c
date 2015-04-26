#include <stdio.h> 
#include "msp430f1611.h"

#include "tscribe-telos.h"
#include "tscribe-bit-pack.h"
#include "tscribe-flash.h"
#include "tscribe.h"

/* flash */
tscribe_mode_t tscribe_mode;
tscribe_flash_state_t tscribe_flash_state;
unsigned long next_flash_block_addr;
action_on_grant_t action_on_grant = NO_ACTION;

/* user button */
tscribe_button_state_t tscribe_button_state;
unsigned int button_count;

/* compression */
unsigned char tscribe_gen_buf1[GEN_BUF_SIZE];
unsigned char tscribe_gen_buf2[GEN_BUF_SIZE];
unsigned char tscribe_gen_buf3[GEN_BUF_SIZE];
unsigned char* tscribe_gen_buf_prev;
unsigned char* tscribe_gen_buf_cur;
bool tscribe_gen_start_compress;

/* state for RLE encoding */
#if defined(TSCRIBE_COUNT_RLE_STATE) || defined(TSCRIBE_LOG_RLE_STATE) \
 || defined(TSCRIBE_COUNT_COMPRESSED_TIMER) || defined(TSCRIBE_LOG_COMPRESSED_TIMER)
unsigned int tscribe_state_values[MAX_STATE_READS];
unsigned char tscribe_state_counts[MAX_STATE_READS];
unsigned int *tscribe_last_TXR = tscribe_state_values;
#endif

/* buffer for timer encoding */
#define TIMER_BUF_SIZE 128
unsigned int tscribe_timer_buf[TIMER_BUF_SIZE];
unsigned int tscribe_timer_first = 0;
unsigned int tscribe_timer_next = 0;
unsigned char tscribe_timer_empty = 1;

/* other state for encoding */
unsigned int tscribe_tar;
unsigned int tscribe_tbr;

/* flush */
bool try_flush = false;

//XXX
/* testing */
//struct bit_pack_test_entry bit_pack_log[BIT_PACK_LOG_CAP];
//unsigned bit_pack_log_next = 0;

/* Call at begining of main. */
void
tscribe_init(void) {
	/* disable watchdog and intialize LEDS */
	TSCRIBE_DISABLE_WATCHDOG();
	TSCRIBE_INIT_LEDS();

	/* initialize bit-pack buffers */
	TSCRIBE_INIT_BUF(int, INT_BUF_SIZE);
	TSCRIBE_INIT_BUF(state, STATE_BUF_SIZE);
	TSCRIBE_INIT_BUF(compressed_gen, COMPRESSED_GEN_BUF_SIZE);

	/* initialize compression buffers */
	tscribe_gen_buf_next = tscribe_gen_buf1;
	tscribe_gen_buf_prev = tscribe_gen_buf2;
	tscribe_gen_buf_cur = tscribe_gen_buf3;
	tscribe_gen_buf_next_idx = 0;
	tscribe_gen_start_compress = false;
	tscribe_lzrw_init();

	/* initialize stat counters */
	tscribe_raw_int_count = 0;
	tscribe_compressed_int_count = 0;

	tscribe_raw_state_count = 0;
	tscribe_compressed_state_count = 0;

	tscribe_raw_timer_count = 0;
	tscribe_compressed_timer_count = 0;

	tscribe_raw_general_count = 0;
	tscribe_compressed_general_count = 0;

	tscribe_loop_count = 0;

	/* initialize user button */
	button_count = 0;

	/* initialize flash */
	next_flash_block_addr = TSCRIBE_FLASH_START;
	tscribe_flash_state = FLASH_FREE;
	tscribe_flash_init();

	/* initialize flash page codes */
	tscribe_int_code = TSCRIBE_INT_PAGE;
	tscribe_state_code = TSCRIBE_STATE_TIMER_PAGE;
	tscribe_gen_code = TSCRIBE_GEN_PAGE;
	tscribe_flush_values_code = TSCRIBE_FLUSH_VALUES_PAGE;
	tscribe_flush_counts_code = TSCRIBE_FLUSH_COUNTS_PAGE;

	/* initialize flash test */
	#ifdef TSCRIBE_TEST_FLASH
	tscribe_next_test_byte = 0;
	#endif

	/* initialize other state */
	tscribe_tar = 0;
	tscribe_tbr = 0;

	/* get mode */
	if (TSCRIBE_USER_BUTTON_PRESSED()) {
      		tscribe_mode = TSCRIBE_INTERACTIVE;
		tscribe_button_state = PRESSED;
	} else {
		tscribe_button_state = OPEN;
		tscribe_mode = TSCRIBE_RECORD;
	}
}

void
tscribe_interactive_mode(void) {
	volatile unsigned int count;
	TSCRIBE_LED_OFF(TSCRIBE_RED_LED);
	TSCRIBE_LED_ON(TSCRIBE_GREEN_LED);
	TSCRIBE_LED_OFF(TSCRIBE_BLUE_LED);
	while(true) {
		if (tscribe_button_state == PRESSED && !TSCRIBE_USER_BUTTON_PRESSED()) {
			for (count=0; count<256; count++);
			if (!TSCRIBE_USER_BUTTON_PRESSED()) {
				tscribe_button_state = OPEN;
				TSCRIBE_LED_OFF(TSCRIBE_BLUE_LED);
			}
		} else if (tscribe_button_state == OPEN && TSCRIBE_USER_BUTTON_PRESSED()) {
			for (count=0; count<256; count++);
			if (TSCRIBE_USER_BUTTON_PRESSED()) {
				tscribe_button_state = PRESSED; 
				tscribe_button_pressed();
				TSCRIBE_LED_ON(TSCRIBE_BLUE_LED);
			}
		}
	}
}

/* Call after system has booted. */
void
tscribe_booted(void) {
      	if (tscribe_mode == TSCRIBE_INTERACTIVE) {
		tscribe_flash_init();
		tscribe_interactive_mode();
	}
}

void
tscribe_gen_buf_shuffel () {
	unsigned char* tmp_next;
	unsigned int s;

	SPLHIGH(s);
	tscribe_gen_buf_next_idx = 0;
	if (tscribe_gen_start_compress == false) {
		tmp_next = tscribe_gen_buf_next;
		tscribe_gen_buf_next = tscribe_gen_buf_prev;
		tscribe_gen_buf_prev = tscribe_gen_buf_cur;
		tscribe_gen_buf_cur = tmp_next;
		tscribe_gen_start_compress = true;
	} else {
/*
		TSCRIBE_LED_ON(TSCRIBE_RED_LED);
		TSCRIBE_LED_ON(TSCRIBE_GREEN_LED);
		TSCRIBE_LED_ON(TSCRIBE_BLUE_LED);
*/
		tscribe_gen_code |= TSCRIBE_ERR_COMPRESSOR_BUSY;
	}
	SPLX(s);
}

unsigned int tscribe_flash_code;
unsigned char *tscribe_flash_terminate1;
unsigned char *tscribe_flash_start2;
unsigned char *tscribe_flash_terminate2;
bool *tscribe_flash_full;
unsigned char **tscribe_flash_first_word;
unsigned int tscribe_flash_header;

/* Assumes that buffer contains at least TSCRIBE_FLASH_DATA_PAYLOAD_SIZE */
inline void
tscribe_buf_to_flash(
	unsigned char *buf,
	unsigned char *terminate,
	unsigned int next_bit,
	/* following are passed by reference, may be modified */
	unsigned int *code,
	bool *full,
	unsigned char **first_word) {

	unsigned char *no_wrap_terminate;
	
	*code |= 0xfe00; // writing 254 bytes
	tscribe_flash_code = *code;
	*code &= TSCRIBE_PAGE_TYPE_MASK; // clear error flags for next time
	tscribe_flash_full = full;
	tscribe_flash_first_word = first_word;

	no_wrap_terminate = *first_word + TSCRIBE_FLASH_DATA_PAYLOAD_SIZE;
	if (no_wrap_terminate >= terminate) {
		/* wraps around end of buffer */
		tscribe_flash_terminate1 = terminate;
		tscribe_flash_start2 = buf;
		tscribe_flash_terminate2 = (unsigned int)buf + ((unsigned int)no_wrap_terminate - (unsigned int)terminate);
	} else {
		tscribe_flash_terminate1 = no_wrap_terminate;
		tscribe_flash_start2 = tscribe_flash_terminate1;
		tscribe_flash_terminate2 = tscribe_flash_terminate1;
	}

	action_on_grant = PROGRAM_PAGE;
	tscribe_flash_state = FLASH_SCHEDULED;
        tscribe_request_bus();

	//code 1024
	//printf("\ncode %u", tscribe_flash_code);

	//printf("<f %p t1 %p s2 %p t2 %p c %u>\n",
	//	*tscribe_flash_first_word,
	//	tscribe_flash_terminate1,
	//	tscribe_flash_start2,
	//	tscribe_flash_terminate2,
	//	tscribe_flash_code);

	/* when request granted */
	#define DO_PROGRAM_PAGE					\
	tscribe_flash_program_page(				\
		next_flash_block_addr,				\
		*tscribe_flash_first_word,			\
		tscribe_flash_terminate1,			\
		tscribe_flash_start2,				\
		tscribe_flash_terminate2,			\
		tscribe_flash_code);

	/* after page is programed */
	#define AFTER_PROGRAM_PAGE				\
	*tscribe_flash_first_word = tscribe_flash_terminate2;	\
	next_flash_block_addr += TSCRIBE_FLASH_PAGE_SIZE;	\
	if (next_flash_block_addr >= TSCRIBE_FLASH_SIZE) {	\
		next_flash_block_addr = 0;			\
	}							\
	*tscribe_flash_full = false;				\
	tscribe_flash_state = FLASH_WIP;
}

/* Assumes that buffer contains at least TSCRIBE_FLASH_DATA_PAYLOAD_SIZE,
   and that flash is ready to be written to. */
inline void
tscribe_buf_to_flash_immediate(
	unsigned char *buf,
	unsigned char *terminate,
	unsigned int next_bit,
	/* following are passed by reference, may be modified */
	unsigned int *code,
	bool *full,
	unsigned char **first_word) {

	unsigned char *no_wrap_terminate;
	
	*code |= 0xfe00; // writing 254 bytes
	tscribe_flash_code = *code;
	*code &= TSCRIBE_PAGE_TYPE_MASK; // clear error flags for next time
	tscribe_flash_full = full;
	tscribe_flash_first_word = first_word;

	no_wrap_terminate = *first_word + TSCRIBE_FLASH_DATA_PAYLOAD_SIZE;
	if (no_wrap_terminate >= terminate) {
		/* wraps around end of buffer */
		tscribe_flash_terminate1 = terminate;
		tscribe_flash_start2 = buf;
		tscribe_flash_terminate2 = (unsigned int)buf + ((unsigned int)no_wrap_terminate - (unsigned int)terminate);
	} else {
		tscribe_flash_terminate1 = no_wrap_terminate;
		tscribe_flash_start2 = tscribe_flash_terminate1;
		tscribe_flash_terminate2 = tscribe_flash_terminate1;
	}

	tscribe_flash_program_page(
		next_flash_block_addr,
		*tscribe_flash_first_word,
		tscribe_flash_terminate1,
		tscribe_flash_start2,
		tscribe_flash_terminate2,
		tscribe_flash_code);

	*tscribe_flash_first_word = tscribe_flash_terminate2;
	next_flash_block_addr += TSCRIBE_FLASH_PAGE_SIZE;
	if (next_flash_block_addr >= TSCRIBE_FLASH_SIZE) {
		next_flash_block_addr = 0;
	}
	*tscribe_flash_full = false;
	//tscribe_flash_state = FLASH_WIP;
}

/* Used for testing flash write speed */
void
tscribe_write_test_flash (void) {
	unsigned char *flash_read_buf = TSCRIBE_BUF(int);
	unsigned long addr;
	unsigned c;

	putchar('s');
	tscribe_max_dco();

	for (addr=TSCRIBE_FLASH_START; addr<TSCRIBE_FLASH_START+10000; addr+=TSCRIBE_FLASH_PAGE_SIZE) {
		while (tscribe_read_status_register());
		tscribe_flash_program_page(
			addr,
			flash_read_buf,
			flash_read_buf + TSCRIBE_FLASH_PAGE_SIZE - 2,
			0, 0, 0);
	}

	tscribe_restore_dco();
	putchar('e');
	putchar('\r');
	putchar('\n');

	while(true);
}

void
tscribe_print_flash (void) {
	unsigned char *flash_read_buf = TSCRIBE_BUF(int);
	unsigned long addr;
	unsigned c;

	putchar('\r');
	putchar('\n');
	putchar('r');

	for (addr=TSCRIBE_FLASH_START; addr<TSCRIBE_FLASH_SIZE; addr+=TSCRIBE_FLASH_PAGE_SIZE) {
		putchar('\r');
		putchar('\n');

		tscribe_flash_pread(flash_read_buf, TSCRIBE_FLASH_PAGE_SIZE, addr);

		if (flash_read_buf[0] == 0 && flash_read_buf[1] == 0) {
			break;
		}

		for (c=0; c<TSCRIBE_FLASH_PAGE_SIZE; c++) {
			putchar(flash_read_buf[c]);
		}
	}

	putchar(0);
	putchar(0);
	putchar('\r');
	putchar('\n');
}

void
tscribe_button_pressed(void) {
	if (tscribe_mode == TSCRIBE_INTERACTIVE) {
		button_count++;
		if (button_count == 1) {
			TSCRIBE_LED_ON(TSCRIBE_RED_LED);
			TSCRIBE_LED_OFF(TSCRIBE_GREEN_LED);
			TSCRIBE_LED_OFF(TSCRIBE_BLUE_LED);
	
			tscribe_print_flash();
		
			TSCRIBE_LED_OFF(TSCRIBE_RED_LED);
			TSCRIBE_LED_ON(TSCRIBE_GREEN_LED);
			TSCRIBE_LED_OFF(TSCRIBE_BLUE_LED);
		} else if (button_count == 3) {
			TSCRIBE_LED_ON(TSCRIBE_RED_LED);
			TSCRIBE_LED_OFF(TSCRIBE_GREEN_LED);
			TSCRIBE_LED_OFF(TSCRIBE_BLUE_LED);

			tscribe_flash_erase(TSCRIBE_FLASH_SIZE, 0);
			//tscribe_write_test_flash();

			TSCRIBE_LED_OFF(TSCRIBE_RED_LED);
			TSCRIBE_LED_ON(TSCRIBE_GREEN_LED);
			TSCRIBE_LED_OFF(TSCRIBE_BLUE_LED);
		}
	}
}

/* Pointer to the code that is to be asynchronously updated on the completion
   of DO_CHECK_FLASH_STATUS. */
unsigned int *check_flash_code;

volatile unsigned int tscribe_status_register;

/* Schedules a check of the status of flash (i.e., is it in WIP mode). Returns
   immediatly, before check is completed. When check is completed the value of
   tscribe_flash_state is updated.

   Precondition(s):
      tscribe_flash_state == FLASH_WIP && 
      action_on_grant == NO_ACTION

   Postcondition(s):
      Value pointed to by code_ptr is asynchronously updated.
      tscribe_flash_state: is asynchronously updated.
*/
inline void
tscribe_check_flash(unsigned int *code_ptr) {
	action_on_grant = CHECK_FLASH_STATUS;
	check_flash_code = code_ptr;
	tscribe_request_bus();

	#define DO_CHECK_FLASH_STATUS {						\
		tscribe_status_register = tscribe_read_status_register();	\
		tscribe_record_status_register(tscribe_status_register);	\
		if (tscribe_status_register == 0) {				\
			tscribe_flash_state = FLASH_FREE;			\
		} else {							\
			*check_flash_code |= TSCRIBE_WRN_FLASH_BUSY;		\
		}								\
	}
	
	#define AFTER_CHECK_FLASH_STATUS
}

inline void
tscribe_pack_timer() {
	unsigned int s;
	SPLHIGH(s);

	while (!tscribe_timer_empty) {
		unsigned int d = tscribe_timer_buf[tscribe_timer_first];
		if (d < 4) {
			#ifdef TSCRIBE_COUNT_COMPRESSED_TIMER
			{
				//unsigned int size = 2 /* prefix */
				//	+ 5 /* delta */;
				unsigned int size = 1 /* prefix */
					+ 2 /* delta */;
				tscribe_compressed_timer_count += size;
				#ifdef TSCRIBE_COUNT_PER_REG
				tscribe_reg_count[addr] += size;
				#endif
			}
			#endif
			#ifdef TSCRIBE_LOG_COMPRESSED_TIMER
			// d = delta
			// 10dd ddd
			//tscribe_state_code |= TSCRIBE_PACK_BITS(state, (1 << 6) | d, 7);
			// 0dd
			tscribe_state_code |= TSCRIBE_PACK_BITS(state, d, 3);
			#endif
		} else if (d < 64) {
			#ifdef TSCRIBE_COUNT_COMPRESSED_TIMER
			{
				//unsigned int size = 3 /* prefix */
				//	+ 8 /* delta */;
				unsigned int size = 2 /* prefix */
					+ 6 /* delta */;
				tscribe_compressed_timer_count += size;
				#ifdef TSCRIBE_COUNT_PER_REG
				tscribe_reg_count[addr] += size;
				#endif
			}
			#endif
			#ifdef TSCRIBE_LOG_COMPRESSED_TIMER
			// d = delta
			// 110d dddd ddd
			//tscribe_state_code |= TSCRIBE_PACK_BITS(state, (3 << 9) | d, 11);
			// 10dd dddd
			tscribe_state_code |= TSCRIBE_PACK_BITS(state, (1 << 7) | d, 8);
			#endif
		} else {
			#ifdef TSCRIBE_COUNT_COMPRESSED_TIMER
			{
				unsigned int size = 3 /* prefix */ 
					+ 16 /* delta */;
				tscribe_compressed_timer_count += size;
				#ifdef TSCRIBE_COUNT_PER_REG
				tscribe_reg_count[addr] += size;
				#endif
			}
			#endif
			#ifdef TSCRIBE_LOG_COMPRESSED_TIMER
			// d = delta
			// 111d dddd dddd dddd ddd
			//tscribe_state_code |= TSCRIBE_PACK_BITS(state, 7, 3);
			//tscribe_state_code |= TSCRIBE_PACK_BITS(state, d, 16);
			// 110d dddd dddd dddd ddd
			tscribe_state_code |= TSCRIBE_PACK_BITS(state, 6, 3);
			tscribe_state_code |= TSCRIBE_PACK_BITS(state, d, 16);
			#endif
		}
		tscribe_timer_first = (tscribe_timer_first + 1) % TIMER_BUF_SIZE;
		if (tscribe_timer_next == tscribe_timer_first) {
			tscribe_timer_empty = 1;
		}
	}

	SPLX(s);
}

unsigned int test_count = 0;

/* Checks if buffers need to be compressed or dumped to flash. This function
   should be called often by the system (e.g., after the completion of every
   task and interrupt).

   Returns true if some action was performed. This should be used before
   putting the node to sleep (i.e.,
      ...
      while(tscribe_update());
      sleep();
      ...
   ).
*/
//inline
unsigned char
tscribe_update(void) {
	unsigned char keep_awake = 0;
	unsigned length;

	if (tscribe_mode != TSCRIBE_RECORD) {
		return;
	}

	#ifdef TSCRIBE_TEST_FLASH
	if (test_count++ == 1024) {
		#ifdef TSCRIBE_TEST_INT
		//printf("%u\n", tscribe_test_seq[tscribe_next_test_byte]);
		tscribe_int_code |= TSCRIBE_PACK_BITS(int, tscribe_test_seq[tscribe_next_test_byte], 8);
		if (tscribe_next_test_byte == sizeof(tscribe_test_seq)) tscribe_next_test_byte = 0;
		#endif

		#ifdef TSCRIBE_TEST_GEN
		tscribe_gen_buf_next[tscribe_gen_buf_next_idx] = tscribe_test_seq[tscribe_next_test_byte];
		tscribe_gen_buf_next_idx++;
		if (tscribe_gen_buf_next_idx == GEN_BUF_SIZE) {
			//printf("shuffel\r\n");
			tscribe_gen_buf_shuffel();
		} 
		#endif

		if (++tscribe_next_test_byte == sizeof(tscribe_test_seq)) tscribe_next_test_byte = 0;
		test_count = 0;
	} else
	#endif

	#if defined(TSCRIBE_COUNT_COMPRESSED_TIMER) || defined(TSCRIBE_LOG_COMPRESSED_TIMER)
	if (!tscribe_timer_empty) {
		tscribe_pack_timer();
		keep_awake = 1;
	} else 
	#endif
	if (tscribe_gen_start_compress) {
		//printf("compress\n");
		tscribe_max_dco();
		tscribe_gen_code |= tscribe_lzrw_compress (tscribe_gen_buf_cur, GEN_BUF_SIZE);
		tscribe_gen_start_compress = false;
		keep_awake = 1;
		tscribe_restore_dco();
	} else if (TSCRIBE_BUF_LENGTH(compressed_gen) > TSCRIBE_FLASH_DATA_PAYLOAD_SIZE) {
		#ifdef TSCRIBE_LOG_COMPRESSED_GENERAL
		if (tscribe_flash_state == FLASH_FREE) {
			//printf("compressed buffer to flash\r\n");
			TSCRIBE_BUF_TO_FLASH(compressed_gen, tscribe_gen_code);
			#ifdef TSCRIBE_COUNT_COMPRESSED_GENERAL
			tscribe_compressed_general_count += (TSCRIBE_FLASH_DATA_PAYLOAD_SIZE * 8);
			#endif
			keep_awake = 1;
		} else if ((tscribe_flash_state == FLASH_WIP) && (action_on_grant == NO_ACTION)) {
// XXX TODO: how often do we check flash
			tscribe_check_flash(&tscribe_gen_code);
			keep_awake = 1;
			tscribe_gen_code |= TSCRIBE_WRN_FLASH_BUSY;
		}
		#else
		TSCRIBE_BUF_FIRST_WORD(compressed_gen) =
			((((TSCRIBE_BUF_FIRST_WORD(compressed_gen) - TSCRIBE_BUF(compressed_gen))
			+ TSCRIBE_FLASH_DATA_PAYLOAD_SIZE) % COMPRESSED_GEN_BUF_SIZE)
			+ TSCRIBE_BUF_FIRST_WORD(compressed_gen));
		tscribe_compressed_general_count += (TSCRIBE_FLASH_DATA_PAYLOAD_SIZE * 8);
		#endif
	} else if (TSCRIBE_BUF_LENGTH(int) > TSCRIBE_FLASH_DATA_PAYLOAD_SIZE) {
		if (tscribe_flash_state == FLASH_FREE) {
			//printf("int buffer to flash\r\n");
			TSCRIBE_BUF_TO_FLASH(int, tscribe_int_code);
			keep_awake = 1;
		} else if ((tscribe_flash_state == FLASH_WIP) && (action_on_grant == NO_ACTION)) {
			tscribe_check_flash(&tscribe_int_code);
			keep_awake = 1;
		} else {
			tscribe_int_code |= TSCRIBE_WRN_FLASH_BUSY;
		}
	} else if (TSCRIBE_BUF_LENGTH(state) > TSCRIBE_FLASH_DATA_PAYLOAD_SIZE) {
		if (tscribe_flash_state == FLASH_FREE) {
			TSCRIBE_BUF_TO_FLASH(state, tscribe_state_code);
			keep_awake = 1;
		} else if ((tscribe_flash_state == FLASH_WIP) && (action_on_grant == NO_ACTION)) {
			tscribe_check_flash(&tscribe_state_code);
			keep_awake = 1;
		} else {
			tscribe_state_code |= TSCRIBE_WRN_FLASH_BUSY;
		}
	} else if (try_flush) {
		tscribe_flush_state();
	}

	return keep_awake;
}

void
tscribe_flush_state() {
	if (tscribe_flash_state == FLASH_FREE || tscribe_flash_state == FLASH_WIP) {
		//printf("tscribe_flush_state requesting bus\n");
		tscribe_flash_state == FLASH_SCHEDULED_FLUSH;
		action_on_grant = FLUSH_STATE;
		tscribe_request_bus();
		try_flush = false;
	} else {
		//printf("tscribe_flush_state try again\n");
		try_flush = true;
	}

	/* when request granted */
        #define DO_FLUSH_STATE \
	{ \
		unsigned int i; \
		unsigned int *buf; \
		unsigned int *no_wrap_terminate; \
		unsigned int *terminate; \
		unsigned int len; \
		tscribe_mode = TSCRIBE_FLUSH; \
		/* write anything remaining in state/timer buffer */ \
/* XXX */ \
tscribe_restore_dco(); \
/* XXX 
unsigned int *pi; \
for (i=0; i<BIT_PACK_LOG_CAP; i++) { \
printf("b %02x %u %u\n", bit_pack_log[i].word, bit_pack_log[i].value, bit_pack_log[i].size); \
} \
for (pi=TSCRIBE_BUF(state); pi<TSCRIBE_BUF_TERMINATE(state); pi++) { \
printf("d %04x\n", *pi); \
} \
*/ \
		tscribe_pack_timer(); \
		len = TSCRIBE_BUF_LENGTH(state); \
		for (i=0; i < STATE_BUF_SIZE - len; i++) { \
			TSCRIBE_PACK_BITS(state, 0, 8); \
		} \
		while (TSCRIBE_BUF_LENGTH(state) > TSCRIBE_FLASH_DATA_PAYLOAD_SIZE) { \
			while (tscribe_read_status_register());	\
			TSCRIBE_BUF_TO_FLASH_IMMEDIATE(state, tscribe_state_code); \
		} \
		/* write state tables */ \
		for (i=0; i<MAX_STATE_READS; i+=127) { \
			while (tscribe_read_status_register()); \
			tscribe_flash_program_page( \
				next_flash_block_addr, \
				&tscribe_state_values[i], \
				&tscribe_state_values[i+127], \
				0, \
				0, \
				tscribe_flush_values_code | 0xfe00); \
			next_flash_block_addr += TSCRIBE_FLASH_PAGE_SIZE; \
			if (next_flash_block_addr >= TSCRIBE_FLASH_SIZE) { \
				next_flash_block_addr = 0; \
			} \
		} \
		for (i=0; i<MAX_STATE_READS; i+=254) { \
			while (tscribe_read_status_register()); \
			tscribe_flash_program_page( \
				next_flash_block_addr, \
				&tscribe_state_counts[i], \
				&tscribe_state_counts[i+254], \
				0, \
				0, \
				tscribe_flush_counts_code | 0xfe00); \
			next_flash_block_addr += TSCRIBE_FLASH_PAGE_SIZE; \
			if (next_flash_block_addr >= TSCRIBE_FLASH_SIZE) { \
				next_flash_block_addr = 0; \
			} \
		} \
		/* write general to flash */ \
		tscribe_gen_buf_shuffel(); \
		tscribe_gen_code |= tscribe_lzrw_compress (tscribe_gen_buf_cur, GEN_BUF_SIZE); \
		len = TSCRIBE_BUF_LENGTH(compressed_gen); \
		for (i=0; i < GEN_BUF_SIZE - len; i++) { \
			TSCRIBE_PACK_BITS(compressed_gen, 0, 8); \
		} \
		while (TSCRIBE_BUF_LENGTH(compressed_gen) > TSCRIBE_FLASH_DATA_PAYLOAD_SIZE) { \
			while (tscribe_read_status_register()); \
			TSCRIBE_BUF_TO_FLASH_IMMEDIATE(compressed_gen, tscribe_gen_code); \
		} \
	}

	/* after page is programed */
	#define AFTER_FLUSH_STATE
}

void
tscribe_bus_granted (void) {
	unsigned int s;

	SPLHIGH(s);
	tscribe_max_dco();

	tscribe_flash_init();

	switch (action_on_grant) {
	case PROGRAM_PAGE:
		DO_PROGRAM_PAGE
		tscribe_release_bus();
		AFTER_PROGRAM_PAGE
		break;
	case CHECK_FLASH_STATUS:
		DO_CHECK_FLASH_STATUS
		tscribe_release_bus();
		AFTER_CHECK_FLASH_STATUS
		break;
	#ifdef TSCRIBE_LOG_RLE_STATE
	case FLUSH_STATE:
		DO_FLUSH_STATE
		tscribe_release_bus();
		AFTER_FLUSH_STATE
		break;
	#endif
	default:
		tscribe_release_bus();
	}

	action_on_grant = NO_ACTION;

	tscribe_restore_dco();
	SPLX(s);
}

//inline
void
tscribe_record_int(unsigned int vector, void *return_address) {
	#ifdef TSCRIBE_COUNT_RAW_INT
	tscribe_raw_int_count += 8 /* vector */
		+ 16 /* return address */
		+ 16 /* loop counter */;
        #endif

	#ifdef TSCRIBE_SIMLOG_RAW_INT
	LOG_EMULATION(vector);
	LOG_EMULATION((unsigned int)return_address);
	LOG_EMULATION(((unsigned int)return_address) >> 8);
	LOG_EMULATION(tscribe_loop_count);
	LOG_EMULATION(tscribe_loop_count >> 8);
	#endif

	#if defined(TSCRIBE_COUNT_COMPRESSED_TIMER) || defined(TSCRIBE_LOG_COMPRESSED_TIMER)
/* need to be careful when capture is used
	switch (vector) {
		case 6 * 2: // timerA0
			tscribe_tar = TACCR0;
			break;
		case 5 * 2: // timerA1
			tscribe_tar = TACCR1;
			break;
		case 13 * 2: // timerB0
			tscribe_tbr = TBCCR0;
			break;
		case 12 * 2: // timerB1
			tscribe_tbr = TBCCR1;
			break;
		defalut: // do nothing
	}
*/
	#endif

	#if defined(TSCRIBE_COUNT_COMPRESSED_INT) || defined(TSCRIBE_LOG_COMPRESSED_INT)
	if (tscribe_loop_count == 0) {
		#ifdef TSCRIBE_COUNT_COMPRESSED_INT
		tscribe_compressed_int_count += 1 /* prefix */
			+ 4 /* vector */;
		#endif
		#ifdef TSCRIBE_LOG_COMPRESSED_INT
		// v = vector/2
		// 0vvv v
		tscribe_int_code |= TSCRIBE_PACK_BITS(int, (vector >> 1), 5);
		#endif
	} else if (tscribe_loop_count < 256) {
		#ifdef TSCRIBE_COUNT_COMPRESSED_INT
		tscribe_compressed_int_count += 2 /* prefix */
			+ 4 /* vector */
			+ 16 /* return address */
			+ 8 /* loop count */;
		#endif
		#ifdef TSCRIBE_LOG_COMPRESSED_INT
		// l = loop count
		// r = return address
		// v = vector/2
		// 10vv vvrr rrrr rrrr rrrr rrll llll ll
		tscribe_int_code |= TSCRIBE_PACK_BITS(int, 0x8000 | (vector << 9) | ((unsigned int)return_address >> 6), 16);
		tscribe_int_code |= TSCRIBE_PACK_BITS(int, (((unsigned int)return_address << 8) | tscribe_loop_count) & 0x3fff, 14);
		#endif
	} else {
		#ifdef TSCRIBE_COUNT_COMPRESSED_INT
		tscribe_compressed_int_count += 2 /* prefix */
			+ 4 /* vector */
			+ 16 /* return address */
			+ 16 /* loop count */;
		#endif
		#ifdef TSCRIBE_LOG_COMPRESSED_INT
		// v = vector/2
		// r = return address
		// l = loop count
		// 11vv vvrr rrrr rrrr rrrr rrll llll llll llll ll
		tscribe_int_code |= TSCRIBE_PACK_BITS(int, 0xC000 | (vector << 9) | ((unsigned int)return_address >> 6), 16);
		tscribe_int_code |= TSCRIBE_PACK_BITS(int, ((unsigned int)return_address << 10) | (tscribe_loop_count >> 6), 16);
		tscribe_int_code |= TSCRIBE_PACK_BITS(int, tscribe_loop_count & 0x003f, 6);
		#endif
	}
	#endif

	tscribe_loop_count = 1;
}

inline void
tscribe_record_status_register(unsigned int status) {
	#ifdef TSCRIBE_SIMLOG_RAW_STATE
	LOG_EMULATION(status);
	#endif

	#ifdef TSCRIBE_COUNT_RAW_STATE
	tscribe_raw_state_count += 8;
	#endif

	#ifdef TSCRIBE_COUNT_RLE_STATE
	tscribe_compressed_state_count += 10;
	#endif

	#ifdef TSCRIBE_LOG_RLE_STATE
	// s = status
	// 0111 1111 11s
	//tscribe_state_code |= TSCRIBE_PACK_BITS(state, 0x3fe | (status == 0 ? 0 : 1), 11);
	// 1111 1111 1s
	tscribe_state_code |= TSCRIBE_PACK_BITS(state, 0x3fe | (status == 0 ? 0 : 1), 10);
	#endif
}

/* Records a read from a state register. The entier register value is not
   recorded, only numbits are recorded after the following operation:
   (value & mask) >> shift
*/
//inline
void
tscribe_record_state(
	unsigned int value, /* the value read from the register */
	unsigned int idx, /* a number that uniquly identifies this register read from
	                     all others in the code */
	unsigned int addr, /* the address of the register */
	unsigned int mask, /* set only for non-deterministic bits */
	unsigned int numbits, /* the number of bits to record */
	unsigned int shift /* the number of bits to shift right before recording */) {

	unsigned int s;
	SPLHIGH(s);

//XXX
//if (lock_test == 1) {
//	printf("ERROR ERROR ERROR\n");
//}
//lock_test = 1;

	#ifdef TSCRIBE_COUNT_RAW_STATE
	if (numbits > 8) {
		unsigned int size = 16 /* 16-bit register */;
		tscribe_raw_state_count += size;
		#ifdef TSCRIBE_COUNT_PER_REG
		tscribe_reg_count[addr] += size;
		#endif
	} else {
		unsigned int size = 8 /* 8-bit register */;
		tscribe_raw_state_count += size;
		#ifdef TSCRIBE_COUNT_PER_REG
		tscribe_reg_count[addr] += size;
		#endif
	}
        #endif

	#ifdef TSCRIBE_SIMLOG_RAW_STATE
	if (numbits > 8) {
		LOG_EMULATION(value);
	} else {
		LOG_EMULATION(value);
		LOG_EMULATION(value >> 8);
	}
	#endif
	
	#ifdef TSCRIBE_COUNT_COMPRESSED_STATE
	{
		tscribe_compressed_state_count += numbits;
		#ifdef TSCRIBE_COUNT_PER_REG
		tscribe_reg_count[addr] += numbits;
		#endif
	}
	#endif

	//idx = addr;
	unsigned int encode_value = (value & mask) >> shift;
	#if defined(TSCRIBE_COUNT_RLE_STATE) || defined(TSCRIBE_LOG_RLE_STATE)
	unsigned int current_value = tscribe_state_values[idx];
        if (encode_value != current_value || tscribe_state_counts[idx] == 255) {
		unsigned int i;
		unsigned int count = tscribe_state_counts[idx];

		#ifdef TSCRIBE_COUNT_RLE_STATE
		unsigned int size = 3 
			+ 6 /* idx */
			+ 8 /* count */
			+ numbits /* numbits */;
		tscribe_compressed_state_count += size;

			#ifdef TSCRIBE_COUNT_PER_REG
			tscribe_reg_count[addr] += size;
			#endif
		#endif

		#ifdef TSCRIBE_LOG_RLE_STATE
		// i = index
		// c = count
		// v = value
		// 0iii iiii iicc cccc ccv*
		/* loop version 
		for (i=0; i<3; i++) {
			unsigned int pack, bits;
			if (i == 0) {
				pack = idx;
				bits = 10;
			} else if (i == 1) {
				pack = counts;
				bits = 8;
			} else {
				pack = current_value;
				bits = numbits;
			}
			tscribe_state_code |= TSCRIBE_PACK_BITS(state, pack, bits);
		}
		*/
		// 111i iiii iccc cccc cv*
		/* loop unrolled version */
		tscribe_state_code |= TSCRIBE_PACK_BITS(state, (7 << 6) | idx, 9);
		tscribe_state_code |= TSCRIBE_PACK_BITS(state, count, 8);
		tscribe_state_code |= TSCRIBE_PACK_BITS(state, current_value, numbits);
		#endif

		tscribe_state_values[idx] = encode_value;
		tscribe_state_counts[idx] = 1;
	} else {
		tscribe_state_counts[idx]++;
	}
	#endif

	#ifdef TSCRIBE_LOG_COMPRESSED_STATE
//XXX
//BIT_PACK_LOG(encode_value, numbits);
	tscribe_state_code |= TSCRIBE_PACK_BITS(state, encode_value, numbits);
	#endif

//XXX
//lock_test = 0;

	SPLX(s);
}

inline void
tscribe_record_timer(
	unsigned int value,
	unsigned int idx,
	unsigned int addr) {

	unsigned int s;
	SPLHIGH(s);

	#ifdef TSCRIBE_COUNT_RAW_TIMER
	{
		unsigned int size = 16 /* 16-bit register */;
		tscribe_raw_timer_count += size;
		#ifdef TSCRIBE_COUNT_PER_REG
		tscribe_reg_count[addr] += size;
		#endif
	}
        #endif

	#ifdef TSCRIBE_SIMLOG_RAW_TIMER
	LOG_EMULATION(value);
	LOG_EMULATION(value >> 8);
	#endif
	
	#if defined(TSCRIBE_COUNT_COMPRESSED_TIMER) || defined(TSCRIBE_LOG_COMPRESSED_TIMER)
	unsigned int d;
	if (addr == 0x0170) { // TAR
		d = value - tscribe_tar;
		tscribe_tar = value;	
	} else { // TBR
		d = value - tscribe_tbr;
		tscribe_tbr = value;	
	}
	if ((tscribe_timer_next != tscribe_timer_first) || tscribe_timer_empty) {
		tscribe_timer_buf[tscribe_timer_next] = d;
		tscribe_timer_next = (tscribe_timer_next + 1) % TIMER_BUF_SIZE;
		tscribe_timer_empty = 0;
	} else {
		tscribe_state_code |= TSCRIBE_ERR_BIT_PACK_OVERFLOW;
		TSCRIBE_LED_ON(TSCRIBE_RED_LED);
		TSCRIBE_LED_ON(TSCRIBE_GREEN_LED);
		TSCRIBE_LED_ON(TSCRIBE_BLUE_LED);
	}
	#endif

	SPLX(s);
}

inline void
tscribe_record_general_byte(
	unsigned char value,
	unsigned int addr) {

	unsigned int s;
	SPLHIGH(s);

	#ifdef TSCRIBE_COUNT_RAW_GENERAL
	{
		unsigned int size = 8 /* value */;
		tscribe_raw_general_count += size;
		#ifdef TSCRIBE_COUNT_PER_REG
		tscribe_reg_count[addr] += size;
		#endif
	}
        #endif

	#ifdef TSCRIBE_SIMLOG_RAW_GENERAL
	LOG_EMULATION(value);
	#endif
	
	#if defined (TSCRIBE_LOG_COMPRESSED_GENERAL) || defined (TSCRIBE_COUNT_COMPRESSED_GENERAL)
	tscribe_gen_buf_next[tscribe_gen_buf_next_idx] = value;
	tscribe_gen_buf_next_idx++;
	if (tscribe_gen_buf_next_idx == GEN_BUF_SIZE) {
		tscribe_gen_buf_shuffel();
	} 
	#endif

	SPLX(s);
}

inline void
tscribe_record_general_word(unsigned int value, unsigned int addr) {
	tscribe_record_general_byte((unsigned char)value, addr);
        tscribe_record_general_byte((unsigned char)(value >> 8), addr);
}

