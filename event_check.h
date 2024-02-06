/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include <setjmp.h>


/* This should only be used after the call to load_and_boot().
   XXX - Is there a reason why this can't be a procedure?  */
#define EVENT_CHECK		{ \
							extern int		running; \
							extern jmp_buf	bootenv; \
							MainEvent(); \
							if (!running) { \
								CLEANUP; \
								longjmp(bootenv,1); \
							} \
						}
