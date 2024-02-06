/* Copyright © 1993. Details in the file COPYRIGHT.txt */

/* Mac BSD Launch Utility header file. */

#include "compiler_environment.h"


#include <string.h>

#define min(a, b)	(((a) < (b)) ? (a) : (b))

typedef Boolean			bool;
typedef char			*caddr_t;
typedef unsigned long	ptrdiff_t,
						u_long;

/* For I/O debugging, the following can choose between		*/
/* gzip-decoding and normal versions of kernel I/O routines	*/
#ifdef NO_GZIP
#define kernel_open		boot_open
#define kernel_read		boot_read
#define kernel_lseek	boot_lseek
#define kernel_rewind	boot_rewind
#define kernel_close	boot_close
#else
#define kernel_open		gzboot_open
#define kernel_read		gzboot_read
#define kernel_lseek	gzboot_lseek
#define kernel_rewind	gzboot_rewind
#define kernel_close	gzboot_close
#endif

#define DEBUG 1
#define PAUSE_BEFORE_LAUNCH 1



/*******************************************/
/* User Interface (Resource) related stuff */
/*******************************************/

#define versID			1
#define windowID 		128


		/* Alerts */
#define bootConfirmAlrt	129
#define not32BitAlrt	130
#define noResourcesAlrt	131
#define noFileAlrt		132
#define	insuffHWAlrt	133
#define incompPrefsAlrt	134
#define createPrefsAlrt 135
#define savePrefsAlrt   136
#define usingVMAlrt		137
#define notOnPPC		138
#define	kernelTAR		139


		/* resource IDs of menus */
#define appleID			128
#define fileID			129
#define editID			130
#define macBSDID		131

		/* File menu command indices */
#define openBootCommand		1
#define savePrefCommand		2
#define pageSetupCommand	4
#define printCommand		5
#define quitCommand			7

		/* Edit menu command indices */
#define cutCommand		1
#define copyCommand		2
#define pasteCommand	3
#define clearCommand	4

		/* MacBSD menu command indices */
#define bootingCommand		1
#define serialCommand		2
#define monitorCommand		3
#define prefCommand			4
#define machineCommand		5
#define bootNowCommand		7
#define stopBootCommand		8


		/* Menu indices */
#define appleM			0
#define fileM			1
#define editM			2
#define macBSDM			3
#define numMenus		4


		/* Common to all dialogs */
#define   okID		1
#define   cancelID	2
#define	  defItemID	3


		/* Items in specific dialogs */
#define bootingDI	128
#define   bootFromMac	5
#define   macFileID		6
#define   setMacFileID  7
#define   bootFromBSD	8
#define   kernID		10
#define	  partID		12
#define   netBootID		13
#define   ATAroot		16
#define   ATAchan		17
#define   ATAdevice		19
#define   SCSIroot		21
#define   scsiID		22
#define	  enableRoot	23
#define	  singleUser	25
#define   askName		26
#define   kernelDebug	27
#define   jumpDebugger  28
#define   autoSetGMT    29
#define   GMTID			31


#define aboutDI		130


#define startupDI	132
#define   autoBootID		4
#define   autoTimeID		6
#define   pauseBoot     	8
#define   debugLevelID		13
#define   logToFileID		14
#define   setLogFileID		15
#define   noEnvDumps    	16
#define   abortNonFatal 	17


#define serDI		135
#define   s_serBEcho		4
#define   s_serConsole		5
#define   s_serConsModem	6
#define   s_serConsPrinter	7
#define   s_serLocalTalk	8
#define   s_serOpenModem	9
#define   s_serModemSpeed   10
#define   s_serModemRaw		11
#define   s_serModemHSKi	12
#define   s_serModemGPi		13
#define   s_serPrintSpeed   14
#define   s_serPrintRaw		15
#define   s_serPrintHSKi	16
#define   s_serPrintGPi		17


#define monitorDI	138
#define	  m_depth		4
#define   m_1bit		5
#define   m_8bit		6
#define   m_16bit		7
#define   m_32bit		8
#define   m_size		9
#define   m_640x480		10
#define   m_800x600		11
#define   m_832x624		12
#define   m_1024x768	13
#define   m_1152x870	14
#define   m_greys		15


#define machineDI	139
#define	  memAmount		5
#define   autoSizeRAM	7
#define   changeMachID	9
#define   machID		11
#define   noWarnPPC		12
#define   disableVbls	14
#define   videoHack		15
#define   allegro		17
#define   noEject		19
#define   disableATalk	21


#define aaSave		1
#define aaDiscard	2
#define aaCancel	3

#define SBarWidth	15


enum eKernType	{unknown=0, aout=1, elf=2};

enum eKernLoc	{NetBSDPart=0, MacOSPart=1, NetBoot=2};

