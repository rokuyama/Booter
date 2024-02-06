/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include "MacBSD.h"

#include <Folders.h>	// For kOnSystemDisk/kPreferencesFolderType/kDontCreateFolder
#include <Resources.h>	// For Get1Resource() prototype
#include <Script.h>		// For smSystemScript



		unsigned char	RAMSize=0;
		user_conf		currentConfiguration;
static	Str255			prefsFileName;
static	short			volRefNum;
static	FSSpec			prefsFileSSpec;
static	OSErr			osError;



static void
LoadDefaultPrefs()
{
	long	directoryID;
	short	fileRefNum;
	Handle	res;
	int		cnt          = sizeof(user_conf);


	/*
	 * Find or create a preferences file.
	 */

	GetIndString (prefsFileName, 128, 1);

	osError = FindFolder (kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder,
							&volRefNum, &directoryID);

	if (osError == noErr)
		osError = FSMakeFSSpec (volRefNum, directoryID, prefsFileName, &prefsFileSSpec);
	if (osError == noErr || osError == fnfErr)
		fileRefNum = FSpOpenResFile (&prefsFileSSpec, fsRdPerm);

	if (fileRefNum != (-1))
	{
		UseResFile (fileRefNum);
		if ((res = Get1Resource('SETU', 128)) == NULL)
		{
			Alert (noResourcesAlrt, NULL);
			fileRefNum = -1;
		}
		else if ((cnt = GetHandleSize(res)) != sizeof(user_conf))
		{
			ReleaseResource (res);
			Alert (incompPrefsAlrt, NULL);
			fileRefNum = -1;
		}
		else
		{
/*			cnt = min (cnt, SizeResource (res) ); */
			cnt = sizeof (user_conf);
			memcpy (&currentConfiguration, *res, cnt);
			ReleaseResource (res);
		}

		CloseResFile (fileRefNum);
	}
}


static void
WritePrefsFile (short fref)
{
	Handle 	res, existRes;
	int		cnt          = sizeof(user_conf);
	Str255 	resourceName = "\pPreferences";


	if ( (res = NewHandle (cnt) ) == 0)
	{
		ErrorPrintf ("Could not NewHandle(%d)\n", cnt);
		Alert (createPrefsAlrt, NULL);
		return;
	}
	
	if (GetHandleSize (res) != cnt)
	{
		Output ("Handle is wrong size\n");
		Alert (createPrefsAlrt, NULL);
		DisposeHandle (res);
		return;
	}
	
	UseResFile (fref);
	HLock (res);

	if ((existRes = Get1Resource('SETU', 128)) != NULL)	/* If the file already has the */
	{													/*   resource we want to add,  */
		RemoveResource (existRes);						/* Remove it */
		if (ResError() != noErr)
		{
			Output ("Could not remove existing SETU resource\n");
			Alert (savePrefsAlrt, NULL);
		}
		ReleaseResource (existRes);
	}

	AddResource (res, 'SETU', 128, resourceName);
	if (ResError () != noErr)
	{
		Output ("Could not add SETU resource\n");
		Alert (savePrefsAlrt, NULL);
	}
	else
	{
/*		cnt = min (cnt), SizeResource (res) );	*/
		memcpy (*res, &currentConfiguration, cnt);
		ChangedResource (res);
		WriteResource (res);
		if (ResError() != noErr)
		{
			Output ("Could not write changed resource\n");
			Alert (savePrefsAlrt, NULL);
		}
	}

	HUnlock (res);
	DisposeHandle (res);
}


static void
SaveDefaultPrefs()
{
	OSErr	osError;
	short	fileRefNum;


	FSpCreateResFile (&prefsFileSSpec, 'BSDB', 'pref', smSystemScript);
	osError = ResError();

	if (osError != noErr && osError != dupFNErr)
	{
		Output ("Count not FSpCreateResFile\n");
		Alert  (createPrefsAlrt, NULL);
	}
	else
	{
		fileRefNum = FSpOpenResFile (&prefsFileSSpec, fsWrPerm);
		
		if (fileRefNum == -1)
		{
			Output ("Could not open Preferences file for writing\n");
			Alert (savePrefsAlrt, NULL);
		}
		else
		{
			WritePrefsFile (fileRefNum);
			CloseResFile   (fileRefNum);
			FlushVol (NULL, volRefNum);
		}
	}
}

#include <ctype.h>

void
LoadUserConfiguration()
{
	user_conf	*p;
#ifdef NIGEL
	unsigned char	*c, *e;
#endif


	p = &currentConfiguration;

	/*
	 * Clear current configuration and set defaults.
	 */

	memset(p, sizeof(user_conf), 0);

	strcpy  ((char *) p->KernelName, "netbsd");
	strcpy  ((char *) p->LogFile.name, "bootlog");
	strcpy  ((char *) p->MacKernel.name, "netbsd.gz");
	CtoPstr ((char *) p->LogFile.name);
	CtoPstr ((char *) p->MacKernel.name);
	p->AutoSetGMT	= 255;
	p->AutoSizeRAM	= 255;
	p->EnableRoot	= 255;
	p->NoEnvDumps	= 255;
	p->MonitorDepth	= 1;
	p->MemAmount	= GetRAMSize();
	p->GMT_bias		= GetGMTBias();

	LoadDefaultPrefs ();

	debugLevel = currentConfiguration.DebugLevel;
	if ((short)currentConfiguration.MemAmount == 0 || currentConfiguration.AutoSizeRAM)
		RAMSize = GetRAMSize();
	else
		RAMSize = currentConfiguration.MemAmount;


	if (currentConfiguration.AutoSetGMT)
		currentConfiguration.GMT_bias = GetGMTBias();

	if (!currentConfiguration.ChangeMachID)
		currentConfiguration.newMACHID = GetMachineType();

	/*		"\p1.9.4"	 : Major = 190, Minor = 4	?	*/
	/*		"\p1.11.4a4" : Major = 111, Minor = 4		*/
	/*		"\p2.0.0"	 : Major = 200, Minor = 0	?	*/

	currentConfiguration.booterMajor = 111;
	currentConfiguration.booterMinor = 0;

#ifdef NIGEL
	c = (*ver)->shortVersion;
	e = c + *c;		/* string length */
	++c;			/* Past len byte */
	++e;			/* One past end */

	while ( c < e )		/* First group of digits */
	{
		if ( isdigit(*c) )
			currentConfiguration.booterMajor = currentConfiguration.booterMajor * 10 + *c - '0';
		if ( *(++c) == '.' )
			break;
	}

	while ( c < e )		/* Second group of digits */
	{
		if ( isdigit(*c) )
			currentConfiguration.booterMajor = currentConfiguration.booterMajor * 10 + *c - '0';
		if ( *(++c) == '.' )
			break;
	}

	if ( currentConfiguration.booterMajor < 101 )
		currentConfiguration.booterMajor *= 10;


	while ( c < e )		/* Third group of digits */
	{
		if ( isdigit(*c) )
			currentConfiguration.booterMinor = currentConfiguration.booterMinor * 10 + *c - '0';
		else
			break;
	}
#endif
}


void
SaveUserConfiguration ()
{
	SaveDefaultPrefs ();
}