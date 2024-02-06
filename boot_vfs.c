/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include "MacBSD.h"


long
boot_read(long fd,unsigned char *buf,long len)
{
	switch ( currentConfiguration.kernelLoc )
	{
		case MacOSPart:		return macos_read(fd, buf, len);
		case NetBSDPart:	return ufs_read  (fd, buf, len);
		case NetBoot:		return bootp_read(fd, buf, len);
		default:
			ErrorPrintf("Internal Error in boot_read(): Unknown kernel location %d.\n",
						currentConfiguration.kernelLoc);
			break;
	}
	return -1;
}


long
boot_lseek(long fd,long pos,long whence)
{
	switch ( currentConfiguration.kernelLoc )
	{
		case MacOSPart:		return macos_lseek(fd, pos, whence);
		case NetBSDPart:	return ufs_lseek  (fd, pos, whence);
		case NetBoot:		return bootp_lseek(fd, pos, whence);
		default:
			ErrorPrintf("Internal Error in boot_lseek(): Unknown kernel location %d.\n",
						currentConfiguration.kernelLoc);
			break;
	}
	return 0;
}

#include <stdio.h>

long
boot_rewind(long fd)
{
	return boot_lseek(fd,0L,SEEK_SET);
}


void
boot_close (long fd)
{
	switch ( currentConfiguration.kernelLoc )
	{
		case MacOSPart:		macos_close(fd);	break;
		case NetBSDPart:	ufs_close  (fd);	break;
		case NetBoot:		bootp_close(fd);	break;
		default:
			ErrorPrintf("Internal Error in boot_close(): Unknown kernel location %d.\n",
						currentConfiguration.kernelLoc);
			break;
	}
}


long
boot_open (char *name, bDev device, short partition, long how)
{
	switch (currentConfiguration.kernelLoc)
	{
		case MacOSPart:	return macos_fsopen (name, how, &currentConfiguration.MacKernel);
		case NetBSDPart:return ufs_open		(name, device, partition, how);
		case NetBoot:	return bootp_open	(name, how);
		default:
			ErrorPrintf("Internal Error in boot_open(): Unknown kernel location %d.\n",
						currentConfiguration.kernelLoc);
			break;
	}

	return -1;
}
