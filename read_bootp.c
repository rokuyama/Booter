/* $Id: read_bootp.c,v 1.7 2001/10/09 05:40:45 hauke Exp $ */

/*
 * Copyright (c) 2001 Hauke Fath
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
 *	This product includes software developed by Hauke Fath
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,     
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Load kernel image via BOOTP and tftp.
 */

#include <string.h>
#include <stdio.h>

#undef OLDROUTINENAMES
#define OLDROUTINENAMES	1	/* XXX */

#include <Memory.h>
#include <OSUtils.h>
#include <MacTCP.h>

#include "udp.h"
#include "bootp.h"
#include "tftp.h"
#include "MacBSD.h"

#define kBootMode		"octet"
#define kKernelBufSize	(2500000L)

/* We fake a file ID and store the buffer ptr elsewhere */
#define kFooFileID		(42)


Boolean gUDPDataPending;

static Ptr myBuffer;
static long myOffset;

/*
 * Get information about ourselves and the server from a BOOTP request,
 * then fetch the kernel image through tftp.
 * Unfortunately, it looks like we have to get the entire image here
 * and deal out whatever chunks are requested through bootp_{lseek,read}
 * from the memory buffer (mainly for extracting parameters from the
 * kernel header). As a result, we'll end up with _two_ kernel
 * images in memory, but I do not see a quick way around that.
 */


long
bootp_open (char *name, long how)
{
#pragma unused (name)
#pragma unused (how)

    OSErr err;
	struct bootpRecord *theBOOTP;
	
	myOffset = 0L;
    myBuffer = NewPtr(kKernelBufSize);	/* enough */
    if (noErr != MemError()) {
        ErrorPrintf("Oops! Not enough memory!\n");
        return -1;
    }
    
    err = BOOTPClient(&theBOOTP, debugLevel);
    DebugPrintf(4, "BOOTPClient() gave %d\n", err);
    
    if (err == noErr) {
    	err = tftpReadFile(theBOOTP->bootFilename, kBootMode, myBuffer, 
			   theBOOTP->serverIPAddr);
		DebugPrintf(4, "tftpReadFile() gave %d\n", err);
    }


    DisposPtr((Ptr)theBOOTP);

	if (err != noErr)
		return -1;
    
    return kFooFileID;
}


void
bootp_close (long foo)
{
#pragma unused (foo)

	if (0 != myBuffer) 
		DisposPtr((Ptr)myBuffer);
}


long
bootp_read (long foo, unsigned char *buf, long len)
{
#pragma unused (foo)

	if (myOffset + len > kKernelBufSize)
	{
		ErrorPrintf ("Went beyond tftp buffer: Offset=%ld, len=%ld\n",
			myOffset, len);
		return -1;
	}
	memcpy(buf, (myBuffer + myOffset), len);
	myOffset += len;
	DebugPrintf (3, "Read %ld bytes from tftp buffer.\n", len);
	return len;
}


long
bootp_lseek (long foo, long offset, long whence)
{
#pragma unused (foo)

    if (whence == SEEK_SET) {
		myOffset = offset;
	} else if (whence == SEEK_CUR) {
		myOffset += offset;
	} else {
		ErrorPrintf("lseek in memory buffer does not support "
			"offsets relative to the buffer end.\n");
		myOffset = -1;
	}
	return myOffset;
}
