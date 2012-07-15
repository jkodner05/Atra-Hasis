/*
 *  Geshtu.h
 *  
 *
 *  Created by Jordan Kodner on 6/12/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define HEADER_SIZE 8	/* # bytes in a PNG header */
#define CH_SIZE 4	/* $ bytes in a PNG "chunk" section */
#define BYTE sizeof(char)

#define IDHR 0x49484452 /* hex representation of ASCII IDHR */
#define IDAT 0x49444154	/* hex representation of ASCII IDAT */
#define IEND 0x49454e44 /* hex representation of ASCII IEND */

#define TRUE 1
#define FALSE 0

typedef struct
{
	unsigned char *size;
	unsigned char *type;
	unsigned char *body;
	unsigned char *crc;
	unsigned int sizenum;
	unsigned int typenum;
} datachunk;

/* void display(unsigned char **chunk); */
/* for testing. displays image by scanline */

unsigned int paeth(unsigned int a, unsigned int b, unsigned int c);
/* implements paeth prediction*/
void filter1(unsigned char *curr);
/* performs PNG filtering method 1 */
void filter(unsigned char *prev, unsigned char *curr, int type);
/* directs PNG filtering by type (only 1 implemented) */
void unfilter1(unsigned char *curr);
/* reverses PNG filtering method 1 */
void unfilter2(unsigned char *prev, unsigned char *curr);
/* reverses PNG filtering method 2 */
void unfilter3(unsigned char *prev, unsigned char *curr);
/* reverses PNG filtering method 3 */
void unfilter4(unsigned char *prev, unsigned char *curr);
/* reverses PNG filtering method 4 */
void unfilter(unsigned char *prev, unsigned char *curr, int type);
/* directs unfiltering of PNG scanline */
char *read_code(datachunk *chunk);
/* reads hidden code out of a chunk */
int write_code(datachunk *chunk, char *msg);
/* writes hidden code to a chunk */

/* Following two functions found online */
/* http://www.koders.com/c/fid699AFE0A656F0022C9D6B9D1743E697B69CE5815.aspx */
u_int32_t chksum_crc32 (unsigned char *block, unsigned int length);    /* calculates crc checksum */
void chksum_crc32gentab ();
/* precalculates crc checksum table */
