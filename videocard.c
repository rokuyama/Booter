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

/*
 * videocard.c 8/16/95 Brian Gaeke <brg@dgate.org>
 *
 * This file does some sRsrcID-interpreting and VBL-disabling that is currently
 * a whole lot easier in MacOS. Once the kernel is able to (a) read the sRsrcID
 * information off the card by itself, (b) handle VBL interrupts reliably and
 * (c) reinitialize the card at will and pick a video mode of the user's choosing
 * (can you say X11?), none of this will be needed. I tried to document it well
 * enough so that it might be useful later in the aforementioned kernel development.
 *
 * Currently it is implemented by having one function to do the video-card walking,
 * viz., video_call(), which takes as a parameter a pointer to a function to call
 * (a video_call_func) which then does the important stuff (turning VBLs off, getting
 * basic information into a video_info, etc.) and returns -1 on error or 0 for success.
 * This may seem like overkill, but it was necessary to avoid major code duplication,
 * and I simply couldn't disable VBLs at the same time as I was getting the video info.
 */

#include "MacBSD.h"
#include <stdio.h>
#include <Devices.h>	// For OpenDriver()
#include <Slots.h>		// For SpBlock
#include <ROMDefs.h>	// sRsrcName
#include "videocard.h"

typedef unsigned long	ptrdiff_t;

	/* In kernel_parse.c */
extern ptrdiff_t		offsetOfKernelString(char *str);


static int vbl_set (video_list *vinfo, unsigned char slotid, unsigned char spid,
					SpBlock *spb, SignedByte onoff);
static int vbl_on (video_list *vinfo, unsigned char slotid, unsigned char spid,
					SpBlock *spb);
static int vbl_off (video_list *vinfo, unsigned char slotid, unsigned char spid,
					SpBlock *spb);
static short open_driver_from_slotid (int slotid, SpBlock *spb, short *drefnum);
static int add_card (video_list *vinfo, unsigned char slotid, unsigned char spid, SpBlock *spb);
static OSErr video_call (video_list *vinfo, video_call_func func);

/*
 * vbl_set
 * 
 * Turn on or off vertical blanking interrupts for the specified video
 * card (onoff: 1 = on, 0 = off.)
 *
 * Note that Cards & Drivers 3rd ed. is ambivalent as to
 * whether this is an optional control routine, so it might not be
 * possible on some cards. (cf. p. 209 and table p. 218)
 */
static int
vbl_set(video_list *vinfo, unsigned char slotid, unsigned char spid,
	SpBlock *spb, SignedByte onoff)
{
#ifdef MPW
#pragma unused vinfo
#endif
	VDParamBlock	pb;
	VDFlagRec		flag;
	OSErr			err = 0;
	short			drvr_refnum = 0;
	
	/* Interesting: one must be careful checking function result's sign
	 * because a negative return from _Open is not necessarily an error.
	 * So open_driver_from_slotid does the right MacOS thing and returns
	 * noErr on success and passes the drefnum back by reference.
	 */
	err = open_driver_from_slotid(slotid,spb,&drvr_refnum);
	if (err != noErr)
		return err;
	DebugPrintf(1, "vbl_set: drefnum %d, slotid 0x%x, spid 0x%x, onoff %d\n",
		drvr_refnum, slotid, spid, onoff);

	/* Clear param block. */
	memset((void *)&pb,0,sizeof(VDParamBlock));
	pb.ioRefNum = drvr_refnum;
	pb.csCode = 7; /* SetInterrupt */
	/*
	 * Trust Apple to use 1 for off and 0 for on. sheesh.
	 * You don't believe me? RTFM. p209 in cards'n'drivers.
	 * This kind of mentality ought to be dragged out and shot. :-(
	 */
	flag.flag = (onoff ? 0 : 1);
	pb.csParam = (Ptr) &flag;
	
	/* Call video driver's SetInterrupt routine */
	err = PBControlSync((ParmBlkPtr) &pb);
	if (err != noErr)
	{
		ErrorPrintf("Error %d turning off interrupts for slot %x\n",err,slotid);
		return err;
	}
	
	/* Do not close the video driver; it would crash the machine. */
	return noErr;
}

