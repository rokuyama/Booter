/* So I went out and bought this nifty integrated development environment from
 * metrowerks, Inc., and proceeded to use it to beat the living daylights out of
 * all my most feared programming problems. Life was good. Then one day, I was
 * working on some old source. Y'know, bring it up to modern standards, get rid
 * of references to low memory and old routine names, learn the difference between
 * char * and unsigned char *, blah blah blah. You know the routine. I got all
 * the bugs worked out that I'd noticed, and I was ready to release it to the
 * 'Net. But then I get an e-mail -- will it compile under THINK C? Argh! I have
 * to support OLD STUFF? You mean I have to be BACKWARD COMPATIBLE? Well, come on,
 * man, this is NetBSD! Our motto is "just sup and recompile and you should be
 * fine!" "Oh, yeah, you've gotta recompile libkvm for the 79th time this week...
 * again." Oh well. I guess I'm a pushover. I do what the users tell me. I hope
 * it does someone some good. :-) Besides, I think this even gives me an excuse
 * to take ALL the references to lowmem globals out of ufs_test.c!! Joy!
 *                                        -- Brian Gaeke, July 31, 1996, 3:59 AM
 */

/* All silliness aside, though, this is in fact a whole file full of one-liners.
 * Conceivably, this whole thing could be #included into ufs_test.c and inlined.
 * But my instinct says that this way is cleaner.
 */

#include "compiler_environment.h"

#include <LowMem.h>		// For LMGetROMBase() and friends


		/* Function prototypes */
#include "lowmem_proto.h"

Ptr GetROMBase(void)
{
	return LMGetROMBase();
}

short GetTimeDBRA(void)
{
	return LMGetTimeDBRA();
}

short GetHWCfgFlags(void)
{
	return LMGetHWCfgFlags();
}

Ptr GetScrnBase(void)
{
	return LMGetScrnBase();
}

Ptr GetSCCRd(void)
{
	return LMGetSCCRd();
}

/* Only one way to get at these undocumented ones for now. */
unsigned short GetADBDelay(void)
{
	return *((unsigned short *)0x0cea);
}

unsigned long GetHWCfgFlag2(void)
{
	return *((unsigned long *)0x0dd0);
}

unsigned long GetHWCfgFlag3(void)
{
	return *((unsigned long *)0x0dd4);
}

unsigned long GetADBReInitJTbl(void)
{
	return *((unsigned long *)0x0dd8);
}