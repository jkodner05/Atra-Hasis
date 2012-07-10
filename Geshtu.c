/*
 *  Geshtu.c
 *  
 *
 *  Created by Jordan Kodner on 6/12/12.
 *
 */

#include "Geshtu.h"

FILE *fin, *fout, *ftext;
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


unsigned int chars_to_int(char *bytes) {
	
	unsigned int *chars = (unsigned int *) bytes;
	
	return	(*chars << 0x18) |				//just inverts the bytes
			(*chars << 0x08 & 0x00FF0000) |
			(*chars >> 0x08 & 0x0000FF00) |
			(*chars >> 0x18);	
}


char *int_to_chars(unsigned int integer) {

	char *chars = malloc(4*BYTE);
	
	chars[3] = (char) (integer & 0x000000FF);	//inverts the bytes
	chars[2] = (char) ((integer & 0x0000FF00) >> 0x08);
	chars[1] = (char) ((integer & 0x00FF0000) >> 0x10);
	chars[0] = (char) (integer >> 0x18);
	
	return chars;
}


void close_files() {
	
	fclose(fin);
	if(fout)
		fclose(fout);
	if(ftext)
		fclose(ftext);
}


void open_files(char *inname, char *outname, char* textname) {
	
	if((fin = fopen(inname, "r")) == NULL) {
		printf("***Input file could not be opened***\n");
	}
	else if(outname && textname) {
		if((fout = fopen(outname, "w")) == NULL) {
			printf("***Output file could not be opened***\n");
			close_files();
		}
		if((ftext = fopen(textname, "r")) == NULL) {
			printf("***Text file could not be opened***\n");
			close_files();
		}
	}
	else {
		fout = NULL;
		ftext = NULL;
	}
}


void free_chunk(unsigned char **chunk) {
	
	unsigned int size = chars_to_int(SIZECH); 
	int x;
	/*for(x = 0; x < 4; x++)
		if(x == 2)
			free(chunk[x]);*/
	free(chunk);
}


void write_out(char *data, unsigned int size) {

	fwrite(data, BYTE, size, fout);
}


char *recalculate_crc(unsigned char **chunk) {
	
	int x;
	int datasize = BYTE*(CH_SIZE + chars_to_int(SIZECH));
	char *data = malloc(datasize);
	char *old_crc;
	
	for (x = 0; x < CH_SIZE; x++) {
		data[x] = TYPECH[x];
	}
	
	for (x = 0; x < datasize - CH_SIZE; x++) {
		data[x+4] = BODYCH[x];
	}
	old_crc = CRCCH;
	CRCCH = int_to_chars(chksum_crc32(data, datasize));
	free(old_crc);
	free(data);
	
	return CRCCH;
}


char *get_header() {
	
	char *header;
	
	header = malloc(BYTE*HEADER_SIZE);
	fread(header, BYTE, HEADER_SIZE, fin);
	
	return header;
}


unsigned char **collate() {
	
	unsigned char **chunk = malloc(BYTE*4), **collated = malloc(BYTE*4);
	unsigned char *size, *type, *body, *crc;
	unsigned int fullsize = 0, currtype = 0, deposit = 0;
	unsigned int x;
	
	size = malloc(BYTE*CH_SIZE);
	type = malloc(BYTE*CH_SIZE);
	body = malloc(BYTE*4);
	crc = malloc(BYTE*CH_SIZE);	
	
	get_header();
	
	chunk = process_chunk();
	currtype = chars_to_int(TYPECH);
	while (currtype != IDAT) {	//passes through non-data chunks
		chunk = process_chunk();
		currtype = chars_to_int(TYPECH);
	}
	
	while (currtype == IDAT) {	//sums sizes of data chunks
		fullsize += chars_to_int(SIZECH);
		chunk = process_chunk();
		currtype = chars_to_int(TYPECH);
	}
	
	body = malloc(BYTE*fullsize);
	size = int_to_chars(fullsize);
	type = int_to_chars(IDAT);

	
	rewind(fin);	//starts over
	get_header();
	
	chunk = process_chunk();
	currtype = chars_to_int(TYPECH);
	while (currtype != IDAT) {
		chunk = process_chunk();
		currtype = chars_to_int(TYPECH);
	}
	
	while (currtype == IDAT) {	//appends all data chunks
		for (x = 0; x < chars_to_int(SIZECH); x++) {
			body[deposit+x] = BODYCH[x];
		}
		deposit += chars_to_int(SIZECH);
		chunk = process_chunk();
		currtype = chars_to_int(TYPECH);
	}
	
	blksize = 0x00000000;	//finds size of first compression block
	blksize = (body[4] << 8) + body[3];
	
	collated[0] = size;
	collated[1] = type;
	collated[2] = body;
	collated[3] = crc;
	collated[3] = recalculate_crc(chunk);
	
	rewind(fin);	//restarts for the next function
	return collated;
}


