/************************************************************************/
/* Startup.c - Functions for managing the Startup preferences dialog	*/
/*																		*/
/* Taken out of Dialogs.c to tidy it up a bit		on 18th Oct. 1997	*/
/*													by Nigel Pearson	*/
/*																		*/
/* Copyright © 1993. Details in the file COPYRIGHT.txt					*/
/************************************************************************/


#include "MacBSD.h"
#include <StandardFile.h>		// For StandardFileReply
#include "DialogMgr.h"

DialogPtr startupDia = NULL;


/* Here, Startup dialog item values are set up from currentConfiguration. */

void
DoStartupDialog (void)
{
	if (startupDia)
	{
		SelectWindow (startupDia);
		return;
	}

	startupDia = GetNewDialog (startupDI, NULL, (WindowPtr) -1);

	SetDialogItemTextByItemnoToNum (startupDia, autoTimeID,   currentConfiguration.TimeOut);
	SetDialogItemTextByItemnoToNum (startupDia, debugLevelID, currentConfiguration.DebugLevel);

	SetDialogItemControlValue (startupDia, autoBootID,    currentConfiguration.AutoBoot       ?1:0);
	SetDialogItemControlValue (startupDia, abortNonFatal, currentConfiguration.AbortNonFatal  ?1:0);
	SetDialogItemControlValue (startupDia, logToFileID,   currentConfiguration.LogToFile      ?1:0);
	SetDialogItemControlValue (startupDia, noEnvDumps,    currentConfiguration.NoEnvDumps     ?1:0);
	SetDialogItemControlValue (startupDia, pauseBoot,     currentConfiguration.PauseBeforeBoot?1:0);

	SetDialogDefaultOutline (startupDia, defItemID);
	ShowWindow (startupDia);
}


/* A hit in the Startup dialog. */

void
HandleStartup (short item)
{
	extern int			debugLevel;
	StandardFileReply	frep;

	switch (item)
	{
		case autoBootID:
		case abortNonFatal:
		case pauseBoot:
		case logToFileID:
		case noEnvDumps:
			(void) ToggleDialogControl(startupDia, item);
			break;
		case autoTimeID:
		case debugLevelID:
			break;
		case setLogFileID:
			StandardPutFile ("\pLogfile: (ignore <replace ?>)",
			                 "\pbootlog", &frep);
			if (frep.sfGood)
				currentConfiguration.LogFile = frep.sfFile;
			break;
		case okID:
			GetStartup ();
			/* fall through */
		case cancelID:
		default:
			DisposDialog (startupDia);
			startupDia = NULL;
			break;
	}
}


/* Called to read new preferences from the Startup dialog into currentConfiguration */

void
GetStartup (void)
{
	extern int	debugLevel;
	short		old_debugLevel;

	if (! startupDia)
		return;

	old_debugLevel = debugLevel;
	debugLevel = currentConfiguration.DebugLevel =
		(short) GetDialogItemTextByItemnoAsNum(startupDia, debugLevelID);
	if (old_debugLevel != debugLevel)
		DebugPrintf (debugLevel, "Debugging at level %d.\n", debugLevel);

	currentConfiguration.TimeOut = GetDialogItemTextByItemnoAsNum(startupDia, autoTimeID);

	currentConfiguration.AbortNonFatal =   GetDialogItemControlValue (startupDia, abortNonFatal) ?1:0;
	currentConfiguration.PauseBeforeBoot = GetDialogItemControlValue (startupDia, pauseBoot)     ?1:0;
	currentConfiguration.AutoBoot =        GetDialogItemControlValue (startupDia, autoBootID)    ?1:0;
	currentConfiguration.LogToFile =       GetDialogItemControlValue (startupDia, logToFileID)   ?1:0;
	currentConfiguration.NoEnvDumps =      GetDialogItemControlValue (startupDia, noEnvDumps)    ?1:0;
}