/*
 * vbl_on
 *
 * video_call_func to turn on video interrupts for the specified card.
 */
static int
vbl_on(video_list *vinfo, unsigned char slotid, unsigned char spid,
	SpBlock *spb)
{
	return vbl_set(vinfo,slotid,spid,spb,1);
}

/*
 * vbl_off
 *
 * video_call_func to turn off video interrupts for the specified card.
 */
static int
vbl_off(video_list *vinfo, unsigned char slotid, unsigned char spid,
	SpBlock *spb)
{
	return vbl_set(vinfo,slotid,spid,spb,0);
}

/*
 * open_driver_from_slotid
 *
 * Open the video card driver for the given slotid/spid/... and return
 * its dRefNum or -1 if an error occurs. THIS ASSUMES A CORRECTLY FILLED-
 * IN SPBLOCK. The easiest way to do this is to pass it the same one that
 * you got back from SRsrcInfo.
 */
static short
open_driver_from_slotid(int slotid, SpBlock *spb, short *drefnum)
{
	OSErr 	err = 0;
	Str255	drvrname;
	
	/* Get name of video driver. */
	spb->spID = sRsrcName;
	err = SGetCString(spb);
	if (err != noErr)
	{
		ErrorPrintf("Error %d getting name of slot 0x%x video driver\n",
			err,slotid);
		return err;
	}
	
	/* Try to open the driver */
	sprintf((char *)drvrname,".%s",spb->spResult);
	err = OpenDriver(CtoPstr((char *)drvrname),drefnum);
	if (err != noErr)
	{
		ErrorPrintf("Error %d trying to open slot 0x%x video driver\n",
			err,slotid);
		return err;
	}
	return noErr;
}

/* 
 * add_card
 * 
 * Called to edit video_list *vinfo whenever a new card is found.
 */
static int
add_card(video_list *vinfo, unsigned char slotid, unsigned char spid,
	SpBlock *spb)
{
#ifdef MPW
#pragma unused spb
#endif
	/* Fill in vInfo with the requisite information. */
	vinfo->cards[vinfo->num_cards].slot = slotid;
	vinfo->cards[vinfo->num_cards].modenum = spid;
	vinfo->num_cards++;
	
	return 0;
}

/*
 * video_call
 *
 * Call func() for each video card, possibly reading/modifying vinfo.
 */
static OSErr
video_call(video_list *vinfo, video_call_func func)
{
	unsigned char	slotid = 0,
					spid = 0;
	SpBlock			spb;
	int				ncards = 0;
	short			drefnum = 0;
	OSErr			err = 0;
	
	/* Not doing slot 0 stuff here. Eventually maybe? Sounds like IIvx RBV thingie
	 * does interrupts, may need this.
	 */
	for (slotid = 0x9; slotid <= 0xE; slotid++)
	{
		/*
		 * I don't know what an external device number is, but 0 seems to work.
		 * I wouldn't otherwise bother setting it except that IM says it's an
		 * input parameter to SRsrcInfo.
		 */
		spb.spSlot = slotid;
		spb.spExtDev = 0;
		
		for (spid = 0x0001; spid < 0x00ff; spid++)
		{
			/*
			 * Do not use spb.spID as the iterator; various slot manager calls
			 * overwrite it.
			 */
			spb.spID = spid;
			
			/*
			 * Query the slot manager as to the type of thing this
			 * sResource represents.
			 */
			err = SRsrcInfo(&spb);
	
			if (err) /* More often than not SRsrcInfo will return an error. */
			{
				if (err == -344) /* smNoMoresRsrcs = -344;  \* No more sResources *\ */
				{
					if (spb.spID == 1)
					{
						/* No card in this slot if we get an smNoMoresRsrcs 
						   error reading board sRsrc. Go on to the next card. */
						break;
					}
					/* Otherwise, it just means the sResource doesn't exist. */
				}
				else
				{
					/* Any other error means we abort. */
					ErrorPrintf("Error %d trying to read sResource for slot 0x%x\n",
						err,spb.spSlot);
					if (currentConfiguration.AbortNonFatal)
					{
						running=0;
						Output ("Aborting on non-fatal error.\n");
					}
					return err;
				}
			}
			else /* No error reading sResource */
			{
				/*
				 * Does this sResource represent an Apple/QuickDraw-compatible
				 * video display device?
				 */
				if ((spb.spCategory == catDisplay) &&
					(spb.spCType == typeVideo) &&
					(spb.spDrvrSW == drSwApple))
				{
					err = (*func)(vinfo,slotid,spid,&spb);
					if (err != noErr)
					{
						ErrorPrintf(
							"Error %d processing video card slot 0x%x sRsrcID 0x%x\n",
							err,slotid,spid);
						if (currentConfiguration.AbortNonFatal)
						{
							running=0;
							Output ("Aborting on non-fatal error.\n");
						}
						return err;
					}

					/*
					 * I don't know how one could have a machine so insanely
					 * configured that this would ever happen. But we check anyway.
					 */
					if (vinfo->num_cards == MAX_CARDS)
					{
						ErrorPrintf("Only the first %d video cards can be configured.\n",
							MAX_CARDS);
						return 1;
					}
					/*
					 * We're done with this card. We are assuming no weirdo video
					 * boards with more than one grf-ish device.
					 */
					break;
				}
			}
		}
	}

	return noErr;
}

