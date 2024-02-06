/*
 *  Sonnet Allegro patch
 *	Allows the accelerator card to run Linux
 *	on an LCII
 * 
 *
 *
 *	Author
 *		Marc LaViolette 
 *		with help from	Eric Moreau
 *						Mikael Forselius
 *
 *	Slightly modified from the original by
 *	Mikael Forselius, 990228. Changes made to
 *	be able to use the code inside the Penguin.
 *	Also removed unneccessary HLock/HUnlock calls.
 *
 *	References 
 *		InsideMac Drivers is a good place to start
 *		and is a vailable free at http://www.apple.com
 *
 *		MacIntosh Revealed vol.3 by Stephen Chernicoff
 *		Hayden Macintosh Library books
 *		ISBN 0-672-48400-5
 *		gives a good intro to drivers
 *		(vol 1 contains the handle stuff)
 *	
 *
 */

#include "MacBSD.h"

#include <Devices.h>		// For AuxDCE
#include <LowMem.h>			// For LMGetUTableBase() prototype

OSErr AllegroDriverFix (void)
{
	/* UTableBase is where unit table resides 				*/
	/* drvrPtr pointer to the driver						*/
	/* drvrHandle is a handle to the driver					*/
	/* *AllegroDCEPtr is a pointer to a AuxDCE struct		*/
	/*		see InsideMac Drivers							*/
	/* **AllegroDCEHandle is a handle to a AuxDCE struct	*/
	/*		see InsideMac Drivers							*/
	/* dCtlFlagsShowsHandle is a int that knows if 			*/
	/* 		the driver is refered to by a handle 			*/
	/*		or a pointer									*/
	/* myErr variable to store OS errors 					*/
	/* drvrRefNum where to look for driver in unit table 	*/
	/*		see InsideMac Drivers							*/
	/* drvrName[] char array containing driver name			*/
	/* drvrNamePtr pointer to driver name in driver header	*/
	/*		see InsideMac Drivers							*/
	/* i counter										 	*/
	/* ByteCmp actual code that should be found in driver 	*/
	/*	starting at (driver address +44A)					*/
	
	Handle *		UTableBase;
	Ptr				drvrPtr;
	Handle			drvrHandle;
	AuxDCE			*AllegroDCEPtr,**AllegroDCEHandle;
	int				dCtlFlagsShowsHandle;
	OSErr			myErr;
	short 			drvrRefNum;
	char			drvrName[28];
	Ptr				drvrNamePtr;
	int 			i;
	short			ByteCmp[4] = {0x7001,0x6002,0x7000,0x6100};
	
	enum
	{
		dOpenedMask		= 0x0020,
		dRamBasedMask	= 0x0040,
		drvrActiveMask	= 0x0080
	};
	
	/* check and see if driver is installed 				*/
	/*														*/
	/* The toolbox function OpenDriver returns an OSErr		*/
	/*		noErr if driver found							*/
	/*														*/
	/* drvrRefNum contains the driver reference number		*/
	/* which is the "logical not" of the unit number		*/
	/*														*/
	/* If the driver is found 								*/
	/*		Make sure it is the right driver				*/
	/* else 												*/
	/*		Warn user and leave well enough alone			*/
	
	Output ("Looking for driver .Accel_68030_Sonnet_Allegro\n");
	myErr = OpenDriver("\p.Accel_68030_Sonnet_Allegro", &drvrRefNum);

	if (myErr == noErr)
	{
		/* find unit table */
		UTableBase = (Handle *)LMGetUTableBase();

		/* get handle to device control entry (DCE) block 	*/
		AllegroDCEHandle = (AuxDCE**)(UTableBase[~drvrRefNum]);	

		/* find driver address in memory 					*/
		AllegroDCEPtr = *(AllegroDCEHandle);						

		/* Check if driver has a handle or a pointer 		*/
		/* if handle dereference and lock Block				*/
		/* else get pointer									*/
		dCtlFlagsShowsHandle = (AllegroDCEPtr->dCtlFlags) & dRamBasedMask;
		
		if(dCtlFlagsShowsHandle)
		{
			drvrHandle = (Handle)(AllegroDCEPtr->dCtlDriver);
			drvrPtr = *drvrHandle;
		}
		else
		{
			drvrPtr = (AllegroDCEPtr->dCtlDriver);
		}
			
		
		/* Make sure it is the right driver 				*/
		/* by checking the name.							*/
		/* This is done by the "unusual" copying			*/
		/* of a string that is in the driver header  		*/
		/* that contains the driver name					*/
		drvrNamePtr = drvrPtr+19;
		for(i = 0;i <= 26;i++)
			drvrName[i] = drvrNamePtr[i];
		drvrName[27] = '\0';
		
		/* and comparing this string with what we want		*/
		/* if true											*/
		/*		Check the machine code 						*/
		/* else 											*/
		/*		warn user									*/
		if(strcmp(drvrName,".Accel_68030_Sonnet_Allegro") == 0)
		{

			/* Check the machine code by comparing code		*/
			/*  around (drvrPtr + 0x44E)					*/
			/* if true										*/
			/*		modify it 								*/
			/* else 										*/
			/*		warn user								*/
			if( (ByteCmp[0] == *(short*)(drvrPtr + 0x44A)) &&
				(ByteCmp[1] == *(short*)(drvrPtr + 0x44C)) &&
				(ByteCmp[2] == *(short*)(drvrPtr + 0x44E)) &&
				(ByteCmp[3] == *(short*)(drvrPtr + 0x450)))
			{
		
				/* Modify the control routine of the driver */
				*(short*)(drvrPtr + 0x44E) = 0x6004;
				Output ("Found and modified driver\n");
			}
			else
			{
				Output ("Found the driver, but it has the wrong machine code\n");
				Output ("Did you by chance run the Booter twice,\n");
				Output ("or Marc LaViolette's Allegro_finale program?\n");
			}
		}
		else 
			Output ("Found a driver but not the right one\n");
	}
	else 
		Output ("Did not find the driver\n");

	return 0;
}