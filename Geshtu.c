/*
 *  Geshtu.c
 *  
 *	
 *  Jordan Kodner, June 2012.
 *
 */

#include "Geshtu.h"

u_int32_t crc_tab[256];
unsigned int scanlen, step, blksize;


/* chksum_crc() -- to a given block, this one calculates the
 *				crc32-checksum until the length is
 *				reached. the crc32-checksum will be
 *				the result.
 */ //found online
u_int32_t chksum_crc32 (unsigned char *block, unsigned int length)
{
	register unsigned long crc;
	unsigned long i;
	
	crc = 0xFFFFFFFF;
	for (i = 0; i < length; i++)
	{
		crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
	}
	return (crc ^ 0xFFFFFFFF);
}


/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
 *				calculate the crcTable for crc32-checksums.
 *				it is generated to the polynom [..]
 */ //found online
void chksum_crc32gentab ()
{
	unsigned long crc, poly;
	int i, j;
	
	poly = 0xEDB88320L;
	for (i = 0; i < 256; i++)
	{
		crc = i;
		for (j = 8; j > 0; j--)
		{
			if (crc & 1)
			{
				crc = (crc >> 1) ^ poly;
			}
			else
			{
				crc >>= 1;
			}
		}
		crc_tab[i] = crc;
	}
}


/*void display(unsigned char **chunk) 
{
	unsigned int count = 7;
	unsigned int type = chars_to_int(TYPECH);
	unsigned int size = chars_to_int(SIZECH);
	int shift, x;
	
	if (type != IDAT)	return;
	
	while (count < size) 
	{
		printf("location: %d\n", count);
		for (x = 0; x < step; x++) 
			if (count + x < size) 
				printf("%X ",BODYCH[count+x]);
		printf("\n\n");
		count += step;
	}
}*/


unsigned int paeth(unsigned int a, unsigned int b, unsigned int c) 
{
	
	unsigned int p, pa, pb, pc;
	
	p = a + b - c;
	pa = abs(p-a);
	pb = abs(p-b);
	pc = abs(p-c);
	
	if ((pa <= pb) && (pa <= pc))	return a;
	else if (pb <= pc)				return b;
	else							return c;
}


void filter(unsigned char *prev, unsigned char *curr, int type) 
{	
	switch (type) 
	{
		case 0:
			break;
		case 1:
			//not implemented
			break;
		case 2:
			//not implemented
			break;
		case 3:
			//not implemented
			break;
		case 4:
			//not implemented
			break;
		default:
			break;
	}
}


void unfilter1(unsigned char *curr) 
{	
	unsigned int pos;
	
	for (pos = 4; pos < step; pos++) 
		curr[pos] += curr[pos-3];
}


void unfilter2(unsigned char *prev, unsigned char *curr) 
{
	unsigned int pos;
	
	for (pos = 1; pos < step; pos++) 
		curr[pos] += prev[pos];
}


void unfilter3(unsigned char *prev, unsigned char *curr) 
{	
	unsigned int pos;
	
	curr[1] += prev[1]/2;
	curr[2] += prev[2]/2;
	curr[3] += prev[3]/2;
	for (pos = 4; pos < step; pos++) 
		curr[pos] += (curr[pos-3]+prev[pos])/2;
}


void unfilter4(unsigned char *prev, unsigned char *curr) 
{	
	unsigned int pos;
	
	curr[1] += paeth(0, prev[1], 0);
	curr[2] += paeth(0, prev[2], 0);
	curr[3] += paeth(0, prev[3], 0);
	for (pos = 4; pos < step; pos++) 
		curr[pos] += paeth(curr[pos-3], prev[pos], prev[pos-3]);
}


void unfilter(unsigned char *prev, unsigned char *curr, int type) 
{
	curr[0] = 0;
	switch (type) 
	{
		case 0:
			break;
		case 1:
			unfilter1(curr);
			break;
		case 2:
			unfilter2(prev, curr);
			break;
		case 3:
			unfilter3(prev, curr);
			break;
		case 4:
			unfilter4(prev, curr);
			break;
		default:
			break;
	}
}


