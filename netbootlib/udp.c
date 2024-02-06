/* $Id: udp.c,v 1.5 2001/10/04 19:05:03 hauke Exp $ */

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
#include <stdio.h>

#ifndef OLDROUTINENAMES
#define OLDROUTINENAMES	1	/* XXX */
#endif

#include <Memory.h>
#include <OSUtils.h>
#if defined(UNIVERSAL_INTERFACES_VERSION) && (UNIVERSAL_INTERFACES_VERSION >= 0x210)
#include <TextUtils.h>
#else
#include <TextEdit.h>
#endif
#include <Events.h>
#include <Devices.h>
#include <ENET.h>
#include <MacTCP.h>

#include "AddressXlation.h"
#include "udp.h"
#include "errlog.h"


/* Local routines */
static pascal void ResolverResultProc(struct hostInfo *aHinfoPtr, char *someData);

static void print_enet_addr(Enet_addr *enet_addr, char *aMAC);
       

static StreamPtr	gListenerStream;			/* for AsyncNotification() */




/* 
 * Set up a MacTCP i/o stream 
 *
 * XXX Don't forget to set the local port in 'struct udpDesc'
 */
OSErr UDPCreateStream(struct udpDesc *ud, UDPNotifyProcPtr anASR)
{
    UDPiopb upb;
	unsigned long recvBufferSize;
	Ptr recvBuffer;
    OSErr err;
    
	/* See MacTCP Programmer's guide, p.15 */
	recvBufferSize = (2 * kMaxRecvBufferSize + 256);
	recvBuffer = NewPtr(recvBufferSize);
	
	if ((err = MemError()) == noErr) {
		/* Create the stream */
		upb.ioCRefNum = ud->MacTCPDrvr;
		upb.csCode = UDPCreate;
		upb.csParam.create.rcvBuff = recvBuffer;
		upb.csParam.create.rcvBuffLen = recvBufferSize;
		upb.csParam.create.localPort = ud->myPort;
		upb.csParam.create.notifyProc = NewUDPNotifyProc(anASR);
	#ifdef __SYSEQU__
		upb.csParam.create.userDataPtr = *(long *)CurrentA5;		/* XXX */
	#else
		upb.csParam.create.userDataPtr = (Ptr)SetCurrentA5();
	#endif
		err = PBControlSync((ParmBlkPtr)&upb);
	}
	if (err == noErr) {
		ud->stream = upb.udpStream;
		ud->myPort = upb.csParam.create.localPort;
	}
    return err;
}


/* 
 * Release the MacTCP i/o stream
 * 
 * XXX Where can I spit errors to? We cannot get out prematurely 
 * because we have to free all used ressources...
 */
OSErr UDPReleaseStream(struct udpDesc *ud)
{
    UDPiopb upb;
    OSErr err;

	
    upb.ioCRefNum = ud->MacTCPDrvr;
    upb.csCode = UDPRelease;
    upb.udpStream = ud->stream;
    err = PBControlSync((ParmBlkPtr)&upb);
    
	if (err == noErr) {
		/* 
		 * If the UDPRelease call fails, we cannot rely on the UDPiopb
		 * containing reasonable data. Trying to free a garbage pointer 
		 * trashes the heap; better live with a small memory leak.
		 */
    	DisposePtr(upb.csParam.create.rcvBuff);
		err = MemError();
	}
	DebugPrintf(4, "Released UDP stream [%d]\n", err);
	return err;
}


/* 
 * Send an UDP packet 
 */
OSErr UDPSend(struct udpDesc *ud)
{
    UDPiopb upb;
    EventRecord theEvent;
    struct wdsEntry theData[2];
    OSErr err;
    
    theData[0].length = ud->dataLength;
    theData[0].ptr = (Ptr)ud->buffer;
    theData[1].length = 0;
    theData[1].ptr = nil;

    upb.udpStream = ud->stream;
    upb.ioCompletion = nil;
    upb.ioCRefNum = ud->MacTCPDrvr;
    upb.csCode = UDPWrite;
    upb.csParam.send.reserved = 0;
    upb.csParam.send.remoteHost = ud->itsAddr;
    upb.csParam.send.remotePort = ud->itsPort;
    upb.csParam.send.checkSum = true;
    upb.csParam.send.wdsPtr = (Ptr)theData;
    err = PBControlAsync((ParmBlkPtr)&upb);
    while (upb.ioResult > 0)
        EventAvail(everyEvent, &theEvent);
    return upb.ioResult;
}


