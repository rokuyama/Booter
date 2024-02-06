/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include "MacBSD.h"
#include "boot_asm_glue.h"

#include <stdio.h>		// For vsprintf() prototype
#include <stdarg.h>		// For va_list
#include <stdlib.h>		// For atol() prototype
#include <time.h>		// For time_t

#include <Devices.h>	// For OpenDriver(), CloseDriver()
#include <Gestalt.h>	// For gestalt68020
#include <Shutdown.h>	// For sdOnUnmount/sdOnRestart/ShutDwnUPP
#include <Traps.h>		// For _MemoryDispatch

#define __P(X)	X
#include "loadfile.h"
#include "lowmem_proto.h"
#include "event_check.h"

#include "sys/reboot.h"


	/* You should not change these two defines, at all. */

/* For converting between MacOS epoch and BSD epoch. */
#define DIFF19041970	2082844800

/* Address at which to load kernel. */
#define LOAD_ADDR	0



/* MAJOR TO-DOs in ufs_test.c...
 * 1. string table is fighting with envbuf for space. 1 possible fix for this
 *    in the code but currently #if'd out: Call SetPtrSize on the buffer once
 *    we learn how big the string table is. It would be nice to look and see
 *    if we can use some kernel a.out-loading code in here, but I doubt it
 *    will be feasible.
 * 2. Get rid of environment variables completely and set variables directly.
 *    This could pose problems for stuff like mac68k_machine, which is a
 *    struct, and so structure alignment would need to be assured. Also, it
 *    requires changes in the kernel, so that the booter's hard work is not
 *    simply overwritten by the kernel's getenv() calls.
 * 3. Get rid of environment variables that are no longer used, e.g.,
 *    MACOS_VIDEO, MACOS_SCC.
 * 4. Use bitfields to construct flags & dimensions.
 * 5. Could we use the Booter to dynamically construct the Mac ROM Glue
 *    tables that are now contained in the kernel, so that every kernel
 *    doesn't have to have the ROM vectors for every Macintosh model?
 * 6. Entering a SCSI ID of something like 42 seriously confuses the booter.
 * 7. On machines that have it, it would be cool to use the Thread Manager
 *    and put copyunix in its own thread, to alleviate the whole EVENT_CHECK
 *    business.
 */




       long				flags;			/* This is passed to the kernel via D4 */
static long				boothowto;		/* And this via environment variables */
       long				dimensions;
       long				SingleUser	= 0;
       char				*envbuf		= NULL;
static unsigned long	endsym=0;
static long				scc_logical;
static long				machineid, memsize, processor;
static char				*dataspace;
static char				*boot_buffer = NULL;
static char				*boot_load;
static unsigned long	boot_length,  boot_scsi_id;
static void				*boot_entry, *boot_screen_base;
static short			modem_refnum = -1;

static void				setenvl			(char *var, long val);
static void				printenv		(void);
static void				SetupEnv		(void);
pascal void				shutdownProc	(void);
static long				kern_geteof		(int fd);
static unsigned long	max_avail_mem	(void);

       copycode_entry	harry;

void
BootError (char *fmt, ...)
{
	va_list ap;
	char errstr[255];

	va_start(ap, fmt);
	vsprintf(errstr, fmt, ap);
	va_end(ap);
	ErrorPrintf("Boot error: %s\n", errstr);
	StopRun(-1);
}

static void
setenvstr (char *var, char *val)
{
	char *s;
	
	/*
	 * Start at beginning of envbuf, and move s to point to the end (to \0\0)
	 *                                                          s---------^^
	 */
	s = envbuf;
	while (*s != 0 || *(s+1) != 0) {
		s++;
	}
	s++;
	/* Copy variable name, equal sign. */
	while (*var)
		*s++ = *var++;
	*s++ = '=';
	/* Copy ASCII string */
	while (*val)
		*s++ = *val++;
	/* And write a new end-marker. */
	*s++ = 0;
	*s++ = 0;
}

