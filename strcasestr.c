/*
 * Copyright (c) 1992 Brian Gaeke. The following notices also apply to this
 * work, by virtue of their having been attached to the code from which
 * this code is derived:
 *
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <string.h>
#include <ctype.h>
#include "MacBSD.h"

/*
 * OK, I was lazy. I dug up my old case-insensitive strstr.
 *
 * From: "This file is part of the DGate Utilities Library, version 1.0.5."
 *       $Id: strcasestr.c,v 1.5 1995/04/23 16:15:17 brg Exp $
 *
 * Find the first occurrence of find in s.
 * (but do it non-case-sensitively.)
 * derived from BSD Net2 lib/libc/string/strstr.c:
 *     (#)strstr.c    5.2 (Berkeley) 1/26/91
 */
char *
strcasestr (register const char *s, register const char *find)
{
    register char c, sc;
    register size_t len;

    if ((c = *find++) != 0)
    {
		c = tolower(c);
        len = strlen(find);
        do
        {
            do
            {
                if ((sc = *s++) == 0)
                    return NULL;
            } while (tolower(sc) != c);
        } while (strncasecmp(s, find, len) != 0);
        s--;
    }
    return (char *)s;
}
