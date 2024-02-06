/************************************************************************/
/* DialogMgr.h - Some excellent inline functions for Dialog Management	*/
/*																		*/
/* Taken out of Dialogs.c to tidy it up a bit		on 16th May 1997	*/
/*													by Nigel Pearson	*/
/* Copyright the original author, Brian Gaeke							*/
/************************************************************************/


#include <stdarg.h>					// For varargs

#include <ConditionalMacros.h>		// For UNIVERSAL_INTERFACES_VERSION

/* A lot of the "cleaner" version of this file depends on a few (ok, 9)
 * small functions. If you are philosophically opposed to small functions,
 * go ahead and set SMALL_FUNCTIONS_ARE_EVIL to 1. But if you like small
 * functions, leave it at 0. Either way, I hope these functions and the
 * little bits of commenting I did help future maintainers add options a
 * little more easily. I don't know, I just thought it was a creeping
 * horror. And the thought of adding about 500 casts to ControlHandle to
 * get it to compile in CodeWarrior with prototypes on didn't appeal to me...
 *       -- Brian Gaeke, July 29 1996
 */
#define SMALL_FUNCTIONS_ARE_EVIL 0

#if SMALL_FUNCTIONS_ARE_EVIL
	#define SMALLFUNC	/* NOTHING */
#else
	#if defined (__SC__)
		#define SMALLFUNC	/* Inlining not possible */
	#else
		#define SMALLFUNC	inline
	#endif
#endif


/* Function prototypes */
static SMALLFUNC void	GetDialogItemTextByItemno(DialogPtr dlog, short itemno, unsigned char *buf);
static SMALLFUNC long	GetDialogItemTextByItemnoAsNum(DialogPtr dlog, short itemno);
static SMALLFUNC void	GetDialogItemTextByItemnoAsCStr(DialogPtr dlog, short itemno, char *cstr);
static SMALLFUNC void	SetDialogItemTextByItemno(DialogPtr dlog, short itemno, unsigned char *str);
static SMALLFUNC void	SetDialogItemTextByItemnoToNum(DialogPtr dlog, short itemno, long num);
static SMALLFUNC void	SetDialogItemTextByItemnoToCStr(DialogPtr dlog, short itemno, char *cstr);
static SMALLFUNC Handle	GetDialogItemHandle(DialogPtr dlog, short itemno);
static SMALLFUNC void	SetDialogItemControlValue(DialogPtr dlog, short itemno, short value);
static SMALLFUNC short	GetDialogItemControlValue(DialogPtr dlog, short itemno);
static SMALLFUNC void	HiliteDialogControl(DialogPtr dlog, short itemno, short hiliteVal);
static SMALLFUNC short	ToggleDialogControl(DialogPtr dlog, short itemno);
static SMALLFUNC void	GroupSet(DialogPtr dlog, short numInGroup, short itemOn, ...);
static SMALLFUNC void	FakeButtonHilite (DialogPtr dialog, short item);
static SMALLFUNC void	GroupHilite(DialogPtr dlog, short numInGroup, short itemOn, ...);


/* Given a dialog pointer and dialog item number, returns the item's text
   in buf. */
static SMALLFUNC void
GetDialogItemTextByItemno(DialogPtr dlog, short itemno, unsigned char *buf)
{
	Handle itemH;

	itemH = GetDialogItemHandle(dlog, itemno);
	GetDialogItemText(itemH, (unsigned char *) buf);
}

/* Given a dialog pointer and dialog item number, returns the item's text
   as a long. */
static SMALLFUNC long
GetDialogItemTextByItemnoAsNum(DialogPtr dlog, short itemno)
{
	Str255 str;
	long num;

	GetDialogItemTextByItemno(dlog, itemno, str);
	StringToNum(str, &num);
	return num;
}

/* Given a dialog pointer and dialog item number, returns the item's text as
   a C string in cstr. */
static SMALLFUNC void
GetDialogItemTextByItemnoAsCStr(DialogPtr dlog, short itemno, char *cstr)
{
	Str255 str;

	GetDialogItemTextByItemno(dlog, itemno, str);
	strcpy(cstr, PtoCstr(str));
}

/* Given a dialog pointer and dialog item number, sets the item's text to str. */
static SMALLFUNC void
SetDialogItemTextByItemno(DialogPtr dlog, short itemno, unsigned char *str)
{
	Handle itemH;

	itemH = GetDialogItemHandle(dlog, itemno);
	SetDialogItemText(itemH, str);
}

/* Given a dialog pointer and dialog item number, sets the item's text to a
   string containing num. */
