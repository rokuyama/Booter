/************************************************************************/
/* Change the screen depth and or resolution before booting the kernel	*/
/*																		*/
/* Written on 14th May 1997 by Nigel Pearson for the NetBSD/Mac68k port	*/
/*																		*/
/* Based on the original idea and code by Dan Jacobowitz				*/
/*										<youngdrow@mail.geocities.com>	*/
/************************************************************************/
/* For later, maybe allow multi-monitor control so that:				*/
/* 1) a particular video card (or internal video) can be selected		*/
/*    as the main monitor (the one NetBSD uses for the console)			*/
/* 2) different depths and resolutions can apply to different monitors	*/
/************************************************************************/


#include "MacBSD.h"

#include <Palettes.h>

#include "RequestVideo.h"		/* Display Manager stuff */



static char			savedDepth = 0;
static short		currentDepth;
static char			currentModeGreys;



static void SaveDepth ()
{
	GDHandle	mainDevice = GetMainDevice();

	HLock ((Handle)mainDevice);

	if (((GDPtr)*mainDevice) -> gdFlags & 0x0001)	/* Colour mode */
		currentModeGreys = 0;
	else
		currentModeGreys = 1;

	switch (((GDPtr)*mainDevice) -> gdMode)
	{
		case oneBitMode:       currentDepth = 1;  break;
		case twoBitMode:       currentDepth = 2;  break;
		case fourBitMode:      currentDepth = 4;  break;
		case eightBitMode:     currentDepth = 8;  break;
		case sixteenBitMode:   currentDepth = 16; break;
		case thirtyTwoBitMode: currentDepth = 16; break;
		default:               currentDepth = -1; break;
	}

	HUnlock ((Handle)mainDevice);

	savedDepth = 1;
}


static char			gotVideoSetting = 0;
VideoRequestRec		originalRec;


void RestoreDepth ()
{
	if (savedDepth)
	{
		GDHandle	mainDevice = GetMainDevice();

		if (HasDepth (mainDevice, currentDepth, currentModeGreys, 0) )
			SetDepth (mainDevice, currentDepth, currentModeGreys, 0);
		savedDepth = 0;
	}

	if (gotVideoSetting)
	{
		RVSetVideoRequest (&originalRec);
		gotVideoSetting = 0;
	}
}


void MonitorChange (unsigned char depth, unsigned char greys, unsigned char size)
{
	GDHandle		mainDevice = GetMainDevice();


	SaveDepth ();
	if (!depth)
		depth = currentDepth;

	if ( size && (GetDisplayMgrVers() > 1) )
	{
		VideoRequestRec	requestRec;
		int				width, height;

		switch (size)
		{
			case m_640x480:  width=640,  height=480; break;
			case m_800x600:  width=800,  height=600; break;
			case m_832x624:  width=832,  height=624; break;
			case m_1024x768: width=1024, height=768; break;
			case m_1152x870: width=1152, height=870; break;
		}

		/* Do some Display Manager magic here!!! */

		requestRec.screenDevice		= nil;					/* any screen */
		requestRec.reqBitDepth		= depth;
		requestRec.reqHorizontal	= width;
		requestRec.reqVertical		= height;
		requestRec.displayMode		= (unsigned long) 0;	/* must init to nil */
		requestRec.depthMode		= (unsigned long) 0;	/* must init to nil */
		requestRec.requestFlags		= 0;						
		requestRec.requestFlags		= 1<<kAllValidModesBit;	/* give me the HxV over bit depth,
															   and only safe video modes */

		RVRequestVideoSetting (&requestRec);
		if (requestRec.screenDevice == nil)
			DebugPrintf (5, "Display Manager - Couldn't get current video setting\n"); 
		{
			originalRec.screenDevice = requestRec.screenDevice;		/* this screen */
			RVGetCurrentVideoSetting (&originalRec);
			gotVideoSetting = 1;

			RVSetVideoRequest (&requestRec);
			if (noErr != RVConfirmVideoRequest (&requestRec) )
				RVSetVideoRequest (&originalRec);
		}

		DebugPrintf (5, "Display Manager - Set depth %d, width %d, height %d\n",
															depth, width, height);
		if (!greys)
			return; 
	}

	if (greys && (depth > 8) )
	{
		Output ("Cannot change to thousands or millions of greys. Setting 256 greys.\n");
		depth = 8;
	}

	if (HasDepth (mainDevice, depth, greys, 0) )
	{
		SetDepth (mainDevice, depth, greys, 0);
		DebugPrintf (5, "SetDepth() to depth %d, greys %d", depth, greys);
	}
	else
	{
		ErrorPrintf ("Cannot change to depth %d ", (int) depth);
		if (greys)
			Output (" and GreyScale");
		Output ("\n");
	}
}



#include "DialogMgr.h"

DialogPtr monitorDia = NULL;


