/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <9P2000.h>

int
jehanne_dirwstat(const char *name, Dir *d)
{
	uint8_t *buf;
	int r;

	r = jehanne_sizeD2M(d);
	buf = jehanne_malloc(r);
	if(buf == nil)
		return -1;
	jehanne_convD2M(d, buf, r);
	r = jehanne_wstat(name, buf, r);
	jehanne_free(buf);
	return r;
}
