/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include "MacBSD.h"
#include "DialogMgr.h"

#include <StandardFile.h>	// For StandardFileReply


pascal void DoDefaultOutline (WindowPtr theWindow, short theItem);
static void HandleAbout (short item);
static void HandleBoot(short item);


static char      macFileTyped = false;
static DialogPtr aboutDia = NULL;
static DialogPtr bootDia = NULL;
extern DialogPtr startupDia;
extern DialogPtr serialDia;
extern DialogPtr machineDia;
extern DialogPtr monitorDia;


/*
 * DoButtonOutline() puts an outline around the user item we give it.
 * Don't even bother trying to use a button here; it just Doesn't Work.
 */
static void
DoButtonOutline(ControlHandle ctl)
{
	Rect itemRect;
	PenState curPen;
	GrafPtr savePort;
	short buttonOval;

	if (ctl != nil) {
		GetPort(&savePort);
		SetPort((*ctl)->contrlOwner);
		GetPenState(&curPen);
		itemRect = (*ctl)->contrlRect;
		buttonOval = (itemRect.bottom - itemRect.top) / 2;
		if ((*ctl)->contrlHilite != 0)
			PenPat(&qd.black);
		else
			PenPat(&qd.gray);
		PenNormal();
		PenSize(3, 3);
		InsetRect(&itemRect, -4, -4);
		FrameRoundRect(&itemRect, buttonOval, buttonOval);
		SetPenState(&curPen);
		SetPort(savePort);
	}
}


/*
 * Use this handler for drawing the outline around the default button.
 */
pascal void
DoDefaultOutline(WindowPtr theWindow, short theItem)
{
#ifdef MPW
#pragma unused theItem
#endif
	Handle itemHndl;
	Rect itemRect;
	short kind;

	GetDialogItem((DialogPtr)theWindow, ok, &kind, &itemHndl, &itemRect);
	DoButtonOutline((ControlHandle)itemHndl);
}


/*
 * Utility routine to set up default button outline for a dialog.  The
 * user item specified by `theItem' will be used for the outline; it
 * should have the same location and size as the button it overlays.
 */
void
SetDialogDefaultOutline(DialogPtr theDialog, short theItem)
{
	Handle itemHndl;
	Rect itemRect;
	short kind;

	GetDialogItem(theDialog, theItem, &kind, &itemHndl, &itemRect);
	SetDialogItem(theDialog, theItem, kind, (Handle)DoDefaultOutline, &itemRect);
}


/* Called to respond to user's choice of About... in Apple menu. */
void
DoAbout (void)
{
	if (aboutDia)
	{
		SelectWindow (aboutDia);
		return;
	}

	ParamText((*ver)->shortVersion,NULL,NULL, NULL);
	aboutDia = GetNewDialog(aboutDI, NULL, (WindowPtr) -1);

	SetDialogDefaultOutline(aboutDia, defItemID);
	ShowWindow(aboutDia);
}


/* Since About dialog has only one activated item, a hit in the About
   dialog means the About box should go away. */
static void
HandleAbout (short item)
{
	if (item == okID)
	{
		DisposDialog (aboutDia);
		aboutDia = NULL;
	}
}


static void
AutoSetGMTtext ()
{
	if (GetDialogItemControlValue (bootDia, autoSetGMT) )
		SetDialogItemTextByItemnoToNum (bootDia, GMTID, GetGMTBias() );
}


