/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#define __CGBASE__		// Prevent include of CGBase.h (u_int32_t defn clash on MPW)
#define __FCNTL__		// Prevent include of fcntl.h  (ssize_t   defn clash on MPW)

#include "MacBSD.h"
#include "BSDprefix.h"

#ifdef CW11
	#define _STAT		// Prevent of include of stat.mac.h (clash for dev_t)
	#define mode_t	u_long
#endif

#define BOOT_AOUT		// Use aout_exec() in loadfile.c */
#define BOOT_ELF		// Use elf_exec()  in loadfile.c */

#include "loadfile.c"	// Unmodified version of NetBSD's /src/sys/lib/libsa/loadfile.c

#include <sys/exec_aout.h>
static int aout_exec __P((int, struct exec *, u_long *, int));

#include <sys/exec_elf.h>
static int elf_exec __P((int, Elf_Ehdr *, u_long *, int));


/* Globals to store info about the parsed kernel. used for symbol manipulation later */

enum eKernType	kernType;			/* What sort of kernel did we read */

u_long			kVec[MARK_MAX+1];	/* Position of some kernel sections */

u_long			__LDPGSZ;			/* Kernel Page size */

static char		*symspace;			/* Symbol references structures */
static u_long	lensyms;

static char		*strings;			/* String table */
static u_long	lenstrings;


/* Like loadfile(), but uses Booter IO & debug messages */

bool
kernel_parse(int fd, int flags)
{
	union
	{
		Elf32_Ehdr	elf;
		struct exec	exec;
	}
					head;
	long			i, r;
	int				offset=0;


	kernType = unknown;
	kVec[MARK_MAX] = 0;

	/* If this file has a TAR header, we try to seek past it	*/
	/* and read the first file in the archive, but aout_exec()	*/
	/* does its own seek, so this only works for ELF kernels.	*/

	if ( kernel_is_TAR(fd) )
		offset=512;

	/* Read in and interpret kernel header. */

	if ( kernel_lseek (fd, offset, SEEK_SET) == -1 )
	{
		BootError("Cannot seek to start of kernel file\n");
		return false;
	}

	i = sizeof(head);
	r = kernel_read(fd, (void *)&head, i);
	if (r != i)
	{
		BootError("Failed to read header from kernel image - read %ld bytes.\n", r);
		return false;
	}

	if ( memcmp(head.elf.e_ident, ELFMAG, SELFMAG) == 0 )
	{
		switch ( head.elf.e_ident[EI_CLASS] )
		{
			case ELFCLASS32:
				Output("32 bit ELF format\n");
				break;
			case ELFCLASS64:
				Output("64 bit ELF format (not supported)\n");
				return false;
			default:
				BootError("Unknown ELF format (not supported)\n");
				return false;
		}

		kernType = elf;

		__LDPGSZ = 8192;		/* What page size does an ELF kernel use? */

		r = elf_exec(fd, &head.elf, kVec, flags);
	}
	else
	{
		/* Put up an alert if the kernel is in a TAR archive */

		if ( offset )
			Alert(kernelTAR, NULL);

		i = N_GETMAGIC(head.exec);

		switch ( i )
		{
			case OMAGIC:
				DebugPrintf(1, "Magic = OMAGIC (old impure format)\n");
				break;
			case NMAGIC:		/* Most of our modern kernels are NMAGIC. */
				DebugPrintf(1, "Magic = NMAGIC (read-only text)\n");
				break;
			case ZMAGIC:
				DebugPrintf(1, "Magic = ZMAGIC (demand-paged executable)\n");
				break;
			case QMAGIC:
				DebugPrintf(1, "Magic = QMAGIC (compact demand-load executable)\n");
				break;
			default:
				DebugPrintf(1, "Magic = 0x%lx (%ld)\n", i, i);
				BootError("This kernel is not in a format which the booter can execute.\n");
				return false;
		}

		/* Determine whether we have a 68k kernel here (sanity-check.) */
		switch ( N_GETMID(head.exec) )
		{
			case MID_M68K:		/* m68k BSD binary with 8K page sizes */
				Output("MID_M68K executable\n");
				__LDPGSZ = 8192;
				break;
			case MID_ZERO:		/* Denotes a really, really, REALLY old kernel. */
				Output("NOMID executable. Assuming ");
			case MID_M68K4K:	/* m68k BSD binary with 4K page sizes */
				Output("MID_M68K4K executable.\n");
				__LDPGSZ = 4096;
				break;
			default:
				BootError("The kernel does not appear to be a 68k kernel.\n");
				return false;
		}

		kernType = aout;

		r = aout_exec(fd, &head.exec, kVec, flags);
	}


	/* The entry is an absolute address. copyunix() relocates the buffer, so we */
	/* need to make it into a relative address (i.e. an offset into the buffer).*/

	kVec[MARK_ENTRY] -= kVec[MARK_START];		/* do the opposite of LOADADDR() */


	if (r == 0)
	{
		DebugPrintf(1, "=0x%lx\n", kVec[MARK_END] - kVec[MARK_START]);
		DebugPrintf(2, "kVec[MARK_START]=0x%lx\n", kVec[MARK_START]);
		DebugPrintf(2, "kVec[MARK_ENTRY]=0x%lx\n", kVec[MARK_ENTRY]);
		DebugPrintf(2, "kVec[MARK_NSYM] =0x%lx\n", kVec[MARK_NSYM]);
		DebugPrintf(2, "kVec[MARK_SYM]  =0x%lx\n", kVec[MARK_SYM]);
		DebugPrintf(2, "kVec[MARK_END]  =0x%lx\n", kVec[MARK_END]);
		DebugPrintf(2, "kVec[MARK_MAX]  =0x%lx\n", kVec[MARK_MAX]);

		if ( flags & LOAD_KERNEL)
		{
			Output("\n");

			if ( kernType == aout )
			{
				symspace	= (caddr_t) kVec[MARK_SYM];
				lensyms		= (u_long)  kVec[MARK_NSYM];
				strings		= symspace + lensyms;
				lenstrings	= * ((u_long *) strings);	/* Str. table starts with length */
			}

			if ( kernType == elf )		/* MARK_SYM actually points to stored ELF header */
				if ( ! getElfSptrs(fd, kVec[MARK_SYM]) )
					return false;

			if ( ! lensyms )
			{
				ErrorPrintf("\nWarning - this kernel has no symbols.\n");
				ErrorPrintf("          Booting may be unsuccessful!\n\n");
			}

			DebugPrintf(2, "symspace=%ld(0x%lx),symlen=%ld(0x%lx),",
									symspace, symspace, lensyms, lensyms);
			DebugPrintf(2, "strings=%ld(0x%lx),lenstrings=%ld(0x%lx)\n",
									strings,strings,lenstrings,lenstrings);
//hex_file_dump(3, "dumpfile-syms", (unsigned char *)symspace, lensyms);
//hex_file_dump(3, "dumpfile-strs", (unsigned char *)strings,  lenstrings);
		}

		return true;
	}

	kernType = unknown;

	return false;
}


