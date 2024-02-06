/* Copyright © 1993. Details in the file COPYRIGHT.txt */


/*
 * This is kept here so that one remembers to change it as well as the
 * prototype for copycode. In case that was not clear, THESE SHOULD BE
 * IDENTICAL IN TYPE: copycode_entry is a pointer to a copycode-like function.
 */
typedef void (*copycode_entry)	(void *from, void *to, unsigned long len,
								 void *entry, void *screenbase,
													unsigned long scsi_id);

#if defined(__MWERKS__)
	asm
#endif
		void			copycode(void *from, void *to, unsigned long len,
								 void *entry, void *screenbase,
													unsigned long scsi_id);

#if defined(__MWERKS__)
	asm
#endif
		void			disable_intr(void);

/* These globals are defined in boot_asm_glue and accessed by copy_and_boot.c */

#ifdef NEED_ASM_EXTERN_VARS
	extern long				dimensions;
	extern char				*envbuf;
	extern long				flags;
	extern copycode_entry	harry;
	extern long				SingleUser;
#endif
