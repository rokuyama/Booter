/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include <stdarg.h>
#include <stdio.h>
#include "MacBSD.h"


static char string[1024];
static short fileref;

void
Output(char *str)
{
	extern ControlHandle 	vScroll;
	extern WindowPtr		mainWindow;
	WindowPeek				p;
	char					*a, *b, nlseen = 0;
	long					len;

	p = (WindowPeek)mainWindow;
	if (p->visible == FALSE) {
		ShowWindow (mainWindow);
		UpdateWindow (mainWindow);
	}
	a = b = str;
	while (*a) {
		*b = *a;
		if ((*a == '\n') || (*a == '\r')) {
			nlseen = 1;
#ifdef MPW
			*b = '\n';
#else
			*b = '\r';
#endif
			if ((*a == '\r') && (*(a+1) == '\n')) a++;
		}
		a++; b++;
	}
	*b='\0';
	len = strlen (str);
	TEInsert (str, len, TEH);
	if (nlseen) ShowSelect ();

	if (currentConfiguration.LogToFile && fileref) {
		if (FSWrite (fileref, &len, str)) {
			CloseLogFile ();
			Output  ("failed to write to logfile\n");
		}
		FlushVol (0, 0);
	}
}


void
ErrorPrintf (char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vsprintf(string, fmt, ap);
	va_end(ap);
	
	Output(string);
}


void
DebugPrintf (int debug_level, char *fmt, ...)
{
	extern int	debugLevel;
	va_list		ap;
	
	if (debugLevel < debug_level)
		return;
	va_start(ap, fmt);
	vsprintf(string, fmt, ap);
	va_end(ap);
	
	Output(string);
}


void
hex_dump (int debug_level, unsigned char *blk, long bytes)
{
	long byte;
   
	if (debugLevel < debug_level)
		return;
	for(byte = 0; byte < bytes ; byte++){
		if(byte % 16 == 0)
			DebugPrintf(debug_level, "\n(0x%08lx) ", (long)(blk + byte));
		DebugPrintf(debug_level, "%02lX ", (long)blk[byte]);
	}
	DebugPrintf(debug_level, "\n");
}


#include <ctype.h>

void
hex_file_dump (int debug_level, char *dumpFileName, unsigned char *blk, u_long bytes)
{
#define CHAR_DUMP
	long byte;
	FILE *dump;


	if (debugLevel < debug_level)
		return;

	dump = fopen(dumpFileName, "w");
	if (dump == NULL)
	{
		ErrorPrintf ("Cannot open dumpfile\n");
		return;
	}
#ifdef RAW_DUMP
	fwrite(blk, bytes, 1, dump);
#else
	fprintf(dump, "From 0x%08lx for 0x%08lx", blk, bytes);
	for ( byte = 0; byte < bytes ; byte+=16 )
	{
#ifdef CHAR_DUMP
		char	print[16];
#endif
		int		i;

		fprintf(dump, "\n(+0x%06lx) ", byte);

		for ( i = 0; i < 16; ++i )
#ifdef CHAR_DUMP
		{
			char	c = blk[byte+i];

//			if ( isprint(c) )
			if ( c < (char)127 && c > 31 )
				print[i] = c;
			else
				print[i] = ' ';
#endif
			fprintf(dump, "%02lX ", (long)blk[byte+i]);
#ifdef CHAR_DUMP
		}
		fprintf(dump, "%16.16s", print);
#endif
	}
	fprintf(dump, "\n");
#endif
	fclose(dump);
}


void
OpenLogFile (void)
{
	OSErr err;
	unsigned char filename[64];
	
	if (FSpOpenDF (&currentConfiguration.LogFile, fsWrPerm, &fileref)) {
		/* This failed (perhaps it doesn't exist),
		   so try to create a new one. */
		FSpCreate (&currentConfiguration.LogFile, 'ttxt', 'TEXT', smSystemScript);
		if ((err = FSpOpenDF (&currentConfiguration.LogFile, fsWrPerm, &fileref)) != noErr) {
			/* Failed again, no way to recover. */
			char str[8];
	
			sprintf (str, "%d", err);
			ParamText (currentConfiguration.LogFile.name, CtoPstr (str), 0, 0);
			Alert (noFileAlrt, NULL);
			fileref = 0;
			return;
		}
	}
	SetFPos (fileref, fsFromLEOF, 0);
	strcpy ((char *)filename, (char *)currentConfiguration.LogFile.name);
	ErrorPrintf ("Logging to %s\n", PtoCstr (filename));
}


void
CloseLogFile (void)
{
	if (! fileref) return;
	
	if (FSClose (fileref)) {
		Output ("Failed to close log file\n");
		fileref = 0;
		return;
	}
	fileref = 0;
	Output ("Closing log file\n");
}


