/* $Id: tftp-client.c,v 1.2 2001/10/04 19:04:52 hauke Exp $ */

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
 * Macintosh tftp client
 */

#include <stdlib.h>
#include <stdio.h>

#undef OLDROUTINENAMES
#define OLDROUTINENAMES	1	/* XXX */

#include <Memory.h>
#include <OSUtils.h>
#include <TextUtils.h>
#include <MacTCP.h>

#include "udp.h"
#include "bootp.h"
#include "tftp.h"

#define kBootFile	"netbsd"
#define kBootMode	"octet"

#define kTFTPServerName	"espresso.causeuse.org"
/* #define kTFTPServerName	"scree.tangro.de" */


Boolean gUDPDataPending;


void
main(void)
{
    OSErr err;
    ip_addr tftpServerAddress;
    Ptr theBuffer;
    char theDottedQuad[32];
    struct bootpRecord *theBOOTP;
    
    err = ResolveHostname(kTFTPServerName, &tftpServerAddress);
    if (noErr != err) {
        printf("ResolveHostname() failed (%d).\n", err);
        exit(1);
    } else {
        num2ipAddr(tftpServerAddress, theDottedQuad);
        /* P2CStr(theDottedQuad); */    
        printf("Contacting server on %s [%s]...\n", kTFTPServerName,
               theDottedQuad);
    }               
    theBuffer = NewPtr(4000000);	/* enough */
    if (noErr != MemError()) {
        printf("Oops! Not enough memory!\n");
        exit(1);
    }
    
    err = BOOTPClient(&theBOOTP);
    printf("BOOTPClient() gave %d\n", err);
    
    if (err == noErr) {
    	err = tftpReadFile(theBOOTP->bootFilename, kBootMode, theBuffer, 
			   theBOOTP->serverIPAddr);
    }
    
    printf("tftpReadFile() gave %d\n", err);

    DisposPtr((Ptr)theBOOTP);
    DisposPtr((Ptr)theBuffer);
    
    return;
}