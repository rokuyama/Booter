/* machdep.c - Lots of stuff that helps the booter decide what to do
 * based on what hardware configuration is running.
 */

#include "MacBSD.h"

#include <ConditionalMacros.h>	// For UNIVERSAL_INTERFACES_VERSION
#include <Gestalt.h>			// For Gestalt() prototype, gestaltProcessorType et c.
#include <LowMem.h>				// For LMGetHWCfgFlags
#include <Resources.h>			// For Get1Resource() prototype
#include <Traps.h>				// For _InitGraf and _Unimplemented


static	long	GetGestalt(OSType);

static	long
GetGestalt(OSType id)
{
	OSErr	err;
	long	response;
	
	err = Gestalt(id, &response);
	if (err != noErr) return -1;
	return response;
}

long
GetProcessor(void)
{
	return GetGestalt(gestaltProcessorType);
}

long
GetMMU(void)
{
	return GetGestalt(gestaltMMUType);
}

bool
CheckHardware(void)
{
	long processor = GetProcessor();
	long mmu = GetMMU();

	if (processor < gestalt68020)
		return 0;
	
	if (mmu < gestalt68851)
		return 0;

	return 1;
}

bool
MacOS32BitMapping(void)
{
	long has32bit = GetGestalt(gestaltAddressingModeAttr);
	
	if (has32bit < 0)
		return 0;
	/* Need to remember that the gestalt Attr things are bit NUMBERS, not MASKS. */
	return (has32bit & (1<<gestalt32BitAddressing)) ? 1 : 0;
}

bool
HaveVM(void)
{
	long vmAttr = GetGestalt(gestaltVMAttr);
	
	if (vmAttr < 0)
		return 0;
	return (vmAttr & (1<<gestaltVMPresent)) ? 1 : 0;
}

bool
UsingVM(void)
{
	long 	logical=0, physical=0;
	OSErr 	gestaltErr;

	gestaltErr = Gestalt (gestaltLogicalRAMSize, &logical);
	gestaltErr = Gestalt (gestaltPhysicalRAMSize, &physical);

	return logical > physical;
}

short
GetRAMSize (void)
{
	long	mem_bytes = GetGestalt(gestaltPhysicalRAMSize);
	
	mem_bytes /= ((long)1024 * 1024);
	return (unsigned char) mem_bytes;
}

short 
GetGMTBias(void)
{
	unsigned long	GMT_Offset;	/* GMT offset is initially in seconds */
	MachineLocation myLocation;

	ReadLocation(&myLocation);
	GMT_Offset=myLocation.u.gmtDelta;
	
	/* Now we go through contortions to 
		set the high order byte of the long
		word(occupied in the MachineLocation
		record by the DST flag).              */
	GMT_Offset&=0x00ffffff;
	if(GMT_Offset&0x00100000)
		GMT_Offset|=0xff000000;
		
	/* The booter wants the offset in minutes */
	/* so we oblige it.   SCB 4/19/97.        */
	return((short) ((signed long) GMT_Offset / (long) 60 ));
}

long
GetMachineType(void)
{
	return GetGestalt(gestaltMachineType);
}


short
GetDisplayMgrVers (void)
{
	long	DMStuff = GetGestalt (gestaltDisplayMgrAttr);

	if (DMStuff < 0)
	{
		DebugPrintf (1, "GetGestalt (gestaltDisplayMgrAttr) failed\n");
		return 0;
	}

	if (! DMStuff & (1 << gestaltDisplayMgrPresent) )
	{
		DebugPrintf (1, "DisplayMgr not present\n");
		return 0;
	}

	if (GetGestalt (gestaltDisplayMgrVers) >= 0x00020000)
		return 2;
	else
		return 1;
}


bool
HasControlStrip (void)
{
	long	ControlStripStuff = GetGestalt (gestaltControlStripAttr);

	if (ControlStripStuff < 0)
	{
		DebugPrintf (1, "GetGestalt (gestaltControlStripAttr) failed\n");
		return 0;
	}

	return (ControlStripStuff & (1 << gestaltControlStripExists) ) ? 1 : 0;
}

// CodeWarrior 9 has Universal Interfaces 2.1.2,
// but I don't know when the desktop picture appeared
//
#if ! defined (UNIVERSAL_INTERFACES_VERSION) || UNIVERSAL_INTERFACES_VERSION < 0x0300
enum {
        /* the 'dkpx' selector is installed by the Desktop Pictures INIT */
    gestaltDesktopPicturesAttr = 'dkpx',		/* bit zero -> control panel is installed */
    gestaltDesktopPicturesInstalled = 0,		/* bit one -> a picture is currently displayed */
    gestaltDesktopPicturesDisplayed = 1
};
#endif

bool
HasDesktopPicture (void)
{
	long	DPxStuff = GetGestalt(gestaltDesktopPicturesAttr);


	if (DPxStuff < 0)
	{
		DebugPrintf (1, "GetGestalt (gestaltDesktopPicturesAttr) failed\n");
		return 0;
	}

	return ( (DPxStuff & (1 << gestaltDesktopPicturesInstalled) )
			&& (DPxStuff & (1 << gestaltDesktopPicturesDisplayed) ) );
}