/* This does a rough parse of the ELF section headers to set pointers to,	*/
/* and get the length of, the symbol and string tables that were read in.	*/
/*																			*/
/* Bug:																		*/
/* If there is more than one table, only stores the details of the last one	*/

bool
getElfSptrs(int fd, u_long hdrPtr)
{
#ifdef MPW
#pragma unused fd
#endif
	int			section;
	Elf_Ehdr	*elf	= (Elf_Ehdr *) hdrPtr;
	Elf_Shdr	*SHs	= (Elf_Shdr *) (hdrPtr + elf->e_shoff);
	paddr_t		offset	= kVec[MARK_START];
	size_t		SHsize	= elf->e_shnum * sizeof(Elf_Shdr);

	DebugPrintf(5, "shoff=%ld(0x%lx), shentsize=%ld(0x%lx)", elf->e_shoff, elf->e_shoff);
	DebugPrintf(5, ", shnum=%ld(0x%lx), shstrndx=%ld(0x%lx)\n",
								elf->e_shstrndx,  elf->e_shstrndx);

	if ( elf->e_shentsize != sizeof(Elf_Shdr) )
	{
		ErrorPrintf("e_shentsize wrong?\n");
		return false;
	}

	for (section = 0; section < elf->e_shnum; ++section)
	{
		Elf_Shdr *SH = SHs+section;		// Shortcut for this section header

		DebugPrintf(3,"sh_type=%ld,sh_offset=%ld,sh_size=%ld\n",
						SH->sh_type, SH->sh_offset, SH->sh_size);

		if ( SH->sh_type == SHT_SYMTAB )
		{
			symspace = (char *) hdrPtr + SH->sh_offset;
			lensyms = SH->sh_size;
		}

		if ( SH->sh_type == SHT_STRTAB && section != elf->e_shstrndx )
		{
			strings = (char *) hdrPtr + SH->sh_offset;
			lenstrings = SH->sh_size;
		}
	}

	return true;
}


