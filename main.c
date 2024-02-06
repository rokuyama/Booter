/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include "MacBSD.h"
#include <setjmp.h>				// To jump into copy of kernel buffer
#include <ConditionalMacros.h>	// For UNIVERSAL_INTERFACES_VERSION
#include <Devices.h>			// For OpenDeskAcc() prototype
#include <Resources.h>			// For Get1Resource()	,,
#include <Sound.h>				// For SysBeep()		,,
#include <StandardFile.h>		// For StandardFileReply, StandardGetFile()
#include <Scrap.h>				// For ZeroScrap()
#include <ToolUtils.h>			// For LoWord() & HiWord()

int				running=0, startboot=0;
TEHandle		TEH;
int				debugLevel;
jmp_buf			bootenv;
VersRecHndl		ver;

static MenuHandle		myMenus[numMenus];
static unsigned long	time_boot_start=0L;
static int				linesInFolder;
static long				kern_fd=-1;

#if defined (UNIVERSAL_INTERFACES_VERSION) && UNIVERSAL_INTERFACES_VERSION > 0x0212 \
											&& UNIVERSAL_INTERFACES_VERSION < 0x0214
	/* The qd global has been removed from the headers? */
	QDGlobals			qd;
#endif


static void SetUpMenus (void);
static void DoFile (int item);
static void DoMacBSDMenu (int item);


#ifdef MPW
	void main (void)
#else
	pascal void main (void)
#endif
{
	InitGraf (&qd.thePort);
	InitFonts ();
	FlushEvents (everyEvent, 0);
	InitWindows ();
	InitMenus ();
	TEInit ();
	InitDialogs (0L);
	InitCursor ();
	MaxApplZone ();

	ver = (VersRec **) Get1Resource ('vers', versID);
	if (ver == 0)
	{
		Alert (noResourcesAlrt, NULL);
		ExitToShell ();
	}

	LoadUserConfiguration ();
	SetUpCursors ();
	SetUpMenus ();
	SetUpWindows ();

	if ( RunningOnPPC () && ! currentConfiguration.NoWarnPPC)
		Alert (notOnPPC, NULL);
	if (! CheckHardware () )
	{
		Alert (insuffHWAlrt, NULL);
//		ExitToShell ();
	}
	if (! MacOS32BitMapping () )
	{
		Alert (not32BitAlrt, NULL);
		ExitToShell ();
	}
	if ( UsingVM() && ! RunningOnPPC() )
	{
		Alert (usingVMAlrt, NULL);
		ExitToShell ();
	}

	DebugPrintf (1, "Debugging at level %d.\n", debugLevel);
	if (currentConfiguration.AutoBoot) {
		ErrorPrintf("Autobooting after %d seconds. (SCSI ID = %d)\n",
			currentConfiguration.TimeOut, (int) currentConfiguration.SCSIID);
		GetDateTime (&time_boot_start);
	}
	
	while (MainEvent () == 1) /*process events...*/;
}


void
load_and_boot (void)
{
	bDev	dev;
	char	kern[265];

	if (running) return;

	if (setjmp (bootenv)) {
		if (kern_fd >= 0) kernel_close (kern_fd);
		return;
	}
	
	SelectWindow (mainWindow);
	running = 1;
	startboot = 0;
	MaintainMenus ();
	MaintainCursor ();
	if (currentConfiguration.LogToFile)
		OpenLogFile ();

	if (debugLevel > 1 || currentConfiguration.LogToFile)
		OutputMachineDetails ();

	Output ("Booting...\n");

	switch (currentConfiguration.kernelLoc)
	{
		Str63	str;

		case MacOSPart:
			memcpy (str, currentConfiguration.MacKernel.name, sizeof (Str63) );
			PtoCstr (str);
			strcpy (kern, (char *)str);
			break;
		case NetBSDPart:
			dev.devType = currentConfiguration.ATAdisk ? ATA : SCSI;
			dev.buss = currentConfiguration.Channel;
			dev.id = currentConfiguration.SCSIID;
			strcpy (kern, (const char *)currentConfiguration.KernelName);
			break;
		case NetBoot:
			strcpy (kern, "<name supplied by server>");
			break;
		default:
			ErrorPrintf ("Internal Error in load_and_boot(): Unknown kernel location %d.\n",
						 currentConfiguration.kernelLoc);
			return;
	}

	kern_fd = kernel_open (kern, dev, -1, 0L);
	if (kern_fd == -1)
	{
		ErrorPrintf ("Could not open kernel \"%s\".\n", kern);
		running = 0;
		StopRun(kern_fd);
		return;
	}

	copyunix (kern_fd);
  
	StopRun (kern_fd);		/* Should never get here anyway */
}


