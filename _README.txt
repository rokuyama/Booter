This is the source code and documentation for the BSD/Mac68k Booter.

Compilation is supported under CodeWarrior (version 9 and later),
and Macintosh Programmer's Workshop. MPW-GM can currently be FTPd
from here:

ftp.apple.com/developer/Tool_Chest/Core_Mac_OS_Tools/MPW_etc./MPW-GM_Images


How to build in MPW

1) Double-click the file 'Makefile'

   This should open the MPW IDE, set its current directory to the source dir,
   and open the Makefile for editing. After that happens, you can close the
   Makefile window to prevent accidental modification of the make rules.

2) In the Build menu, select Build

   A dialog box will pop up asking for the name of the program to build.
   Type in booter and click OK. A lot of compile commands will scroll by
   in the Worksheet window, and at the end the 'BSD/Mac68k Booter' program
   will be generated in the source directory.

   You can also build the clean or distclean programs, which are not real
   programs, but rules to delete all the stuff you just built. The only
   difference between the two rules is that distclean deleted everything,
   while the clean rule leaves the program, and the zlib and netboot libs.


How to build on CodeWarrior:

1) Double-click the file 'Booter, CodeWarrior 9'

   The CodeWarrior IDE should start up. If you are using a newer version,
   then it will try to convert the project to its own format.
   If it fails, it may be necessary to build a new project file instead.
   Add all the .c files except loadfile.c

2) In the Project menu, select Make

   The source code files will be highlighted one-by-one, and when it gets
   past the last group (Unix-ish Libraries), the 'BSD/Mac68k Booter'
   application should be available in the source directory.


How to build in Think C:

The latest source (v2 of the Booter) does not currently build in Think C.
If you must do this, grabbing the Booter1.11.5a3 source is the only way.