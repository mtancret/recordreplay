/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _TSCRIBE_FLASH_H
#define _TSCRIBE_FLASH_H

/**
 * \file
 *         Macros adapted from spi.h and xmem.h in Contiki.
 *         
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

/* SPI input/output registers. */
#define SPI_TXBUF U0TXBUF
#define SPI_RXBUF U0RXBUF

                                /* USART0 Tx ready? */
#define SPI_WAITFOREOTx() while ((U0TCTL & TXEPT) == 0)
                                /* USART0 Rx ready? */
#define SPI_WAITFOREORx() while ((IFG1 & URXIFG0) == 0)
                                /* USART0 Tx buffer ready? */
#define SPI_WAITFORTxREADY() while ((IFG1 & UTXIFG0) == 0)

/*
 * SPI bus - M25P80 external flash configuration.
 */

#define RADIO_CS        2       /* P4.2 Output */
#define FLASH_PWR       3       /* P4.3 Output */
#define FLASH_CS        4       /* P4.4 Output */
#define FLASH_HOLD      7       /* P4.7 Output */

/* Enable/disable flash access to the SPI bus (active low). */

#define SPI_FLASH_ENABLE()  ( P4OUT &= ~BV(FLASH_CS) )
#define SPI_FLASH_DISABLE() ( P4OUT |=  BV(FLASH_CS) )
#define SPI_RADIO_DISABLE() ( P4OUT |=  BV(RADIO_CS) )

#define SPI_FLASH_HOLD()    ( P4OUT &= ~BV(FLASH_HOLD) )
#define SPI_FLASH_UNHOLD()  ( P4OUT |=  BV(FLASH_HOLD) )

#define XMEM_ERASE_UNIT_SIZE (64*1024L)

#define SCK            1  /* P3.1 - Output: SPI Serial Clock (SCLK) */
#define MOSI           2  /* P3.2 - Output: SPI Master out - slave in (MOSI) */
#define MISO           3  /* P3.3 - Input:  SPI Master in - slave out (MISO) */

#define BV(x) (1<<(x))

#define SPI_WAITFORTx_BEFORE() SPI_WAITFORTxREADY()
#define SPI_WAITFORTx_AFTER()
#define SPI_WAITFORTx_ENDED() SPI_WAITFOREOTx()

//extern unsigned char spi_busy;

/* Write one character to SPI */
#define SPI_WRITE(data)                         \
  do {                                          \
    SPI_WAITFORTx_BEFORE();                     \
    SPI_TXBUF = data;                           \
    SPI_WAITFOREOTx();                          \
  } while(0)

/* Write one character to SPI - will not wait for end
   useful for multiple writes with wait after final */
#define SPI_WRITE_FAST(data)                    \
  do {                                          \
    SPI_WAITFORTx_BEFORE();                     \
    SPI_TXBUF = data;                           \
    SPI_WAITFORTx_AFTER();                      \
  } while(0)

/* Read one character from SPI */
#define SPI_READ(data)   \
  do {                   \
    SPI_TXBUF = 0;       \
    SPI_WAITFOREORx();   \
    data = SPI_RXBUF;    \
  } while(0)

/* Flush the SPI read register */
#define SPI_FLUSH()      \
  do {                   \
    IFG1 &= ~URXIFG0;    \
  } while(0);
// data = SPI_RXBUF;     \

#define  SPI_FLASH_INS_WREN        0x06
#define  SPI_FLASH_INS_WRDI        0x04
#define  SPI_FLASH_INS_RDSR        0x05
#define  SPI_FLASH_INS_WRSR        0x01
#define  SPI_FLASH_INS_READ        0x03
#define  SPI_FLASH_INS_FAST_READ   0x0b
#define  SPI_FLASH_INS_PP          0x02
#define  SPI_FLASH_INS_SE          0xd8
#define  SPI_FLASH_INS_BE          0xc7
#define  SPI_FLASH_INS_DP          0xb9
#define  SPI_FLASH_INS_RES         0xab

void tscribe_flash_init(void);

unsigned tscribe_read_status_register(void);

void tscribe_wait_ready(void);

void tscribe_flash_pread(void *buf, int nbytes, unsigned long offset);

long tscribe_flash_erase(long nbytes, unsigned long offset);

void tscribe_flash_program_page(unsigned long offset, unsigned char* start1, unsigned char* terminate1, unsigned char* start2, unsigned char* terminate2, unsigned int header);

#endif /* _TSCRIBE_FLASH_H */