/* Here, booting dialog is set up from currentConfiguration. */
void
DoBootingDialog (void)
{
	if (bootDia)
	{
		SelectWindow (bootDia);
		return;
	}

	bootDia = GetNewDialog (bootingDI, NULL, (WindowPtr) -1);

	SetDialogItemTextByItemnoToNum (bootDia, GMTID,  currentConfiguration.GMT_bias);

	SetDialogItemTextByItemnoToCStr (bootDia, kernID, (char *)currentConfiguration.KernelName);
	SetDialogItemTextByItemnoToCStr (bootDia, partID, (char *)currentConfiguration.PartName);

	SetDialogItemTextByItemno (bootDia, macFileID, currentConfiguration.MacKernel.name);

	SetDialogItemControlValue (bootDia, autoSetGMT,    currentConfiguration.AutoSetGMT    ?1:0);
	SetDialogItemControlValue (bootDia, singleUser,    currentConfiguration.SingleUser    ?1:0);
	SetDialogItemControlValue (bootDia, enableRoot,    currentConfiguration.EnableRoot    ?1:0);
	SetDialogItemControlValue (bootDia, askName,       currentConfiguration.AskName       ?1:0);
	SetDialogItemControlValue (bootDia, kernelDebug,   currentConfiguration.KernelDebug   ?1:0);
	SetDialogItemControlValue (bootDia, jumpDebugger,  currentConfiguration.JumpDebugger  ?1:0);

	switch ( currentConfiguration.kernelLoc )
	{
		case NetBSDPart:
			SetDialogItemControlValue (bootDia, bootFromBSD, 1);
			SetDialogItemControlValue (bootDia, bootFromMac, 0);
			SetDialogItemControlValue (bootDia, netBootID,   0);
			HiliteDialogControl (bootDia, setMacFileID, 255);
			break;
		case MacOSPart:
			SetDialogItemControlValue (bootDia, bootFromBSD, 0);
			SetDialogItemControlValue (bootDia, bootFromMac, 1);
			SetDialogItemControlValue (bootDia, netBootID,   0);
			HiliteDialogControl (bootDia, setMacFileID, 0);
			break;
		case NetBoot:
			SetDialogItemControlValue (bootDia, bootFromBSD, 0);
			SetDialogItemControlValue (bootDia, bootFromMac, 0);
			SetDialogItemControlValue (bootDia, netBootID,   1);
			HiliteDialogControl (bootDia, setMacFileID, 255);
	}

	if (currentConfiguration.ATAdisk)
	{
		SetDialogItemControlValue (bootDia, ATAroot,  1);
		SetDialogItemControlValue (bootDia, SCSIroot, 0);
		SetDialogItemTextByItemnoToNum (bootDia, ATAchan,   currentConfiguration.Channel);
		SetDialogItemTextByItemnoToNum (bootDia, ATAdevice, currentConfiguration.SCSIID);
	}
	else
	{
		SetDialogItemControlValue (bootDia, ATAroot,  0);
		SetDialogItemControlValue (bootDia, SCSIroot, 1);
		SetDialogItemTextByItemnoToNum (bootDia, scsiID, currentConfiguration.SCSIID);
	}

	AutoSetGMTtext ();

	SetDialogDefaultOutline (bootDia, defItemID);
	ShowWindow (bootDia);
}


/* A hit in the booting dialog. */
static void
HandleBoot(short item)
{
	StandardFileReply	frep;

	switch (item)
	{
		case autoSetGMT:
			(void) ToggleDialogControl(bootDia, item);
			AutoSetGMTtext ();
			break;
		case singleUser:
		case enableRoot:
		case askName:
		case kernelDebug:
		case jumpDebugger:
			(void) ToggleDialogControl(bootDia, item);
			break;
		case bootFromBSD:
		case bootFromMac:
		case netBootID:
			GroupSet(bootDia, 3, item, bootFromBSD, bootFromMac, netBootID);
			if (GetDialogItemControlValue (bootDia, bootFromMac) )
				HiliteDialogControl (bootDia, setMacFileID, 0);
			else
				HiliteDialogControl (bootDia, setMacFileID, 255);
			break;
		case setMacFileID:
			StandardGetFile (nil, -1, nil, &frep);
			if (frep.sfGood)
			{
				currentConfiguration.MacKernel = frep.sfFile;
				SetDialogItemTextByItemno (bootDia, macFileID,
											currentConfiguration.MacKernel.name);
			}
			break;
		case macFileID:
		case scsiID:
		case ATAchan:
		case ATAdevice:
		case kernID:
		case partID:
			break;
		case ATAroot:
		case SCSIroot:
			GroupSet(bootDia, 2, item, ATAroot, SCSIroot);
			break;
		case GMTID:
			AutoSetGMTtext ();
			break;
		case okID:
			GetBoot ();
			/* no break */
		case cancelID:
		default:
			DisposDialog(bootDia);
			bootDia = NULL;
			break;
	}
}


/* Called to read new settings from the booting dialog
   into currentConfiguration */
