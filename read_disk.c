/* Copyright © 1993. Details in the file COPYRIGHT.txt */

#include "MacBSD.h"
#include <stdio.h>		// For sprintf() prototype
#ifndef MPW
	#include <unix.h>
#endif


static short read_ata  (short ATAdev, short ATAbuss,
									  u_long blknum, unsigned char *buf, u_long length);
static short read_scsi (short SCSIid, u_long blknum, unsigned char *buf, u_long length);


int
read_disk (bDev dev, long blknum, unsigned char *buf, long length)
{
	DebugPrintf(9, "Reading %ld bytes from block %ld of %s into %lx\n",
									length, blknum, disk_name(dev), buf);

	if (dev.devType == SCSI)
		return read_scsi (dev.id, blknum, buf, length);
	else
		return read_ata  (dev.id, dev.buss, blknum, buf, length);
}


static 	char buff[50];

char *
disk_name (bDev dev)
{
	buff[0] = '\0';

	if (dev.devType == SCSI)
		sprintf (buff, "SCSI buss %d, ID %d", dev.buss, dev.id);
	else
		sprintf (buff, "ATA buss %d, device %d", dev.buss, dev.id);

	return buff;
}



	/* Stuff for IDE (ATA) disks */

#include "ATA.h"

#define ATA_WAIT	600			/* Wait time before ATA timeout. */

typedef	struct Identify		/* Returned by kATAMgrDriveIdentify */
{
	short	Signature;			/* Word 00: Constant value							*/
	short	NumCyls;			/* Word 01:	Number of cylinders (default mode)		*/
	short	Reserved0;			/* Word 02:	Constant value of 0						*/
	short	NumHds;				/* Word 03:	Number of heads (default mode)			*/
	short	TrkBytes;			/* Word 04:	Number of unformatted bytes/track		*/
	short	SecBytes;			/* Word 05:	Number of unformatted bytes/sector		*/
	short	NumSecs;			/* Word 06:	Number of sectors/track					*/
	short	VU[3];				/* Word 07-09:	Vendor unique						*/
	short	Serial[10];			/* Word 10-19:	Serial Number (right-justified)		*/
	short	BufType;			/* Word 20:	Buffer Type								*/
	short	BufSize;			/* Word 21:	Buffer size in 512 byte increments		*/
	short	NumECC;				/* Word 22:	Number of ECC bytes						*/
	short	FirmRev[4];			/* Word 23-26:	Firmware revision (left-justified)	*/
	short	ModelNum[20];		/* Word 27-46:	Model number (left-justified)		*/
	short	MultCmds;			/* Word 47:	R/W multiple commands not impl = 0		*/
	short	DblXferFlag;		/* Word 48:	Double transfer flag					*/
	short	Capabilities;		/* Word 49: LBA, DMA, IORDY support indicator		*/
	short	Reserved1;			/* Word 50: Reserved								*/
	short	PIOTiming;			/* Word 51: PIO transfer timing mode				*/
	short	DMATiming;			/* Word 52:	DMA transfer timing mode				*/
	short	Extension;			/* Word 53: extended info support					*/
	short	CurCylinders;		/* Word 54: number of current cylinders				*/
	short	CurHeads;			/* Word 55: number of current heads					*/
	short	CurSPT;				/* Word 56: number of current sectors per track		*/
	long	CurCapacity;		/* Word 57-58: current capacity in sectors			*/
	short	MultSectors;		/* Word 59: Multiple sector setting					*/
	long	LBACapacity;		/* Word 60-61: total sectors in LBA mode			*/
	short	SWDMA;				/* Word 62: single word DMA support					*/
	short	MWDMA;				/* Word 63: multi word DMA support					*/
	short	APIOModes;			/* Word 64:	Advanced PIO Xfr mode supported			*/
	short	MDMATiming;			/* Word 65:	Minimum Multiword DMA Xfr Cycle			*/
	short	RDMATiming;			/* Word 66:	Recommended Multiword DMA Cycle			*/
	short	MPIOTiming;			/* Word 67:	Min PIO XFR Time W/O Flow Control		*/
	short	PIOwRDYTiming;		/* Word 68:	Min PIO XFR Time with IORDY flow ctrl	*/
	short	Reserved2[59];		/* Work  69-127: ? */
	short	Vendor[32];			/* Word 128-159: ? */
	short	Reserved3[96];		/* Word 160-255: ? */
} IdentifyBlock;

IdentifyBlock info;

#define LBA_CAPABLE 0x0200		/* In Capabilities */