void DoMonitorDialog (void)
{
	ControlHandle	ctl;
	int				depth, size;

	if (monitorDia)
	{
		SelectWindow (monitorDia);
		return;
	}
	monitorDia = GetNewDialog (monitorDI, NULL, (WindowPtr) -1);
	depth = currentConfiguration.MonitorDepth;
	size  = currentConfiguration.MonitorSize;

	SetDialogItemControlValue(monitorDia, m_depth, depth);
	
	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_1bit);
	if (depth == 0 || depth == 1)
		SetControlValue (ctl, 1);
	HiliteControl   (ctl, depth ?0:255);
	
	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_8bit);
	SetControlValue (ctl, (depth == 8) );
	HiliteControl   (ctl, depth ?0:255);

	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_16bit);
	SetControlValue (ctl, (depth == 16) );
	HiliteControl   (ctl, depth ?0:255);

	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_32bit);
	SetControlValue (ctl, (depth == 32) );
	HiliteControl   (ctl, depth ?0:255);

	SetDialogItemControlValue(monitorDia, m_greys, currentConfiguration.setToGreys);

	SetDialogItemControlValue(monitorDia, m_size, size);
	if ( GetDisplayMgrVers() > 1 )						/* If we have Display Manager, */
		HiliteDialogControl (monitorDia, m_size, 0);	/* enable size changing stuff  */
	else
		HiliteDialogControl (monitorDia, m_size, 255);
	
	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_640x480);
	if (size == 0 || size == m_640x480)
		SetControlValue (ctl, 1);
	HiliteControl   (ctl, size ?0:255);
	
	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_800x600);
	SetControlValue (ctl, (size == m_800x600) );
	HiliteControl   (ctl, size ?0:255);

	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_832x624);
	SetControlValue (ctl, (size == m_832x624) );
	HiliteControl   (ctl, size ?0:255);

	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_1024x768);
	SetControlValue (ctl, (size == m_1024x768) );
	HiliteControl   (ctl, size ?0:255);

	ctl = (ControlHandle) GetDialogItemHandle(monitorDia, m_1152x870);
	SetControlValue (ctl, (size == m_1152x870) );
	HiliteControl   (ctl, size ?0:255);

	SetDialogDefaultOutline (monitorDia, defItemID);
	ShowWindow (monitorDia);
}


/* Called to read settings from the Monitor dialog into currentConfiguration */

void GetMonitor (void)
{
	if (! monitorDia)
		return;

	currentConfiguration.MonitorDepth = 0;		/* Assume no changes specified */
	currentConfiguration.MonitorSize = 0;

	if (GetDialogItemControlValue(monitorDia, m_depth) )
	{
		if (GetDialogItemControlValue(monitorDia, m_1bit) )
			currentConfiguration.MonitorDepth = 1;
		else if (GetDialogItemControlValue(monitorDia, m_8bit) )
			currentConfiguration.MonitorDepth = 8;
		else if (GetDialogItemControlValue(monitorDia, m_16bit) )
			currentConfiguration.MonitorDepth = 16;
		else if (GetDialogItemControlValue(monitorDia, m_32bit) )
			currentConfiguration.MonitorDepth = 32;
	}

	currentConfiguration.setToGreys
		= GetDialogItemControlValue (monitorDia, m_greys);   /* ?255:0; */
	
	if (GetDialogItemControlValue(monitorDia, m_size) )
	{
		if (GetDialogItemControlValue(monitorDia, m_640x480) )
			currentConfiguration.MonitorSize = m_640x480;
		else if (GetDialogItemControlValue(monitorDia, m_800x600) )
			currentConfiguration.MonitorSize = m_800x600;
		else if (GetDialogItemControlValue(monitorDia, m_832x624) )
			currentConfiguration.MonitorSize = m_832x624;
		else if (GetDialogItemControlValue(monitorDia, m_1024x768) )
			currentConfiguration.MonitorSize = m_1024x768;
		else if (GetDialogItemControlValue(monitorDia, m_1152x870) )
			currentConfiguration.MonitorSize = m_1152x870;
	}
}


/* A click in the monitor dialog. */

void HandleMonitor (short item)
{
	switch (item) {
		case m_greys:
			(void) ToggleDialogControl (monitorDia, m_greys);
			break;
		case m_depth:
			if (ToggleDialogControl (monitorDia, m_depth) )
			{
				GroupHilite (monitorDia, 4, 0, m_1bit, m_8bit, m_16bit, m_32bit);
				currentConfiguration.MonitorDepth = 1;			/* Set default depth of B & W */
				GroupSet (monitorDia, 4, m_1bit, m_1bit, m_8bit, m_16bit, m_32bit);
			}
			else
				GroupHilite (monitorDia, 4, 255, m_1bit, m_8bit, m_16bit, m_32bit);
			break;
		case m_1bit:
		case m_8bit:
		case m_16bit:
		case m_32bit:
			GroupSet (monitorDia, 4, item, m_1bit, m_8bit, m_16bit, m_32bit);
			break;
		case m_size:
			if (ToggleDialogControl (monitorDia, m_size) )
			{
				GroupHilite (monitorDia, 5, 0, m_640x480, m_800x600,
									m_832x624, m_1024x768, m_1152x870);
				currentConfiguration.MonitorSize = m_640x480;	/* Set default 640x480 resolution */
				GroupSet (monitorDia, 5, m_640x480, m_640x480, m_800x600,
										m_832x624, m_1024x768, m_1152x870);
			}
			else
				GroupHilite (monitorDia, 5, 255, m_640x480, m_800x600,
										m_832x624, m_1024x768, m_1152x870);
			break;
		case m_640x480:
		case m_800x600:
		case m_832x624:
		case m_1024x768:
		case m_1152x870:
			GroupSet (monitorDia, 5, item, m_640x480, m_800x600,
								m_832x624, m_1024x768, m_1152x870);
			break;
		case okID:
			GetMonitor ();
			/* no break */
		case cancelID:
		default:
			DisposeDialog (monitorDia);
			monitorDia = NULL;
			break;
	}
}