#define CLEANUP		/* NOTHING */
#define SECTOR_SIZE 512



enum eBootDevType	{SCSI=0, ATA=1};

typedef struct booterDev
{
	enum eBootDevType	devType;
	short				buss,	/* SCSI    or ATA buss   */
						id;		/* SCSI id or ATA device */
} bDev;



typedef struct user_configuration_s
{
	short			booterMajor,		/* Version of the Booter which created this config, */
					booterMinor;		/* for preferences file compatibility checking */

		/* Booting stuff */
	enum eKernLoc	kernelLoc;			/* Is the kernel in BSD or Mac OS land? */
	FSSpec			MacKernel;			/* Mac OS file containing kernel */
	short			GMT_bias,			/* How far from UTC time? */
					newMACHID;			/* Override value for machine id.
										   Actually in Machine dialog */ 
	unsigned char	SingleUser,			/* Boot into single user? */
					SCSIID,				/* Where is '/' ? */
					MemAmount,			/* How many MB of RAM.
										   Actually in machine dialog */
					AutoSizeRAM,		/* Guess memory size.
										   Actually in machine dialog */
					KernelName[256],	/* BSD file containing kernel */
					PartName[80],		/* Which partition on SCSIID to use */
					EnableRoot,
					AskName,
					NoDisableVBLs,		/* In Machine dialog now */
					PauseBeforeBoot,	/* These two are actually in */
					AbortNonFatal,		/*  the Startup dialog now   */
					AutoSetGMT,			/* Guess GMT_bias ? */
					VideoHack,			/* LC475 video address un-mapping.
										   Now in Machine dialog */
					JumpDebugger,		/* Jump into kdb after boot */
					ChangeMachID,		/* Override clock-chipped machine id.
										   Actually in Machine dialog */
					SonnetAllegro,		/* Disable driver for this. 
										   Actually now in machine dialog */
					ATAdisk,			/* Is root actually an ATA disk */
					NoEject,			/* Don't eject removable disks */
					Channel,			/* What IO buss is the ATA disk on? */
					DisableATalk,		/* Mainly for CommSlot Ether. cards */
					fillerB[12];		/* room to grow! */


		/* Monitor stuff */
	unsigned char	MonitorDepth,
					MonitorSize,
					setToGreys,
					fillerM[49];		/* room to grow! */


		/* Startup preferences stuff */
	FSSpec			LogFile;			/* Mac OS file to write debugging to */
	unsigned char	AutoBoot,
					TimeOut,
					DebugLevel,			/* Debugging level */
					LogToFile,			/* Copy window output to Mac OS file */
					KernelDebug,		/* Kernel debug output. Was formerly
										   GreyBars. Now in Booting dialog. */
					NoEnvDumps,
					NoWarnPPC,			/* No PPC warning, for UI testing.
										   actually in Machine dialog. */
					fillerP[9];			/* Room to grow! */


		/* Serial stuff */
	unsigned long	ModemFlags,
					PrinterFlags,
					ModemHSKiClock,		/* External serial clock speeds */
					ModemGPiClock,
					PrinterHSKiClock,
					PrinterGPiClock,
					ModemDSpeed,		/* Default speeds for ports */
					PrinterDSpeed;
	unsigned char	OpenModemPort,		/* For PowerBooks */
					SerBootEcho,
					SerConsole,
					fillerS[10];		/* Room to grow! */

} user_conf;


/***********************************/
/* Prototypes and Global Variables */
/***********************************/

		/* in Allegro_finale.c */
OSErr AllegroDriverFix	(void);


		/* in appkill.c */
void KillAllOtherApps	(void);


		/* in boot_vfs.c: */
void boot_close	(long fd);
long boot_read	(long fd, unsigned char *buf, long len);
long boot_lseek	(long fd, long pos, long whence);
long boot_rewind(long fd);
long boot_open	(char *name, bDev dev, short part, long how);


		/* in copy_and_boot.c: */
void BootError (char *fmt, ...);
void copyunix  (long io);


		/* in Dialog.c: */
void SetDialogDefaultOutline(DialogPtr theDialog, short theItem);
void DoAbout				(void);
void DoBootingDialog		(void);
void GetBoot				(void);
int  DoDialog				(EventRecord *event);


		/* in kernel_parse.c */
extern	enum eKernType	kernType;	/* What sort of kernel did we read */
extern	u_long			kVec[];		/* Position of the kernel sections */
extern	u_long			__LDPGSZ;	/* Kernel Page size */

bool		kernel_parse			(int fd, int flags);
bool		getElfSptrs				(int fd, u_long hdrPtr);
ptrdiff_t	offsetOfKernelString	(char *str);
bool		isStringInKernel		(char *str);
bool		kernelStringToAddress	(char *str, long *kernOffset);
bool		setKernelLongVal		(char *str, long newval);
bool		kernel_is_TAR			(long fd);



		/* in gzip.c */
