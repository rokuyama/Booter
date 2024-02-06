#   File:		Makefile
#   Targets:	"BSD/Mac68k Booter"/booter, netbootlib, zlib, clean, distclean
#   Created:	Wednesday, September 18, 2002 01:55:32 PM by Nigel
#				Based on work by Hauke Fath between May and November 2001
#	Revision:	$Id$

Program			= "BSD/Mac68k Booter"
Cmds			= MakeCommands

ObjDir			= :MPW:
ZlibDir			= :zlib:
Zlib			= {ZlibDir}zlib.lib
NetbootLibDir	= :netbootlib:
NetbootLib		= {NetbootLibDir}netboot.lib

BSD				= :NetBSD includes:
BSDincludes		= "{BSD}","{BSD}Fabricated:","{BSD}Renamed:","{BSD}Unchanged:"

#Sym-68K         = -sym on

COptions		= {Sym-68K} -mc68020 -align mac68k -opt speed -proto strict {MemModelOptions}

AOptions		= -wb

A				= ASM


### Source Files ###
# The three groups of files are User Interface, MacOS interaction, and Booting code.
# In the executable, they are divided into three different segments

SrcFiles		= Dialogs.c ¶
				  Machine.c ¶
				  Monitors.c ¶
				  Prefs.c ¶
				  Serial.c ¶
				  Startup.c ¶
				  Window.c ¶
				  ¶
				  main.c ¶
				  Output.c ¶
				  read_disk.c ¶
				  read_macos.c ¶
				  ¶
				  boot_asm_glue.a ¶
				  boot_vfs.c ¶
				  copy_and_boot.c ¶
				  gzip.c ¶
				  kernel_parse.c ¶
				  read_bootp.c ¶
				  read_part.c ¶
				  read_ufs.c
				  

### Object Files ###

ObjFiles-68K	= "{ObjDir}Dialogs.o" ¶
				  "{ObjDir}Machine.o" ¶
				  "{ObjDir}Monitors.o" ¶
				  "{ObjDir}Prefs.o" ¶
				  "{ObjDir}Serial.o" ¶
				  "{ObjDir}Startup.o" ¶
				  "{ObjDir}Window.o" ¶
				  ¶
				  "{ObjDir}appkill.o" ¶
				  "{ObjDir}lowmem.o" ¶
				  "{ObjDir}machdep.o" ¶
				  "{ObjDir}main.o" ¶
				  "{ObjDir}Output.o" ¶
				  "{ObjDir}read_bootp.o" ¶
				  "{ObjDir}read_disk.o" ¶
				  "{ObjDir}read_macos.o" ¶
				  "{ObjDir}RequestVideo.o" ¶
				  "{ObjDir}videoaddr.o" ¶
				  "{ObjDir}videocard.o" ¶
				  ¶
				  "{ObjDir}Allegro_finale.o" ¶
				  "{ObjDir}boot_asm_glue.a.o" ¶
				  "{ObjDir}boot_vfs.o" ¶
				  "{ObjDir}copy_and_boot.o" ¶
				  "{ObjDir}gzip.o" ¶
				  "{ObjDir}kernel_parse.o" ¶
				  "{ObjDir}read_part.o" ¶
				  "{ObjDir}read_ufs.o" ¶
				  "{ObjDir}strcasecmp.o" ¶
				  "{ObjDir}strcasestr.o"


### Libraries ###

LibFiles-68K	= "{Libraries}MathLib.o" ¶
				  "{CLibraries}StdCLib.o" ¶
				  "{Libraries}IntEnv.o" ¶
				  "{Libraries}Interface.o" ¶
				  "{Libraries}MacRuntime.o"

LocalLibs-68K	= {NetbootLib} {Zlib}


### Resources ###

RsrcFile-68K	= BSD_Mac_Booter.rsrc


### Default Rules ###
# Note that I have changed the C rule so that the generated file is not .c.o
# and that the directory default rule is necessary to use it with an output dir

{ObjDir} Ä :

.o	Ä  .c
	{C} {depDir}{default}.c -o {targDir}{default}.o {COptions}


### Build Rules ###

netbootlib		Ä	{NetbootLib}
{NetbootLib}	ÄÄ	$OutOfDate
	Directory {NetbootLibDir}
	Make netbootlibFromParent > {Cmds}
	{Cmds}

zlib	Ä	{Zlib}
{Zlib}	ÄÄ	$OutOfDate
	Directory {ZlibDir}
	Make zlibFromParent > {Cmds}
	{Cmds}