int
MainEvent (void) 
{
	EventRecord		myEvent;
	WindowPtr		whichWindow;
	Rect			r;
	unsigned long	curtime, diff;
	short			waittime;
	DialogPtr		dialog;
	short			item;
			
	
	if (running)
		waittime = 0;
	else
		waittime = 30;

	/* We have 32-bit mode, so we have WaitNextEvent also */
	if (WaitNextEvent (everyEvent, &myEvent, waittime, nil))
	{
		if (IsDialogEvent (&myEvent))
			return DoDialog (&myEvent);
		switch (myEvent.what)
		{
			case mouseDown:
				switch (FindWindow (myEvent.where, &whichWindow))
				{
					case inDesk: 
						SysBeep (10);
						break;
					case inMenuBar:
						MaintainMenus ();
						return (DoCommand (MenuSelect (myEvent.where)));
					case inSysWindow:
						SystemClick (&myEvent, whichWindow);
						break;
					case inDrag:
						DragWindow (whichWindow, myEvent.where, &dragRect);
						break;
					case inGrow:
						if (ours (whichWindow))
							MyGrowWindow (whichWindow, myEvent.where);
						break;
					case inContent:
						if (whichWindow != FrontWindow ())
							SelectWindow (whichWindow);
						else 
							if (ours (whichWindow))
								DoContent (whichWindow, &myEvent);
						break;
					default: ;
				} /* end switch FindWindow */
				break;
			case keyDown:
			case autoKey: 
				{
					char theChar;
					
					theChar = myEvent.message & charCodeMask;
					if ((myEvent.modifiers & cmdKey) != 0)
					{
						MaintainMenus ();
						return (DoCommand (MenuKey (theChar)));
					}
				}
				break;
			case activateEvt:
				if (ours ((WindowPtr)myEvent.message))
				{
					r = (*mainWindow).portRect;
					r.top = r.bottom - (SBarWidth + 1);
					r.left = r.left - (SBarWidth + 1);
					InvalRect (&r);
					if (myEvent.modifiers & activeFlag) {
						TEActivate (TEH);
						ShowControl (vScroll);
						TEFromScrap ();
					} else {
						TEDeactivate (TEH);
						HideControl (vScroll);
						ZeroScrap ();
						TEToScrap ();
					}
				}
				break;
			case updateEvt: 
				if (ours ((WindowPtr)myEvent.message))
					UpdateWindow (mainWindow);
				break;
			default: ;
		} /* end of case myEvent.what */
	}
	else
	{
		/* Null Event */
		TEIdle (TEH);
		if (IsDialogEvent (&myEvent))
			DialogSelect (&myEvent, &dialog, &item);
		if (time_boot_start && !running)
		{
			GetDateTime (&curtime);
			diff = curtime - time_boot_start;
			if (((long) diff > 0) && (diff > currentConfiguration.TimeOut))
			{
				time_boot_start = 0;
				load_and_boot ();
			}
		}
		if (!running && startboot)
			load_and_boot ();
	}
	return 1;
}


int
DoCommand (long mResult)
{
	int		theItem;
	Str255	name;
	WindowPtr window;
	
	theItem = LoWord (mResult);
	switch (HiWord (mResult))
	{
		case appleID:
			if (theItem == 1)
				DoAbout ();
			else
			{
				GetItem (myMenus[appleM], theItem, name);
				OpenDeskAcc (name);
				SetPort (mainWindow);
			}
			break;
		case fileID: 
			DoFile (theItem);
			break;
		case editID: 
			if (SystemEdit (theItem - 1))
				break;
			window = FrontWindow ();
			if (! ours (window))	/* One of our Dialogs. */
			{
				switch (theItem)
				{
					case cutCommand:
						DlgCut (window);
						break;
					case copyCommand:
						DlgCopy (window);
						break;
					case pasteCommand:
						DlgPaste (window);
						break;
					case clearCommand:
						DlgDelete (window);
						break;
				}
			}
			break;
		case macBSDID:
			DoMacBSDMenu (theItem);
			break;
	}
	HiliteMenu (0);
	return 1;
}


