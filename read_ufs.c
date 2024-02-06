/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#define __CGBASE__		// Prevent include of CGBase.h (u_int32_t defn clash on MPW)

#include "MacBSD.h"

#include <stdio.h>		// For SEEK_SET/CUR/END
#include <stdlib.h>		// For malloc() prototype

//#include <string.h>
//#include <time.h>

#include "BSDprefix.h"
#include "sys/param.h"
#include "sys/types.h"
#include "ufs/ufs/dinode.h"
#include "ufs/ffs/fs.h"
#include "ufs/ufs/dir.h"

#include "event_check.h"
 
#define DEV_BSIZE SECTOR_SIZE


union
{
  unsigned char buf[SBSIZE]; /* 8192 bytes */
  struct fs fs;  /* Sizeof(fs)=1378 */
} sb;

long numfd=0;
struct fdtable {
  long used;
  bDev dev;
  long part;
  long startsec; 		/* First sector of partition */
  struct fs fs;			/* Superblock of that partition */
  struct dinode dinode;
  long pos;				/* Pointer into the file */
  long ind[1024*3];		/* Indirect blocks */
} *fdtable;


static void			 read_bsd_block  (bDev dev, long startsec, long block, unsigned char *buf);
static struct dinode get_dinode      (bDev dev, long startsec, long inum);
static void			 read_file_block (long fd,  long block, unsigned char *buf);
static unsigned char *load_bsd_file  (bDev dev, long startsec, struct dinode dinode);
static long			 find_file 		 (bDev dev, long startsec, char *filename, struct dinode *dinode);


/* Should only be called after the super-block has been read in. */
static void
read_bsd_block (bDev dev, long startsec, long block, unsigned char *buf)
{
  /* "buf" should be sb.fs.fs_bsize bytes long */
  long sector;

#define CACHE_SIZE 32
#ifdef CACHE_SIZE
  long i,oldest;
  static unsigned char *cache=NULL;
  static long cache_block[CACHE_SIZE],lru[CACHE_SIZE],n;
  static long cache_id = -1;
  
  if (cache == NULL || cache_id != dev.id )
  {
  	if( cache_id != dev.id )
    {
      cache_id = dev.id;
      for (i=0;i<CACHE_SIZE;i++)
      {
        cache_block[i]= -1;
        lru[i]=0;
      }
      n=0;    
      free( cache );
    }
    cache=malloc(CACHE_SIZE*sb.fs.fs_bsize);
    if (cache == NULL)
    { 
      printf ("Could not malloc() for fs cache in readd_bsd_block()\n");
      exit(0);
    }
  }
#endif

  DebugPrintf (2, "read_bsd_block(): Reading block %ld into %lx\n", block, buf);

#ifdef CACHE_SIZE
  for (i=0;i<CACHE_SIZE;i++)
  {
    if (cache_block[i] == block)
    {
      n++;
      lru[i]=n;
      memcpy(buf,cache+i*sb.fs.fs_bsize,sb.fs.fs_bsize);
      DebugPrintf (5, "read_bsd_block(): Block %ld cached in cache_block %ld\n",
						block, i);
      return;
    }
  }
  DebugPrintf (4, "read_bsd_block(): Block %ld not cached\n", block);
#endif
      
  sector = startsec + (block << sb.fs.fs_fsbtodb);  /* used to be 2 */
  (void) read_disk (dev, sector, buf, sb.fs.fs_bsize);

#ifdef CACHE_SIZE
  for (i=0;i<CACHE_SIZE;i++)
    if (cache_block[i] == -1)
      break;
  if (i == CACHE_SIZE)
  {
    for (i=0;i<CACHE_SIZE;i++)
      if (i == 0 || lru[i] < lru[oldest])
        oldest=i;
    i=oldest;
  }
  n++;
  lru[i]=n;
  memcpy(cache+i*sb.fs.fs_bsize,buf,sb.fs.fs_bsize);
  cache_block[i]=block;
  DebugPrintf(5, "*");
#endif
}

