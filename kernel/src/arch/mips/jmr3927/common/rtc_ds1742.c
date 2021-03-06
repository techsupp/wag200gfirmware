/***********************************************************************
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *              ahennessy@mvista.com       
 *
 * arch/mips/jmr3927/common/rtc_ds1742.c
 * Based on arch/mips/ddb5xxx/common/rtc_ds1386.c
 *     low-level RTC hookups for s for Dallas 1742 chip.
 *
 * Copyright (C) 2000-2001 Toshiba Corporation 
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ***********************************************************************
 */


/*
 * This file exports a function, rtc_ds1386_init(), which expects an
 * uncached base address as the argument.  It will set the two function
 * pointers expected by the MIPS generic timer code.
 */

#include <linux/types.h>
#include <linux/time.h>
#include <linux/rtc.h>

#include <asm/time.h>
#include <asm/addrspace.h>

#include <asm/jmr3927/ds1742rtc.h>
#include <asm/debug.h>

#define	EPOCH		2000

#undef BCD_TO_BIN
#define BCD_TO_BIN(val) (((val)&15) + ((val)>>4)*10)

#undef BIN_TO_BCD
#define BIN_TO_BCD(val) ((((val)/10)<<4) + (val)%10)

#define WRITE_RTC(val, addr) jmr3927_nvram_out(val, addr)
#define READ_RTC(addr)       jmr3927_nvram_in(addr)

static unsigned long rtc_base;

void rtc_ds1742_wait( void )
{
  volatile u8 val;

  val = READ_RTC(RTC_SECONDS) & RTC_SECONDS_MASK;
  while (val == (READ_RTC(RTC_SECONDS) & RTC_SECONDS_MASK)) ;

  return;
}

static unsigned long
rtc_ds1742_get_time(void)
{
	unsigned int year, month, day, hour, minute, second;
	unsigned int century;


	WRITE_RTC(RTC_READ, RTC_CONTROL);
	second = BCD_TO_BIN(READ_RTC(RTC_SECONDS) & RTC_SECONDS_MASK);
	minute = BCD_TO_BIN(READ_RTC(RTC_MINUTES));
	hour = BCD_TO_BIN(READ_RTC(RTC_HOURS));
	day = BCD_TO_BIN(READ_RTC(RTC_DATE));
	month = BCD_TO_BIN(READ_RTC(RTC_MONTH));
	year = BCD_TO_BIN(READ_RTC(RTC_YEAR));
	century = BCD_TO_BIN(READ_RTC(RTC_CENTURY) & RTC_CENTURY_MASK);
	WRITE_RTC(0, RTC_CONTROL);

	year += EPOCH;

	return mktime(year, month, day, hour, minute, second);
}
extern void to_tm(unsigned long tim, struct rtc_time * tm);

static int 
rtc_ds1742_set_time(unsigned long t)
{
	struct rtc_time tm;
	u8 year, month, day, hour, minute, second;
	u8 cmos_year, cmos_month, cmos_day, cmos_hour, cmos_minute, cmos_second;
	int cmos_century;

	WRITE_RTC(RTC_READ, RTC_CONTROL);
	cmos_second = (u8)(READ_RTC(RTC_SECONDS) & RTC_SECONDS_MASK);
	cmos_minute = (u8)READ_RTC(RTC_MINUTES);
	cmos_hour = (u8)READ_RTC(RTC_HOURS);
	cmos_day = (u8)READ_RTC(RTC_DATE);
	cmos_month = (u8)READ_RTC(RTC_MONTH);
	cmos_year = (u8)READ_RTC(RTC_YEAR);
	cmos_century = READ_RTC(RTC_CENTURY) & RTC_CENTURY_MASK;

	WRITE_RTC(RTC_WRITE, RTC_CONTROL);

	/* convert */
	to_tm(t, &tm);

	/* check each field one by one */
	year = BIN_TO_BCD(tm.tm_year - EPOCH);
	if (year != cmos_year) {
		WRITE_RTC(year,RTC_YEAR);
	}

	month = BIN_TO_BCD(tm.tm_mon);
	if (month != (cmos_month & 0x1f)) {
		WRITE_RTC((month & 0x1f) | (cmos_month & ~0x1f),RTC_MONTH);
	}

	day = BIN_TO_BCD(tm.tm_mday);
	if (day != cmos_day) {
	
		WRITE_RTC(day, RTC_DATE);
	}

	if (cmos_hour & 0x40) {
		/* 12 hour format */
		hour = 0x40;
		if (tm.tm_hour > 12) {
			hour |= 0x20 | (BIN_TO_BCD(hour-12) & 0x1f);
		} else {
			hour |= BIN_TO_BCD(tm.tm_hour);
		}
	} else {
		/* 24 hour format */
		hour = BIN_TO_BCD(tm.tm_hour) & 0x3f;
	}
	if (hour != cmos_hour) WRITE_RTC(hour, RTC_HOURS);

	minute = BIN_TO_BCD(tm.tm_min);
	if (minute !=  cmos_minute) {
		WRITE_RTC(minute, RTC_MINUTES);
	}

	second = BIN_TO_BCD(tm.tm_sec);
	if (second !=  cmos_second) {
		WRITE_RTC(second & RTC_SECONDS_MASK,RTC_SECONDS);
	}
	
	/* RTC_CENTURY and RTC_CONTROL share same address... */
	WRITE_RTC(cmos_century, RTC_CONTROL);

	return 0;
}

void
rtc_ds1742_init(unsigned long base)
{
	u8  cmos_second;

	/* remember the base */
	rtc_base = base;
	db_assert((rtc_base & 0xe0000000) == KSEG1);

	/* set the function pointers */
	rtc_get_time = rtc_ds1742_get_time;
	rtc_set_time = rtc_ds1742_set_time;

	/* clear oscillator stop bit */
	WRITE_RTC(RTC_READ, RTC_CONTROL);
	cmos_second = (u8)(READ_RTC(RTC_SECONDS) & RTC_SECONDS_MASK);
	WRITE_RTC(RTC_WRITE, RTC_CONTROL);
	WRITE_RTC(cmos_second, RTC_SECONDS); /* clear msb */
	WRITE_RTC(0, RTC_CONTROL);
}