bool
HasAppleScript (void)
{
	long ScriptStuff = GetGestalt (gestaltAppleEventsAttr);

	if (ScriptStuff < 0 )
	{
		DebugPrintf (1, "GetGestalt (gestaltAppleEventsAttr) failed\n");
		return 0;
	}

	return ( ScriptStuff & (1 << gestaltAppleEventsPresent) ) ? 1 : 0;
}

#include "ATA.h"


/* This is based on ATAManagerPresent(), from TechNote 1098 */

bool
HasATA (void)
{
	UInt16	configFlags = LMGetHWCfgFlags();

	if ( ! (configFlags & 0x0080) )			/* Flag set if hardware is present */
		return false;

	if (TrapAvailable (kATATrap) )			/* Does the ATA manager exist */
		return true;

	return false;
}

/* (see Inside Mac VI 3-8) */
bool
TrapAvailable(short theTrap)
{
	TrapType                trapType;

#define NumToolboxTraps() (						\
	(NGetTrapAddress(_InitGraf, ToolTrap)		\
		== NGetTrapAddress(0xAA6E, ToolTrap))	\
	    ? 0x200 : 0x400							\
    )
#define GetTrapType(theTrap) (						\
	(((theTrap) & 0x800) != 0) ? ToolTrap : OSTrap	\
    )

	
	trapType = GetTrapType(theTrap);
	
	if (trapType == ToolTrap) {
	    theTrap &= 0x07FF;
	    if (theTrap >= NumToolboxTraps())
		theTrap = _Unimplemented;
	}
	
	return (
	    NGetTrapAddress(theTrap, trapType)
	    != NGetTrapAddress(_Unimplemented, ToolTrap)
	);
}


bool
RunningOnPPC (void)
{
	long	arch = GetGestalt (gestaltSysArchitecture);

	if (arch < 0)
	{
		DebugPrintf (1, "GetGestalt (gestaltSysArchitecture) failed\n");
		return 0;
	}

	return (arch == gestaltPowerPC);
}


/* This was stolen from pp. 214-215, chap. 12, "Macworld Mac Programming FAQs" */

static	char
GetNumGDevices (void)
{
	char		count = 0;
	GDHandle	device;

	for ( device = GetDeviceList(); device != nil; device = GetNextDevice (device) )
		if ( (TestDeviceAttribute (device, screenDevice) )
					&& (TestDeviceAttribute (device, screenActive) ) )
			++count;

	return count;
}


typedef struct sizeResource
{
	char		flags[2];

	long		preferred;	/* ??? */
	long		minimum;	/* ??? */
} sizeRes;


#include <stdio.h>

#define Printf ErrorPrintf


void
OutputMachineDetails (void)
{
	Handle	mem;
	char	tmp;
	char	systemVersion[6];			/* System version theoretically 4 hex digits long */
	char	versionCopy[10],			/* Enough to fit 1.11.5a5 plus a bit */
			verLength;

	systemVersion[0] = '\0';
	sprintf (systemVersion, "%lx", GetGestalt (gestaltSystemVersion) );
	systemVersion[5] = '\0';
	systemVersion[4] = systemVersion[2];
	systemVersion[3] = '.';
	systemVersion[2] = systemVersion[1];
	systemVersion[1] = '.';

	verLength = *((*ver)->shortVersion);
	memcpy(versionCopy, ((*ver)->shortVersion) + 1, verLength);
	versionCopy[verLength] = '\0';
	Printf ("Booter version   : %s\n",  versionCopy);
	Printf ("Free heap memory : %ldKB", FreeMem() / 1024);

	mem = Get1Resource ('SIZE', 0);			/* preferred memory size resource */

	if ( mem == 0 )
		mem = Get1Resource ('SIZE', -1);	/* linked-in memory size resource */

	if ( mem == 0 )
		mem = Get1Resource ('SIZE', 1);		/* minimum memory size resource */

	if ( mem != 0)
		Printf (" (out of %ldKB total requested)\n",
				((sizeRes *) *mem)->preferred / 1024);
														
	Output ("-------------------------------------\n");

	Printf ("Macintosh type (MACHID) : %ld\n",  GetMachineType() );
	Printf ("RAM size (physical RAM) : %dMB\n", (int) GetRAMSize() );
	Printf ("System file version     : %s\n",   systemVersion);

	if ( HasATA() )
		Output ("Has ATA. ");
	if ( HasAppleScript() )
		Output ("Has AppleScript. ");
	if ( HasControlStrip() )
		Output ("Has Control Strip. ");
	if ( HasDesktopPicture() )
		Output ("Has Desktop Picture. ");
	if ( (tmp = GetDisplayMgrVers() ) > 0 )
		Printf ("Has Display manager version %d. ", (int) tmp );
	if ( (tmp = GetNumGDevices() ) > 1 )
		Printf ("Has %d screens! ", (int) tmp);

	Output ("\n-------------------------------------\n");
}