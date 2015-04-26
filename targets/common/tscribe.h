#ifndef _TSCRIBE_LOGGER_H
#define _TSCRIBE_LOGGER_H

//#define TSCRIBE_TEST_FLASH
//#define TSCRIBE_TEST_INT
//#define TSCRIBE_TEST_GEN

//#define TSCRIBE_COUNT_PER_REG

//#define TSCRIBE_COUNT_RAW_INT
//#define TSCRIBE_SIMLOG_RAW_INT
//#define TSCRIBE_COUNT_COMPRESSED_INT
#define TSCRIBE_LOG_COMPRESSED_INT

//#define TSCRIBE_COUNT_RAW_STATE
//#define TSCRIBE_SIMLOG_RAW_STATE
//#define TSCRIBE_COUNT_COMPRESSED_STATE
//#define TSCRIBE_COUNT_RLE_STATE
//#define TSCRIBE_LOG_COMPRESSED_STATE
#define TSCRIBE_LOG_RLE_STATE

//#define TSCRIBE_COUNT_RAW_TIMER
//#define TSCRIBE_SIMLOG_RAW_TIMER
//#define TSCRIBE_COUNT_COMPRESSED_TIMER
#define TSCRIBE_LOG_COMPRESSED_TIMER

//#define TSCRIBE_COUNT_RAW_GENERAL
//#define TSCRIBE_SIMLOG_RAW_GENERAL
//#define TSCRIBE_COUNT_COMPRESSED_GENERAL
#define TSCRIBE_LOG_COMPRESSED_GENERAL

//XXX
/* testing */
//#define BIT_PACK_LOG_CAP 512
//struct bit_pack_test_entry {
//	unsigned int value;
//	unsigned int size;	
//	unsigned char word;
//};
//#define BIT_PACK_LOG(V, S) \
//if (bit_pack_log_next < BIT_PACK_LOG_CAP) { \
//	bit_pack_log[bit_pack_log_next].value = V; \
//	bit_pack_log[bit_pack_log_next].size = S; \
//	bit_pack_log[bit_pack_log_next].word = (unsigned char) tscribe_state_buf_next_word; \
//	bit_pack_log_next++; \
//}
//extern struct bit_pack_test_entry bit_pack_log[BIT_PACK_LOG_CAP];
//extern unsigned bit_pack_log_next;

/******************************************************************************
 * Emulator
 */
#define EMULATOR_LOG_PORT 0xfffa
#define EMULATOR_COM_PORT 0xfffc
#define IS_EMULATION (*((unsigned int*)EMULATOR_COM_PORT))
#define LOG_EMULATION(B) { *((unsigned int*)EMULATOR_LOG_PORT) = B; }

/******************************************************************************
 * Flash
 */
#define TSCRIBE_FLASH_START 0
#define TSCRIBE_FLASH_PAGE_SIZE 256
#define TSCRIBE_FLASH_DATA_PAYLOAD_SIZE 254
#define TSCRIBE_FLASH_SECTOR_SIZE (TSCRIBE_FLASH_PAGE_SIZE * 256L)
#define TSCRIBE_FLASH_SIZE (TSCRIBE_FLASH_SECTOR_SIZE * 16L)

#define TSCRIBE_ERR_BUFF_NOT_READY (1<<4)
#define TSCRIBE_ERR_BIT_PACK_OVERFLOW (1<<5)
#define TSCRIBE_WRN_FLASH_BUSY (1<<6)
#define TSCRIBE_ERR_COMPRESSOR_BUSY (1<<7)

#define TSCRIBE_GEN_PAGE 0
#define TSCRIBE_STATE_TIMER_PAGE 1
#define TSCRIBE_INT_PAGE 2
#define TSCRIBE_FLUSH_VALUES_PAGE 3
#define TSCRIBE_FLUSH_COUNTS_PAGE 4

#define TSCRIBE_PAGE_TYPE_MASK (TSCRIBE_GEN_PAGE | TSCRIBE_STATE_TIMER_PAGE | TSCRIBE_INT_PAGE | TSCRIBE_FLUSH_VALUES_PAGE | TSCRIBE_FLUSH_COUNTS_PAGE)

unsigned int tscribe_int_code;
unsigned int tscribe_state_code;
unsigned int tscribe_gen_code;
unsigned int tscribe_flush_values_code;
unsigned int tscribe_flush_counts_code;