booter		Ä	{Program}
{Program}	ÄÄ  {ObjFiles-68K} {LocalLibs-68K} {LibFiles-68K} {RsrcFile-68K}
	ILink -o {Targ} {Sym-68K} ¶
				{ObjFiles-68K} {LocalLibs-68K} {LibFiles-68K}
	DeRez {RsrcFile-68k} > {ObjDir}Booter_gen.r
	Rez {ObjDir}Booter_gen.r -o {Targ} -append -c 'BSDB' -t 'APPL'
	If "{Sym-68K}" =~ /-sym Å[nNuU]Å/
		ILinkToSYM {Targ}.NJ -mf -sym 3.2 -c 'sade'
	End
	Delete {ObjDir}Booter_gen.r {Targ}.NJ

clean		ÄÄ	$OutOfDate
	Directory {NetbootLibDir}
	Make cleanFromParent > {Cmds}
	{Cmds}
	Directory {ZlibDir}
	Make cleanFromParent > {Cmds}
	{Cmds}
	Delete {NetbootLibDir}{Cmds}
	Delete {ZlibDir}{Cmds}
	Delete -i {Program} {ObjFiles-68K}

distclean	ÄÄ	$OutOfDate
	Directory {NetbootLibDir}
	Make distcleanFromParent > {Cmds}
	{Cmds}
	Directory {ZlibDir}
	Make distcleanFromParent > {Cmds}
	{Cmds}
	Delete {NetbootLibDir}{Cmds}
	Delete {ZlibDir}{Cmds}
	Delete -i {Program} {ObjFiles-68K}


### Required Dependencies ###
# These add segment specification arguments, and file-specific include paths

"{ObjDir}Dialogs.o"			ÄÄ	Dialogs.c
	{C} {Deps} -o {Targ} {COptions} -s UserInterface

"{ObjDir}Machine.o"			ÄÄ	Machine.c
	{C} {Deps} -o {Targ} {COptions} -s UserInterface

"{ObjDir}Monitors.o"		ÄÄ	Monitors.c
	{C} {Deps} -o {Targ} {COptions} -s UserInterface

"{ObjDir}Prefs.o"			ÄÄ	Prefs.c
	{C} {Deps} -o {Targ} {COptions} -s UserInterface

"{ObjDir}Serial.o"			ÄÄ	Serial.c
	{C} {Deps} -o {Targ} {COptions} -s UserInterface

"{ObjDir}Startup.o"			ÄÄ	Startup.c
	{C} {Deps} -o {Targ} {COptions} -s UserInterface

"{ObjDir}Window.o"			ÄÄ	Window.c
	{C} {Deps} -o {Targ} {COptions} -s UserInterface

"{ObjDir}Allegro_finale.o"	ÄÄ	Allegro_finale.c
	{C} {Deps} -o {Targ} {COptions} -s Booter

"{ObjDir}boot_vfs.o"		ÄÄ	boot_vfs.c
	{C} {Deps} -o {Targ} {COptions} -s Booter

"{ObjDir}copy_and_boot.o"	ÄÄ	copy_and_boot.c
	{C} {Deps} -o {Targ} {COptions} -s Booter	-i {BSDIncludes}

"{ObjDir}gzip.o"			ÄÄ	gzip.c
	{C} {Deps} -o {Targ} {COptions} -s Booter	-i {ZlibDir}

"{ObjDir}kernel_parse.o"	ÄÄ	kernel_parse.c
	{C} {Deps} -o {Targ} {COptions} -s Booter	-i {ObjDir},{BSDIncludes} #-e -l zot

"{ObjDir}read_bootp.o"		ÄÄ	read_bootp.c
	{C} {Deps} -o {Targ} {COptions} -s Booter	-i {NetbootLibDir}

"{ObjDir}read_part.o"		ÄÄ	read_part.c
	{C} {Deps} -o {Targ} {COptions} -s Booter	-i {BSDIncludes}

"{ObjDir}read_ufs.o"		ÄÄ	read_ufs.c
	{C} {Deps} -o {Targ} {COptions} -s Booter	-i {BSDIncludes}

"{ObjDir}strcasecmp.o"		ÄÄ	strcasecmp.c
	{C} {Deps} -o {Targ} {COptions} -s Booter

"{ObjDir}strcasestr.o"		ÄÄ	strcasestr.c
	{C} {Deps} -o {Targ} {COptions} -s Booter


### Optional Dependencies ###
### Build this target to generate "include file" dependencies. ###

Dependencies  Ä  $OutOfDate
	MakeDepend ¶
		-append {MAKEFILE} ¶
		-ignore "{CIncludes}" ¶
		-objdir "{ObjDir}" ¶
		-objext .o ¶
		{Includes} ¶
		{SrcFiles}
