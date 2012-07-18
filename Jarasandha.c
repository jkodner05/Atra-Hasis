/* Anand Sundaram. June 2012. */
/* This program takes 2 file names as inputs. */
/* Data from the first file is encrypted. */
/* The resulting ciphertext is stored in the second file. */
/* This code was refactored for the Atra-Hasis project. */
/* This is now a helper function file for the main program. */

#include "Jarasandha.h"

/* Rotates the bits of unsigned input */
unsigned rightrot(unsigned x, int n)
{
  int i, lsb;
  unsigned spin = x;
  size_t len = 8*sizeof(x);
  for (i = 0; i < n; i++) {
    lsb = (spin & 1);
    spin >>= 1;
    if (lsb)
      spin |= lsb << (len - 1);
  }
  return spin;
}

/* Takes a \0 - delimited password input */
/* Generates and returns an EOF-delimited encryption key */
char *keygen(char *pass)
{
  int i, j;
  unsigned long x;
  int len = strlen(pass);
  char *key = malloc(37 * sizeof(char));
  unsigned long **rands = malloc((len + 1) * sizeof(unsigned long *));
  unsigned long *garble = malloc((len + 1) * sizeof(unsigned long));
  for (i = 0; i < len - 1; i++)
    for (j = 1; j < (pass[i+1] & ULONG_MAX); j++) {
      garble[i] = rightrot((unsigned long) pass[i], j);
      garble[i] |= (pass[i+1] & (1 << (j & CHAR_MAX))) << (j & CHAR_MAX);
    }
  for (j = 1; j < (pass[0] & ULONG_MAX); j++) {
    garble[i] = rightrot((unsigned long) pass[i], j);
    garble[i] ^= (pass[0] & (1 << (j & CHAR_MAX))) << (j & CHAR_MAX);
  }

  for (i = 0; i < len; i++) {
    x = garble[i];
    rands[i] = malloc(37 * sizeof(unsigned long));
    for (j = 0; j < 36; j++)
      rands[i][j] = (x *= 69069, x += 362437);
  }

  for (i = 0; i < 36; i++) {
    x = ULONG_MAX;
    for (j = 0; j < len; j++)
      x ^= rands[j][i];
    key[i] = x & CHAR_MAX;
  }
  key[i] = EOF;
  return key;
}

/* Calculates the hash value of a cipher */
int hash(char cipher[], int keyLen)
{
  int i, mod, temp;
  mod = cipher[0] ^ cipher[keyLen - 1];
  for (i = 1; i < keyLen - 1; i++) {
    temp = cipher[i - 1] & rightrot(cipher[i], cipher[i + 1] & 7);
    temp |= cipher[i] & rightrot(cipher[i + 1], cipher[i - 1] & 7);
    temp |= cipher[i + 1] & rightrot(cipher[i -1], cipher[i] & 7);
    temp ^= ~(cipher[i - 1] & cipher[i] & cipher[i+1]);
    mod = rightrot(mod, i & 7);
    mod ^= temp;
  }
  return mod;
}

/* Rotates a new character into a cipher key */
void rotateKey(char key[], char new, int keyLen)
{
  int i;
  for (i = 0; i < keyLen - 1; i++)
    key[i] = key[i+1];
  key[i] = new;
}

/* A "Feistel" function, albeit an unconventional one. */
/* Nonlinearly and noninvertibly garbles a string */
void feistel(char *codeString, char *roundKey,
	     int msgLen, int keyLen)
{
  int i, c, mod;
  char tempKey[keyLen];
  for (i = 0; i < keyLen; i++)
    tempKey[i] = roundKey[i];

  for (i = 0; i < msgLen; i++) {
    mod = hash(tempKey, keyLen);
    c = (int) codeString[i];
    mod %= c + 5;
    mod &= CHAR_MAX;
    /* Incorporate plaintext into cipher for next character */
    rotateKey(tempKey, codeString[i], keyLen);
    /* Replace plaintext with ciphertext character */
    codeString[i] = mod;
  }
}

/* Use the Feistel network to embed ciphertext in message */
void codeSwap(char msg[], char codeString[], int msgLen)
{
  int i;
  /* XOR encode half of message */
  for (i = 0; i < msgLen / 2; i++)
    codeString[i] ^= msg[msgLen / 2 + i];

  /* Reverse plaintext half and swap / shift it to other side */
  for (i = 0; i < msgLen / 2; i++)
    msg[msgLen - i - 1] = msg[i];

  /* Embed ciphertext half over original position of plaintext half */
  for (i = 0; i < msgLen / 2; i++)
    msg[i] = codeString[i];
}

