/* $Id: bootp.c,v 1.4 2001/10/04 19:04:43 hauke Exp $ */

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
 * UDP connect/send/receive functions
 * 
 * XXX things
 * 
 *   What to do wrt. OLDROUTINENAMES?
 *   Think C support: PBControl{Sync,Async} vs. PBControl(xxx, true|false)
 * 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#undef OLDROUTINENAMES
#define OLDROUTINENAMES	1	/* XXX */

#include <Memory.h>
#include <Devices.h>
#include <MacTCP.h>

#include "AddressXlation.h"
#include "udp.h"
#include "bootp.h"
#include "errlog.h"


static OSErr BOOTPRequest(Enet_addr aMAC);
static OSErr BOOTPReply(struct bootpRecord *aBOOTP);
void printBOOTP(const struct bootpRecord *aBOOTP);


/* The BOOTP transaction ID */
static unsigned long myTransactionID;

/* The udp connection "handle" */
static struct udpDesc	*myUDP = nil;


static OSErr BOOTPRequest(Enet_addr aMAC)
{
    struct bootpRecord *req;
    OSErr err;
    
	srand((unsigned)clock());

	/* Clean the message buffer */
	memset(myUDP->buffer, 0, sizeof(myUDP->buffer));

	/* Set up BOOTP request */
	req = (struct bootpRecord *)myUDP->buffer;
	myUDP->dataLength = sizeof(struct bootpRecord);
	
	req->opcode = kBOOTPRequest;
	req->hwtype = k10MBitEthernet;
	req->hwAddressLen = sizeof(Enet_addr);
	/* We already know our own IP address, so tell the server */
	req->clientIPAddr = req->yourIPAddr = myUDP->myAddr;
	memcpy(req->clientHWAddress, &aMAC, sizeof(Enet_addr));
	myTransactionID = req->transactionID = rand();
	
	err = UDPSend(myUDP);

    return err;
}


static OSErr BOOTPReply(struct bootpRecord *aBOOTP)
{
	OSErr err;
	struct bootpRecord *recvdata;
	char addrbuf[20];
	
	err = (-1);
	if (nil != aBOOTP) {
		myUDP->dataLength = sizeof(struct bootpRecord);
		err = UDPRecv(myUDP);
	}
	if (err == noErr) {
		num2ipAddr(myUDP->itsAddr, addrbuf);
		recvdata = (struct bootpRecord *)myUDP->buffer;
		/* Check for transaction ID before doing anything else */
		if (myTransactionID == recvdata->transactionID) {
			/* We do not copy what we don't understand */
			BlockMove(recvdata, aBOOTP, sizeof(struct bootpRecord));
		} else {
			ErrorPrintf("Got a reply from %s, but wrong transaction ID.\n", addrbuf);
		}
	}
	return err;
}


/* Get a file from the server */
OSErr BOOTPClient(struct bootpRecord **aBOOTP, short verbose)
{
    OSErr	err;
	Enet_addr theEnetAddr;
	
    
    myUDP = (struct udpDesc *)NewPtr(sizeof(struct udpDesc));
    if ((err = MemError()) == noErr) {
        /* Open the MacTCP driver */
        err = OpenDriver("\p.IPP", &myUDP->MacTCPDrvr);
    }
	if (err == noErr) {
		err = GetMyEnetAddress(&theEnetAddr, myUDP->MacTCPDrvr);
		if (err != noErr)
			DebugPrintf(4, "Asked for MAC address, got error %d\n", err);
#if 1 /* XXX Pismo hack */
		if (err != noErr) {
			/* Power Macs don't have .ENET */
			theEnetAddr.en_hi = 0x0030;
			theEnetAddr.en_lo = 0x6510BB82;
			err = noErr;
			DebugPrintf(4, "Using fixed address 00:30:65:10:BB:82\n", err);
		}
#endif
	}
    if (err == noErr) {
        /* Create the connection stream */
   		myUDP->myPort = kBOOTPClientPort;
 		err = UDPCreateStream(myUDP, AsyncNotification);
    }
    if (err == noErr) {
        myUDP->timeout = kTimeout;
        myUDP->itsPort = kBOOTPServerPort;

		/* Broadcast the request -- we do not know the server */
        myUDP->itsAddr = 0xFFFFFFFF;

		/* 
		 * We have either set the IP statically in MacTCP, or we 
		 * got it from the server.
		 */
        err = GetMyIPAddress(&myUDP->myAddr, myUDP->MacTCPDrvr);
    }
    if (err == noErr) {
        err = BOOTPRequest(theEnetAddr);
		DebugPrintf(4, "BOOTPRequest() gave %d\n", err);

    }
    if (err == noErr) {
		*aBOOTP = (struct bootpRecord *)NewPtrClear(sizeof(struct bootpRecord));
		err = MemError();
	}
	if (err == noErr) {
        /* We can have the BOOTP record, receive data */
        err = BOOTPReply(*aBOOTP);
		DebugPrintf(4, "BOOTPReply() gave %d\n", err);
    }
	if (err == noErr && verbose) {
		printBOOTP(*aBOOTP);
	}
    /* Deconstruct the connection */
    UDPReleaseStream(myUDP);
    if (myUDP != nil)
    	DisposPtr((Ptr)myUDP);
    return err;
}


void printBOOTP(const struct bootpRecord *aBOOTP)
{
	char txt[1000];
	char line[100];
	
	sprintf(txt, "\n\nBOOTP request:\n");
	sprintf(line, "Server %s offers file %s\n", 
		aBOOTP->serverHostname, aBOOTP->bootFilename);
	strcat(txt, line);
	
	Output(txt);
}