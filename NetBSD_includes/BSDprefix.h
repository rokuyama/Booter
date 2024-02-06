/*******************************************************************************/
/* To compile the actual NetBSD header files, we need to predefine some stuff. */

#define __lint__					/* So that sys/cdefs.h defines __RENAME(X) */

#define __mode_t		u_long
#define __off_t			long
#define __pid_t			long

/* CodeWarrior 9, 10 and MPW don't support long long, so we:  */

typedef	struct { u_long msb, lsb; }	u_int64_t;
typedef	struct { long	msb, lsb; }	int64_t;


typedef unsigned char	__uint8_t;
typedef char			__int8_t;
typedef unsigned short	__uint16_t;
typedef short			__int16_t;
typedef unsigned int	__uint32_t;
typedef int				__int32_t;
typedef u_int64_t		__uint64_t;
typedef int64_t			__int64_t;

/*******************************************************************************/
