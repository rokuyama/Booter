/* $Id: tftp.h,v 1.2 2001/10/04 19:04:57 hauke Exp $ */

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
#ifndef TFTP_H
#define TFTP_H

/*
 * tftp library routines
 * 
 * We support only one connection at a time (the UDP descriptor ptr is 
 * a static variable). This should be no limitation as we are just a 
 * reading client. 
 * 
 * See R. Stevens: TCP/IP Illustrated, Vol. 1 (Ch. 15 TFTP)
 *     RFC 1350 (TFTP Rev. 2)
 */


OSErr tftp_RRQ(char *aFilename, char *aMode);
OSErr tftp_RECV(Ptr aBuffer, unsigned long *length);

/* 
 * Read a file into memory via tftp 
 * The caller has to make sure that the buffer exists and is large enough.
 */
OSErr tftpReadFile(char *aFilename, char *aMode, 
                   Ptr aBuffer, ip_addr anIPAddress);

#endif /* TFTP_H */