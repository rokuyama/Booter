/*-
 * Copyright (c) 1993, 1994 Allen Briggs, Chris Caputo, Michael Finch,
 *							Brad Grantham, and Lawrence Kesteloot.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ABOVE COPYRIGHT HOLDERS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
 
/*
 * part.h -- MacOS-partition-map-related defines &c.
 * 
 * Used to be labelled cryptically,
 *  ALICE 5/19/92 BG
 *
 *  Handy dandy header info.
 *
 */


/*
 * Partition name fragment to force usage of a certain partition as a root
 * partition by the Booter. This is only believed by read_part.c. The actual kernel
 * algorithm involves heavier-duty "Block zero block" stuff that doesn't depend on
 * shifty things like partition names, I think.
 */
#define ROOT_IDENTIFIER "Root" 
/*
 * Partition name fragment to preclude usage of a certain partition as a root
 * partition by the Booter. What this really means is, if there's an Apple_UNIX_SVR2
 * partition, whose name has "Swap" in it, it will not be used as the root partition.
 */
#define SWAP_IDENTIFIER "Swap"
/*
 * Both ROOT_IDENTIFIER and SWAP_IDENTIFIER will be disregarded if you specify a
 * partition name in the "Partition Name" box in the Booting dialog. All of this mess 
 * should be turned into "compat" code once NetBSD-partitions-in-Mac-partitions and
 * NetBSD-disklabelled-disks are supported by NetBSD/mac68k.
 */