/* 
 * set_video_info
 * 
 * Fill in mac68k_vrsrc_cnt and mac68k_vrsrc_vec with information about
 * the video drivers currently in use in the system.
 */
void
set_video_info(unsigned char *buf)
{
	ptrdiff_t	kernOffset;
	int x;
	OSErr err;
	video_list vinfo;
	unsigned short *vrsrc_vecptr, *vrsrc_vecbase;
	
	/*
	 * This used to be done only after actually getting the info,
	 * but that was only necessary when getting the info turned the
	 * VBL interrupts off too. Now that the steps are separate anyway,
	 * this can be done first.
	 */
	if ( ! isStringInKernel("_mac68k_vrsrc_cnt") )
	{
		DebugPrintf(1, "Old kernel; not setting video info.\n");
		return;
	}

	memset(&vinfo,0,sizeof(vinfo));
	/*
	 * Call add_card repeatedly to retrieve the slot manager's idea
	 * of what video driver(s) are active.
	 */
	err = video_call(&vinfo,add_card);
	if (err != noErr)
	{
		ErrorPrintf("Warning: error %d getting video info, nothing passed to kernel\n",
			err);
		return;
	}
	
	/* Do the easy part first - set the number of cards */
	setKernelLongVal("_mac68k_vrsrc_cnt", vinfo.num_cards);
	
	/* Now look up the array and set its elements individually */
	if ( ( kernOffset = offsetOfKernelString ("_mac68k_vrsrc_vec") ) != -1 )
	{
		vrsrc_vecbase = vrsrc_vecptr = (unsigned short *) (buf + kernOffset);
		/*
		 * Each array element looks like 0xRRSS where RR is the sRsrcID of the 
		 * video driver's sResource, and SS is the slot on which the sResource
		 * was found.
		 */
		for (x = 0; x < vinfo.num_cards; x++)
			*vrsrc_vecptr++ = (vinfo.cards[x].modenum << 8) | (vinfo.cards[x].slot);
		vrsrc_vecptr = vrsrc_vecbase;
	}
	DebugPrintf(1, "\nSet _mac68k_vrsrc_vec to {");
	for (x=0; x<MAX_CARDS; x++)
		DebugPrintf(1, "0x%x ",*vrsrc_vecptr++);
	DebugPrintf(1, "}.\n");
}

/*
 * turn_off_interrupts
 *
 * Turn off interrupts on all active video cards.
 */
void
turn_off_interrupts(void)
{	
	video_list vinfo;
	OSErr err;
	
	/* This should be done no matter how old the kernel is. */
	memset(&vinfo,0,sizeof(vinfo));
	err = video_call(&vinfo,vbl_off);
	if (err != noErr)
	{
		ErrorPrintf("Warning - error %d turning off interrupts...booting anyway!\n",err);
	}
}
