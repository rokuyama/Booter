/* $Id: tftp_int.h,v 1.2 2001/10/04 19:05:00 hauke Exp $ */

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
 * internal tftp declarations
 */

/* tftp transfer mode is "binary data" */
#define TFTP_MODE_BINARY	"octet"

/* TFTP well-known port */
enum { kTFTPPort = 69 };

/* Max. length of specified filename */
enum { kMaxFileNameLength = 128 };

/* tftp packet types */
enum {
    kRRQ 	= 1,
    kWRQ	= 2,
    kDATA	= 3,
    kACK	= 4,
    kERROR	= 5
};

/* Size of a TFTP kDATA block */
enum { kTFTPBlockSize = 512 };


/* template for (read|write) request */
struct tftpReq {
    unsigned short opcode;
    char filename[1];
};

/* template for data block */
struct tftpData {
    unsigned short opcode;
    unsigned short blockno;
    char data[512];
};

/* template for ack message */
struct tftpAck {
    unsigned short opcode;
    unsigned short blockno;
};

/* template for error message */
struct tftpErr {
    unsigned short opcode;
    unsigned short errno;
    char message[1];
};


static OSErr tftp_ACK(short aBlockNum);
static OSErr tftp_ERR(short anErrNum, char *aMessage);
static void msgdump(char *aBuffer);