/****************************************************************************/

static struct dinode
get_dinode (bDev dev, long startsec, long inum)
{
  /* Given an inode number, returns the inode */

  static long last= -1;
  static struct dinode dinode[4]; /* sizeof(struct dinode)=128, so four per sector */
  long sector;

  sector=fsbtodb(&sb.fs,ino_to_fsba(&sb.fs,inum))+(inum%sb.fs.fs_inopb)/4L;
  DebugPrintf (2, "get_diinode(%ld) = sector %ld\n",inum,sector);
  if (last != sector)
  {
	EVENT_CHECK;  
    (void) read_disk (dev, startsec + sector, (unsigned char *)&dinode, SECTOR_SIZE);
    last=sector;
  }

  return dinode[inum%4];
}

/****************************************************************************/

static void
read_file_block (long fd,long block,unsigned char *buf)
{
  /* buf should be at least fs_bsize long */
  
  long realblock;
  
  if (fd >= 0 && fd < numfd && fdtable[fd].used)
  {
    if (block < NDADDR)
      realblock = fdtable[fd].dinode.di_db[block];
    else
      realblock = fdtable[fd].ind[block-12];
    read_bsd_block(fdtable[fd].dev,fdtable[fd].startsec,realblock,buf);
	EVENT_CHECK;  
  }
}

/****************************************************************************/

static unsigned char *
load_bsd_file (bDev dev, long startsec, struct dinode dinode)
{
  /* Given the dinode of a file, loads it into a buffer and returns
  a pointer to that buffer.  Remember to free() the buffer after
  use. */

	unsigned char *buf;
	static long *ind = NULL;
	long i,j,block,done,count,size;

	if( ind == NULL )
	{
		ind = (long *) malloc( sb.fs.fs_nindir * sizeof(long) );
		if( ind == NULL )
 		{
	      printf ("Could not malloc() indirect block ptr in load_bsd_file()\n");
	      exit(0);
	    }
	}

	size=((long)dinode.di_size.lsb + (sb.fs.fs_bsize-1)) & ~(sb.fs.fs_bsize-1);
	buf=(unsigned char *)malloc(size);
	if (!buf)
	{
		ErrorPrintf ("load_bsd_file: malloc() failed on size %ld\n",size);
		exit(1);
	}

	done=FALSE;
	count=0;
	EVENT_CHECK;  
	for (i=0;i<NDADDR && !done;i++)
	{
		block=dinode.di_db[i];
		if (block == 0)
			done=TRUE;
		else
		{
			EVENT_CHECK;  
			read_bsd_block(dev,startsec,block,buf+count);
			count+=sb.fs.fs_bsize;
			DebugPrintf (1, "%ld%% done\n",count*100/size);
		}
	}

	EVENT_CHECK;  
	for (i=0;i<NIADDR && !done;i++)    /* If there were indirect blocks */
	{
		if (dinode.di_ib[i] == 0)
			done=TRUE;
		else
		{
			DebugPrintf (3, "Indirect addresses: %ld\n",(long)dinode.di_ib[i]);
			read_bsd_block(dev,startsec,dinode.di_ib[i],(unsigned char *)ind);
			EVENT_CHECK;  
			DebugPrintf (3, "Indirect: ");
			for (j=0;j<sb.fs.fs_nindir && !done;j++)
			{
				if (ind[j] == 0)
					done=TRUE;
				else
				{
					read_bsd_block(dev,startsec,ind[j],buf+count);
					EVENT_CHECK;  
					count+=sb.fs.fs_bsize;
					DebugPrintf (3, "%ld%% done\n",count*100/size);
				}
			}
		}
	}
	EVENT_CHECK;  
	
	return buf;
}

/****************************************************************************/

