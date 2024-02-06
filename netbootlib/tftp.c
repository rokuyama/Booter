/* $Id: tftp.c,v 1.4 2001/10/04 19:04:55 hauke Exp $ */

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
 * tftp client
 * 
 * XXX things:
 * 
 *   What to do wrt. OLDROUTINENAMES?
 *   
 */

#include <string.h>
#include <stdio.h>

#undef OLDROUTINENAMES
#define OLDROUTINENAMES	1	/* XXX */

#include <Memory.h>
#include <Devices.h>
#include <MacTCP.h>

#include "udp.h"
#include "tftp.h"
#include "tftp_int.h"
#include "errlog.h"


/* The tftp connection "handle" */
static struct udpDesc	*myUDP = nil;


/* 
 * Send a Read Request 
 * 
 * Format:
 * 
 *   opcode    filename    0     mode     0
 *------------------------------------------------------   
 * [2 bytes]   [N bytes]   1   [N bytes]  1
 * 
 */
static OSErr tftp_RRQ(char *aFilename, char *aMode)
{
    char *bufPtr;
    struct tftpReq *req;
    OSErr err;
    
    if (strlen(aFilename) > kMaxFileNameLength)
        return (-1);			/* XXX */

    /* Clean the message buffer */
    memset(myUDP->buffer, 0, sizeof(myUDP->buffer));

    /* Set up tftp message */
    req = (struct tftpReq *)myUDP->buffer;
    
    req->opcode = kRRQ;
    strcpy(req->filename, aFilename);
    
    /* 
     * Move past the terminating NUL of the first string to set up 
     * the mode string. 
     */
    bufPtr = req->filename + strlen(aFilename) + 1;
    strcpy(bufPtr, aMode);

#ifdef NETLIB_DEBUG
    msgdump(myUDP->buffer);
#endif

    myUDP->dataLength = kTFTPBlockSize;		/* XXX */
    
    err = UDPSend(myUDP);
    return err;
}


/* 
 * Receive the requested data 
 * 
 * The server listens on well-known port 69, but sends the first reply 
 * (block number one) from an ephemeral port. We have to pick up the 
 * new port number and send to it instead of 69.
 * 
 * After that, any change of port terminates the connection.
 */
static OSErr tftp_RECV(Ptr aBuffer, unsigned long *length)
{
    unsigned short oldBlock;
	long thePackets;
    udp_port oldPort;
    struct tftpData *recvdata;
    unsigned short thePayload;
    OSErr err;
	
    *length = 0;
    oldBlock = 0;
    
    err = UDPRecv(myUDP);
    if (err != noErr)
        return err;
        
    /* Get the new port number from the first packet received */
    oldPort  = myUDP->itsPort;
    DebugPrintf(4, "tftp_RECV() tftp server switching to port %u\n", oldPort);
    
	/*
	 * We pull over the requested file in a tight loop. Eventually, 
	 * we probably want to handle events even during the transfer.
	 * As it is, we hog the CPU.
	 */
	thePackets = 0;
    do {
        /* Did the packet originate from the right port? */
        if (oldPort != myUDP->itsPort) {
            ErrorPrintf("tftp_RECV() expected packet from port %u "
					"-- got %u, aborting.\n", oldPort, myUDP->itsPort);
            err = (-1);
            break;
        }
        
        /* 
		 * Did we get the right kind of packet? 
         * XXX Check for known message types, handle errors 
		 */ 
        recvdata = (struct tftpData *)myUDP->buffer;
        if (recvdata->opcode != kDATA) {
            DebugPrintf(4, "tftp_RECV() got [%u] -- not a data message.\n", 
                   recvdata->opcode);
            return noErr;
        }
        /* Correct block number? If not, ignore. */
    	if (recvdata->blockno == oldBlock + 1) {
            oldBlock = recvdata->blockno;
            
            /* copy data, update pointers - we want the real sizes. */
			thePayload = myUDP->dataLength - 2 * sizeof(short);
            BlockMove(recvdata->data, aBuffer, thePayload);
            aBuffer += thePayload;
            *length += thePayload;
            DebugPrintf(7, "tftp_RECV() received block [%u], payload %d.\n", 
				recvdata->blockno, thePayload);

			/* Print a dot every 50K */
			if (thePackets++ % 100 == 0)
            	Output("."); 
            
            /* Short read? End of transmission. */
            if (myUDP->dataLength < kTFTPBlockSize) {
				DebugPrintf(4, "Block %u short read, EOT\n", recvdata->blockno); 
                break;
			}
			
            err = tftp_ACK(oldBlock);
        } else {
            /* XXX Log & abort */
            ErrorPrintf("tftp_RECV() expected block %u, got %u.\n",
                   oldBlock + 1, recvdata->blockno);
            err = (-1);
            break;
        }
        if (err == noErr) {
        	err = UDPRecv(myUDP);
        }
    } while (err == noErr);

	return err;
}