/* Use the Feistel network to retrace path to plain text */
void decodeSwap(char *msg, char codeString[], int msgLen)
{
  int i;
  /* XOR decode more coded half of message */
  for (i = 0; i < msgLen / 2; i++)
    codeString[i] ^= msg[i];

  /* Reverse less coded half and swap / shift it to other side */
  for (i = 0; i < msgLen / 2; i++)
    msg[i] = msg[msgLen - i - 1];

  /* Embed decoded half over original position of less coded half */
  for (i = 0; i < msgLen / 2; i++)
    msg[i + msgLen / 2] = codeString[i];
}

/* Encrypts input with a Feistel network */
int encrypt(char msg[])
{
  int i, j, len, keyLen, msgLen;
  char cipher[MAXLINE];
  char roundKey[MAXLINE];

  memset(cipher, '\0', MAXLINE);
  memset(roundKey, '\0', MAXLINE);

  keyLen = sprintf(cipher, "%s", KEY);

  /* Count size of message */
  msgLen = strlen(msg);
  char codeString[msgLen / 2];

  /* Make sure message has even length */
  if (msgLen % 2 > 0)
    msgLen++;
   
  /* Encode plain text */
  for (i = 1; i <= ROUNDS; i++) {
    /* Generate round key from primary key */
    memset(cipher, '\0', MAXLINE);
    memset(roundKey, '\0', MAXLINE);
    
    keyLen = sprintf(cipher, "%s", KEY);
    len = sprintf(roundKey, "%s", KEY);

    for (j = 0; j < i; j++)
      rotateKey(roundKey, roundKey[0], keyLen);
    feistel(roundKey, cipher, len, keyLen);
    for (j = 0; j < i; j++)
      rotateKey(roundKey, roundKey[0], keyLen);
    
    /* Use Feistel function to encode half of message with round key */
    memset(codeString, '\0', msgLen / 2);
    for (j = 0; j < msgLen / 2; j++)
      codeString[j] = msg[j];
    feistel(codeString, roundKey, msgLen / 2, keyLen);

    /* XOR encode and swap halves */
    codeSwap(msg,codeString,msgLen);
  }

  return msgLen;
}

/* Decrypts with a Feistel network */
void decrypt(char msg[], int msgLen)
{
  int i, j, len, keyLen;
  char cipher[MAXLINE];
  char roundKey[MAXLINE];
  char codeString[msgLen / 2];

  /* Decode cipher text */
  for (i = ROUNDS; i > 0; i--) {
    /* Generate round key from primary key */
    memset(cipher, '\0', MAXLINE);
    memset(roundKey, '\0', MAXLINE);
    
    keyLen = sprintf(cipher, "%s", KEY);
    len = sprintf(roundKey, "%s", KEY);

    for (j = 0; j < i; j++)
      rotateKey(roundKey, roundKey[0], keyLen);
    feistel(roundKey, cipher, len, keyLen);
    for (j = 0; j < i; j++)
      rotateKey(roundKey, roundKey[0], keyLen);
    
    /* Use Feistel function to decode half of message with round key */
    memset(codeString, '\0', msgLen / 2);
    for (j = 0; j < msgLen / 2; j++)
      codeString[j] = msg[msgLen - j - 1];
    feistel(codeString, roundKey, msgLen / 2, keyLen);

    /* XOR decode and swap halves */
    decodeSwap(msg,codeString,msgLen);
  }
}


/* INPUT: string representing ENTIRE text of file to be encoded.
 * EOF delimited.
 * OUTPUT: string representing entire encoded text.
 * EOF delimited. */