unsigned char **process_chunk() {
		
	unsigned char **chunk, *size, *type, *body, *crc;
	unsigned int bodysize;
	
	size = malloc(BYTE*CH_SIZE);
	type = malloc(BYTE*CH_SIZE);
	body = malloc(BYTE*4);
	crc = malloc(BYTE*CH_SIZE);	
	
	fread(size, BYTE, CH_SIZE, fin);
	fread(type, BYTE, CH_SIZE, fin);
	bodysize = chars_to_int(size);	//calculates size of payload
	body = malloc(BYTE*bodysize);
	fread(body, BYTE, bodysize, fin);
	fread(crc,  BYTE, CH_SIZE, fin);
	
	chunk = malloc(BYTE*4);
	SIZECH = size;
	TYPECH = type;
	BODYCH = body;
	CRCCH = crc;
	
	if (chars_to_int(TYPECH) == IDHR) {	//calculate length of scanline in pixels and bytes
		scanlen = chars_to_int(BODYCH);
	//	printf("width: %u\n", scanlen);
	//	printf("height: %u\n", chars_to_int(&BODYCH[4]));
		step = (scanlen*3)+1;
	}
	
	return chunk;
}


void display(unsigned char **chunk) {
	unsigned int count = 7;
	unsigned int type = chars_to_int(TYPECH);
	unsigned int size = chars_to_int(SIZECH);
	int shift, x;
	
	if (type != IDAT) {
		return;
	}
	
	while (count < size) {
		printf("location: %d\n", count);
		for (x = 0; x < step; x++) {
			if (count + x < size) {
				printf("%X ",BODYCH[count+x]);
			}
		}
		printf("\n\n");
		count += step;
	}
}


unsigned int paeth(unsigned int a, unsigned int b, unsigned int c) {
	
	unsigned int p, pa, pb, pc;
	
	p = a + b - c;
	pa = abs(p-a);
	pb = abs(p-b);
	pc = abs(p-c);
	
	if ((pa <= pb) && (pa <= pc)) {
		return a;
	}
	else if (pb <= pc) {
		return b;
	}
	return c;
}