static long
find_file (bDev dev,long startsec,char *filename,struct dinode *dinode)
{
  /* Given the dinode of a directory, will look through it and find
  the file "filename".  Returns 0 and puts the dinode in "dinode", or returns -1 if
  it doesn't find it. */

	struct dinode dir_dinode;
	long j,dirsize;
	struct direct *dir;
	unsigned char *buf;

	dir_dinode=get_dinode(dev,startsec,ROOTINO);          /* 2 */
	EVENT_CHECK;  
	dirsize=dir_dinode.di_size.lsb;
	buf=load_bsd_file(dev,startsec,dir_dinode);
	EVENT_CHECK;  
	dir=(struct direct *)buf;
	j=0;
	do
	{
		if (dir -> d_ino != 0)
		{
			*dinode=get_dinode(dev,startsec,dir -> d_ino);
			EVENT_CHECK;  
			DebugPrintf (1, "\"%s\" (inode %ld)",dir -> d_name,(long)dir -> d_ino);
			DebugPrintf (1, "  %ld bytes\n",(long)dinode -> di_size.lsb);
			if (strcmp(dir -> d_name,filename) == 0)
			{
				DebugPrintf (4, "First sector = %d\n",(int)dinode -> di_db[0]);
				free(buf);
				return 0;
			}
		}
		j+=dir -> d_reclen;
		dir=(struct direct *)(buf + j);
	} while (j < dirsize);
	EVENT_CHECK;  
	ErrorPrintf ("File \"%s\" not found.\n",filename);
	free(buf);
	return -1;
}

/****************************************************************************/

long
ufs_open (char *name, bDev dev, short part, long how)
{
	static long init=0;
	long startsec, sec, i;
 
	if (!init)
	{
		init=1;
		fdtable=malloc(sizeof(struct fdtable) * 10);
		if (fdtable == NULL)
		{
			ErrorPrintf ("Cannot malloc() fd table (%ld bytes) in ufs_open()\n",
							sizeof(struct fdtable) * 10);
			return -1;
		}
	}

	EVENT_CHECK;  
	if (how != 0)
	{
		Output ("ufs_open() currently only supports RD_ONLY.\n");
		return -1;
	}
  
	for (i=0;i<numfd;i++)
	{
		if (!fdtable[i].used)
			break;
	}
	if (i == numfd)
		numfd++;
	if (numfd > 10)
	{
		Output ("ufs_open() - Out of file descriptors.\n");
		return -1;
	}
	fdtable[i].used=0;  /* Mark it as unused until it is ready */


	EVENT_CHECK;  
	startsec=get_part_sector(dev,part);/* in read_part.c -- returns first sector of partition */
	EVENT_CHECK;  
  
	if (startsec < 0)
	{
		if (part == -1)
			ErrorPrintf ("Couldn't locate any partitions on %s\n", disk_name(dev));
		else
			ErrorPrintf ("Partition %ld is non-existent.\n", part);
		return -1;
	}


	fdtable[i].dev = dev;
	fdtable[i].part=part;
	fdtable[i].startsec=startsec;
  
	sec=startsec;
	
	/* CPC: Major Kludge to allow both drives to be used. */
		sec+=BBSIZE/512;  /* 8k for Net2 ufs */
	
	EVENT_CHECK;  

	if (!read_disk (dev, sec, sb.buf, SBSIZE) )
	{
		ErrorPrintf ("Could not read from disk at %s\n", disk_name(dev));
		return -1;
	}

	DebugPrintf (1, "Magic from fs: %ld\n",sb.fs.fs_magic);
	DebugPrintf (1, "Magic from .h: %ld\n",(long)FS_MAGIC);
	if (sb.fs.fs_magic != FS_MAGIC)
	{
		Output ("Magic numbers do not match -- Improper UFS partition.\n");
		return -1;
	}

	fdtable[i].fs = sb.fs;

	EVENT_CHECK;  
	if (find_file(dev,startsec,name,&fdtable[i].dinode))
		return -1;  /* File not found */  
    
    /* Looks like this code will put a limitation of a few megs on us... */
	EVENT_CHECK;  
	if (fdtable[i].dinode.di_ib[0])
	{
		DebugPrintf (4, "ufs_open() reading block %ld into %lx\n",
						fdtable[i].dinode.di_ib[0],(unsigned char *)fdtable[i].ind);
		read_bsd_block(dev,startsec,fdtable[i].dinode.di_ib[0],(unsigned char *)fdtable[i].ind);
	}
	EVENT_CHECK;  
	if (fdtable[i].dinode.di_ib[1])
	{
		DebugPrintf (4, "ufs_open() reading block %ld into %lx\n",
						fdtable[i].dinode.di_ib[1],(unsigned char *)fdtable[i].ind+1024);
		read_bsd_block(dev,startsec,fdtable[i].dinode.di_ib[1],(unsigned char *)(fdtable[i].ind+1024));
	}
	EVENT_CHECK;  
	if (fdtable[i].dinode.di_ib[2])
	{
		DebugPrintf (4, "ufs_open() reading block %ld into %lx\n",
						fdtable[i].dinode.di_ib[2],(unsigned char *)fdtable[i].ind+2048);
		read_bsd_block(dev,startsec,fdtable[i].dinode.di_ib[2],(unsigned char *)(fdtable[i].ind+2048));
	}
	EVENT_CHECK;  

	fdtable[i].pos=0;
	fdtable[i].used=1;
	return i;
}