static SMALLFUNC void
SetDialogItemTextByItemnoToNum(DialogPtr dlog, short itemno, long num)
{
	Str255 str;

	NumToString(num, str);
	SetDialogItemTextByItemno(dlog, itemno, str);
}

/* Given a dialog pointer and a dialog item number, sets the item's text to
   a Pascal copy of the C string in cstr. */
static SMALLFUNC void
SetDialogItemTextByItemnoToCStr(DialogPtr dlog, short itemno, char *cstr)
{
	Str255 pstrbuf;

	strcpy((char *)pstrbuf, cstr);
	CtoPstr((char *)pstrbuf);
	SetDialogItemTextByItemno(dlog, itemno, pstrbuf);
}

/* Given a dialog pointer and a dialog item number, returns a handle to the
   item. */
static SMALLFUNC Handle
GetDialogItemHandle(DialogPtr dlog, short itemno)
{
	short itemt;
	Handle itemH;
	Rect itemR;

	GetDialogItem(dlog, itemno, &itemt, &itemH, &itemR);
	return itemH;
}

/* Given a dialog pointer and the number of a dialog item which is a control,
   sets the control's value to value. */
static SMALLFUNC void
SetDialogItemControlValue(DialogPtr dlog, short itemno, short value)
{
	ControlHandle ctl;

	ctl = (ControlHandle) GetDialogItemHandle(dlog, itemno);
	SetControlValue(ctl, value);
}

/* Given a dialog pointer and the number of a dialog item which is a control,
   returns the control's current value. */
static SMALLFUNC short
GetDialogItemControlValue(DialogPtr dlog, short itemno)
{
	ControlHandle ctl;

	ctl = (ControlHandle) GetDialogItemHandle(dlog, itemno);
	return GetControlValue(ctl);
}

/* Given a dialog pointer and the number of a dialog item which is a control,
   sets the control's highlight setting to hiliteVal. */
static SMALLFUNC void
HiliteDialogControl(DialogPtr dlog, short itemno, short hiliteVal)
{
	ControlHandle ctl;

	ctl = (ControlHandle) GetDialogItemHandle(dlog, itemno);
	HiliteControl(ctl, hiliteVal);
}

/* Given a dialog pointer and the number of a dialog item which is a control,
   toggles that control's value. Returns the control's new value. */
static SMALLFUNC short
ToggleDialogControl(DialogPtr dlog, short itemno)
{
	ControlHandle ctl;
	short newval;

	ctl = (ControlHandle) GetDialogItemHandle(dlog, itemno);
	newval = 1 - GetControlValue(ctl);
	SetControlValue(ctl, newval);
	return newval;
}

/* GroupSet implements radio button groups in dialogs cleanly. 
   Given a dialog pointer, the number of buttons in the button group, 
   the one that should be marked on, and a list of ALL the buttons in
   the group (INCLUDING the one that should be marked on), GroupSet
   implements the new setting. */
static SMALLFUNC void
GroupSet(DialogPtr dlog, short numInGroup, short itemOn, ...)
{
	va_list ap;
	int x;
	int itemno;
	
	va_start(ap, itemOn);
	for (x = 0; x < numInGroup; x++) {
		itemno = va_arg(ap, int); /* not short due to arg. promotion. */
		SetDialogItemControlValue(dlog, itemno, 0);
	}
	SetDialogItemControlValue(dlog, itemOn, 1);
	va_end(ap);
}


/* Produces that nifty phantom click effect when return is hit to dismiss a dialog. */
static SMALLFUNC void
FakeButtonHilite (DialogPtr dialog, short item)
{
	ControlHandle	buttonH;
#if defined (UNIVERSAL_INTERFACES_VERSION) && UNIVERSAL_INTERFACES_VERSION >= 0x0300
	unsigned
#endif
	long			throwAway;

	buttonH = (ControlHandle) GetDialogItemHandle(dialog, item);
	if (buttonH) {
		HiliteControl (buttonH, 1);
		Delay (8, &throwAway);
		HiliteControl (buttonH, 0);
	}
}


/* Change the highlight on a set of radio buttons */
static SMALLFUNC void
GroupHilite(DialogPtr dlog, short numInGroup, short hiliteVal, ...)
{
	va_list ap;
	int x;
	int itemno;
	
	va_start(ap, hiliteVal);
	for (x = 0; x < numInGroup; x++)
	{
		itemno = va_arg(ap, int); /* not short due to arg. promotion. */
		HiliteDialogControl(dlog, itemno, hiliteVal);

	}
	va_end(ap);
}