void
StopRun (long fd)
{
	running = 0;
	time_boot_start = 0;
	Output ("\n*********** Boot Stopped. ***********\n");
	if (fd > 0)
		kernel_close(fd);
	if (currentConfiguration.MonitorDepth ||
		currentConfiguration.setToGreys ||
		currentConfiguration.MonitorSize)
	{
		Output ("Restoring Monitor settings...\n");
		RestoreDepth ();
	}
	MaintainMenus ();
	MaintainCursor ();
	CloseLogFile ();
}


void
MaintainMenus (void)
{
	if (running)
		DisableItem( myMenus[macBSDM], bootNowCommand );
	else
		EnableItem ( myMenus[macBSDM], bootNowCommand );

	if (running  || time_boot_start)
	{
		DisableItem( myMenus[macBSDM], bootingCommand );
		DisableItem( myMenus[macBSDM], serialCommand );
		DisableItem( myMenus[macBSDM], monitorCommand );
		DisableItem( myMenus[macBSDM], prefCommand );
		DisableItem( myMenus[macBSDM], machineCommand );
		DisableItem( myMenus[fileM],   pageSetupCommand );
		DisableItem( myMenus[fileM],   printCommand );
		DisableItem( myMenus[fileM],   savePrefCommand );
		DisableItem( myMenus[fileM],   openBootCommand );
	}
	else
	{
		EnableItem(  myMenus[macBSDM], bootNowCommand );
		EnableItem(  myMenus[macBSDM], bootingCommand );
		EnableItem(  myMenus[macBSDM], serialCommand );
		EnableItem(  myMenus[macBSDM], monitorCommand );
		EnableItem(  myMenus[macBSDM], prefCommand );
		EnableItem(  myMenus[macBSDM], machineCommand );
		DisableItem( myMenus[fileM],   pageSetupCommand );
		DisableItem( myMenus[fileM],   printCommand );
		EnableItem(  myMenus[fileM],   savePrefCommand );
		EnableItem( myMenus[fileM],    openBootCommand );
	}
	
	if (ours (FrontWindow ()))
	{
		DisableItem (myMenus[editM], cutCommand);
		DisableItem (myMenus[editM], copyCommand);
		DisableItem (myMenus[editM], clearCommand);
		DisableItem (myMenus[editM], pasteCommand);
	}
	else
	{
		EnableItem (myMenus[editM], cutCommand);
		EnableItem (myMenus[editM], copyCommand);
		EnableItem (myMenus[editM], clearCommand);
		EnableItem (myMenus[editM], pasteCommand);
	}
}


static void
SetUpMenus (void)
{
	int		i;
	
	myMenus[appleM] = GetMenu (appleID);
	AddResMenu (myMenus[appleM], 'DRVR');
	myMenus[fileM] = GetMenu (fileID);
	myMenus[editM] = GetMenu (editID);
	myMenus[macBSDM] = GetMenu (macBSDID);
	for (i = appleM; i < numMenus + appleM; i++)
		InsertMenu (myMenus[i], 0);
	DrawMenuBar ();
}



static void
GetAllOptions ()
{
	GetBoot ();
	GetSerial ();
	GetMonitor ();
	GetStartup ();
	GetMachine ();
}

static void
DoFile (int item)
{
	StandardFileReply	frep;

	switch (item)
	{
		case openBootCommand:
			StandardGetFile(nil, -1, nil, &frep);
			if ( frep.sfGood )
			{
				currentConfiguration.MacKernel = frep.sfFile;
				startboot = 1;
			}
			break;
		case savePrefCommand:
			GetAllOptions ();
			SaveUserConfiguration();
			break;
		case quitCommand:
			ExitToShell();
			break;
		case pageSetupCommand:
			break;
		case printCommand:
			break;
		default:
			Output ("Huh?  Unknown item in file menu.\n");
			break;
	}
}

static void
DoMacBSDMenu (int item)
{
	switch (item)
	{
		case bootingCommand:
			DoBootingDialog();
			break;
		case serialCommand:
			DoSerialDialog();
			break;
		case monitorCommand:
			DoMonitorDialog();
			break;
		case prefCommand:
			DoStartupDialog();
			break;
		case machineCommand:
			DoMachineDialog();
			break;
		case bootNowCommand:
			GetAllOptions ();
			startboot = 1;
			break;
		case stopBootCommand:
			StopRun(-1);
			break;
		default:
			Output ("Huh?  Unknown item in BSD/Mac68k menu.\n");
			break;
	}
}