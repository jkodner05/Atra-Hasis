#include <stdio.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

/* Encryption key should have at least as many characters as */
/* the number of enryption rounds */
#define KEY "XXHQ\'GGG\n529\t95???..!!~12v1PP75r"
#define ROUNDS 32
#define ENCODE 1
#define DECODE 0
/* Use even MAXLINE such that MAXLINE > length of key,
 * but MAXLINE < (2 * length of key)
 */
#define MAXLINE 60
#define BYTE sizeof(char)

int encrypt(char msg[]);

void decrypt(char msg[], int msgLen);

char *encrypt_text(const char *msg);

char *decrypt_text(const char *msg);

void feistel(char codeString[], char roundKey[],
	     int msgLength, int keyLength);

int hash(char cipher[], int keyLength);

void rotateKey(char cipher[], char new, int keyLength);

void codeSwap(char msg[], char codeString[], int msgLength);

void decodeSwap(char msg[], char codeString[], int msgLength);
