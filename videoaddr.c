/************************************************************************/
/* videoaddr.c - Function for determining information about the screen	*/
/*				 to pass to the kernel for colsole output				*/
/*																		*/
/* Taken out of ufs_test.c to tidy it up a bit		on 8th Nov. 1997	*/
/*													by Nigel Pearson	*/
/*																		*/
/* Copyright © 1993. Details in the file COPYRIGHT.txt					*/
/************************************************************************/


#include "MacBSD.h"

long	rowbytes		= 0,
		screendepth		= 0,
		screenHeight	= 0,
		screenWidth		= 0,
		video_logical	= (long) NULL,
		videoaddress	= (long) NULL;


#ifdef USING_THEPORT

void
getVideoAddress (GDHandle maindev)
{
	long	screen32, scr_slot, scr_addr;
	long	screen24;


	/* This is the old code to determine the video address.
	 * It uses qd.thePort->portBits.baseAddr and bases its calculations on that.
	 * But what happens w/o a video device?
	 */
	screen24 = (unsigned long)qd.thePort->portBits.baseAddr;
	video_logical = screen24;		/* want mapped video address ; Booter ver 1 */
	/* We may get a 32 bit address here, check MF. */
	if (screen24 & 0xf0000000) {
		/* 32 bit address. */
		screen32 = screen24;
	} else {
		/* 24 bit address. */
		/* Unfortunately, NULL looks like a 24-bit address, too. -BRG */
		ParamText("\pYour machine does not appear to be 32-bit addressing mode.",
		          "\pPlease set 32-bit addressing in the Memory Control Panel.",
		          "\pYou may need to install Mode32 in order to do so.",
		          "\pWill boot anyway, but you're gonna be hosed!");
		NoteAlert (128, NULL);

		scr_slot = (screen24 & 0xf00000) >> 20;
		scr_addr = (screen24 & 0xfffff);
		screen32 = 0xf0000000 | (scr_slot << 24) | scr_addr;
	}
	videoaddress = screen32;
	screendepth  = (**(**GetMainDevice()).gdPMap).pixelSize;
	rowbytes     = qd.thePort->portBits.rowBytes * screendepth;
	screenWidth = qd.thePort->portBits.bounds.right - qd.thePort->portBits.bounds.left;
	screenHeight = qd.thePort->portBits.bounds.bottom - qd.thePort->portBits.bounds.top;
}

#else

void
getVideoAddress (GDHandle maindev)
{
	PixMapHandle	maindevpmap;


	/* It looks reasonable, from empirical evidence, to assume that video_logical
	 * and videoaddress can be set to the same thing, since they end up being the
	 * same thing on 32-bit systems anyway.....and as of some time ago, MACOS_VIDEO
	 * and MACOS_SCC are not even used in the kernel anymore.
	 * 
	 * This code is also preferable because it gives us a documented and supported
	 * way to determine whether the video base address we are given by the Graphics
	 * Device Manager is 24-bit or 32-bit, even though it is not likely that it is
	 * going to be 24-bit because the Booter only runs in 32-bit mode. 
	 */

	if (maindev) {
		maindevpmap = (*maindev)->gdPMap;
		/* Determine video address. */
		videoaddress = (long) (*maindevpmap)->baseAddr;
		if ((*maindevpmap)->pmVersion != 4) {
			/* IM: Imaging with QuickDraw says that if pmVersion is 4, it is
			 * *definitely* a 32-bit clean address. Because we are working in 32-bit
			 * mode, it is very very likely to be a 32-bit clean address *anyway*.
			 * It is unknown whether there are any possible configurations where this
			 * is *not* the case. Just to be on the safe side, we will try and ensure
			 * that it is a 32-bit address even if the version is not 4. This is not all
			 * that important anyway. I'm just paranoid, I guess.
			 */
			DebugPrintf(2, "Stripping video addr 0x%lx 'cause pmVersion = %d, not 4.\n",
				videoaddress, (*maindevpmap)->pmVersion);
			videoaddress = (long) StripAddress((void *) videoaddress);
		}
		/* This global should go away and is only present for compatibility with
		 * ancient kernels.
		 */
		video_logical = videoaddress;
		/* Determine screen depth (in bits per color component). */
		screendepth = (*maindevpmap)->pixelSize;
		/* Top two bits used for flags, so mask it out with 0xC000. We're moving this
		 * into a long, but it's a short (and potentially originally negative value:
		 * mask it out with 0xFFFF. By the way, it really is the *wrong* thing to do
		 * here to multiply rowbytes times screendepth, because we are taking it out
		 * of the main device *pixmap* -- which is already adjusted for a different bit
		 * depth -- as opposed to the old code in which we were taking rowbytes out of
		 * a *bitmap* and adjusting it ourselves for the current bit depth.
		 */
		rowbytes = ((*maindevpmap)->rowBytes & ~0xC000) & 0xFFFF;
		/* Determine screen size. IM: Imaging with QuickDraw says to use gdRect for
		 * this instead of (*gdPMap)->bounds.
		 */
		screenWidth = (*maindev)->gdRect.right - (*maindev)->gdRect.left;
		screenHeight = (*maindev)->gdRect.bottom - (*maindev)->gdRect.top;
	} else /* No main device => no video. Blank out params. */ {
		video_logical = videoaddress = (long) NULL;
		screendepth = rowbytes = screenWidth = screenHeight = 0;
	}

	if (currentConfiguration.VideoHack)
	{
		ErrorPrintf("videoaddress was %lx", videoaddress);
		video_logical = videoaddress = 0xF9000000;
		ErrorPrintf(", now set to %lx", videoaddress);
	}
}

#endif