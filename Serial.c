/************************************************************************/
/* Serial.c - Functions for managing the Serial dialog and preferences  */
/*																		*/
/* Taken out of Dialogs.c to tidy it up a bit		on 17th May 1997	*/
/*													by Nigel Pearson	*/
/* Copyright the original author, Bill Studenmund						*/
/************************************************************************/


#include "MacBSD.h"
#include "DialogMgr.h"

DialogPtr serialDia = NULL;


/*
 * Set up the Serial Preferences dialog
 *
 * This dialog keeps lots of data used by the serial drivers.  Serial Console and
 * Serial Echo are kept on the main dialog also as they are more likely to change
 * than the other parameters. They are here as they are definitely Serial port
 * related.
 *
 * The config variables ModemFlags and PrinterFlags hold the flags passed to the
 * Kernel.	0x0001 requests that the tty defaults be the same as after an stty raw
 * 			0x0002 warns ther might be LocalTalk on the port; the serial driver
 *					sets the default baud rate to 1.
 *
 * OpenModemPort flags the booter to open the modem serial port before boot.
 *	PowerBooks have powersaver systems, and can turn off the serial ports when
 *	not in use. NetBSD doesn't know how to turn them on, so we have MacOS do it.
 *
 * ModemHSKiClock, ModemGPiClock, PrinterHSKiClock, and PrinterGPiClock hold
 * 	the frequency available on the respective input line. the clock is in Hz.
 *	HSKi is CTS and GPi is DCD in UNIX parlence. Though PrinterGPi clock is not
 *	actually usable as a clock source (it can't get fed to the RTxC pin as the Modem
 *	GPi clock can), it might be present. As the clock sources make the system ignore
 *	interrupts from the respective pin in addition to being a baud source, PrinterGPi
 *	effectivly turns off DCD interrupts on the printer port.
 *
 * Bill Studenmund, May 22, 1996
 */

void DoSerialDialog (void)
{
	ControlHandle ctl;

	if (serialDia) {
		SelectWindow (serialDia);
		return;
	}
	serialDia = GetNewDialog (serDI, NULL, (WindowPtr) -1);

	SetDialogItemControlValue(serialDia, s_serBEcho,   (currentConfiguration.SerBootEcho)?1:0);
	SetDialogItemControlValue(serialDia, s_serConsole, (currentConfiguration.SerConsole) ?1:0);

	ctl = (ControlHandle) GetDialogItemHandle(serialDia, s_serConsModem);
	SetControlValue (ctl, ((currentConfiguration.SerConsole & 0x02) == 0x00)?1:0);
	HiliteControl(ctl, (currentConfiguration.SerConsole)?0:255);
	
	ctl = (ControlHandle) GetDialogItemHandle(serialDia, s_serConsPrinter);
	SetControlValue(ctl, ((currentConfiguration.SerConsole & 0x02) == 0x02)?1:0);
	HiliteControl(ctl, (currentConfiguration.SerConsole)?0:255);

	SetDialogItemControlValue(serialDia, s_serModemRaw,  (currentConfiguration.ModemFlags)?1:0);
	SetDialogItemControlValue(serialDia, s_serPrintRaw,  (currentConfiguration.PrinterFlags & 0x01)?1:0);
	SetDialogItemControlValue(serialDia, s_serLocalTalk, (currentConfiguration.PrinterFlags & 0x02)?1:0);
	SetDialogItemControlValue(serialDia, s_serOpenModem, (currentConfiguration.OpenModemPort)?1:0);

					/* If configuration fields are 0, set default port speeds of 9600 baud */
	if (! currentConfiguration.ModemDSpeed)
		currentConfiguration.ModemDSpeed = 9600;

	if (! currentConfiguration.PrinterDSpeed)
		currentConfiguration.PrinterDSpeed = 9600;

	SetDialogItemTextByItemnoToNum(serialDia, s_serModemSpeed, currentConfiguration.ModemDSpeed);
	SetDialogItemTextByItemnoToNum(serialDia, s_serModemHSKi,  currentConfiguration.ModemHSKiClock);
	SetDialogItemTextByItemnoToNum(serialDia, s_serModemGPi,   currentConfiguration.ModemGPiClock);
	SetDialogItemTextByItemnoToNum(serialDia, s_serPrintSpeed, currentConfiguration.PrinterDSpeed);
	SetDialogItemTextByItemnoToNum(serialDia, s_serPrintHSKi,  currentConfiguration.PrinterHSKiClock);
	SetDialogItemTextByItemnoToNum(serialDia, s_serPrintGPi,   currentConfiguration.PrinterGPiClock);

	SetDialogDefaultOutline(serialDia, defItemID);
	ShowWindow (serialDia);
}


