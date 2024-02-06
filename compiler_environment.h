/*			compiler_environment.h			*/
//			$Revision$
/* Differences between MetroWerks and MPW	*/


/* CodeWarrior 11 seems to include some extra headers */
#if __MWERKS__ >= 0x1100
	#define CW11
#endif


/* MPW has no prefix file (i.e. precompiled headers.),	*/
/* so we need to define a few things for easy parsing	*/
#if defined (__SC__)
	#define MPW
	#define __STDC__		1
	#define OLDROUTINENAMES	1

	#define __STDDEF__			// Prevent include of stddef.h (ptrdiff_t clash)

	/* Types needed to parse MacBSD.h */
	#include <Dialogs.h>		// For DialogPtr, but also pulls in Boolean, FSSpec & OSErr

	/* Needed by appkill.c, DialogMgr.h, main.c, Output.c, Prefs.c, Window.c */
	#include <TextUtils.h>
	#define PtoCstr	P2CStr		// see TextUtils.h
	#define CtoPstr	C2PStr
#endif