static void
setenvl (char *var, long val)
{
	char buf[32];
	
	/* Store ASCII-coded unsigned long value */
	sprintf (buf, "%lu", val);

	setenvstr (var, buf);
}


/* Here's a better way to print the environment. (BRG 1.9.3b2) */
static void
printenv (void)
{
	char *s = envbuf, *valstr, var[64];
	unsigned long val;

	s++; /* Hmm... Envbuf starts with a zero. Ok.... */
	while (*s != 0) {	/* When we get to the end of the envbuf, there are two zero bytes. */
		if (sscanf(s, "%s", var) == 1) /* read this var=val pair in */
		{
			/* find equals */
			if ((valstr = strchr(var, '=')) != NULL) {
				*valstr++ = 0;
				/* convert to long */
				val = atol(valstr);
				/* print it */
				ErrorPrintf("Env: “%s” set to '%s' (%ld, 0x%lx).\n",
								var, valstr, val, val);
			}
		}
		while (*s != 0) s++;	/* Advance s over this var=val pair */
		s++;	/* Advance s over the zero byte */
	}
}


/* Switch the caches off. */
/* Use Apple's routines which will automagically detect '020/'030/'040,
 * as suggested by Mikael Forselius <mikaelf@olando.se>
 */
static void
TurnCachesOff(void)
{
	FlushInstructionCache();
	FlushDataCache();

	SwapInstructionCache(false);
	SwapDataCache(false);
}


void
SetupEnv(void)
{
	u_long	buff_offset = boot_buffer - boot_load;

	if (currentConfiguration.ATAdisk)
	{
		char	devSpec[4];

		sprintf (devSpec, "%d,%d", currentConfiguration.Channel,
									currentConfiguration.SCSIID);
		setenvstr ("ROOT_ATA_DEV", devSpec);
	}
	else
		setenvl ("ROOT_SCSI_ID",	boot_scsi_id);

	setenvl ("SINGLE_USER",	SingleUser);
	setenvl ("VIDEO_ADDR",	videoaddress);
	setenvl ("ROW_BYTES",	rowbytes);
	setenvl ("FLAGS",		flags);	/* Should continue to pass boot flags here */
	setenvl ("SCREEN_DEPTH",screendepth);
	setenvl ("DIMENSIONS",	dimensions);
	setenvl ("BOOTTIME",	time((time_t *) 0) - DIFF19041970);
	setenvl ("GMTBIAS",		currentConfiguration.GMT_bias);
	setenvl ("BOOTERVER",	currentConfiguration.booterMajor);
	setenvl ("MACOS_VIDEO",	video_logical);
	setenvl ("MACOS_SCC",	scc_logical);
	setenvl ("MACHINEID",	machineid);
	setenvl ("MEMSIZE",		memsize);
	setenvl ("GRAYBARS",	currentConfiguration.KernelDebug);
	setenvl ("SERIALECHO",	currentConfiguration.SerBootEcho);
	if (currentConfiguration.SerConsole & 0x01)
		setenvl ("SERIALCONSOLE", (currentConfiguration.SerConsole & 0x02) ? 2 : 1);
	else
		setenvl ("SERIALCONSOLE", 0);
	setenvl ("SERIAL_MODEM_FLAGS",	currentConfiguration.ModemFlags);
	setenvl ("SERIAL_MODEM_HSKICLK",currentConfiguration.ModemHSKiClock);
	setenvl ("SERIAL_MODEM_GPICLK",	currentConfiguration.ModemGPiClock);
	setenvl ("SERIAL_PRINT_FLAGS",	currentConfiguration.PrinterFlags);
	setenvl ("SERIAL_PRINT_HSKICLK",currentConfiguration.PrinterHSKiClock);
	setenvl ("SERIAL_PRINT_GPICLK",	currentConfiguration.PrinterGPiClock);
	setenvl ("SERIAL_MODEM_DSPEED",	currentConfiguration.ModemDSpeed);
	setenvl ("SERIAL_PRINT_DSPEED",	currentConfiguration.PrinterDSpeed);
	setenvl ("PROCESSOR",	processor - gestalt68020);
	setenvl ("END_SYM",		endsym);
	setenvl ("ROMBASE",		(unsigned long)GetROMBase());
	setenvl ("TIMEDBRA",	GetTimeDBRA());
	setenvl ("ADBDELAY",	GetADBDelay());
	
	/* Walter's boot environment (1.8-1.9 BRG) */
	setenvl ("HWCFGFLAGS",		GetHWCfgFlags());
	setenvl ("HWCFGFLAG2",		GetHWCfgFlag2());
	setenvl ("HWCFGFLAG3",		GetHWCfgFlag3());
	setenvl ("ADBREINIT_JTBL",	GetADBReInitJTbl());

	setenvl ("BOOTHOWTO",	boothowto);
	setenvl ("MARK_START",	kVec[MARK_START] - buff_offset);
	setenvl ("MARK_SYM",	kVec[MARK_SYM]   - buff_offset);
	setenvl ("MARK_END",	kVec[MARK_END]   - buff_offset);
}