static short
read_ata (short ATAdev, short ATAbuss, u_long blknum, unsigned char *buf, u_long length)
{
	ataDevConfiguration	config;
	ataIdentify			query;
	ataIOPB				cmd;
	short				lba;
	OSErr				error;
    u_long				bytes, cyl, head, id, sector, slave;
	unsigned char		data[SECTOR_SIZE];

static	bool			reportConfigError = 1;		/* Only report this error once */


	if ( ! HasATA() )
	{
		ErrorPrintf ("Computer does not have any ATA hardware and software\n");
		return 0;
	}

	if ( debugLevel > 0 )
		if ( sizeof(info) != 512 )
		{
			ErrorPrintf ("IdentifyBlock has unexpected size - %d\n", sizeof(info) );
			return 0;
		}

	id = (ATAdev && 0xFF) * 256 + (ATAbuss && 0xFF);

	memset((void *) &config, 0, sizeof(config));
	config.ataPBDeviceID 		= id;
	config.ataPBFunctionCode 	= kATAMgrGetDrvConfiguration;
	config.ataPBVers			= kATAPBVers2;

	
	error = ataManager((ataPB*) &config );
	if ( error == noErr )
	{
		if ( config.ataDeviceType == kATADeviceATAPI )
		{	Output("Cannot boot from ATAPI device\n");  }

		if ( config.ataDeviceType == kATADeviceUnknown )
		{	Output("Drive has unknown protocol\n");  }

		if ( config.ataDeviceType == kATADevicePCMCIA )
		{	Output("Cannot boot from PCMCIA device\n");  }

		if ( config.ataDeviceType != kATADeviceATA )
		{	return 0;  }
	}
	else
		if ( reportConfigError )
		{
			Output("Warning - cannot get configuration of ATA drive");
			ErrorPrintf(" %ld, error %ld\n", id, error);
			reportConfigError = 0;
		}

	memset((void *) &query, 0, sizeof(query));
	query.ataPBDeviceID		= id;
    query.ataPBTimeOut		= ATA_WAIT;
    query.ataPBFlags		= mATAFlagIORead | mATAFlagByteSwap;
	query.ataPBFunctionCode	= kATAMgrDriveIdentify;
	query.ataPBVers			= kATAPBVers1;
    query.ataPBBuffer		= (unsigned char *) &info;
	memset((void *) &info, 0, sizeof(info));

	error = ataManager ((ataPB*) &query);
	if ( error != noErr )
	{
		ErrorPrintf ("Cannot identify ATA drive %ld, error %ld\n", id, error); 
		return 0;
	}

	DebugPrintf (10, "ATA dev %ld, ", id);

	if ( (info.Capabilities & LBA_CAPABLE) )
	{
		DebugPrintf (10, "LBA capable - Size=%ld", info.LBACapacity);
		lba = 0x40;
	}
	else
	{
		DebugPrintf (10, "standard addressing - Size=%ld", info.CurCapacity);
		lba = 0;
	}

	DebugPrintf (10, ", Cylinders=%d, Heads=%d, Sectors/track=%d\n",
							info.NumCyls, info.NumHds, info.NumSecs);

	memset((void *) &cmd, 0, sizeof(cmd));
	cmd.ataPBFunctionCode	= kATAMgrExecIO;
	cmd.ataPBVers			= kATAPBVers1;
	cmd.ataPBDeviceID		= id;
	cmd.ataPBFlags			= mATAFlagTFRead | mATAFlagIORead ;
	cmd.ataPBTimeOut		= ATA_WAIT;

	if ( ATAdev )
		slave = 0x10;
	else
		slave = 0x0;

	while ( length > 0 )
	{
		if ( lba )
		{
			sector	= blknum & 0xFF;
			head	= (blknum >> 24) & 0xF;
			cyl		= (blknum >> 8)  & 0xFFFF;
		}
		else
		{
			sector	= (blknum % info.CurSPT) + 1;
			cyl		= blknum / info.CurSPT;
			head	= cyl % info.CurHeads;
			cyl		= cyl / info.CurHeads;
		}
		cmd.ataPBBuffer = data;
		cmd.ataPBByteCount = SECTOR_SIZE;
		cmd.ataPBLogicalBlockSize = SECTOR_SIZE;

		cmd.ataPBTaskFile.ataTFCount = 1;
		cmd.ataPBTaskFile.ataTFSector = sector;
	    cmd.ataPBTaskFile.ataTFCylinder = cyl;

				      			   /* std | L/C | drive | head */
		cmd.ataPBTaskFile.ataTFSDH = 0xA0 | lba | slave | head;
		cmd.ataPBTaskFile.ataTFCommand = kATAcmdRead;

		DebugPrintf (11, "Reading block %d ... ", blknum);

		error = ataManager((ataPB*) &cmd);
	    if (error != noErr)
		{
			ErrorPrintf ("Cannot read from ATA drive %ld, error %ld\n", id, error); 
			return 0;
		}

		if ( length < SECTOR_SIZE )
		{
			bytes = length;
			DebugPrintf (11, "(using %d bytes of %d)\n", bytes, SECTOR_SIZE);
		}
		else
		{
			bytes = SECTOR_SIZE;
			length -= bytes;
			DebugPrintf (11, "%d bytes remaining\n", length);
		}

		memcpy(buf, data, bytes);
		buf    += bytes;
		++blknum;
	}

	return 1;
}



