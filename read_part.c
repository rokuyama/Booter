/* Copyright © 1993. Details in the file COPYRIGHT.txt */
 
/*
 * read_part.c, created 5/19/92 by Brad Grantham (Alice).
 *  Partition Map reading utilities.
 * Updated 12/3/95 by Brian Gaeke for cleanup & to use NetBSD's dpme.h instead of 
 *   outdated part.h.
 * Updated 19th December 2001 to understand disklabels.
 *    Thanks to Christian Groessler for the idea and code.
 */

#define __CGBASE__		// Prevent include of CGBase.h (u_int32_t defn clash on MPW)

#include "MacBSD.h"
#include <stdlib.h>

#include "BSDprefix.h"
#include "sys/types.h"
#include "sys/disklabel.h"

#include "partnames.h"


/* Local function prototypes: */

static	u_int	dkcksum				(struct disklabel *lp);
static	long	get_disklabel_sector(bDev dev, char *name);
static	long	locate_partition	(bDev dev, char *name);




/*
 * Read a partition descriptor from a Mac formatted disk
 */

long
get_part_entry (bDev dev, long partnum, struct part_map_entry *part)
{
	long blknum;
	unsigned char blkbuf[1024];

	blknum = 1 + (partnum * sizeof(struct part_map_entry)) / SECTOR_SIZE;


	if ( ! read_disk (dev, blknum, blkbuf, SECTOR_SIZE) )
	{
		DebugPrintf (2, "get_part_entry (%d, %d, part) failed to read block %ld",
															dev.id, partnum, blknum);
		return 0;
	}

	*part = *((struct part_map_entry *)
				(blkbuf + (partnum * sizeof(struct part_map_entry)) % SECTOR_SIZE));
	return 1;
}


/*
 * I would like to aim toward having the Booter think exactly as the Kernel does in
 * terms of which partition should be which, & to share as much code as possible
 * between the two programs. A major obstacle in this is Think C's inherent brain
 * damage, such as not knowing wtf a long long is. Consequently today's definition of
 * an off_t, as well as certain things in struct dinode, are quite beyond its
 * comprehension, making inclusion of more modern files (read: dpme.h) from NetBSD
 * proper a sure recipe for filesystem barf. Maybe when I get my hands on CodeWarrior... 
 */


/*
 * Locate a partition to boot from.
 * If there is an exact match with the provided name, use that.
 * Otherwise, guess intelligently.
 */

long
locate_partition (bDev dev, char *name)
{
	struct blockzeroblock	*bzb;
	struct part_map_entry	ent;

	int		nonswap   = -1,		/* First non-swap partition */
			rootnamed = -1;		/* First partition with Root in its name */
	long	partnum,
			lastPart;

	lastPart = read_Block0 (dev);
	if (lastPart == -1)
		return -1;
	lastPart = 99;				/* At the moment, we can't count partitions */

	for ( partnum = 0; partnum < lastPart; ++partnum )
	{
		DebugPrintf (2, "\nreading partition # %ld ", partnum);
		if ( ! get_part_entry (dev, partnum, &ent) || ent.pmSig != 0x504D )
		{
			DebugPrintf (2, ", partition is invalid\n");
			break;
		}

		DebugPrintf (2, "'%s' ", ent.pmPartName);
		DebugPrintf (3, "(%s) ", ent.pmPartType);

		//
		// Partition selection strategy:
		//
		// 1. If the partition's Unix installed flags say it is root,
		//    use it, no matter what.
		//
		bzb = (struct blockzeroblock *) ent.pmBootArgs;

		DebugPrintf (3, "Magic = %lx, Type = %x, Flags = %x, ",
						(int)bzb->bzbMagic, bzb->bzbType, bzb->bzbFlags);

		if ( bzb->bzbMagic == BZB_MAGIC &&
			 bzb->bzbType   & BZB_TYPEFS &&
			 bzb->bzbFlags  & BZB_ROOTFS )
			return partnum;

		//
		// 2. If this partition's name is the name they gave us,
		//    use it, no matter what.
		//
		if ( strcasecmp(ent.pmPartName, name) == 0 )
			return partnum;

		//
		// 3. Use the *FIRST* Unix partition with "Root" in its name.
		//
		if( ( strcasecmp(ent.pmPartType, PART_TYPE_UNIX) == 0 ) &&
			( strcasestr(ent.pmPartName, ROOT_IDENTIFIER) != NULL ) )
			if ( rootnamed == -1 )
				rootnamed = partnum;

		//
		// 4. Or, use the first Unix partition
		//    that does NOT have "Swap" in its name.
		//
		if( (strcasecmp(ent.pmPartType, PART_TYPE_UNIX)==0) &&
			(strcasestr(ent.pmPartName, SWAP_IDENTIFIER)==NULL) )
			if ( nonswap == -1 )
				nonswap = partnum;
	}     	

	return ( ( rootnamed == -1 ) ? nonswap : rootnamed);
}



