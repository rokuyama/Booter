#	File:		Makefile
#	Target:		zlib/zlib.lib, clean, distclean
#				plus versions to be run from Makefile in the parent directoru
#	Created:	Sunday, May 6, 2001 07:30:39 PM
#	Revision:	$Id$


MAKEFILE		= Makefile
�MondoBuild�	= {MAKEFILE}	# Make blank to avoid rebuilds when makefile is modified

ObjDir          = :
Includes		=

#Sym-68K			= -sym on

COptions        = {Includes} {Sym-68K} -ansi on -mc68020 -opt speed


### Source Files ###

SrcFiles        = adler32.c �
				  crc32.c �
				  infblock.c �
				  infcodes.c �
				  inffast.c �
				  inflate.c �
				  inftrees.c �
				  infutil.c


### Object Files ###

ObjFiles-68K    = "{ObjDir}adler32.o" �
				  "{ObjDir}crc32.o" �
				  "{ObjDir}infblock.o" �
				  "{ObjDir}infcodes.o" �
				  "{ObjDir}inffast.o" �
				  "{ObjDir}inflate.o" �
				  "{ObjDir}inftrees.o" �
				  "{ObjDir}infutil.o"


### Default Rules ###

{ObjDir} � :

.o  �  .c  {�MondoBuild�}
	{C} {depDir}{default}.c -o {targDir}{default}.o {COptions}


### Build Rules ###

zlib		�	zlib.lib

zlib.lib	��  {ObjFiles-68K} {�MondoBuild�}
	Lib �
		-o {Targ} �
		{ObjFiles-68K} �
		{Sym-68K} �
		-mf -d �
		-sg zlib

clean  ��
	Delete -i {ObjFiles-68K}

distclean  ��
	Delete -i {ObjFiles-68K} {ObjDir}zlib.lib

cleanFromParent �� clean
	SetDirectory ::

distcleanFromParent �� distclean
	SetDirectory ::

zlibFromParent �� zlib
	SetDirectory ::


### Optional Dependencies ###
### Build this target to generate "include file" dependencies. ###

Dependencies	�	$OutOfDate
	MakeDepend �
		-append {MAKEFILE} �
		-ignore "{CIncludes}" �
		-objdir "{ObjDir}" �
		-objext .o �
		{Includes} �
		{SrcFiles}