/* Is the supplied string in the kernel's string table? */

bool
isStringInKernel (char *str)
{
	return offsetOfKernelString(str) != -1;
}


/* If the supplied string is anywhere in the kernel's string table,		*/
/* return the offset from the start of the table, otherwise return -1.	*/
/*																		*/
/* Bug:																	*/
/* Only searches through one string table. ELF kernels may have more!	*/

ptrdiff_t
offsetOfKernelString (char *str)
{
	char		*p = strings;
	ptrdiff_t	offset;

	if ( kernType == aout )
		p += sizeof(u_long);		/* String table starts with table length */

	if ( kernType == elf )			/* String table starts with a NULL? */
		++p;

	DebugPrintf(5, "offsetOfString(%s) - ", str);

	while ( (offset = p - strings) < lenstrings )
	{
		if ( strcmp(p, str) == 0 )
			break;

		if ( kernType == elf &&			/* ELF kernels do not usually have symbols */
			 *str == '_' &&				/* which start with an underscore,	*/
			 strcmp(p, str+1) == 0 )	/* so we ignore the first character */
			break;

		p += strlen(p) + 1;
	}

	if ( p >= strings + lenstrings)
	{
		/* it's not in the string list. fail. */
		DebugPrintf(5, "failed\n");
		return -1;
	}

	DebugPrintf(5, "offset %ld(0x%lx)\n", offset, offset);
	return offset;
}


/* Given a string, find the kernel storage representing that string,	*/
/* and store the kernel offset of that storage in the supplied longword */
/*																		*/
/* Bug:																	*/
/* Only searches through one symbol table. ELF kernels may have more!	*/

bool
kernelStringToAddress (char *str, long *kernOffset)
{
	ptrdiff_t	offset;


	offset = offsetOfKernelString(str);
	if ( offset == -1 )
		return false;

	if ( kernType == aout )
	{
		struct nlist
		{
			union
			{
				char *n_name;
				long n_strx;
			}				n_un;
			unsigned char	n_type;
			char			n_other;
			short			n_desc;
			unsigned long	n_value;
		};

		struct nlist	*sym = (struct nlist *) symspace;

//		while ( (caddr_t) sym < strings )		/* String space is just after the symbols */
		while ( (caddr_t) sym < symspace + lensyms )
		{
			if ( sym -> n_un.n_strx == offset )
			{
				*kernOffset = sym -> n_value;
				DebugPrintf(4, "kernelStringToAddress(%s) = %ld\n", str, sym -> n_value);
				return true;
			}

			sym++;
		}
	}
	else
	{
		Elf32_Sym	*sym = (Elf32_Sym *) symspace;

//		while ( (caddr_t) sym < strings )		/* String space is just after the symbols */
		while ( (caddr_t) sym < symspace + lensyms )
		{
			if ( sym -> st_name == offset )
			{
				*kernOffset = sym -> st_value;
				DebugPrintf(4, "kernelStringToAddress(%s) = %ld\n", str, sym -> st_value);
				return true;
			}

			sym++;
		}
	}

	DebugPrintf(4, "kernelStringToAddress(%s) failed\n", str);
	return false;
}


/* Given a string, find the kernel storage representing that string,	*/
/* and copy the supplied longword into that storage location.			*/
/*			Rewrite of set_long_val() from ufs_test.c					*/

bool
setKernelLongVal (char *varstr, long newval)
{
	long	kernOffset,
			*tmpPtr;

	if ( kernelStringToAddress(varstr, &kernOffset) )
	{
		tmpPtr = (long *) ( (caddr_t)kVec[MARK_START] + kernOffset);
		*tmpPtr = newval;
		DebugPrintf(1, "\nSet %s to %#lx.\n", varstr, (long) newval);
		return true;
	}
	else
		ErrorPrintf("\nFailed to set kernel symbol %s.\n", varstr);

	return false;
}


/* Look for a TAR header in a kernel file. */

bool
kernel_is_TAR (long fd)
{
	unsigned char	tarHdr[8];


	if ( kernel_lseek(fd, 257, SEEK_SET) == -1
			|| kernel_read(fd, tarHdr, 8) != 8 )
		return 0;

	if ( memcmp(tarHdr, "ustar\0", 6) == 0 )
	{
		Output("POSIX TAR archive\n");
		return 1;
	}

	if ( memcmp(tarHdr, "ustar\040\040\0", 8) == 0 )
	{
		Output("GNU TAR archive\n");
		return 1;
	}

	return 0;
}