#include <SCSI.h>

int
read_Block0 (bDev dev)
{
	Block0	ddMap;

	if ( ! read_disk (dev, 0, (unsigned char *) &ddMap, sizeof (Block0) ) )
	{
		DebugPrintf (2, "Failed to read Block0 from disk.\n");
		return -1;
	}

	if ( ddMap.sbSig != 0x4552 )
	{
		DebugPrintf (2, "Device Descriptor Map signature invalid (%d). Is the disk formatted?\n",
																						ddMap.sbSig);
		return -1;
	}

	if ( ddMap.sbBlkSize != SECTOR_SIZE )
	{
		DebugPrintf (2, "BlockSize of disk (%d) is not that expected (%d)\n",
												ddMap.sbBlkSize, SECTOR_SIZE);
		return -1;
	}

	DebugPrintf (2, "Block count = %ld\n", ddMap.sbBlkCount);

	return 0;		/* At the moment, I don't know how to count the valid partitions */
}


static unsigned char cmd6[6];

#define SCSI_WAIT	600			/* Wait time before SCSI timeout. */

/* Can handle drives with # sectors no greater than 2^21, and sector size of 512 bytes */

static short
read_scsi (short SCSIid, u_long blknum, unsigned char *buf, u_long length)
{
	int error;
	short stat, msg;
	long num_blocks;
	struct SCSIInstr instrs[10];

	num_blocks = length / SECTOR_SIZE;
	if (num_blocks * SECTOR_SIZE < length )
		num_blocks++;
	DebugPrintf(10, "Reading %ld blocks of %d bytes\n", num_blocks, SECTOR_SIZE);

	if ((error = SCSIGet()) != noErr)
	{
		ErrorPrintf("Error %d on SCSIGet()\n", error);
		return 0;
	}

	if ((error = SCSISelect(SCSIid)) != noErr)
	{
		ErrorPrintf ("Error %d on SCSISelect().\n", error);
		ErrorPrintf ("\n********** Is the SCSI ID (%d) wrong? **********\n\n", (int)SCSIid);
		return 0;
	}

	cmd6[0] = 8;		                                 /* read */
	cmd6[1] = (0 << 5) | ((blknum & 0x1f0000) >> 16) ;   /* LUN and addr H */
	cmd6[2] = (blknum & 0xff00) >> 8;                    /* addr M */
	cmd6[3] = blknum & 0xff;                             /* addr L */
	cmd6[4] = (unsigned char) (num_blocks & 0xff);       /* num blocks to transfer */
	cmd6[5] = 0;                                         /* control bits (ignored) */

	instrs[0].scOpcode = scInc;			/* command scInc */
	instrs[0].scParam1 = (long)buf;		/* buffer pointer */
	instrs[0].scParam2 = length;		/* transfer size */
	instrs[1].scOpcode = scStop;		/* command stop */
	instrs[1].scParam1 = 0;
	instrs[1].scParam2 = 0;

	if ((error = SCSICmd((Ptr) cmd6, 6)) != noErr)
	{
		ErrorPrintf("Error %d on SCSICmd()\n", error);
		SCSIComplete(&stat, &msg, SCSI_WAIT);
		return 0;
	}

	if ((error = SCSIRead((Ptr) instrs)) != noErr)
	{
		ErrorPrintf ("Error %d on SCSIRead().\n", error);
		ErrorPrintf ("\n********** Is the disk in SCSI ID %d offline? **********\n\n", (int)SCSIid);
		SCSIComplete(&stat, &msg, SCSI_WAIT);
		return 0;
	}

	if ((error = SCSIComplete(&stat, &msg, SCSI_WAIT)) != noErr)
	{
		ErrorPrintf("Error %d on SCSIComplete()\n", error);
		return 0;
	}

	hex_dump (15, buf, length);
	return 1;
}