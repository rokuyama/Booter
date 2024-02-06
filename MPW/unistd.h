/* unistd.h - fabricated for MPW, needed by loadfile.c	*/
/*														*/
/* Normally (e.g. on Unix or under CodeWarrior),		*/
/* unistd.h	would include fcntl.h, but because the MPW	*/
/* version of that has	a typedef for ssize_t,			*/
/* we cannot include it									*/

extern int open(const char *path, int oflag);
extern int close(int fildes);