/* A hit in the serial dialog. */

void HandleSerial (short item)
{
	extern int			debugLevel;

	switch (item) {
		case s_serConsole:
			if (ToggleDialogControl(serialDia, s_serConsole))
				GroupHilite (serialDia, 2, 0, s_serConsModem, s_serConsPrinter);
			else
				GroupHilite (serialDia, 2, 255, s_serConsModem, s_serConsPrinter);
			break;
		case s_serConsModem:
		case s_serConsPrinter:
			GroupSet(serialDia, 2, item, s_serConsPrinter, s_serConsModem);
			break;
		case s_serBEcho:
		case s_serModemRaw:
		case s_serPrintRaw:
		case s_serLocalTalk:
		case s_serOpenModem:
			(void) ToggleDialogControl(serialDia, item);
			break;
		case s_serModemHSKi: /* All fall through to the break */
		case s_serModemGPi:
		case s_serModemSpeed:
		case s_serPrintHSKi:
		case s_serPrintGPi:
		case s_serPrintSpeed:
			break;
		case okID:
			GetSerial ();	/* no break */
		case cancelID:
		default:
			DisposeDialog (serialDia);
			serialDia = NULL;
			break;
	}
}


/* Called to read settings from the Serial dialog into currentConfiguration */

void GetSerial (void)
{
	if (! serialDia)
		return;

	if (GetDialogItemControlValue(serialDia, s_serConsole) )
	{
		currentConfiguration.SerConsole = 1;
		if (GetDialogItemControlValue(serialDia, s_serConsPrinter) )
			currentConfiguration.SerConsole |= 0x02;
	}
	else
		currentConfiguration.SerConsole = 0;

	currentConfiguration.ModemFlags = GetDialogItemControlValue(serialDia, s_serModemRaw)?1:0;

	/* This is not a bug, look closely. Two different bits are being set. */
	currentConfiguration.PrinterFlags  = GetDialogItemControlValue(serialDia, s_serPrintRaw) ?1:0;
	currentConfiguration.PrinterFlags |= GetDialogItemControlValue(serialDia, s_serLocalTalk)?2:0;	

	currentConfiguration.SerBootEcho   = GetDialogItemControlValue(serialDia, s_serBEcho)    ?1:0;
	currentConfiguration.OpenModemPort = GetDialogItemControlValue(serialDia, s_serOpenModem)?1:0;

	currentConfiguration.ModemHSKiClock   = GetDialogItemTextByItemnoAsNum(serialDia, s_serModemHSKi);
	currentConfiguration.ModemGPiClock    = GetDialogItemTextByItemnoAsNum(serialDia, s_serModemGPi);
	currentConfiguration.ModemDSpeed      = GetDialogItemTextByItemnoAsNum(serialDia, s_serModemSpeed);
	currentConfiguration.PrinterHSKiClock = GetDialogItemTextByItemnoAsNum(serialDia, s_serPrintHSKi);
	currentConfiguration.PrinterGPiClock  = GetDialogItemTextByItemnoAsNum(serialDia, s_serPrintGPi);
	currentConfiguration.PrinterDSpeed    = GetDialogItemTextByItemnoAsNum(serialDia, s_serPrintSpeed);
}