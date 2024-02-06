/* Copyright © 1998. Details in the file COPYRIGHT.txt */


#include "MacBSD.h"
#include "DialogMgr.h"


DialogPtr machineDia = NULL;


static void
autoSizeRAMtext ()
{
	if (GetDialogItemControlValue (machineDia, autoSizeRAM) )
		SetDialogItemTextByItemnoToNum (machineDia, memAmount, GetRAMSize() );
}


static void
changeMACHIDtext ()
{
	if (! GetDialogItemControlValue (machineDia, changeMachID) )
		SetDialogItemTextByItemnoToNum (machineDia, machID, GetMachineType() );
}


/* Here, machine dialog is set up from currentConfiguration. */
void
DoMachineDialog (void)
{
	if (machineDia)
	{
		SelectWindow (machineDia);
		return;
	}

	machineDia = GetNewDialog (machineDI, NULL, (WindowPtr) -1);

	SetDialogItemTextByItemnoToNum (machineDia, memAmount, currentConfiguration.MemAmount);
	SetDialogItemTextByItemnoToNum (machineDia, machID,    currentConfiguration.newMACHID);

	SetDialogItemControlValue (machineDia, allegro,      currentConfiguration.SonnetAllegro	?1:0);
	SetDialogItemControlValue (machineDia, autoSizeRAM,  currentConfiguration.AutoSizeRAM   ?1:0);
	SetDialogItemControlValue (machineDia, changeMachID, currentConfiguration.ChangeMachID  ?1:0);
	SetDialogItemControlValue (machineDia, disableVbls,  currentConfiguration.NoDisableVBLs ?1:0);
	SetDialogItemControlValue (machineDia, noWarnPPC,    currentConfiguration.NoWarnPPC     ?1:0);
	SetDialogItemControlValue (machineDia, noEject,      currentConfiguration.NoEject       ?1:0);
	SetDialogItemControlValue (machineDia, videoHack,    currentConfiguration.VideoHack     ?1:0);
	SetDialogItemControlValue (machineDia, disableATalk, currentConfiguration.DisableATalk  ?1:0);

	autoSizeRAMtext ();
	changeMACHIDtext ();

	SetDialogDefaultOutline (machineDia, defItemID);
	ShowWindow (machineDia);
}


/* A hit in the booting dialog. */
void
HandleMachine(short item)
{
	switch (item)
	{
		case autoSizeRAM:
			(void) ToggleDialogControl(machineDia, item);
			autoSizeRAMtext ();
			break;
		case changeMachID:
			(void) ToggleDialogControl(machineDia, item);
			changeMACHIDtext ();
			break;
		case allegro:
		case disableVbls:
		case noWarnPPC:
		case noEject:
		case videoHack:
		case disableATalk:
			(void) ToggleDialogControl(machineDia, item);
			break;
		case machID:
			changeMACHIDtext ();
			break;
		case memAmount:
			autoSizeRAMtext ();
			break;
		case okID:
			GetMachine ();
			/* no break */
		case cancelID:
		default:
			DisposDialog(machineDia);
			machineDia = NULL;
			break;
	}
}


/* Called to read new settings from the machine dialog
   into currentConfiguration */
void
GetMachine (void)
{
	if (! machineDia)
		return;

	if (GetDialogItemControlValue (machineDia, changeMachID) )
		currentConfiguration.newMACHID = (short) GetDialogItemTextByItemnoAsNum (machineDia, machID);
	else
		currentConfiguration.newMACHID = (short) GetMachineType ();

	
	if (GetDialogItemControlValue (machineDia, autoSizeRAM) )
		RAMSize = GetRAMSize();
	else
		RAMSize = (unsigned char) GetDialogItemTextByItemnoAsNum (machineDia, memAmount);
	currentConfiguration.MemAmount = RAMSize;

	currentConfiguration.SonnetAllegro = GetDialogItemControlValue (machineDia, allegro)      ?1:0;
	currentConfiguration.AutoSizeRAM   = GetDialogItemControlValue (machineDia, autoSizeRAM)  ?1:0;
	currentConfiguration.ChangeMachID  = GetDialogItemControlValue (machineDia, changeMachID) ?1:0;
	currentConfiguration.NoDisableVBLs = GetDialogItemControlValue (machineDia, disableVbls)  ?1:0;
	currentConfiguration.NoWarnPPC     = GetDialogItemControlValue (machineDia, noWarnPPC)	  ?1:0;
	currentConfiguration.NoEject       = GetDialogItemControlValue (machineDia, noEject)	  ?1:0;
	currentConfiguration.VideoHack     = GetDialogItemControlValue (machineDia, videoHack)	  ?1:0;
	currentConfiguration.DisableATalk  = GetDialogItemControlValue (machineDia, disableATalk) ?1:0;
}