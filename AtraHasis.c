/*
 *  AtraHasis.c
 *  
 *
 *  Jordan Kodner, Anand Sundaram, July 2012.
 *
 */

#include "AtraHasis.h"

unsigned int scanlen, step, blksize;

unsigned int chars_to_int(unsigned char *bytes) 
{
	unsigned int *chars = (unsigned int *) bytes;
	
	return	(*chars << 0x18) |				//just inverts the bytes
	(*chars << 0x08 & 0x00FF0000) |
	(*chars >> 0x08 & 0x0000FF00) |
	(*chars >> 0x18);	
}


unsigned char *int_to_chars(unsigned int integer) 
{
	unsigned char *chars = malloc(4*BYTE);
	
	chars[3] = (char) (integer & 0x000000FF);	//inverts the bytes
	chars[2] = (char) ((integer & 0x0000FF00) >> 0x08);
	chars[1] = (char) ((integer & 0x00FF0000) >> 0x10);
	chars[0] = (char) (integer >> 0x18);
	
	return chars;
}

void free_chunk(datachunk *chunk) 
{	
	free(chunk);
}


unsigned char *recalculate_crc(datachunk *chunk) 
{
	int datasize = BYTE*(CH_SIZE + chunk->sizenum);
	int x;
	unsigned char *data = (unsigned char *)malloc(datasize);
	unsigned char *old_crc;
	
	for (x = 0; x < CH_SIZE; x++) 
		data[x] = chunk->type[x];
	
	for (x = 0; x < datasize - CH_SIZE; x++) 
		data[x+4] = chunk->body[x];
	
	old_crc = chunk->crc;
	chunk->crc = (unsigned char *)int_to_chars(chksum_crc32(data, datasize));
	free(old_crc);
	free(data);
	
	return chunk->crc;
}


void close_files() 
{
	fclose(fin);
	if(fout)	fclose(fout);
	if(ftext)	fclose(ftext);
}


void open_files(char *inname, char *outname, char* textname) 
{
	if((fin = fopen(inname, "r")) == NULL) 
	{
		printf("***Input file could not be opened***\n");
	}
	else if(outname && textname) 
	{
		if((fout = fopen(outname, "w")) == NULL) 
		{
			printf("***Output file could not be opened***\n");
			close_files();
		}
		if((ftext = fopen(textname, "r")) == NULL) 
		{
			printf("***Text file could not be opened***\n");
			close_files();
		}
	}
	else 
	{
		fout = NULL;
		ftext = NULL;
	}
}

void write_out(unsigned char *data, unsigned int size) 
{
	fwrite(data, BYTE, size, fout);
}

unsigned char *get_header() 
{	
	unsigned char *header;
	
	header = malloc(BYTE*HEADER_SIZE);
	fread(header, BYTE, HEADER_SIZE, fin);
	
	return header;
}

datachunk *process_chunk() 
{		
	datachunk *chunk;
	unsigned char *size, *type, *body, *crc;
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
	
	chunk = malloc(BYTE*sizeof(datachunk));
	chunk->size = size;
	chunk->type = type;
	chunk->body = body;
	chunk->crc = crc;
	chunk->sizenum = chars_to_int(size);
	chunk->typenum = chars_to_int(type);
	
	if (chunk->typenum == IDHR)	//calculate length of scanline in pixels and bytes
	{
		scanlen = chars_to_int(body);
		step = (scanlen*3)+1;
	}
	
	return chunk;
}