/* Acknowledge a received data block */
static OSErr tftp_ACK(short aBlockNum)
{
    struct tftpAck *ack;
    OSErr err;
    
    memset(myUDP->buffer, 0, sizeof(myUDP->buffer));
    ack = (struct tftpAck *)myUDP->buffer;

    ack->opcode = kACK;
    ack->blockno = aBlockNum;

#ifdef NETLIB_DEBUG
    msgdump(myUDP->buffer);
#endif

    err = UDPSend(myUDP);
    return err;
}


/* Send an error message (is this client work, at all?) */
static OSErr tftp_ERR(short anErrNum, char *aMessage)
{
    struct tftpErr *error;
    OSErr err;

    if (strlen(aMessage) > kMaxFileNameLength)
        return (-1);			/* XXX */

    memset(myUDP->buffer, 0, sizeof(myUDP->buffer));
    error = (struct tftpErr *)myUDP->buffer;

    error->opcode = kERROR;
    error->errno = anErrNum;

    strcpy(error->message, aMessage);
    
#ifdef NETLIB_DEBUG
    msgdump(myUDP->buffer);
#endif

    err = UDPSend(myUDP);
    return err;
}


#ifdef NETLIB_DEBUG
static void msgdump(char *aBuffer)
{
    short opcode;
    short blockno;
    short errno;
    char stemp[10];
    char linedump[600];
    
    memset(linedump, '\0', sizeof(linedump));
    
    opcode = *(short *)aBuffer;
    switch (opcode) {
      case kRRQ:
        strcat(linedump, "RRQ: ");
        strcat(linedump, aBuffer + 2);
        strcat(linedump, "  mode: ");
        strcat(linedump, aBuffer + strlen(aBuffer + 2) + 1);
        break;
        
      case kWRQ:
        strcat(linedump, "WRQ: ");
        strcat(linedump, aBuffer + 2);
        strcat(linedump, "  mode: ");
        strcat(linedump, aBuffer + strlen(aBuffer + 2) + 1);
        break;
        
      case kDATA:
        strcat(linedump, "DATA block: ");
        blockno = *(short *)(aBuffer + 2);
        sprintf(stemp, "%5dh\n", blockno);
        strcat(linedump, stemp);
		/* XXX hexdump of data block here */
        break;
        
      case kACK:
        strcat(linedump, "ACK block: ");
        blockno = *(short *)(aBuffer + 2);
        sprintf(stemp, "%5dh\n", blockno);
        strcat(linedump, stemp);
		break;
		
      case kERROR:
        strcat(linedump, "ERROR: ");
        errno = *(short *)(aBuffer + 2);
        sprintf(stemp, "%5dh\n", errno);
        strcat(linedump, stemp);
        strcat(linedump, strcat(linedump, aBuffer + 4));
        break;
    }
    strcat(linedump, "\n");
	DebugPrintf(9, "%s%", linedump);
}
#endif /* NETLIB_DEBUG */


/* Get a file from the server */
OSErr tftpReadFile(char *aFilename, char *aMode, 
                   Ptr aBuffer, ip_addr anIPAddress)
{
    OSErr	err, nerr;
    unsigned long buflen;
	char addrbuf[20];
	
    myUDP = (struct udpDesc *)NewPtr(sizeof(struct udpDesc));
    if ((err = MemError()) == noErr) {
        /* Open the MacTCP driver */
        err = OpenDriver("\p.IPP", &myUDP->MacTCPDrvr);
    }
	
    if (err == noErr) {
        /* Create the sender & receiver streams */
		myUDP->myPort = kTFTPPort;
    	err = UDPCreateStream(myUDP, AsyncNotification);
    }
    if (err == noErr) {
        myUDP->timeout = kTimeout;
        
		/* TFTP well-known port (server switches with first packet) */
        myUDP->itsPort = kTFTPPort;

        myUDP->itsAddr = anIPAddress;
        err = GetMyIPAddress(&myUDP->myAddr, myUDP->MacTCPDrvr);
    }
    if (err == noErr) {
        /* Connected; request a binary file */
		num2ipAddr(anIPAddress, addrbuf);
		DebugPrintf(4, "Requesting %s from %s...\n", aFilename, addrbuf);
        err = tftp_RRQ(aFilename, aMode);
    }
    if (err == noErr) {
        /* We can have the file, receive data */
		Output("TFTP transfer ");
        err = tftp_RECV(aBuffer, &buflen);
    }
    if (err == noErr) {
		Output(" done.\n");    
        DebugPrintf(1, "Received %u bytes of data.\n", buflen);
    }
    else {
		Output(" failed.\n");    
        DebugPrintf(1, "Received %u bytes of data (err = %d).\n",
               buflen, err);
    }

    /* Deconstruct the connection */
    nerr = UDPReleaseStream(myUDP);
    if (myUDP != nil)
    	DisposPtr((Ptr)myUDP);
    return err;
}

        