typedef enum {
	TSCRIBE_REPLAY = 1, // mode where emulator is replaying trace
	TSCRIBE_RECORD = 2, // mode where execution is being recorded
	TSCRIBE_INTERACTIVE = 3, // mode where user can dump and erase flash
	TSCRIBE_FLUSH = 4, // set when flushing buffers to flash
} tscribe_mode_t;

typedef enum {
	NO_ACTION,
	PROGRAM_PAGE,
	CHECK_FLASH_STATUS,
	FLUSH_STATE
} action_on_grant_t; 

typedef enum {
	FLASH_FREE,
	FLASH_SCHEDULED,
	FLASH_SCHEDULED_FLUSH,
	FLASH_WIP /* write in progress */
} tscribe_flash_state_t; 

void tscribe_interactive_mode(void);

/******************************************************************************
 * System calls
 */

/* OS specific functions defined in targets/OS. */
void tscribe_request_bus(void);
void tscribe_release_bus(void);

/* Callback for tscribe_request_bus */
void tscribe_bus_granted(void);

/* The OS must call these functions.
   tscribe_init - at the beging of main
   tscribe_booted - after the system is configured
   tscribe_update - on a regular basis, e.g., after every task completion
   tscribe_button_pressed - notification that the user button has been pressed */
void tscribe_init(void);
void tscribe_booted(void);
inline unsigned char tscribe_update(void);
void tscribe_flush_state(void);
void tscribe_button_pressed(void);

typedef enum {
	PRESSED,
	OPEN
} tscribe_button_state_t; 

/******************************************************************************
 * Logging
 */

#define MAX_STATE_READS 64
//#define MAX_STATE_READS 256

unsigned char* tscribe_gen_buf_next;
unsigned int tscribe_gen_buf_next_idx;

/******************************************************************************
 * Counters
 */
volatile unsigned int tscribe_loop_count;

unsigned long tscribe_raw_int_count;
unsigned long tscribe_compressed_int_count;

unsigned long tscribe_raw_state_count;
unsigned long tscribe_compressed_state_count;

unsigned long tscribe_raw_timer_count;
unsigned long tscribe_compressed_timer_count;

unsigned long tscribe_raw_general_count;
unsigned long tscribe_compressed_general_count;

#ifdef TSCRIBE_COUNT_PER_REG
unsigned long tscribe_reg_count[512];
#endif

#define TOTAL_STATE_COUNT ((tscribe_raw_state_count + tscribe_compressed_state_count))
#define TOTAL_TIMER_COUNT ((tscribe_raw_timer_count + tscribe_compressed_timer_count))
#define TOTAL_GENERAL_COUNT ((tscribe_raw_general_count + tscribe_compressed_general_count))

#define TOTAL_RAW_REGISTER_COUNT ((tscribe_raw_state_count + tscribe_raw_timer_count + tscribe_raw_general_count))
#define TOTAL_COMPRESSED_REGISTER_COUNT ((tscribe_compressed_state_count + tscribe_compressed_timer_count + tscribe_compressed_general_count))

#define TOTAL_RAW_INT_COUNT ((tscribe_raw_int_count))
#define TOTAL_COMPRESSED_INT_COUNT ((tscribe_compressed_int_count))

#define TOTAL_RAW_COUNT ((tscribe_raw_int_count + tscribe_raw_state_count + tscribe_raw_timer_count + tscribe_raw_general_count))
#define TOTAL_COMPRESSED_COUNT ((tscribe_compressed_int_count + tscribe_compressed_state_count + tscribe_compressed_timer_count + tscribe_compressed_general_count))

#define TOTAL_REGISTER_COUNT ((TOTAL_RAW_REGISTER_COUNT + TOTAL_COMPRESSED_REGISTER_COUNT))
#define TOTAL_INT_COUNT ((TOTAL_RAW_INT_COUNT + TOTAL_COMPRESSED_INT_COUNT))
#define TOTAL_COUNT ((TOTAL_RAW_COUNT + TOTAL_COMPRESSED_COUNT))

/******************************************************************************
 * Flash test
 */
#ifdef TSCRIBE_TEST_FLASH
//static unsigned char tscribe_test_seq[9] = {0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0x1, 0x5};
static unsigned char tscribe_test_seq[9] = {1, 2, 3, 4, 5, 6, 7, 0xF, 0};
unsigned int tscribe_next_test_byte;
#endif

#endif