void gzboot_close (long fd);
long gzboot_read  (long fd, unsigned char *buf, long len);
long gzboot_lseek (long fd, long offset, long whence);
long gzboot_rewind(long fd);
long gzboot_open  (char *name, bDev dev, short part, long how);


		/* in main.c */
extern int			 running, startboot;
extern TEHandle		 TEH;
extern int			 debugLevel;
extern VersRecHndl   ver;

void load_and_boot	(void);
int  MainEvent		(void);
int  DoCommand		(long mResult);
void StopRun		(long fd);
void MaintainMenus	(void);
void UpdateWindow	(WindowPtr theWindow);
void ShowSelect		(void);


		/* in machdep.c */
long	GetProcessor		(void);
long	GetMMU				(void);
bool	CheckHardware		(void);
bool	MacOS32BitMapping	(void);
bool	HaveVM				(void);
bool	UsingVM				(void);
short	GetRAMSize			(void);
short	GetGMTBias			(void);
long	GetMachineType		(void);
short	GetDisplayMgrVers	(void);
bool	HasControlStrip		(void);
bool	HasDesktopPicture	(void);
bool	HasAppleScript		(void);
bool	HasATA				(void);
bool	TrapAvailable		(short theTrap);
bool	RunningOnPPC		(void);
long	GetSystemVersion	(void);
void	OutputMachineDetails(void);


		/* in Machine.c */
void DoMachineDialog (void);
void GetMachine      (void);
void HandleMachine   (short item);


		/* in Monitors.c */
void RestoreDepth    (void);
void MonitorChange   (unsigned char depth, unsigned char greys, unsigned char size);
void DoMonitorDialog (void);
void GetMonitor      (void);
void HandleMonitor   (short item);


		/* in Output.c */
void Output		  (char *str);
void ErrorPrintf  (char *fmt, ...);
void DebugPrintf  (int debug_level, char *fmt, ...);
void hex_dump	  (int debug_level, unsigned char *blk, long bytes);
void hex_file_dump(int debug_level, char *dumpFileName, unsigned char *blk, u_long bytes);
void OpenLogFile  (void);
void CloseLogFile (void);


		/* in Prefs.c */
extern unsigned char RAMSize;
extern user_conf	 currentConfiguration;

void LoadUserConfiguration (void);
void SaveUserConfiguration (void);


		/* in read_disk.c: */
int read_Block0 (bDev dev);
int read_disk	(bDev dev, long blknum, unsigned char *buf, long length);
char *disk_name (bDev dev);


		/* in read_macos.c: */
void macos_close (long fd);
long macos_read  (long fd, unsigned char *buf, long len);
long macos_lseek (long fd, long pos, long whence);
long macos_open  (char *name, long how);
long macos_fsopen(char *name, long how, FSSpec *fspec);


		/* in read_bootp.c: */
long bootp_open  (char *name, long how);
long bootp_lseek (long fd, long pos, long whence);
long bootp_read  (long fd, unsigned char *buf, long len);
void bootp_close (long fd);


		/* in read_part.c: */
long get_part_entry  (bDev dev, long partnum, struct part_map_entry *part);
long get_part_sector (bDev dev, long part);


		/* in read_ufs.c: */
long ufs_open  (char *name, bDev dev, short part, long how);
long ufs_lseek (long fd, long pos, long whence);
long ufs_read  (long fd, unsigned char *buf, long len);
void ufs_close (long fd);


		/* in Serial.c */
void DoSerialDialog (void);
void GetSerial      (void);
void HandleSerial   (short item);


		/* in Startup.c */
void DoStartupDialog (void);
void GetStartup		 (void);
void HandleStartup	 (short item);



		/* in strcasecmp.c: */
int strcasecmp  (const char *s1, const char *s2);
int strncasecmp (const char *s1, const char *s2, size_t n);


		/* in strcasestr.c: */
char *strcasestr (register const char *s, register const char *find);


		/* in videoaddr.c */
extern long	screenWidth,
			screenHeight,
			video_logical,
			videoaddress,
			screendepth,
			rowbytes;

void getVideoAddress (GDHandle maindev);


		/* in videocard.c: */
void set_video_info		 (unsigned char *buf);
void turn_off_interrupts (void);


		/* in Window.c */
extern	WindowPtr		mainWindow;
extern	Rect			dragRect;
extern	ControlHandle	vScroll;

void SetUpWindows	(void);
void SetUpCursors	(void);
void MaintainCursor	(void);
char ours			(WindowPtr w);
void MyGrowWindow	(WindowPtr w, Point p);
void DoContent		(WindowPtr theWindow, EventRecord *theEvent);