void filter(unsigned char *prev, unsigned char *curr, int type) {
	
	switch (type) {
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


void unfilter1(unsigned char *curr) {
	
	unsigned int pos;
	for (pos = 4; pos < step; pos++) {
		curr[pos] += curr[pos-3];
	}
}


void unfilter2(unsigned char *prev, unsigned char *curr) {
	
	unsigned int pos;
	for (pos = 1; pos < step; pos++) {
		curr[pos] += prev[pos];
	}
}


void unfilter3(unsigned char *prev, unsigned char *curr) {
	
	unsigned int pos;
	curr[1] += prev[1]/2;
	curr[2] += prev[2]/2;
	curr[3] += prev[3]/2;
	for (pos = 4; pos < step; pos++) {
		curr[pos] += (curr[pos-3]+prev[pos])/2;
	}
}


void unfilter4(unsigned char *prev, unsigned char *curr) {
	
	unsigned int pos, predictor;
	curr[1] += paeth(0, prev[1], 0);
	curr[2] += paeth(0, prev[2], 0);
	curr[3] += paeth(0, prev[3], 0);
	for (pos = 4; pos < step; pos++) {
		curr[pos] += paeth(curr[pos-3], prev[pos], prev[pos-3]);
	}
}


void unfilter(unsigned char *prev, unsigned char *curr, int type) {

	curr[0] = 0;
	switch (type) {
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


int read_code(unsigned char **chunk) {
	
	unsigned char *prevline = malloc(BYTE*step);
	unsigned char *currline = malloc(BYTE*step);
	unsigned int type = chars_to_int(TYPECH);
	unsigned int size = chars_to_int(SIZECH);
	unsigned char charmask = 0x01;
	unsigned char matchmask = 0xFE;
	unsigned int count;
	unsigned int stepct = 0;
	unsigned int shift;
	char curr = '\0';
	unsigned int x, linepos;
	unsigned int oldblksize;
	
	if(type != IDAT) {
		return;
	}
	
	shift = 0;
	for (count = 7; count < size; count += step) {	//for each scanline
		oldblksize = blksize;
		x = 0;
		for(linepos = 0; linepos < step; linepos++) {	//for each byte
			if ((linepos + count)  == blksize+7) {		//if start of compression header reached
				blksize = 0x00000000;					//calc next and skip past this
				blksize = (BODYCH[linepos+count+2] << 8) + BODYCH[linepos+count+1];
				blksize += oldblksize + 5;
				count += 5;
				x++;
			}
			if (count+linepos < size) {
				currline[linepos] = BODYCH[linepos + count];
			}
		}
		unfilter(prevline, currline, currline[0]);		//unfilter compression header free line
		blksize = oldblksize;
		count -= 5*x;
		for(linepos = 0; linepos < step; linepos++) {	//return unfiltered line to its place
			if ((linepos + count)  == blksize+7) {
				blksize = 0x00000000;
				blksize = (BODYCH[count+linepos+2] << 8) + BODYCH[count+linepos+1];
				blksize += oldblksize + 5;
				count += 5;
			}
			if (count+linepos < size) {
				BODYCH[count+linepos] = currline[linepos];
				if (linepos) {
					curr |= ((BODYCH[count+linepos] & charmask) << shift);	//rebuild each char
					
					if (shift == 7) {	//output char when it's reconstructed
						printf("%c",curr);
						if (!curr) {
							return TRUE;
						}
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
	
	return FALSE;
}


int write_code(unsigned char **chunk) {
	
	unsigned char prevline[BYTE*step];
	unsigned char currline[BYTE*step];
	unsigned int type = chars_to_int(TYPECH);
	unsigned int size = chars_to_int(SIZECH);
	unsigned char charmask = 0x01;
	unsigned char matchmask = 0xFE;
	char ch, shift, crcflag;
	unsigned int count = 7;
	unsigned int linepos, x=0;
	unsigned int oldblksize;
	
	write_out(SIZECH, CH_SIZE);
	write_out(TYPECH, CH_SIZE);
	if(type != IDAT) {
		write_out(BODYCH, size);
		write_out(CRCCH, CH_SIZE);
		return type == IEND;
	}

	ch = getc(ftext);
	crcflag = ch != EOF;
	shift = 0;
	for (count = 7; count < size; count += step) {
		//printf("size - count %u\n", size-count);
		oldblksize = blksize;
		x = 0;
		for(linepos = 0; linepos < step; linepos++) {
			if ((linepos + count)  == blksize+7) {
				blksize = 0x00000000;
				blksize = (BODYCH[linepos+count+2] << 8) + BODYCH[linepos+count+1];
				blksize += oldblksize + 5;
				count += 5;
				x++;
			}
			if (count+linepos < size) {
				currline[linepos] = BODYCH[linepos + count];
			}
		}
		unfilter(prevline, currline, currline[0]);
		blksize = oldblksize;
		count -= 5*x;
		for(linepos = 0; linepos < step; linepos++) {
			if ((linepos + count)  == blksize+7) {
				blksize = 0x00000000;
				blksize = (BODYCH[count+linepos+2] << 8) + BODYCH[count+linepos+1];
				blksize += oldblksize + 5;
				count += 5;
			}
			if (count+linepos < size) {
				BODYCH[count+linepos] = currline[linepos];
				if (linepos) {
					BODYCH[count+linepos] &= matchmask;	//hide each bit of the char
					BODYCH[count+linepos] |= ((ch >> shift) & charmask);
					
					if (shift == 7) {
						ch = getc(ftext);
						if (ch == EOF) {
							ch = '\0';
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
	
	write_out(BODYCH, size);
	if (crcflag) {
		CRCCH = recalculate_crc(chunk);
	}
	write_out(CRCCH, CH_SIZE);
	return FALSE;
}


main(int argc, char *argv[]) {	
	
	int x, done = FALSE;
	char *header;
	unsigned char **chunk, **IDATchunk;
	
	chksum_crc32gentab();
	
	if(argc > 4 || argc < 2) {
		printf("\n\t***INCORRECT ARGUMENT FORMAT***\n\n");
		printf("to ENCODE:\t ./steganography [input image] [input text] [output image]\n");
		printf("to DECODE:\t ./steganography [input image]\n\n");
		return 0;
	}
	else if(argc == 2) {
		printf("DECODING MESSAGE.");
		open_files(argv[1], NULL, NULL);
		printf(".");
		IDATchunk = collate();
		
		printf(".\n");
		printf("\nSTART DECODED MESSAGE:\n");
		read_code(IDATchunk);
		printf("\n:END DECODED MESSAGE\n\n");
	}
	else if(argc == 4) {
		printf("ENCODING MESSAGE");
		open_files(argv[1], argv[2], argv[3]);
				
		IDATchunk = collate();
		printf(".");

		header = get_header();
		write_out(header, HEADER_SIZE);
		free(header);
		
		chunk = process_chunk();
		while (chars_to_int(TYPECH) != IDAT) {
			write_code(chunk);
			free_chunk(chunk);
			chunk = process_chunk();
		}
		write_code(IDATchunk); 
		free_chunk(IDATchunk);
		printf(".");
		
		while (chars_to_int(TYPECH) == IDAT) {
			chunk = process_chunk();
		}
		write_code(chunk);
		free_chunk(chunk);
		
		while (chars_to_int(TYPECH) != IEND) {
			chunk = process_chunk();
			write_code(chunk);
			free_chunk(chunk);
		}
		printf(".\n");
		
		printf("MESSAGE ENCODED SUCCESSFULLY\n");
	}
	close_files();
	
	return 0;
}