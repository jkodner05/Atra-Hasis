/*
 *  AtraHasis.c
 *  
 *
 *  Jordan Kodner, Anand Sundaram, July 2012.
 *
 */

#include "AtraHasis.h"

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

void write_out(char *data, unsigned int size) 
{
	fwrite(data, BYTE, size, fout);
}

char *get_header() 
{	
	char *header;
	
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
	
	size = malloc(BYTE*CH_SIZE);
	type = malloc(BYTE*CH_SIZE);
	body = malloc(BYTE*4);
	crc = malloc(BYTE*CH_SIZE);	
	
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
	collated->crc = recalculate_crc(chunk);
	collated->sizenum = chars_to_int(size);
	collated->typenum = chars_to_int(type);
	
	rewind(fin);	//restarts for the next function
	return collated;
}


int write_body(datachunk *chunk, char *msg)
{
	write_out(chunk->size, CH_SIZE);
	write_out(chunk->type, CH_SIZE);
	if(chunk->typenum != IDAT) 
	{
		write_out(chunk->body, chunk->sizenum);
		write_out(chunk->crc, CH_SIZE);
		return chunk->typenum == IEND;
	}
	write_code(chunk, msg);
	write_out(chunk->body, chunk->sizenum);
	chunk->crc = recalculate_crc(chunk);
	write_out(chunk->crc, CH_SIZE);
	return FALSE;
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
	return encrypt_text(msg);
}


char *decode_msg(char *msg)
{
	return decrypt_text(msg);
}

main(int argc, char *argv[]) 
{
	int x;
	int done = FALSE;
	char *header, *msg;
	datachunk *chunk, *IDATchunk;
	
	//creates checksum table. Only needs to be called once
	chksum_crc32gentab();
	
	if(argc > 4 || argc < 2)	//if the format is wrong...
	{	
		printf("\n\t***INCORRECT ARGUMENT FORMAT***\n\n");
		printf("to ENCODE:\t ./steganography [input image] [input text] [output image]\n");
		printf("to DECODE:\t ./steganography [input image]\n\n");
		return 0;
	}
	else if(argc == 2)	//if decode format is entered
	{	
		printf("DECODING MESSAGE.");
		open_files(argv[1], NULL, NULL);
		printf(".");
		IDATchunk = collate();
		
		printf(".\n");
		printf("\nSTART DECODED MESSAGE:\n");
		msg = read_code(IDATchunk);
		msg = decode_msg(msg);
		printf("\n%s\n", msg);
		printf("\n:END DECODED MESSAGE\n\n");
	}
	else if(argc == 4)	//if encode format is entered
	{
		printf("ENCODING MESSAGE");
		open_files(argv[1], argv[2], argv[3]);
		msg = encode_msg();
		IDATchunk = collate();
		printf(".");
		
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
		write_body(IDATchunk, msg); 
		free_chunk(IDATchunk);
		printf(".");
		
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
		printf(".\n");
		
		printf("MESSAGE ENCODED SUCCESSFULLY\n");
	}
	close_files();
	
	return 0;
	
}