long
get_part_sector (bDev dev, long part)
{
	struct part_map_entry ent;
	long backup=-1,backup2=-1;
   
	if (part == -1)
		part = locate_partition(dev, (char *)currentConfiguration.PartName);

	if (part == -1)
		return get_disklabel_sector(dev, (char *)currentConfiguration.PartName);

	if ( get_part_entry (dev, part, &ent) == -1 )
		return -1;

	DebugPrintf(2, "\nChose partition %ld, name '%s'\ntype '%s'\n",
   							part, ent.pmPartName, ent.pmPartType);
	DebugPrintf(2, "blocks in map %d, start block %ld, length %ld\n\n",
   							(int)ent.pmMapBlkCnt, ent.pmPyPartStart, ent.pmPartBlkCnt);
	return ent.pmPyPartStart;
}


/*
 * Compute checksum for disk label.
 * taken from kern/subr_disk.c
 */
static u_int
dkcksum(struct disklabel *lp)
{
	u_short *start, *end;
	u_short sum = 0;

	start = (u_short *)lp;
	end = (u_short *)&lp->d_partitions[lp->d_npartitions];
	while (start < end)
		sum ^= *start++;
	return (sum);
}


static long
get_disklabel_sector (bDev dev, char *name)
{
    int l;
    int slice;
	unsigned char blkbuf[SECTOR_SIZE];
	struct disklabel *blk_start, *blk_end;
	struct disklabel *dlp;
    long ps;

    DebugPrintf(1, "get_disklabel_sector() entered...\n");

    l = strlen(name);

	if ( ! l )
	{
		ErrorPrintf("No disklabel/partition name supplied\n");
		return -1;
	}

	if ( l < 3 )
	{
		ErrorPrintf("Disklabel/partition name '%s' is too short - need at least 3 chars\n",
					name);
		return -1;
	}

    if ( name[l - 2] < '0' || name[l - 2] > '9' )
	{
		ErrorPrintf("Disklabel/Partition name '%s' improperly formed\n", name);
		ErrorPrintf("Expected something like 'wd0a' or 'sd9a'\n");
		return -1;
	}

	slice = name[l - 1] - 'a';
    if ( slice < 0 || slice > MAXPARTITIONS )
	{
		ErrorPrintf("Partition slice ('%c' from '%s') is out of range (a - %c)\n",
					slice + 'a', name, 'a' + MAXPARTITIONS);
		return -1;
	}

    /* read 1st sector of disk */
	if ( ! read_disk(dev, 0, blkbuf, SECTOR_SIZE) )
	{
		ErrorPrintf("Failed to read first sector from %s\n", disk_name(dev) );
		return -1;
	}

    DebugPrintf(1, "get_disklabel_sector() scanning for slice %d\n", slice);

    /* search routine taken from mac68k/disksubr.c */
	blk_start = (struct disklabel *)blkbuf;
    /*
	blk_end = (struct disklabel *)(blkbuf + (NUM_PARTS << DEV_BSHIFT) -
	    sizeof(struct disklabel));
    */
    blk_end = (struct disklabel *)((char *)blkbuf + SECTOR_SIZE - sizeof(struct disklabel));

	for (dlp = blk_start; dlp <= blk_end; 
	     dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC) {
			/* Sanity check */
			if (dlp->d_npartitions <= MAXPARTITIONS && 
			    dkcksum(dlp) == 0) {
                /* .. */
			} else {
				ErrorPrintf("Checksum or partition number (%d) error\n",
							dlp->d_npartitions);
                return -1;
            }
			break;
		}
	}

    DebugPrintf(1, "get_disklabel_sector() found disklabel\n");
    ps = dlp->d_partitions[slice].p_offset;
    DebugPrintf(1, "get_disklabel_sector() partition start: %ld\n", ps);
    return ps;
}
