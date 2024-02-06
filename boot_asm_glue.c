/* Copyright © 1993. Details in the file COPYRIGHT.txt */

/* Assembler code which was formerly in copy_and_boot.c	*/
/*			(and, before that, ufs_test.c)				*/

#include "MacBSD.h"
#define  NEED_ASM_EXTERN_VARS
#include "boot_asm_glue.h"


static inline asm void
DisableExtCache(void)	/*  _HWPriv D0=#05 */
{
	MOVEQ	#0x05,	D0
	DC.W	_HWPriv
}

static inline asm void
FlushExtCache (void)	/*  _HWPriv D0=#06 */
{
	MOVEQ	#0x06,	D0
	DC.W	_HWPriv
}


/*
 * This code is moved to the beginning of the loaded kernel image,
 * and is the last thing called before we hit locore.
 *
 * Invariably, it is what is pointed to by harry.
 */
asm void
copycode (void *from, void *to, unsigned long len,
		  void *entry, void *screenbase, unsigned long scsi_id)
{
	MACHINE 68020
/*
	Secret BCDL White board codes !!!
	D4 
	(0)(1)- processor
	(2)(6)- machine
	(7)(11)- ram amount in megs
	(16)- serial boot
	(17)- greybars
	(18)- single user
	A4 - width (low 16) height  (hi 16)
	D6 - boot drive and part
	D5 - video address
	A3 - rowbytes
	A2 - video depth
	A5 - reserved for bus faults, core dumps, and dog shooting
	     NEVER EVER PUT STUFF in a5
*/

/* CW9 assembler requires this to get at function parameters. */
	FRALLOC
		
	/* Copy. */
	MOVE.L	from,   A0
	MOVE.L	to,     A1
	MOVE.L	len,    D0
@again:
	MOVE.L	(A0)+, (A1)+
	SUBQ.L	#1,    D0
	BNE		@again

	/* Load essential variables into registers the kernel can read. */
	/* These are all for older kernels */
	MOVE.L  scsi_id,     D6			/* Boot Device Scsi Id */
	MOVE.L	SingleUser,  D7			/* Boot Env */
	MOVE.L	screenbase,	 D5
	MOVE.L  rowbytes,    A3			/* rowbytes global */
	MOVE.L	screendepth, A2			/* bitdepth of screen */
	MOVE.L	dimensions,  A4

	/* 1.4.2 ( ??????? ) and later only use these two: */
	MOVE.L	flags,       D4			/* processor, machine, etc stuff */
	MOVE.L	envbuf,      A1			/* envbuf global */
	SUB.L	from,        A1

	/* Branch to kernel code! */
	MOVE.L	entry,       A0
	JMP		(A0)

	/* Never got here. */

	/* Again, CW9 requires an FRFREE for every FRALLOC, and, since the only
	 * assembly is in asm functions, an RTS at the end. Never mind that we won't
	 * get here... grr!
	 */
	FRFREE
	RTS
	RTS
}


asm void
disable_intr(void)
{
	MOVE	SR,			D0
	ORI.W	#0x0700,	D0
	MOVE	D0, 		SR
}
