#	File:		Makefile
#	Targets:	netbootlib/netboot.lib, clean, distclean
#				plus versions to be run from Makefile in the parent directoru
#	Created:	Wednesday, May 16, 2001 08:06:42 AM
#	Revision:	$Id$


MAKEFILE        = Makefile
¥MondoBuild¥    = {MAKEFILE}	# Make blank to avoid rebuilds when makefile is modified

ObjDir          = :
Includes		=

#Sym-68K			= -sym on

COptions        = {Includes} {Sym-68K} -mc68020 -opt speed -frames -mbg on ¶
                  -proto strict -ansi relaxed -typecheck strict


### Source Files ###

SrcFiles        = bootp.c ¶
				  dnr.c  ¶
				  tftp.c ¶
				  udp.c


### Object Files ###

ObjFiles-68K    = "{ObjDir}bootp.o" ¶
				  "{ObjDir}dnr.o"  ¶
				  "{ObjDir}tftp.o" ¶
				  "{ObjDir}udp.o"


### Default Rules ###

{ObjDir} Ä :

.o 	Ä  .c  {¥MondoBuild¥}
	{C} {depDir}{default}.c -o {targDir}{default}.o {COptions}


### Build Rules ###

netbootlib		Ä	netboot.lib

netboot.lib		ÄÄ	{ObjFiles-68K} {¥MondoBuild¥}
		lib ¶
			-o {Targ} ¶
			{ObjFiles-68K} ¶
			{Sym-68K} ¶
			-mf -d ¶
			-sg netbootlib

clean ÄÄ
	Delete -i {ObjFiles-68K}

distclean ÄÄ
	Delete -i {ObjFiles-68K} {ObjDir}netboot.lib

cleanFromParent ÄÄ clean
	SetDirectory ::

distcleanFromParent ÄÄ distclean
	SetDirectory ::

netbootlibFromParent ÄÄ netbootlib
	SetDirectory ::


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