void
GetBoot (void)
{
	Str255	filename;


	if (! bootDia)
		return;

	GetDialogItemTextByItemnoAsCStr (bootDia, kernID, (char *)currentConfiguration.KernelName);
	GetDialogItemTextByItemnoAsCStr (bootDia, partID, (char *)currentConfiguration.PartName);

	GetDialogItemTextByItemno (bootDia, macFileID, filename);
	memcpy (currentConfiguration.MacKernel.name, filename, sizeof (Str63) );

	if (GetDialogItemControlValue (bootDia, autoSetGMT) )
		currentConfiguration.GMT_bias = GetGMTBias();
	else
		currentConfiguration.GMT_bias = (short) GetDialogItemTextByItemnoAsNum (bootDia, GMTID);

	if (GetDialogItemControlValue (bootDia, ATAroot) )
	{
		currentConfiguration.ATAdisk = 1;
		currentConfiguration.Channel = GetDialogItemTextByItemnoAsNum (bootDia, ATAchan);
		currentConfiguration.SCSIID  = GetDialogItemTextByItemnoAsNum (bootDia, ATAdevice);
	}
	else
	{
		currentConfiguration.ATAdisk = 0;
		currentConfiguration.SCSIID = (unsigned char)
										GetDialogItemTextByItemnoAsNum (bootDia, scsiID);
	}

	if ( GetDialogItemControlValue (bootDia, bootFromMac) )
		currentConfiguration.kernelLoc = MacOSPart;
	if ( GetDialogItemControlValue (bootDia, bootFromBSD) )
		currentConfiguration.kernelLoc = NetBSDPart;
	if ( GetDialogItemControlValue (bootDia, netBootID) )
		currentConfiguration.kernelLoc = NetBoot;

	currentConfiguration.AskName =		 GetDialogItemControlValue (bootDia, askName)	   ?1:0;
	currentConfiguration.AutoSetGMT  =	 GetDialogItemControlValue (bootDia, autoSetGMT)   ?1:0;
	currentConfiguration.EnableRoot =	 GetDialogItemControlValue (bootDia, enableRoot)   ?1:0;
	currentConfiguration.SingleUser =	 GetDialogItemControlValue (bootDia, singleUser)   ?1:0;
	currentConfiguration.KernelDebug =	 GetDialogItemControlValue (bootDia, kernelDebug)  ?1:0;
	currentConfiguration.JumpDebugger =	 GetDialogItemControlValue (bootDia, jumpDebugger) ?1:0;
}


/* A hit on a dialog box. Determine which dialog was in the foreground,
   process any command- or accelerator-keys, and finally hand off to HandleXxxxx().
   (Where Xxxxx is one of our (currently) 4 dialogs.) */
int
DoDialog (EventRecord *event)
{
	char 		key;
	DialogPtr	dialog;
	short		item, sel = 0;

	/* Find out which Dialog is meant. */
	dialog = FrontWindow ();
	/* Handle ESC, Return and the command keys. */
	if (event->what == keyDown) {
		key = event->message & charCodeMask;
		if (event->modifiers & cmdKey) {
			MaintainMenus ();
			if (key == 'x' || key == 'X') {
				DlgCut (dialog);
				return 1;
			} else if (key == 'c' || key == 'C') {
				DlgCopy (dialog);
				return 1;
			} else if (key == 'v' || key == 'V') {
				DlgPaste (dialog);
				return 1;
			} else
				return (DoCommand (MenuKey (key)));
		} else if (key == 0x03 || key == 0x0d) { /* Return, Enter */
			FakeButtonHilite (dialog, okID);
			item = okID;
			sel = 1;
		} else if (key == 0x1b) { /* Escape */
			FakeButtonHilite (dialog, cancelID);
			item = cancelID;
			sel = 1;
		}
	}

	/* Call DialogSelect only, if the event isn't handled
	   by the above keys. */
	if (! sel)
		sel = DialogSelect (event, &dialog, &item);
	if (sel)
	{
		if (dialog == aboutDia)
			HandleAbout (item);
		else if (dialog == startupDia)
			HandleStartup (item);
		else if (dialog == bootDia)
			HandleBoot (item);
		else if (dialog == serialDia)
			HandleSerial (item);
		else if (dialog == monitorDia)
			HandleMonitor (item);
		else if (dialog == machineDia)
			HandleMachine (item);
	}
	return 1;
}