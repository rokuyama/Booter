/* $Id: udp.h,v 1.2 2001/10/04 19:05:06 hauke Exp $ */

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
 * UDP interfaces
 */

#ifndef UDP_H
#define UDP_H

#define kMaxRecvBufferSize		(0x00002000L)
#define kTimeout				10				/* seconds */

#define Min(a,b)	( ( (a) < (b) ) ? (a) : (b) )

/* udp connection descriptor */
struct udpDesc {
    short			MacTCPDrvr;					/* the driver ID */
    StreamPtr		stream;						/* MacTCP stream */
    ip_addr			myAddr;						/* local IP address */
    ip_addr			itsAddr;					/* remote IP address */
    udp_port		myPort;						/* local IP port */
    udp_port		itsPort;					/* remote IP port */
    unsigned short	timeout;
    
    unsigned short	dataLength;					/* amount of data in buffer */
    char			buffer[kMaxRecvBufferSize];
};

extern Boolean gUDPDataPending;


OSErr UDPCreateStream(struct udpDesc *ud, UDPNotifyProcPtr anASR);
OSErr UDPReleaseStream(struct udpDesc *ud);

OSErr UDPSend(struct udpDesc *ud);
OSErr UDPRecv(struct udpDesc *ud);

OSErr ResolveHostname(char *aName, ip_addr *anIPAddr);

OSErr GetMyIPAddress(ip_addr *anIPAddr, short aDrvrID);
OSErr GetMyEnetAddress(Enet_addr *anEnetAddr, short aDrvrID);

void num2ipAddr(ip_addr anIPAddr, char *aDottedQuad);

/* This is an UDPNotifyProc -- see <MacTCP.h> */
pascal void AsyncNotification(StreamPtr anUDPStream, unsigned short anEventCode,
                              Ptr someData, struct ICMPReport *anICMPMsg);

#endif /* UDP_H */