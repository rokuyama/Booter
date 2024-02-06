/* gzip handling...
 *
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "MacBSD.h"
#include <stdio.h>
#include <stdlib.h>
#include "zlib.h"
#undef EOF				// For some reason, on CW11 this is in stdio.h

extern int errno;

#define MAX_BOOTER_FILES	2

struct booter_files {
	int				bf_used;
	long			fd;
	z_stream		stream;
	int				z_err;			/* error code for last stream operation */
	int				z_eof;			/* set if end of input file */
	unsigned char	*inbuf;			/* input buffer */
	unsigned long	crc;			/* crc32 of uncompressed data */
	int				transparent;	/* 1 if input file is not a .gz file */
} booter_files[MAX_BOOTER_FILES];

#define EOF (-1) /* needed by compression code */

static int	gz_magic[2] = {0x1f, 0x8b};	/* GZIP magic # */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

#define Z_BUFSIZE 4096

void *zcalloc(void *opaque, unsigned long items, unsigned long size);
void zcfree(void *opaque, void *ptr);
static void check_header(struct booter_files *bf);

void *
zcalloc(void *opaque, unsigned long items, unsigned long size)
{
#ifdef MPW
#pragma unused opaque
#endif
	return (calloc(items, size));
}

void
zcfree(void *opaque, void *ptr)
{
#ifdef MPW
#pragma unused opaque
#endif
	free(ptr);
}

static int
get_byte(bf)
    struct booter_files *bf;
{
    if (bf->z_eof) return EOF;
    if (bf->stream.avail_in == 0) {
		errno = 0;
		bf->stream.avail_in = boot_read(bf->fd, bf->inbuf, Z_BUFSIZE);
		if (bf->stream.avail_in != Z_BUFSIZE) {
		    bf->z_eof = 1;
		    if (errno) bf->z_err = Z_ERRNO;
		    return EOF;
		}
		bf->stream.next_in = bf->inbuf;
    }
    bf->stream.avail_in--;
    return *(bf->stream.next_in)++;
}

static unsigned long
getLong (bf)
    struct booter_files *bf;
{
    unsigned long x = (unsigned long)get_byte(bf);
    int c;

    x += ((unsigned long)get_byte(bf))<<8;
    x += ((unsigned long)get_byte(bf))<<16;
    c = get_byte(bf);
    if (c == EOF) bf->z_err = Z_DATA_ERROR;
    x += ((unsigned long)c)<<24;
    return x;
}

static struct booter_files *
lookup_bf(long fd)
{
	int	i;

	for (i=0 ; i<MAX_BOOTER_FILES ; i++) {
		if ((booter_files[i].fd == fd) && (booter_files[i].bf_used)) {
			return &booter_files[i];
		}
	}
	return NULL;
}

void
gzboot_close(long fd)
{
	struct booter_files	*bf;

	bf = lookup_bf(fd);
	if (!bf)
		return;

	inflateEnd(&(bf->stream));

	if (bf->inbuf) free(bf->inbuf);
	bf->inbuf = NULL;

	boot_close(fd);
}

long
gzboot_read(long fd,unsigned char *buf,long len)
{
	struct booter_files	*bf;
	unsigned char *start = buf; /* starting point for crc computation */

	bf = lookup_bf(fd);
	if (!bf)
		return -1;

	if (bf->z_err == Z_DATA_ERROR || bf->z_err == Z_ERRNO) return -1;
	if (bf->z_err == Z_STREAM_END)
	{
		Output("Huh????\n\n\n\n");
		return 0;  /* EOF */
	}

	bf->stream.next_out = buf;
	bf->stream.avail_out = len;

	while (bf->stream.avail_out != 0) {

		if (bf->transparent) {
			/* Copy first the lookahead bytes: */
			unsigned int n = bf->stream.avail_in;
			if (n > bf->stream.avail_out) n = bf->stream.avail_out;
			if (n > 0) {
				memcpy(bf->stream.next_out, bf->stream.next_in, n);
				bf->stream.next_out += n;
				bf->stream.next_in   += n;
				bf->stream.avail_out -= n;
				bf->stream.avail_in  -= n;
			}
			if (bf->stream.avail_out > 0) {
				bf->stream.avail_out -= boot_read(fd, bf->stream.next_out, bf->stream.avail_out);
			}
			return (long)(len - bf->stream.avail_out);
		}

		if (bf->stream.avail_in == 0 && !bf->z_eof) {

			errno = 0;
			bf->stream.avail_in = boot_read(fd, bf->inbuf, Z_BUFSIZE);
//			if (bf->stream.avail_in == 0) {
			if (bf->stream.avail_in != Z_BUFSIZE) {
				bf->z_eof = 1;
				if (errno) {
					bf->z_err = Z_ERRNO;
					break;
				}
			}
			bf->stream.next_in = bf->inbuf;
		}
		bf->z_err = inflate(&(bf->stream), Z_NO_FLUSH);

		if (bf->z_err == Z_STREAM_END) {
			/* Check CRC and original size */
			bf->crc = crc32(bf->crc, start, (unsigned long)(bf->stream.next_out - start));
			start = bf->stream.next_out;

			if (getLong(bf) != bf->crc || getLong(bf) != bf->stream.total_out) {
				bf->z_err = Z_DATA_ERROR;
			} else {
				/* Check for concatenated .gz files: */
				check_header(bf);
				if (bf->z_err == Z_OK) {
					inflateReset(&(bf->stream));
					bf->crc = crc32(0L, Z_NULL, 0);
				}
			}
		}
		if (bf->z_err != Z_OK || bf->z_eof) break;
	}
	bf->crc = crc32(bf->crc, start, (unsigned long)(bf->stream.next_out - start));

	return (long)(len - bf->stream.avail_out);
}

