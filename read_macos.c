/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include "MacBSD.h"
#include <stdio.h>
#ifdef MPW
	#undef __FCNTL__	// Actually allow this to be included
#endif
#include <fcntl.h>

void
macos_close (long fd)
{
	DebugPrintf (3, "Closing Mac OS file\n");

	if (FSClose (fd) != noErr)
		ErrorPrintf ("Could not close Mac OS file %ld\n", fd);
}


long
macos_read (long fref, unsigned char *buf, long len)
{
	long	amt;
	OSErr	err;


	amt = len;
	err = FSRead (fref, &amt, buf);

	if (err == noErr)
	{
		DebugPrintf (3, "Read %ld bytes from Mac OS file\n", amt);
		return amt;
	}

	if (err == eofErr)
	{
		DebugPrintf (3, "Hit EOF after reading %ld bytes from Mac OS file\n", amt);
		return amt;
	}

	ErrorPrintf ("Could not read %ld bytes from Mac OS file\n", len);
	return -1L;
}


long
macos_lseek (long fref, long offset, long whence)
{
	short	posMode;
	long	newPos;

    switch (whence)
    {
		case SEEK_SET:	posMode = fsFromStart; break;
		case SEEK_CUR:	posMode = fsFromMark; break;
		case SEEK_END:	posMode = fsFromLEOF; break;
		default:		posMode = fsAtMark;
    }

	if (SetFPos (fref, posMode, offset) != noErr)
		return -1;

	GetFPos (fref, &newPos);
	return newPos;
}


static short
unixToMacPerm (long unixPerm)
{
	switch (unixPerm)
	{
		case O_RDONLY : return fsRdPerm; break;
		case O_WRONLY : return fsWrPerm; break;
		case O_RDWR   : return fsRdWrPerm; break;
		default : return fsCurPerm;
	}
}


static void
unixPermToMacPos (long fref, long unixPerm)
{
	if (unixPerm & O_APPEND)
		SetFPos (fref, fsFromLEOF, 0L);
	else								/* default is same as O_EXCL */
		SetFPos (fref, fsFromStart, 0L);
}


long
macos_open (char *name, long how)
{
	short	fref;
	FSSpec	fspec;

	DebugPrintf (3, "Opening Mac OS file '%s' in current directory ...", name);

	fspec.vRefNum = 0;
	fspec.parID   = 0L;
	strcpy  ((char *)fspec.name, name);
	CtoPstr ((char *)fspec.name);

	if (FSpOpenDF (&fspec, unixToMacPerm (how), &fref) != noErr)
	{
		Output ("Failed.\n");
		fref = -1;
	}

	unixPermToMacPos (fref, how);
	return fref;
}


long
macos_fsopen (char *name, long how, FSSpec *fspec)
{
	short	fref;

	DebugPrintf (3, "Opening Mac OS file '%s' ...", name);

	if (FSpOpenDF (fspec, unixToMacPerm (how), &fref) != noErr)
	{
		Output ("\nCould not open using FSSpec. Trying current directory ...");
		fspec -> vRefNum = 0;
		fspec -> parID   = 0L;
		if (FSpOpenDF (fspec, unixToMacPerm (how), &fref) != noErr)
		{
			Output ("Failed.\n");
			fref = -1;
		}
	}

	unixPermToMacPos (fref, how);
	return fref;
}