/*
 * This shutdown manager procedure sets up the kernel "environment" and
 * then calls copycode() (indirectly).
 */
pascal void
shutdownProc (void)
{
	FlushVol(0,0);		/* Does this flush everything? */

	/* Flush caches. (1.9 BRG) */
	FlushInstructionCache();
	FlushDataCache();

	/* Turn off caches - 1.9.2 */
	TurnCachesOff();

	/* Disable interrupts. */
	disable_intr();

	/* Call copycode-in-the-kernel. */
	(*harry)(boot_buffer, boot_load, boot_length, boot_entry,
		boot_screen_base, boot_scsi_id);
}


static unsigned long
max_avail_mem(void)
{
	Size growSize;

	return (unsigned long) MaxMem(&growSize);
}


/* The CLEANUP for copyunix (which runs 'till the end of the file) 
 * requires the buffer--for the kernel--to be disposed.
 */
#undef CLEANUP
#define CLEANUP { if (buf) DisposPtr ((Ptr) buf); return; }



/* This is the humongous booting procedure. Handle with care.
 * Booter 2.0 will split this thing up into 1.0e6 little procedures
 * that are easy to understand. :)
 */
void
copyunix (long io)
{
	long	i;
    char	*buf = NULL;
	caddr_t	bufstart = 0;
	long 	bufend = 0;
    char	*copyptr;
	long	copycode_len;
    long	len;
    long	clofset;
	u_long	avail_mem;
	OSErr	err;
	int		shutdownType;


	EVENT_CHECK;

	/* Calculate total size of kernel buffer. */
	/* The kernel buffer looks like:
	 *  kernel text
	 *  kernel data
	 *  kernel bss (empty)
	 *  kernel symbol table
	 *  kernel string list (where we search for symbols)  [ must hunt for size ]
	 *  envbuf   [ whatever's left ]
	 *  unused space
	 *  a copy of copycode() <-- harry points to entrypoint
	 */
	if ( ! kernel_parse(io, COUNT_KERNEL) )
	{
		BootError("kernel_parse(COUNT_KERNEL) failed\n");
		return;
	}

	EVENT_CHECK;

	clofset = __LDPGSZ - 1;

	DebugPrintf(1, "PageSize = %ld, Start %ld, entry %ld, NSym %ld, Sym %ld, End %ld\n",
						__LDPGSZ, kVec[MARK_START], kVec[MARK_ENTRY],
							kVec[MARK_NSYM], kVec[MARK_SYM], kVec[MARK_END]);

	len = kVec[MARK_END] + 262144;	/* envbuf, unused space, copy of copycode() */
//	len = 1802669;

	EVENT_CHECK;

	avail_mem = max_avail_mem();
	if (len > avail_mem)
	{
		BootError("Not enough free memory to load this kernel.\n"
			"Try increasing the Booter's memory partition in the,\n"
			"Finder, quitting other running programs, or disabling\n"
			"unneeded system extensions.\n");
		return;
	}
	EVENT_CHECK;

	/* Allocate sufficient memory for the kernel buffer. */
	buf = (char *) NewPtrClear(len);
	if (buf == NULL) {
		BootError("Failed to allocate %ld bytes.\n",len);
		return;
	}
	EVENT_CHECK;
	DebugPrintf (1, "Allocated %ld bytes.\n", len);
	bufstart = buf; /* Save pointer to original start of buffer. */
	bufend = (long) (bufstart) + len;
	/* Force the kernel memory to be contiguous and fixed in physical memory.
	 * The area is marked uncacheable, too, as a side effect. Note that this is
	 * not possible in all 7.x installations,  notably some Minimal Installs.
	 * (Therefore, we check for the existence of VM, and of the _MemoryDispatch
	 * trap -- just to be paranoid, before trying.)
	 */
	if (HaveVM() && TrapAvailable(_MemoryDispatch)) {
		if ((err = LockMemoryContiguous (buf, len)) != noErr) {
			BootError("Error #%d locking kernel memory.\n"
				"Try turning off Virtual Memory in the Memory control panel.\n",err);
			CLEANUP;
		} else {
			DebugPrintf(1, "Successfully locked memory.\n");
		}
	}
	EVENT_CHECK;
//memset(buf,len,0);
	/* We must start loading on a logical page boundary. */
	while ((long)bufstart & clofset)
		bufstart++, len--;

	kVec[MARK_START] = (u_long)bufstart;
	if ( ! kernel_parse(io, LOAD_KERNEL) )
	{
		BootError("kernel_parse(LOAD_KERNEL) failed\n");
		CLEANUP;
	}
//hex_file_dump(7, "dumpfile-buff", (unsigned char *)bufstart, len);	// bufstart is page aligned


if ( kernelStringToAddress("end", (long *)&endsym) )
	DebugPrintf(1,"_end=%ld(0x%lx)\n", endsym, endsym);

	EVENT_CHECK;

	/* The kernel buffer now looks like:
	 *	buf								page alignment padding
	 *	bufstart
	 *			addr[MARK_START]		kernel text
	 *									kernel data
	 *									kernel bss
	 *			addr[MARK_SYM]			kernel symbol table or ELF section headers
	 *									kernel string list
	 *			addr[MARK_END]			envbuf
	 *									unused space
	 *	bufend - 1K?					a copy of copycode()
	 */

	envbuf		= (caddr_t)kVec[MARK_END];
	endsym		= envbuf - bufstart;

	EVENT_CHECK;
	/* Set boot advice flags (from sys/reboot.h) */
	boothowto = (currentConfiguration.SingleUser   ? RB_SINGLE  : 0) |
				(currentConfiguration.AskName      ? RB_ASKNAME : 0) |
				(currentConfiguration.JumpDebugger ? RB_KDB     : 0);
	if ( ! setKernelLongVal("_boothowto", boothowto) )
	{
		Output("Failed to set variable boothowto\n");
/*		BootError("Failed to set variable boothowto\n");
		CLEANUP
 */
	}

	/*
	 * OK.  Now we can get to the even uglier business of booting.
	 * Eeeehhh, who's kidding whom? It's all ugly.
	 */
	DebugPrintf (1, "start address = 0x%lx.\n", kVec[MARK_ENTRY]);

	/* Initialize boot environment (which goes after string table). */
	envbuf[0] = 0;
	envbuf[1] = 0;
	DebugPrintf (1, "total kernel buffer space used = %ld (0x%lx).\n", (long)(envbuf-buf),
			(long)(envbuf-buf));

	EVENT_CHECK;

	copyptr = (char *) copycode;

	// Check to see if this is really the function,
	// or if it is a pointer into a jump table.
	//
	// A JMP instruction looks like 4E F9 XX XX XX XX

	if ( copyptr[0] == 0x4E && (unsigned char) copyptr[1] == 0xF9 )
		copyptr = (char *) * ((u_long *) (copyptr + 2));


	// Search for the end of copycode. (Assuming it just conrains one RTS)
	// An RTS instruction looks like 4E 75

	for ( copycode_len = 0; copycode_len < 1024; copycode_len ++ )
		if ( copyptr[copycode_len] == 0x4E && copyptr[copycode_len+1] == 0x75 )
		{
			copycode_len += 2;		/* We want to include the RTS instruction! */
			break;
		}

	if ( copycode_len < 70 )
		copycode_len = 70;


	DebugPrintf (1, "The warm and fuzzy copycode() is at 0x%08lx for 0x%03lx\n",
													(long)copyptr, copycode_len);
    DebugPrintf (6, "Hex dump of copycode():\n");
	EVENT_CHECK;
    hex_dump (6, (unsigned char *)copyptr, copycode_len);

	EVENT_CHECK;

	/* Round len down to the next lowest logical page boundary,
	 * so that the start and end of the buffer are both on logical page
	 * boundaries. This depends on the fact that there is extra space at
	 * the end of the buffer!!
	 */
   	len -= (clofset+1); /* LAK says I need this. */
    len &= ~clofset;
	/* Copy copycode() to the last 1KB of the buffer. */
    harry = (copycode_entry)(bufstart + len - 1024);

	EVENT_CHECK;
    for ( i = 0; i < copycode_len; i++ )
    	*(((char *)harry)+i) = copyptr[i];
	EVENT_CHECK;

	DebugPrintf(1, "harry() is at 0x%lx\n",(unsigned long)harry);
	DebugPrintf(6, "Hex dump of Harry(), our friend!\n");
	DebugPrintf(6, "(...should be the same as copycode(), above...)\n");
	hex_dump(6, (unsigned char *)harry, copycode_len);
	EVENT_CHECK;
	
//hex_file_dump(7, (unsigned char *)bufstart, len);	// bufstart is page aligned
//CLEANUP
//return;
	/* Offset entry point in proportion to address at which we are
	 * loading the kernel. This shouldn't really have any effect since we
	 * load the kernel at address 0.
	 */
    kVec[MARK_ENTRY] += LOAD_ADDR;

	boot_buffer = bufstart;
	boot_length = (unsigned long) (len/4);
	boot_entry = (void *) kVec[MARK_ENTRY];
	boot_load = (void *) LOAD_ADDR;
	boot_screen_base = (void *) GetScrnBase();
	boot_scsi_id = (unsigned long) currentConfiguration.SCSIID;
	DebugPrintf (1, "\nfrom = 0x%08lx, to = 0x%08lx, len = %ld, entry = 0x%08lx\n",
	             boot_buffer, boot_load, boot_length, boot_entry);

	EVENT_CHECK;

	processor = GetProcessor();
	machineid = currentConfiguration.newMACHID;

	if ( HasAppleScript() && ! RunningOnPPC() )
	{	/* Try to kill off other running programs before booting (1.9 BRG) */
		Output ("Attempting to kill all running programs...\n");
		KillAllOtherApps ();
	}

	EVENT_CHECK;

	if (currentConfiguration.MonitorDepth ||
		currentConfiguration.setToGreys ||
		currentConfiguration.MonitorSize)
	{
		Output ("Changing Monitor settings...\n");
		MonitorChange (currentConfiguration.MonitorDepth,
						currentConfiguration.setToGreys,
						currentConfiguration.MonitorSize);
	}

	EVENT_CHECK;

	if (currentConfiguration.SonnetAllegro)
		AllegroDriverFix ();

	if ( currentConfiguration.DisableATalk )
	{
		short	driver;

		// I wish it were this simple!

		if ( OpenDriver("\p.ATP", &driver) == noErr
					&& CloseDriver(driver) == noErr )
			Output ("Tried to close AppleTalk\n");
		else
			Output ("Failed to close AppleTalk\n");
	}

	if ((modem_refnum == -1) && (currentConfiguration.OpenModemPort != 0))
		(void) OpenDriver("\p.AOut", &modem_refnum);



	Output ("  Bye-bye...\n");
	Output ("        So I sez to him...  The real way\n");
	Output ("        that it should be done is to...\n");
	
	DebugPrintf (1, "Serial console flags = %d\n", currentConfiguration.SerConsole);


	/* globals */
	/* This global should go away and is only present for compatibility with
	 * ancient kernels.
	 */
	scc_logical = (unsigned long) GetSCCRd();


	/* This code uses GetMainDevice to determine the video parameters.
	 * Hopefully, it won't point to something bogus if there's no video.
	 * 
	 * Also, the usual MacOS axiom of "don't just automatically use MainDevice
	 * (or CurrentDevice) for everything because the user may have two (or more)
	 * monitors, etc." doesn't apply here because autoconfiguration on the BSD
	 * side will take care of that. We hope.
	 * 
	 * As an aside, what would be REALLY cool would be a Monitors control panel-
	 * like interface wherein you could drag a little smiling Daemon icon onto
	 * the monitor on which you wish to have the console appear.
	 */

	getVideoAddress ( GetMainDevice () );


	/* all this inscrutable-looking verbiage seriously needs to be replaced by
	 * something with a sane data structure, preferably unions of bitfields and
	 * longs. But we'll get to that in a later release. :-) -BRG
	 */
	dimensions = screenWidth | (screenHeight << 16);

	flags  = (machineid << 2) | (0x00003 & (processor - gestalt68020));  /* machine & processor */
	flags |= ( ((unsigned long)(currentConfiguration.SerBootEcho & 0x1) ) << 16);	/* serial boot console */
	flags |= ( ((unsigned long)(currentConfiguration.KernelDebug & 0x1) ) << 17);	/* more console debug */
	flags |= ( ((unsigned long)(currentConfiguration.EnableRoot & 0x1) )  << 18);	/* rootdev */
	flags |= ( ((unsigned long)(RAMSize & 0x1f) )  << 7);  /* mem amount */
	flags |= 0x80000000L;  /* ASCII-buffer bit */

	memsize = RAMSize;

	if (currentConfiguration.SingleUser)
		SingleUser = 2;

	/* Print environment variables (1.8 BRG)
	 * 
	 * As of 1.9.3b2, the environment is now calculated *ONLY ONCE*,
	 * just before it is printed.
	 */
	SetupEnv(); /* Calculate environment variables */
	if (! currentConfiguration.NoEnvDumps)
		printenv (); /* Print environment variables */
	EVENT_CHECK;

	/* Fill in video driver information (1.8 BRG) */
	set_video_info ((unsigned char *) bufstart);

	DebugPrintf(1, "\n\n");

	/* Pause before boot (1.8 BRG) */
	if (currentConfiguration.PauseBeforeBoot)
		if (Alert (bootConfirmAlrt, NULL) != 1)
			CLEANUP

//hex_file_dump(7, "dumpfile-buff", (unsigned char *)bufstart, len);	// bufstart is page aligned
	
	EVENT_CHECK;
	CloseLogFile ();
	EVENT_CHECK;

	if (RunningOnPPC () )
	{
		Output("\n\n\nYou can't really run BSD/Mac68k on a PPC machine!\n");
		Output("*****      Aborting boot sequence now.      *****\n");
		StopRun(io);
		return;
	}

	/* Turn off vertical blanking interrupts (1.8 BRG) */
	if (! currentConfiguration.NoDisableVBLs)
		turn_off_interrupts ();


	 EVENT_CHECK; EVENT_CHECK; EVENT_CHECK;			/* Last chance to bail out! */


	/* Install the shutdown procedure. */

	if ( currentConfiguration.NoEject )
		shutdownType = sdOnUnmount;
	else
		shutdownType = sdOnRestart;
	ShutDwnInstall ((ShutDwnUPP) shutdownProc, shutdownType);

	/* go to it! */
	ShutDwnStart ();

	/* SHoule never get here. */
	BootError ("ShutDwnStart() returned. How?\n");
	return;

shread:
	BootError ("Short kernel_read. Kernel file corrupt?\n");
}