datachunk *collate() 
{
	datachunk *chunk = malloc(BYTE*sizeof(datachunk));
	datachunk *collated = malloc(BYTE*sizeof(datachunk));
	unsigned char *size, *type, *body, *crc;
	unsigned int fullsize = 0, currtype = 0, deposit = 0;
	unsigned int x;
	
	size = (unsigned char *)malloc(BYTE*CH_SIZE);
	type = (unsigned char *)malloc(BYTE*CH_SIZE);
	body = (unsigned char *)malloc(BYTE*4);
	crc = (unsigned char *)malloc(BYTE*CH_SIZE);	
	
	get_header();
	
	chunk = process_chunk();
	currtype = chunk->typenum;
	while (currtype != IDAT)	//passes through non-data chunks
	{	
		chunk = process_chunk();
		currtype = chunk->typenum;
	}
	
	while (currtype == IDAT)	//sums sizes of data chunks
	{
		fullsize += chunk->sizenum;
		chunk = process_chunk();
		currtype = chunk->typenum;
	}
	
	body = malloc(BYTE*fullsize);
	size = int_to_chars(fullsize);
	type = int_to_chars(IDAT);
	
	
	rewind(fin);	//starts over
	get_header();
	
	chunk = process_chunk();
	currtype = currtype = chunk->typenum;
	while (currtype != IDAT) 
	{
		chunk = process_chunk();
		currtype = chunk->typenum;
	}
	
	while (currtype == IDAT)	//appends all data chunks
	{
		for (x = 0; x < chunk->sizenum; x++) 
			body[deposit+x] = chunk->body[x];
		deposit += chunk->sizenum;
		chunk = process_chunk();
		currtype = chunk->typenum;
	}
	
	blksize = 0x00000000;	//finds size of first compression block
	blksize = (body[4] << 8) + body[3];
	
	collated->size = size;
	collated->type = type;
	collated->body = body;
	collated->crc = crc;
	collated->crc = recalculate_crc(collated);
	collated->sizenum = chars_to_int(size);
	collated->typenum = chars_to_int(type);
	
	rewind(fin);	//restarts for the next function
	return collated;
}


int write_body(datachunk *chunk, char *msg)
{
	int success;
	write_out(chunk->size, CH_SIZE);
	write_out(chunk->type, CH_SIZE);
	if(chunk->typenum != IDAT) 
	{
		write_out(chunk->body, chunk->sizenum);
		write_out(chunk->crc, CH_SIZE);
		return FALSE;
	}
	success = write_code(chunk, msg);
	write_out(chunk->body, chunk->sizenum);
	chunk->crc = recalculate_crc(chunk);
	write_out(chunk->crc, CH_SIZE);
	return success;
}


char *encode_msg()
{
	unsigned int len = 0;
	char *msg;
	
	while (!feof(ftext)) 
	{
		fgetc(ftext);
		len++;
	}
	
	msg = malloc(len*BYTE + 1);
	rewind(ftext);
	
	len = 0;
	while (!feof(ftext)) 
		msg[len++] = fgetc(ftext);
	
	rewind(ftext);
	printf("%s\n\n", msg);
	return encrypt_text(msg);
}


char *decode_msg(char *msg)
{
	return decrypt_text(msg);
}

int main(int argc, char *argv[]) 
{
	unsigned char *header;
	char *msg;
	datachunk *chunk, *IDATchunk;
	int success = FALSE;
	
	//creates checksum table. Only needs to be called once
	chksum_crc32gentab();
	
	if(argc > 4 || argc < 2)	//if the format is wrong...
	{	
		printf("\n\t***INCORRECT ARGUMENT FORMAT***\n\n");
		printf("to ENCODE:\t ./steganography [input image] [output image] [input text]\n");
		printf("to DECODE:\t ./steganography [input image]\n\n");
		return 0;
	}
	else if(argc == 2)	//if decode format is entered
	{	
		printf("\nDECODING MESSAGE...\n");
		open_files(argv[1], NULL, NULL);
		IDATchunk = collate();
		msg = read_code(IDATchunk);
		msg = decode_msg(msg);
		
		printf("\nSTART DECODED MESSAGE:\n");
		printf("\n%s\n", msg);
		printf("\n:END DECODED MESSAGE\n\n");
	}
	else if(argc == 4)	//if encode format is entered
	{
		printf("\nENCODING THIS MESSAGE...\n\n");
		open_files(argv[1], argv[2], argv[3]);
		msg = encode_msg();
		IDATchunk = collate();
		
		header = get_header();
		write_out(header, HEADER_SIZE);
		free(header);
		
		chunk = process_chunk();
		while (chars_to_int(chunk->type) != IDAT) 
		{
			write_body(chunk, NULL);
			free_chunk(chunk);
			chunk = process_chunk();
		}
		success = write_body(IDATchunk, msg);
		free_chunk(IDATchunk);
		
		while (chars_to_int(chunk->type) == IDAT) 
		{
			chunk = process_chunk();
		}
		write_body(chunk, NULL);
		free_chunk(chunk);
		
		while (chars_to_int(chunk->type) != IEND) 
		{
			chunk = process_chunk();
			write_body(chunk, NULL);
			free_chunk(chunk);
		}
		
		if (success)
			printf("MESSAGE ENCODED SUCCESSFULLY\n\n");
		else 
			printf("MESSAGE ENCODING FAILED\nMaybe your image was too small?\n\n");

	}
	close_files();
	
	return 0;
	
}


