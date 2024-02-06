/*-
 * Copyright (c) 1995 Brian Gaeke.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ABOVE COPYRIGHT HOLDERS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * I DIDN'T DO IT, NOBODY SAW ME DO IT, YOU CAN'T PROVE ANYTHING.
 */

/* Video card sRsrcID interpreting code -- headers, typedefs, and other cruft */

#define MAX_CARDS 6

typedef struct video_list
{
	short num_cards;
	struct
	{
		unsigned char slot;
		unsigned char modenum;
	} cards[MAX_CARDS];
} video_list;

typedef int (*video_call_func)(video_list *, unsigned char, unsigned char,
	SpBlock *);

/*
 * These structures are in Cards & Drivers 3d ed. but are inexplicably
 * missing from Video.h. It's not as if they were useless structures; 
 * on the contrary, all the video driver calls need VDParamBlock. Well,
 * maybe a less brain-damaged development environment than Symantec C 6.0
 * would have them. I really need to get CodeWarrior. :-P
 */
typedef struct VDFlagRec
{
	SignedByte flag;
} VDFlagRec;
typedef VDFlagRec *VDFlagPtr;

typedef struct VDParamBlock
{
	QElemPtr qLink;
	short qType;
	short ioTrap;
	Ptr ioCmdAddr;
	ProcPtr ioCompletion;
	OSErr ioResult;
	StringPtr ioNamePtr;
	short ioVRefNum;
	short ioRefNum;
	short csCode;
	Ptr csParam;
} VDParamBlock;
typedef VDParamBlock *VDParamBlockPtr;