long
ufs_lseek (long fd,long pos,long whence)
{
  if (fd >= 0 && fd < numfd && fdtable[fd].used)
  {
    switch (whence)
    {
      case SEEK_SET: return (fdtable[fd].pos = pos);
      case SEEK_CUR: return (fdtable[fd].pos += pos);
      case SEEK_END: return (fdtable[fd].pos = fdtable[fd].dinode.di_size.lsb + pos);
    }
  }
  return -1;
}

long
ufs_read (long fd,unsigned char *buf,long len)
{
  long size, pos, templen, totallen;
 /*
  * The following gets corrupted when kernel_parse returns. No idea why.

  static unsigned char *tempbuf = NULL;
  */
  unsigned char *tempbuf = NULL;

  
//  if( tempbuf == NULL )
//  {
  	tempbuf = (unsigned char *) malloc( sb.fs.fs_bsize );
  	if( tempbuf == NULL )
  	{
      Output ("Could not malloc() for ufs_read.\n");
      return -1L;
    }
//  }

  if (fd >= 0 && fd < numfd && fdtable[fd].used)
  {
    size = fdtable[fd].dinode.di_size.lsb;
    pos = fdtable[fd].pos;
    if (pos >= size)
      pos = size;
    if (pos < 0)
      pos = 0;
    if (pos + len > size)
      len = size - pos;
    if (len < 0) len=0;
    totallen = len;
    
    if (len && pos % sb.fs.fs_bsize)
    {
      templen = sb.fs.fs_bsize - (pos % sb.fs.fs_bsize);
      if (templen > len)
        templen = len;
      read_file_block(fd,pos/sb.fs.fs_bsize,tempbuf);
      memcpy(buf,tempbuf + pos % sb.fs.fs_bsize,templen);
      buf += templen;
      pos += templen;
      len -= templen;
    }
    
    while (len)
    {
      templen = len;
      if (templen > sb.fs.fs_bsize)
        templen = sb.fs.fs_bsize;
      read_file_block(fd,pos/sb.fs.fs_bsize,tempbuf);
      memcpy(buf,tempbuf,templen);
      buf += templen;
      pos += templen;
      len -= templen;
    }

    fdtable[fd].pos = pos;
	free(tempbuf);
    return totallen;
  }
  free(tempbuf);
  return -1L;
}

void
ufs_close (long fd)
{
  if (fd >= 0 && fd < numfd)
    fdtable[fd].used = 0;
}