char *encrypt_text(const char *msg)
{ 
  int c, i, j, rpos, wpos, msgLen;
  char chunk[MAXLINE];
  char *ciphertext;

  /* Count size of message */
  msgLen = 0;
  while ((c = *(msg + msgLen++)) != EOF)
    ;

  ciphertext = malloc((msgLen + 1) * BYTE);
  memset(chunk, '\0', MAXLINE);

  rpos = wpos = i = 0;
  /* Read in message */
  while ((c = *(msg + rpos++)) != EOF) {
    chunk[i++] = c;
    /* Encrypt chunk by chunk */
    if (i == MAXLINE - 2) {
      i = encrypt(chunk);
      /* Write ciphertext to output */
      for (j = 0; j < i; j++)
      	*(ciphertext + wpos++) = chunk[j];
      memset(chunk, '\0', MAXLINE);
      i = 0;
    }
  }
  /* Write last chunk to output */
  if (i > 0) {
    i = encrypt(chunk);
    for (j = 0; j < i; j++)
      *(ciphertext + wpos++) = chunk[j];
  }
  *(ciphertext + wpos) = EOF;

  return ciphertext;
}

/* INPUT: string representing entire text to decoded.
 * EOF delimited
 * OUTPUT: string representing entire decoded text. 
 * \0 delimited
 */
char *decrypt_text(const char *msg)
{
  int c, i, j, rpos, wpos, msgLen;
  char chunk[MAXLINE];
  char *plaintext;

  /* Count size of message */
  msgLen = 0;
  while ((c = *(msg + msgLen++)) != EOF)
    ;

  plaintext = malloc((msgLen + 2) * BYTE);
  memset(chunk, '\0', MAXLINE);

  rpos = wpos = i = 0;
  /* Read in input */
  while((c = *(msg + rpos++)) != EOF) {
    chunk[i++] = c;
    /* Decode chunk by chunk */
    if (i == MAXLINE - 2) {
      decrypt(chunk, i);
      /* Write decoded text to output */
      for (j = 0; (c = chunk[j]) != '\0'; j++)
	*(plaintext + wpos++) = c;
      memset(chunk, '\0', MAXLINE);
      i = 0;
    }
  }
  /* Write last chunk */
  if (i > 0) {
    decrypt(chunk,i);
    for (j = 0; (c = chunk[j]) != '\0'; j++)
      *(plaintext + wpos++) = c;
  }

  return plaintext;
}

/*
int main(int argc, char *argv[])
{
  // Warn users of wrong number of arguments
  if (argc < 3 || argc > 4) {
    printf("Invalid number of arguments.\n");
    printf("The blockCode program takes 2-3 arguments.\n");
    printf("Enter 2 filenames. ");
    printf("Data will be read from the first file ");
    printf("and written to the second file.\n");
    printf("Use the option -d to decrypt ciphertext. ");
    printf("Input is assumed to be plaintext ");
    printf("and is encrypted by default.\n");
  }

  // If argument count is acceptable, continue
  else {
    int i, c, mode, msgLen;
    char arg[MAXLINE], in[MAXLINE], out[MAXLINE];
    char *input, *output;
    FILE *fin, *fout;
    mode = ENCODE;
    in[0] = '\0';
    out[0] = '\0';
    
    for (i =1; i < argc; i++) {
      sprintf(arg, "%s", argv[i]);
      // Recognize decode option
      if (arg[0] == '-')
	if (arg[1] == 'd' && arg[2] == '\0')
	  mode = DECODE;
	else {
	  printf("\nError: Option not recognized.\n");
	  return 1;
	}
      else if (in[0] == '\0')
	sprintf(in, "%s", arg);
      else
	sprintf(out, "%s", arg);
    }
        
    // Terminate with error message if no input file given 
    if (in[0] == '\0') {
      printf("Error: no input file specified.\n");
      return 1;
    }
    else
      fin = fopen(in, "r");
    
    // Terminate with error message if no output file given 
    if (out[0] == '\0') {
      printf("Error: no output file specified.\n");
      return 1;
    }
    else
      fout = fopen(out, "w");
    
    // Perform cryptographical processes

    // Count file length
    for (msgLen = 0; (c = fgetc(fin)) != EOF; msgLen++)
      ;

    // Reopen file and copy to char pointer on heap
    rewind(fin);
    input = malloc((msgLen + 2) * BYTE);
    output = malloc((msgLen + 2) * BYTE);
    memset(input, '\0', msgLen + 2);
    memset(output, '\0', msgLen + 2);

    for (i = 0; (c = fgetc(fin)) != EOF; i++)
      input[i] = c;
    input[i] = c;

    // Convert input to output
    if (mode == ENCODE)
      output = encrypt_text(input);
    else
      output = decrypt_text(input);

    fwrite(output, BYTE, msgLen + (msgLen % 2), fout);

    fclose(fin);
    fclose(fout);
  }
  return 0;
}
*/