/* 
 * Receive an UDP packet
 * 
 * Return data in buffer <someData> of length <dataLength>. 
 */
OSErr UDPRecv(struct udpDesc *ud)
{
    UDPiopb upb;
    OSErr err;
    
    upb.udpStream = ud->stream;
    upb.ioCRefNum = ud->MacTCPDrvr;
    upb.csCode = UDPRead;
    upb.csParam.receive.timeOut = ud->timeout;
    upb.csParam.receive.secondTimeStamp = 0;
    err = PBControlSync((ParmBlkPtr)&upb);
    if (err != noErr)
        return err;
    
    /* 
	 * return remote port number the packet was sent from 
	 * (in case we let the driver assign a free port)
	 */
    ud->itsPort = upb.csParam.receive.remotePort;
    
  
    /* receive is allowed to return with a zero length buffer */
    if (upb.csParam.receive.rcvBuffLen == 0) {
        ud->dataLength = 0;
        return noErr;
    }
	
    /* Take no more than <buffer length> of data */
    ud->dataLength = Min(upb.csParam.receive.rcvBuffLen, kMaxRecvBufferSize);
    BlockMove(upb.csParam.receive.rcvBuff, ud->buffer, ud->dataLength);
    
    /* Done with the block, give it back to the driver */
    upb.csCode = UDPBfrReturn;
    err = PBControlSync((ParmBlkPtr)&upb);
    
    return err;
}


/* Completion routine for resolver calls */
static pascal void ResolverResultProc(struct hostInfo *aHinfoPtr, char *someData)
{
#pragma unused (aHinfoPtr)
    *someData = 0xFF;
}


/* Resolve a given host name */
OSErr ResolveHostname(char *aHostName, ip_addr *anIPAddr)
{
    struct hostInfo theHInfo;
    OSErr err;
    char done = 0x00;
    extern Boolean gCancel;		/* ??? */

    if ((err = OpenResolver(nil)) == noErr) {
            err = StrToAddr(aHostName, &theHInfo, 
                            NewResultProc(ResolverResultProc), &done);
            if (err == cacheFault) {
                /* wait for cache fault resolver to be called by interrupt */
                while (!done)
		        ;
            }
            CloseResolver();
            if ((theHInfo.rtnCode == noErr) || (theHInfo.rtnCode == cacheFault)) {
                    *anIPAddr = theHInfo.addr[0];
                    strcpy(aHostName, theHInfo.cname);
/*                    aHostName[strlen(aHostName)-1] = '\0';	XXX redundant */
                    return noErr;
            }
    }
    *anIPAddr = 0;
    return err;
}


/* Obtain my own IP address */
OSErr GetMyIPAddress(ip_addr *anIPAddr, short aDrvrID)
{
    struct GetAddrParamBlock ipb;
    OSErr err;
    
    ipb.csCode = ipctlGetAddr;
    ipb.ioCRefNum = aDrvrID;
    err = PBControlSync((ParmBlkPtr)&ipb);
    *anIPAddr = ipb.ourAddress;

	if (err == noErr) {
		char	txtbuf[20];

	    num2ipAddr(ipb.ourAddress, txtbuf);
		DebugPrintf(4, "Found IP  address %s\n", txtbuf);
	}

    return err;
}


