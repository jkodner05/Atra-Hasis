/*
 *  AtraHasis.h
 *  
 *
 *  Jordan Kodner, Anand Sundaram, July 2012.
 *
 */

#include "Jarasandha.h"
#include "Geshtu.h"

FILE *fin, *fout, *ftext;

unsigned int chars_to_int(unsigned char *bytes);
/* converts 4 bytes to little-endian 32-bit int */
unsigned char *int_to_chars(unsigned int integer);
/*converts little-endian 32-bit int to 4 bytes */
void free_chunk(datachunk *chunk);
/* frees chunk data structure */
unsigned char *recalculate_crc(datachunk *chunk);
void close_files();
/* closes in and out image files */
void open_files(char *inname, char *outname, char *textname);
/* opens image files of specified names */
unsigned char *get_header();
/* reads header from image */
datachunk *process_chunk();
/* reads and parses chunk from image */
datachunk *collate();
/* combines all IDAT chunks */
void write_out(unsigned char *data, unsigned int size);
/* writes to out image file */
int write_body(datachunk *chunk, char *msg);
char *encode_msg();
/* encodes input text */
char *decode_msg(char *msg);
/* decodes output text */

