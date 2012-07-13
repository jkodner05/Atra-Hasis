/*
 *  AtraHasis.h
 *  
 *
 *  Jordan Kodner, Anand Sundaram, July 2012.
 *
 */

#include "Geshtu.c"
#include "Jarasandha.c"

FILE *fin, *fout, *ftext;

void close_files();							//closes in and out image files
void open_files(char *inname, char *outname, char *textname);	//opens image files of specified names
char *get_header();							//reads header from image
datachunk *process_chunk();			//reads and parses chunk from image
datachunk *collate();				//combines all IDAT chunks
void write_out(char *data, unsigned int size);	//writes to out image file
int write_body(datachunk *chunk, char *msg);
char *encode_msg();							//encodes input text
char *decode_msg(char *msg);							//decodes output text

