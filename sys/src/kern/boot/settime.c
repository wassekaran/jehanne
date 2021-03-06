/*
 * Copyright (C) 2015-2018 Giacomo Tesio <giacomo@tesio.it>
 *
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <9P2000.h>
#include "../boot/boot.h"

static int32_t lusertime(char*);

char *timeserver = "#s/boot";

static int
setlocaltime(char* timebuf, int s){
	int n, f, t;
	t=0;
	f = open("#r/rtc", ORDWR);
	if(f >= 0){
		if((n = read(f, timebuf, s-1)) > 0){
			timebuf[n] = '\0';
			t = 1;
		}
		close(f);
	}else do{
		jehanne_strcpy(timebuf, "yymmddhhmm[ss]");
		outin("\ndate/time ", timebuf, s);
	}while((t=lusertime(timebuf)) <= 0);
	return t;
}


void
settime(int islocal, int afd, char *rp)
{
	int f;
	int timeset;
	Dir dir[2];
	char timebuf[64];

	jehanne_print("time...");
	timeset = 0;
	if(islocal){
		/*
		 *  set the time from the real time clock
		 */
		timeset=setlocaltime(timebuf, sizeof(timebuf));
	}
	if(timeset == 0){
		/*
		 *  set the time from the access time of the root
		 */
		f = open(timeserver, ORDWR);
		if(f < 0)
			return;
		if(mount(f, afd, "/tmp", MREPL, rp, '9') < 0){
			warning("settime mount");
			close(f);
			if((!islocal) && (setlocaltime(timebuf, sizeof(timebuf)) == 0))
				return;
		} else {
			close(f);
			if(jehanne_stat("/tmp", statbuf, sizeof statbuf) < 0)
				fatal("stat");
			jehanne_convM2D(statbuf, sizeof statbuf, &dir[0], (char*)&dir[1]);
			jehanne_sprint(timebuf, "%ld", dir[0].atime);
			unmount(0, "/tmp");
		}
	}

	if((!islocal) && (jehanne_strcmp(timebuf,"0")==0))
		setlocaltime(timebuf, sizeof(timebuf));

	f = open("#c/time", OWRITE);
	if(write(f, timebuf, jehanne_strlen(timebuf)) < 0)
		warning("can't set #c/time");
	close(f);
	jehanne_print("\n");
}

#define SEC2MIN 60L
#define SEC2HOUR (60L*SEC2MIN)
#define SEC2DAY (24L*SEC2HOUR)

int
g2(char **pp)
{
	int v;

	v = 10*((*pp)[0]-'0') + (*pp)[1]-'0';
	*pp += 2;
	return v;
}

/*
 *  days per month plus days/year
 */
static	int	dmsize[] =
{
	365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static	int	ldmsize[] =
{
	366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 *  return the days/month for the given year
 */
static int *
yrsize(int y)
{

	if((y%4) == 0 && ((y%100) != 0 || (y%400) == 0))
		return ldmsize;
	else
		return dmsize;
}

/*
 *  compute seconds since Jan 1 1970
 */
static int32_t
lusertime(char *argbuf)
{
	char *buf;
	uint32_t secs;
	int i, y, m;
	int *d2m;

	buf = argbuf;
	i = jehanne_strlen(buf);
	if(i != 10 && i != 12)
		return -1;
	secs = 0;
	y = g2(&buf);
	m = g2(&buf);
	if(y < 70)
		y += 2000;
	else
		y += 1900;

	/*
	 *  seconds per year
	 */
	for(i = 1970; i < y; i++){
		d2m = yrsize(i);
		secs += d2m[0] * SEC2DAY;
	}

	/*
	 *  seconds per month
	 */
	d2m = yrsize(y);
	for(i = 1; i < m; i++)
		secs += d2m[i] * SEC2DAY;

	secs += (g2(&buf)-1) * SEC2DAY;
	secs += g2(&buf) * SEC2HOUR;
	secs += g2(&buf) * SEC2MIN;
	if(*buf)
		secs += g2(&buf);

	jehanne_sprint(argbuf, "%ld", secs);
	return secs;
}
