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

/**
 * \file
 *         Device driver for the ST M25P80 40MHz 1Mbyte external memory.
 *         Adapted from xmem.c in Contiki.
 * \author
 *         Björn Grönvall <bg@sics.se>
 *
 *         Data is written bit inverted (~-operator) to flash so that
 *         unwritten data will read as zeros (UNIX style).
 */

#include <stdio.h>
#include <string.h>

#include "msp430f1611.h"
#include <io.h>
#include <signal.h>

#include "tscribe-flash.h"
#include "tscribe.h"

/*
 * Initialize external flash *and* SPI bus!
 */
void
tscribe_flash_init(void)
{
  /* Initalize ports for communication with SPI units. */
  U0CTL  = CHAR + SYNC + MM + SWRST; /* SW  reset,8-bit transfer, SPI master */
  U0TCTL = CKPH + SSEL1 + STC;	/* Data on Rising Edge, SMCLK, 3-wire. */

  U0BR0  = 0x02;		/* SPICLK set baud. */
  U0BR1  = 0;  /* Dont need baud rate control register 2 - clear it */
  U0MCTL = 0;			/* Dont need modulation control. */

  P3SEL |= BV(SCK) | BV(MOSI) | BV(MISO); /* Select Peripheral functionality */
  P3DIR |= BV(SCK) | BV(MISO);	/* Configure as outputs(SIMO,CLK). */

  ME1   |= USPIE0;	   /* Module enable ME1 --> U0ME? xxx/bg */
  U0CTL &= ~SWRST;		/* Remove RESET */

  SPI_FLASH_DISABLE();
  SPI_RADIO_DISABLE();
  SPI_FLASH_UNHOLD();
  P4DIR |= BV(FLASH_CS) | BV(FLASH_HOLD) | BV(FLASH_PWR) | BV(RADIO_CS);
  P4OUT |= BV(FLASH_PWR); /* P4.3 Output, turn on power! */

  /* Release from Deep Power-down */
  SPI_FLASH_ENABLE();
  SPI_WRITE_FAST(SPI_FLASH_INS_RES);
  SPI_WAITFORTx_ENDED();
  SPI_FLASH_DISABLE();		/* Unselect flash. */
}

inline static void
tscribe_write_enable(void)
{
  SPI_FLASH_ENABLE();
  SPI_WRITE(SPI_FLASH_INS_WREN);
  SPI_FLASH_DISABLE();
}

unsigned int
tscribe_read_status_register()
{
  unsigned int u;

  SPI_FLASH_ENABLE();
  
  SPI_WRITE(SPI_FLASH_INS_RDSR);
  SPI_FLUSH();
  SPI_READ(u);

  SPI_FLASH_DISABLE();

  return u;
}

/*
 * Wait for a write/erase operation to finish.
 */
void
tscribe_wait_ready(void)
{
  unsigned u;
  do {
    u = tscribe_read_status_register();
    // Contiki need this?
    //watchdog_periodic();
  } while(u & 0x01);		/* WIP=1, write in progress */
  //return u;
}

/*
 * Erase 64k bytes of data. It takes about 1s before WIP goes low!
 */
static void
tscribe_erase_sector(unsigned long offset)
{
  tscribe_wait_ready();
  tscribe_write_enable();

  SPI_FLASH_ENABLE();
  
  SPI_WRITE_FAST(SPI_FLASH_INS_SE);
  SPI_WRITE_FAST(offset >> 16);	/* MSB */
  SPI_WRITE_FAST(offset >> 8);
  SPI_WRITE_FAST(offset >> 0);	/* LSB */
  SPI_WAITFORTx_ENDED();

  SPI_FLASH_DISABLE();
}

void
tscribe_flash_pread(void *_p, int size, unsigned long offset)
{
  unsigned char *p = _p;
  const unsigned char *end = p + size;

  tscribe_wait_ready();

  SPI_FLASH_ENABLE();

  SPI_WRITE_FAST(SPI_FLASH_INS_READ);
  SPI_WRITE_FAST(offset >> 16);	/* MSB */
  SPI_WRITE_FAST(offset >> 8);
  SPI_WRITE_FAST(offset >> 0);	/* LSB */
  SPI_WAITFORTx_ENDED();
  
  SPI_FLUSH();
  for(; p < end; p++) {
    unsigned char u;
    SPI_READ(u);
    *p = ~u;
  }

  SPI_FLASH_DISABLE();
}

void
tscribe_flash_program_page(
  unsigned long offset,
  unsigned char* start1,
  unsigned char* terminate1,
  unsigned char* start2,
  unsigned char* terminate2,
  unsigned int header)
{
  unsigned char* b;

  tscribe_write_enable();

  SPI_FLASH_ENABLE();
  
  SPI_WRITE_FAST(SPI_FLASH_INS_PP);
  SPI_WRITE_FAST(offset >> 16);	/* MSB */
  SPI_WRITE_FAST(offset >> 8);
  SPI_WRITE_FAST(offset >> 0);	/* LSB */

  SPI_WRITE_FAST(~header >> 8); /* MSB */
  SPI_WRITE_FAST(~header); /* LSB */

  for(b = start1; b < terminate1; b++) {
    SPI_WRITE_FAST(~*b);
  }

  for(b = start2; b < terminate2; b++) {
    SPI_WRITE_FAST(~*b);
  }

  SPI_WAITFORTx_ENDED();

  SPI_FLASH_DISABLE();
}

long
tscribe_flash_erase(long size, unsigned long addr)
{
  unsigned long end = addr + size;

  if(size % XMEM_ERASE_UNIT_SIZE != 0) {
    //PRINTF("flash_erase: bad size\n");
    return -1;
  }

  if(addr % XMEM_ERASE_UNIT_SIZE != 0) {
    //PRINTF("flash_erase: bad offset\n");
    return -1;
  }

  for (; addr < end; addr += XMEM_ERASE_UNIT_SIZE) {
    tscribe_erase_sector(addr);
  }
  return size;
}