char *read_code(datachunk *chunk) 
{	
	unsigned int size = chunk->sizenum;
	unsigned int type = chunk->typenum;
	unsigned int msgpos = 0;
	unsigned int oldblksize, count, shift, linepos, x;
	unsigned char *prevline = malloc(BYTE*step);
	unsigned char *currline = malloc(BYTE*step);
	unsigned char charmask = 0x01;
	char *msg = malloc(BYTE*size/BYTE*3);
	char curr = '\0';
	
	if(type != IDAT)	return NULL;
	
	shift = 0;
	for (count = 7; count < size; count += step)	//for each scanline
	{
		oldblksize = blksize;
		x = 0;
		for(linepos = 0; linepos < step; linepos++)		//for each byte
		{
			if ((linepos + count)  == blksize+7)		//if start of compression header reached
			{
				blksize = 0x00000000;					//calc next and skip past this
				blksize = (chunk->body[linepos+count+2] << 8) + chunk->body[linepos+count+1];
				blksize += oldblksize + 5;
				count += 5;
				x++;
			}
			if (count+linepos < size) 
				currline[linepos] = chunk->body[linepos + count];
		}
		unfilter(prevline, currline, currline[0]);		//unfilter compression header free line
		blksize = oldblksize;
		count -= 5*x;
		for(linepos = 0; linepos < step; linepos++)		//return unfiltered line to its place
		{
			if ((linepos + count)  == blksize+7) 
			{
				blksize = 0x00000000;
				blksize = (chunk->body[count+linepos+2] << 8) + chunk->body[count+linepos+1];
				blksize += oldblksize + 5;
				count += 5;
			}
			if (count+linepos < size) 
			{
				chunk->body[count+linepos] = currline[linepos];
				if (linepos) 
				{
					curr |= ((chunk->body[count+linepos] & charmask) << shift);	//rebuild each char
					
					if (shift == 7)		//output char when it's reconstructed
					{	
						msg[msgpos++] = curr;
						if (curr == EOF /*!curr*/) //entire message was encluded
							return msg;
						curr = '\0';
						shift = 0;
					}
					else
						shift++;
				}
			}
			prevline[linepos] = currline[linepos];
		}
	}
	
	//message was clipped due to size constraints
	msg[msgpos] = EOF;
	return msg;
}


int write_code(datachunk *chunk, char *msg) 
{	
	unsigned int size = chunk->sizenum;
	unsigned int type = chunk->typenum;
	unsigned int count = 7;
	unsigned int x = 0;
	unsigned int msgloc = 0;
	unsigned int linepos, oldblksize;
	unsigned char prevline[BYTE*step];
	unsigned char currline[BYTE*step];
	unsigned char charmask = 0x01;	//0b 0000 0001
	unsigned char matchmask = 0xFE;	//0b 1111 1110
	char ch, shift, endflag;
	
	//if this is a data chunk, can write data here
	ch = msg[msgloc];
	endflag = ch == EOF;
	shift = 0;
	for (count = 7; count < size; count += step) 
	{
		oldblksize = blksize;
		x = 0;
		for(linepos = 0; linepos < step; linepos++) 
		{
			if ((linepos + count)  == blksize+7) 
			{
				blksize = 0x00000000;
				blksize = (chunk->body[linepos+count+2] << 8) + chunk->body[linepos+count+1];
				blksize += oldblksize + 5;
				count += 5;
				x++;
			}
			if (count+linepos < size)
				currline[linepos] = chunk->body[linepos + count];
		}
		unfilter(prevline, currline, currline[0]);
		blksize = oldblksize;
		count -= 5*x;
		for(linepos = 0; linepos < step; linepos++) 
		{
			if ((linepos + count)  == blksize+7) 
			{
				blksize = 0x00000000;
				blksize = (chunk->body[count+linepos+2] << 8) + chunk->body[count+linepos+1];
				blksize += oldblksize + 5;
				count += 5;
			}
			if (count+linepos < size) 
			{
				//chunk->body[count+linepos] = currline[linepos];
				if (linepos) 
				{
					//chunk->body[count+linepos] &= matchmask;	//hide each bit of the char
					//chunk->body[count+linepos] |= ((ch >> shift) & charmask);
					
					if (shift == 7) 
					{
						if (!endflag) 
						{
							ch = msg[++msgloc];
							endflag = ch == EOF;
						}
						shift = 0;
					}
					else
						shift++;
				}
			}
			prevline[linepos] = currline[linepos];
		}
	}
	
	return endflag;
}
