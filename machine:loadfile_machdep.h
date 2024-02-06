#define ELFSIZE 32

/*
#define LOAD_KERNEL		(LOAD_ALL  & ~LOAD_HDR)
#define COUNT_KERNEL	(COUNT_ALL & ~COUNT_HDR)
*/
#define LOAD_KERNEL		LOAD_ALL
#define COUNT_KERNEL	COUNT_ALL

#define LOADADDR(a)		(((u_long)(a)) + offset)

/* Despite the name, this is actually the Load Address of the file */
#define ALIGNENTRY(a)	(0)

#define READ(f, b, c)	kernel_read(f, (void *)LOADADDR(b), c)
#define BCOPY(s, d, c)	memcpy((void *)LOADADDR(d), (void *)(s), (c))
#define BZERO(d, c)		memset((void *)LOADADDR(d), 0, (c))
#define	WARN(a)			BootError	a
#define PROGRESS(a)		ErrorPrintf a
#define ALLOC(a)		malloc(a)
#define FREE(a, b)		free(a)
#define OKMAGIC(a)		1
					/* (m == OMAGIC || m == NMAGIC || m == ZMAGIC) */



/* Extras which loadfile.c uses */

typedef size_t			ssize_t;

#define EFTYPE			0


#define MID_MACHINE		MID_M68K

#define lseek			kernel_lseek
#define read(f, b, c)	kernel_read(f, (void *)(b), c)