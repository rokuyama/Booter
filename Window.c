/************************************************************************/
/* Window.c - Functions for managing the text (console) window.			*/
/*																		*/
/* Taken out of main.c to tidy it up a bit			on 5th July 1998	*/
/*													by Nigel Pearson	*/
/* Copyright © 1993. Details in the file COPYRIGHT.txt					*/
/************************************************************************/

#if defined ( __SC__ )
	#define OLDROUTINENAMES	1			// So Fonts.h defines monaco
#endif

#include "MacBSD.h"

#if defined ( __SC__ )
	#include <ControlDefinitions.h>		// For scrollBarProc(), in*Button, inPage*
#endif

#include <ToolUtils.h>					// For LoWord() & HiWord()


WindowPtr		mainWindow;
Rect			dragRect = { 0, 0, 1024, 1024 };
ControlHandle 	vScroll;

static WindowRecord		wRecord;
static Cursor			editCursor;
static Cursor			waitCursor;
static int				linesInFolder;


pascal void ScrollProc (ControlHandle theControl, short theCode);
static void SetView (WindowPtr w);
static void SetVScroll (void);
static void AdjustText (void);


void
SetUpWindows (void)
{
	Rect		viewRect;
	Rect		vScrollRect;
	
	SetPort ((mainWindow = GetNewWindow (windowID, &wRecord, (WindowPtr)-1L )));
	SetWTitle (mainWindow, CtoPstr ("BSD/Mac68k Launch"));
	TextFont (monaco);
	TextSize (9);
	vScrollRect = (*mainWindow).portRect;
	vScrollRect.left = vScrollRect.right-15;
	vScrollRect.right += 1;
	vScrollRect.bottom -= 14;
	vScrollRect.top -= 1;
	vScroll = NewControl (mainWindow, &vScrollRect, "\p",
							1, 0, 0, 0, scrollBarProc, 0L);
	viewRect = qd.thePort->portRect;
	viewRect.right -= SBarWidth;
	viewRect.bottom -= SBarWidth;
	InsetRect(&viewRect, 4, 4);
	TEH = TENew (&viewRect, &viewRect);
	SetView (qd.thePort);
}


void
MaintainCursor (void)
{
	if (running)
		SetCursor (&waitCursor);
	else
		SetCursor (&qd.arrow);
}


void
SetUpCursors (void)
{
	CursHandle	hCurs;
	
	hCurs = GetCursor (watchCursor);
	waitCursor = **hCurs;
}


char
ours (WindowPtr w)
{
	return ((mainWindow != NULL) && (w == mainWindow));
}


pascal void
ScrollProc (ControlHandle theControl, short theCode)
{
	int	pageSize;
	int	scrollAmt;
	
	if (theCode == 0)
		return ;
	
	pageSize = ((**TEH).viewRect.bottom-(**TEH).viewRect.top) / 
			(**TEH).lineHeight - 1;
			
	switch (theCode) {
		case inUpButton: 
			scrollAmt = -1;
			break;
		case inDownButton: 
			scrollAmt = 1;
			break;
		case inPageUp: 
			scrollAmt = -pageSize;
			break;
		case inPageDown: 
			scrollAmt = pageSize;
			break;
		}
	SetCtlValue( theControl, GetCtlValue(theControl)+scrollAmt );
	AdjustText();
}

void
DoContent (WindowPtr theWindow, EventRecord *theEvent)
{
	int				cntlCode;
	ControlHandle 	theControl;
	GrafPtr			savePort;
	
	GetPort(&savePort);
	SetPort(theWindow);
	GlobalToLocal( &theEvent->where );
	if ((cntlCode = FindControl(theEvent->where, theWindow, &theControl)) == 0) {
	}
	else if (cntlCode == inThumb) {
		TrackControl(theControl, theEvent->where, 0L);
		AdjustText();
	}
	else
		TrackControl(theControl, theEvent->where, &ScrollProc);

	SetPort(savePort);
}

void
MyGrowWindow (WindowPtr w, Point p)
{
	GrafPtr	savePort;
	long	theResult;
	int		oScroll;
	Rect 	r, oView;
	
	GetPort( &savePort );
	SetPort( w );

	SetRect(&r, 80, 80, qd.screenBits.bounds.right, qd.screenBits.bounds.bottom);
	theResult = GrowWindow( w, p, &r );
	if (theResult == 0)
	  return;
	SizeWindow( w, LoWord(theResult), HiWord(theResult), 1);

	InvalRect(&w->portRect);
	oView = (**TEH).viewRect;
	oScroll = GetCtlValue(vScroll);
	
	SetView(w);
	HidePen();
	MoveControl(vScroll, w->portRect.right - SBarWidth, w->portRect.top-1);
	SizeControl(vScroll, SBarWidth+1, w->portRect.bottom - w->portRect.top-(SBarWidth-2));
	ShowPen();

	SetVScroll();
	AdjustText();
	
	SetPort( savePort );
}

static void
SetView (WindowPtr w)
{
	(**TEH).viewRect = w->portRect;
	(**TEH).viewRect.right -= SBarWidth;
	(**TEH).viewRect.bottom -= SBarWidth;
	InsetRect(&(**TEH).viewRect, 4, 4);
	linesInFolder = ((**TEH).viewRect.bottom-(**TEH).viewRect.top)/(**TEH).lineHeight;
	(**TEH).viewRect.bottom = (**TEH).viewRect.top + (**TEH).lineHeight*linesInFolder;
	(**TEH).destRect.right = (**TEH).viewRect.right;
	TECalText(TEH);
}

void
UpdateWindow (WindowPtr theWindow)
{
	GrafPtr	savePort;
	
	GetPort( &savePort );
	SetPort( theWindow );
	BeginUpdate( theWindow );
	EraseRect(&theWindow->portRect);
	DrawControls( theWindow );
	DrawGrowIcon( theWindow );
	TEUpdate( &theWindow->portRect, TEH );
	EndUpdate( theWindow );
	SetPort( savePort );
}

static void
SetVScroll (void)
{
	register int	n;
	
	n = (**TEH).nLines-linesInFolder;

	if ((**TEH).teLength > 0 && (*((**TEH).hText))[(**TEH).teLength-1]=='\r')
		n++;

	SetCtlMax(vScroll, n > 0 ? n : 0);
}

void
ShowSelect (void)
{
	register	int		topLine, bottomLine, theLine;
	
	SetVScroll();
	AdjustText();
	
	topLine = GetCtlValue(vScroll);
	bottomLine = topLine + linesInFolder;
	
	if ((**TEH).selStart < (**TEH).lineStarts[topLine] ||
			(**TEH).selStart >= (**TEH).lineStarts[bottomLine]) {
		for (theLine = 0; (**TEH).selStart >= (**TEH).lineStarts[theLine]; theLine++)
			;
		SetCtlValue(vScroll, theLine - linesInFolder / 2);
		AdjustText();
	}
}

static void
AdjustText (void)
{
	int		oldScroll, newScroll, delta;
	
	oldScroll = (**TEH).viewRect.top - (**TEH).destRect.top;
	newScroll = GetCtlValue(vScroll) * (**TEH).lineHeight;
	delta = oldScroll - newScroll;
	if (delta != 0)
	  TEPinScroll(0, delta, TEH);
}