long
gzboot_lseek(long fd,long pos,long whence)
{
	struct booter_files	*bf;

	bf = lookup_bf(fd);
	if (!bf)
		return -1;

	if (bf->transparent) {
		long res;

		if (whence == SEEK_CUR) {
			pos -= bf->stream.avail_in;
		}
		res = boot_lseek(fd, pos, whence);
		if(res != (long)-1) {
			/* make sure the lookahead buffer is invalid */
			bf->stream.avail_in = 0;
		}
		return (res);
	}
	switch(whence) {
	case SEEK_CUR:
		pos += bf->stream.total_out;
	case SEEK_SET:
		/* if seek backwards, simply start from the beginning */
		if(pos < bf->stream.total_out) {
			long res;
			void *sav_inbuf;

			res = boot_lseek(fd, 0, SEEK_SET);
			if(res == (long)-1)
				return res;
			/* ??? perhaps fallback to close / open */

			inflateEnd(&(bf->stream));

			sav_inbuf = bf->inbuf; /* don't allocate again */
			memset(&bf->stream, '\0', sizeof(z_stream)); /* this resets total_out to 0! */
			bf->z_err = bf->z_eof = 0;
			bf->crc = 0;

			inflateInit2(&(bf->stream), -15);
			bf->stream.next_in = bf->inbuf = sav_inbuf;

			bf->fd = fd;
			check_header(bf); /* skip the .gz header */
		}

		/* to seek forwards, throw away data */
		if(pos > bf->stream.total_out) {
			long toskip = pos - bf->stream.total_out;
			while(toskip > 0) {
#define DUMMYBUFSIZE 4096
				unsigned char dummybuf[DUMMYBUFSIZE];
				long r,len = toskip;
				if (len > DUMMYBUFSIZE) len = DUMMYBUFSIZE;
				if ((r=gzboot_read(fd, dummybuf, len)) != len) {
					ErrorPrintf("read of %ld returned %ld\n", len, r);
					return((long)-1);
				}
				toskip -= len;
			}
		}
		return(pos);
	case SEEK_END:
		break;
	default:
		break;
	}
	return((long)-1);
}

static void
check_header(struct booter_files *bf)
{
    int method; /* method byte */
    int flags;  /* flags byte */
    unsigned int len;
    int c;

    bf->transparent = 0;

    /* Check the gzip magic header */
    for (len = 0; len < 2; len++) {
		c = get_byte(bf);
		if (c != gz_magic[len]) {
		    bf->transparent = 1;
		    if (c != EOF) bf->stream.avail_in++, bf->stream.next_in--;
	 		bf->z_err = bf->stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
	 		return;
		}
    }
    method = get_byte(bf);
    flags = get_byte(bf);
    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
		bf->z_err = Z_DATA_ERROR;
		return;
    }

    /* Discard time, xflags and OS code: */
    for (len = 0; len < 6; len++) (void)get_byte(bf);

    if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
		len  =  (unsigned int)get_byte(bf);
		len += ((unsigned int)get_byte(bf))<<8;
		/* len is garbage if EOF but the loop below will quit anyway */
		while (len-- != 0 && get_byte(bf) != EOF) ;
    }
    if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
		while ((c = get_byte(bf)) != 0 && c != EOF) ;
    }
    if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
		while ((c = get_byte(bf)) != 0 && c != EOF) ;
    }
    if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
		for (len = 0; len < 2; len++) (void)get_byte(bf);
    }
    bf->z_err = bf->z_eof ? Z_DATA_ERROR : Z_OK;
}


long
gzboot_open(char *name, bDev dev, short part, long how)
{
	long	fd;
	int		i;

	fd = boot_open (name, dev, part, how);

	if (fd >= 0) {
		for (i=0 ; i<MAX_BOOTER_FILES ; i++) {
			if (booter_files[i].bf_used)
				continue;

			memset(&booter_files[i], '\0', sizeof(struct booter_files));
			booter_files[i].fd = fd;
			booter_files[i].bf_used = 1;
			booter_files[i].z_err = 0;
			booter_files[i].inbuf = NULL;

			if (inflateInit2(&(booter_files[i].stream), -15) != Z_OK) {
				Output ("Internal Error in gzboot_open: Failed to init for decompression.\n");
				fd = -1;
			} else {

				booter_files[i].inbuf = malloc(Z_BUFSIZE);

				if (booter_files[i].inbuf == NULL) {
					Output ("Internal Error in gzboot_open: unable to allocate buffer.\n");
					fd = -1;
				} else {
					check_header(&booter_files[i]);
					ErrorPrintf("transparent = %d\n", booter_files[i].transparent);
				}
			}

			if (fd == -1) {
				gzboot_close(fd);
			}
			
			break;
		}
	}
	return fd;
}