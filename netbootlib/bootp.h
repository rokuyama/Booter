/* $Id: bootp.h,v 1.3 2001/10/04 19:04:46 hauke Exp $ */

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
 * BOOTP client interfaces
 */

#ifndef BOOTP_H
#define BOOTP_H


enum {
	kBOOTPRequest = 1,
	kBOOTPReply = 2
};

enum {
	kBOOTPServerPort = 67,
	kBOOTPClientPort = 68
};

enum {
	k10MBitEthernet = 1
};

/* 
 * Format of the 300 byte bootp request/reply 
 * See R. Stevens: TCP/IP illustrated, Vol 1 / 16.2
 */
struct bootpRecord {
	char opcode;
	char hwtype;
	char hwAddressLen;
	char hopCount;
	long transactionID;
	short secondsSinceBootstrap;
	short reserved1;
	unsigned long clientIPAddr; 
	unsigned long yourIPAddr; 
	unsigned long serverIPAddr; 
	unsigned long gatewayIPAddr;
	unsigned char clientHWAddress[16];
	char serverHostname[64];
	char bootFilename[128];
	char vendorInfo[64];
};

OSErr BOOTPClient(struct bootpRecord **aBOOTP, short verbose);

#endif /* BOOTP_H */