/* Obtain my own MAC address */
OSErr GetMyEnetAddress(Enet_addr *anEnetAddr, short aDrvrID)
{
	char txtbuf[20];

#if 0	/* XXX ipctlLAPStats call does not return anything useable */
    struct IPParamBlock ipb;
	struct LAPStats *lapStats;
    OSErr err;
    
	memset(&ipb, 0, sizeof(struct IPParamBlock));
    ipb.csCode = ipctlLAPStats;
    ipb.ioCRefNum = aDrvrID;
	ipb.csParam.LAPStatsPB.lapStatsPtr = (struct LAPStats *)NewPtrClear(sizeof(LAPStats));
	
    err = PBControlSync((ParmBlkPtr)&ipb);
	if (err == noErr) {
		lapStats = ipb.csParam.LAPStatsPB.lapStatsPtr;
		/* Check if valid MAC address */
		if (lapStats->ifPhyAddrLength == sizeof(Enet_addr)) {
			*anEnetAddr = *(Enet_addr *)lapStats->ifPhysicalAddress;
			print_enet_addr(anEnetAddr, txtbuf);
			DebugPrintf(4, "Found MAC address %s\n", txtbuf);
			/* DisposPtr(ipb.csParam.LAPStatsPB.lapStatsPtr); */

		} else {
			ErrorPrintf("Address is not a MAC - length %u\n", 
				lapStats->ifPhyAddrLength);
			err = (-1);
		}
	}
	return err;
	
#else

#pragma unused (aDrvrID)

	/* 
	 * Seen in Apple Technical Q&A NW05 
	 *
	 * Ask the Ethernet Driver for the MAC. 
	 * Note we say "the driver"; if you have more than one card 
	 * in the machine, Murphy mandates that we pick the wrong one. 
	 * That's why it would have been nice if we were able to ask 
	 * MacTCP instead: It knows what ethernet driver & card 
	 * it is going through.
	 */

	OSErr err;
	short theEnetRefNum;
	EParamBlock theEPB;
	
	err = OpenDriver("\p.ENET", &theEnetRefNum);
	if (err == noErr) {
		theEPB.ioRefNum = theEnetRefNum;
		theEPB.ioNamePtr = nil;
		theEPB.u.EParms1.ePointer = NewPtrClear(78);
		theEPB.u.EParms1.eBuffSize = 78;		/* see New IM: Networking, 11-37 */
		err = EGetInfo(&theEPB, false);

/*		if (err == noErr) { */
			*anEnetAddr = *(Enet_addr *)theEPB.u.EParms1.ePointer;
			print_enet_addr(anEnetAddr, txtbuf);
			DebugPrintf(4, "Found MAC address %s\n", txtbuf);
/*		} */
		DisposePtr(theEPB.u.EParms1.ePointer);
	}
    return err;
	
#endif
}


/* Print 48 Bit MAC address in 'colon'ed six-tuple to a string */
static void print_enet_addr(Enet_addr *enet_addr, char *aMAC)
{
    sprintf(aMAC, "%x:%x:%lx:%lx:%lx:%lx",
            ((enet_addr->en_hi & 0xFFFF) >> 8), 
            (enet_addr->en_hi & 0x00FF),
            (enet_addr->en_lo >> 24), 
            ((enet_addr->en_lo & 0x00FFFFFF) >> 16),
            ((enet_addr->en_lo & 0x0000FFFF) >> 8), 
            (enet_addr->en_lo & 0x000000FF));
}


/* Print IP address in a string as a dotted quad */
void num2ipAddr(ip_addr anIPAddr, char *aDottedQuad)
{
    sprintf(aDottedQuad, "%lu.%lu.%lu.%lu", 
            (anIPAddr >> 24), 
            ((anIPAddr & 0x00FFFFFF) >> 16),
            ((anIPAddr & 0x0000FFFF) >> 8), 
            (anIPAddr & 0x000000FF));
}


/* Asynchronous notification routine for data transfers */
pascal void AsyncNotification(StreamPtr anUDPStream, 
                              unsigned short anEventCode,
                              Ptr someData,
                              struct ICMPReport *anICMPMsg)
{
#pragma unused (anICMPMsg)
    long oldA5;
    extern Boolean gUDPDataPending;			/* "data received" flag */

    
    oldA5 = SetA5((long)someData);

    if (anEventCode == UDPDataArrival && anUDPStream == gListenerStream)
        gUDPDataPending = true;
        
    SetA5(oldA5);
}
