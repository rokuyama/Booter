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
 * appkill.c 10/6/95 Brian Gaeke <brg@dgate.org>
 * 
 * Application quitting code, using Apple Events and Process Manager. I took
 * this straight out of an earlier program dated 28 December 1992. I've violated
 * my own copyright. Darn. I think I'll sue.
 *
 * I guess it just goes to show how out of touch I am with Mac programming that
 * I still consider Apple Events and the Process Manager "new". whoo. I guess I
 * quit learning when they quit sending me those free Developer CDs. :)
 *
 * One of the more significant improvements that could be made to this code
 * would be to run indent over it. The other would be to change all the variable
 * names to 'foo' with various random numbers appended. A somewhat less important
 * problem is that of error checking, because Allen said, "Don't stop the boot when
 * things go wrong." (Obviously Allen does not work for Saturn. heh... :)
 */

#include "MacBSD.h"
#include <AppleEvents.h>


/* SendKill
 *
 * Sends QUIT apple event to the app with the specified process serial no.
 * No error checking, but it could be hacked in.
 *
 * If you are confused as to why this code doesn't work in your program,
 * try asking yourself whether the 'High Level Event Aware' bit is set in your
 * SIZE resource......
 */

static void
SendKill (ProcessSerialNumber psn, Str255 psName)
{
	OSErr			err;
	AppleEvent		theEvent, reply;
	AEAddressDesc	theAddress;

	memset (&theEvent, 0, sizeof(theEvent) );
	memset (&theAddress, 0, sizeof(theAddress) );
	
	err = AECreateDesc (typeProcessSerialNumber, (Ptr)&psn, sizeof(psn), &theAddress);
	if (err != noErr)
	{
		ErrorPrintf ("AECreateDesc(%s) failed (%d)\n", psName, err);
	}
	err = AECreateAppleEvent (kCoreEventClass, kAEQuitApplication, &theAddress,
							  kAutoGenerateReturnID, kAnyTransactionID, &theEvent);
	if (err != noErr)
	{
		ErrorPrintf ("AECreateAppleEvent(%s) failed (%d)\n", psName, err);
	}
	err = AESend (&theEvent, &reply, kAENoReply + kAEAlwaysInteract + kAECanSwitchLayer,
										kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
	if (err != noErr)
	{
		ErrorPrintf ("AESend(%s) failed (%d)\n", psName, err);
	}
	AEDisposeDesc (&theAddress);
	AEDisposeDesc (&theEvent);
}


extern pascal Boolean SBIsControlStripVisible(void)				TWOWORDINLINE(0x7000, 0xAAF2);
extern pascal void    SBShowHideControlStrip(Boolean showIt)	THREEWORDINLINE(0x303C, 0x0101, 0xAAF2);


/* KillAllOtherApps
 *
 * Sends QUIT apple events to all other apps.
 */

void
KillAllOtherApps (void)
{
	ProcessSerialNumber		psn, me;
	ProcessInfoRec			info;
	Str255					processName;


	psn.highLongOfPSN = 0,	psn.lowLongOfPSN = kNoProcess;
	me.highLongOfPSN = 0, 	me.lowLongOfPSN = kNoProcess;

	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processAppSpec = nil;
	info.processName = processName;
	
	if (GetCurrentProcess (&me) != noErr)
	{
		Output ("GetCurrentProcess() failed\n");
		return;
	}

	while (GetNextProcess(&psn) == noErr)
	{
		/* Whaa, they said I couldn't != structs. It's only after being
		 * awake for 18+ hours that I begin to appreciate what C++ is for.
		 */

		if (memcmp (&psn, &me, sizeof(ProcessSerialNumber) ) == 0)
			continue;

		if (GetProcessInformation (&psn, &info) != noErr)
			Output ("GetProcessInformation() failed\n");
		else
		{
			PtoCstr (processName);
			DebugPrintf (1, "Found process %s\n", processName);
		}

		SendKill (psn, processName);
	}

/****************
	if ( HasControlStrip() && SBIsControlStripVisible () )
		Output ("Anyone know how to kill the control strip?\n\n");
	//	SBShowHideControlStrip (0); */
/****************
	if ( HasDesktopPicture() )
		Output ("How do I clear the Desktop Picture?\n\n"); */
		/* SendToApp("Desktop Pictures", "clear desktop picture"); */

/*	SendToApp("DŽcor Daemon", QUIT); **** Redundant? */
}