 /*
 ============================================================================
 Name        : shared.h
 Author      : Sobuno
 Version     : 0.1
 Description : Common functions for the SHA-256 prover and verifier
 ============================================================================
 */
/*
 *  @brief This is the main implementation file of the signature scheme. All of
 *  the LowMC MPC code is here as well as lower-level versions of sign and
 *  verify that are called by the signature API.
 *
 *  This file is part of the reference implementation of the Picnic signature scheme.
 *  See the accompanying documentation for complete details.
 *
 *  The code is provided under the MIT license, see LICENSE for
 *  more details.
 *  SPDX-License-Identifier: MIT
 */
/*
 *
 * Author: Tan Teik Guan
 * Description : KKW for SHA256
 *
 * Copyright pQCee 2023. All rights reserved
 *
 * “Commons Clause” License Condition v1.0
 *
 * The Software is provided to you by the Licensor under the License, as defined below, subject to the following
 * condition.
 *
 * Without limiting other conditions in the License, the grant of rights under the License will not include, and
 * the License does not grant to you, the right to Sell the Software.
 *
 * For purposes of the foregoing, “Sell” means practicing any or all of the rights granted to you under the License
 * to provide to third parties, for a fee or other consideration (including without limitation fees for hosting or
 * consulting/ support services related to the Software), a product or service whose value derives, entirely or
 * substantially, from the functionality of the Software. Any license notice or attribution required by the License
 * must also include this Commons Clause License Condition notice.
 *
 * Software: KKW_SHA256
 *
 * License: MIT 1.0
 *
 * Licensor: pQCee Pte Ltd
 *
 */

#ifndef SHARED_H_
#define SHARED_H_
#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#ifdef _WIN32
#include <openssl/applink.c>
#endif
#include <openssl/rand.h>
#include "omp.h"

#define VERBOSE FALSE
#define ToBytes(x) (x == 0)? 0:((x-1)/8+1)
#define WORD_SIZE_BITS 32

static const uint32_t hA[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
		0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

static const uint32_t k[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
		0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98,
		0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
		0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6,
		0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3,
		0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138,
		0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e,
		0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
		0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
		0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
		0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814,
		0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

//#define ySize 736
#define ySize 728 
//#define rSize 2912 
#define rSize 45392 // 2912*2 = 5824 
#define NUM_PARTIES 32 
#define NUM_ROUNDS 2 
#define SHA256_INPUTS 64

typedef struct {
	unsigned char x[64];
	uint32_t y[ySize];
} View;

typedef struct {
	uint32_t yp[NUM_PARTIES][8];
	unsigned char h[NUM_PARTIES][32];
} a;

typedef struct {
	unsigned char ke[16];
	unsigned char ke1[16];
	View ve;
	View ve1;
	unsigned char re[4];
	unsigned char re1[4];
} z;

#define RIGHTROTATE(x,n) (((x) >> (n)) | ((x) << (32-(n))))
#define GETBIT(x, i) (((x) >> (i)) & 0x01)
#define SETBIT(x, i, b)   x= (b)&1 ? (x)|(1 << (i)) : (x)&(~(1 << (i)))

// prototype from hash.c
// to implement using sha256 instead of sha3

#define MAX_DIGEST_SIZE 64
#define SHA256_DIGEST_SIZE 32

void handleErrors(void)
{
	ERR_print_errors_fp(stderr);
	abort();
}

/*
EVP_CIPHER_CTX setupAES(unsigned char key[16]) {
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);


	unsigned char *iv = (unsigned char *)"01234567890123456";

	if(1 != EVP_EncryptInit_ex(&ctx, EVP_aes_128_ctr(), NULL, key, iv))
		handleErrors();

	return ctx;


}
*/
void getAllRandomness(unsigned char key[16], unsigned char randomness[rSize]) {
	//Generate randomness: We use 728*32 bit of randomness per key.
	//Since AES block size is 128 bit, we need to run 728*32/128 = 182 iterations

	EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
	unsigned char * iv = (unsigned char *) "01234567890123456";
	EVP_CIPHER_CTX_init(ctx);
	//ctx = setupAES(key);
	unsigned char *plaintext =
			(unsigned char *)"0000000000000000";
	int len;
	if (1 != EVP_EncryptInit_ex(ctx,EVP_aes_128_ctr(),NULL,key,iv))
		handleErrors();

	for(int j=0;j<(rSize/16);j++) {
		if(1 != EVP_EncryptUpdate(ctx, &randomness[j*16], &len, plaintext, strlen ((char *)plaintext)))
			handleErrors();

	}
	EVP_CIPHER_CTX_cleanup(ctx);
	EVP_CIPHER_CTX_free(ctx);
}

uint32_t getRandom32(unsigned char randomness[rSize], int randCount) {
	uint32_t ret;
	memcpy(&ret, &randomness[randCount/8], 4);
	return ret;
}


void init_EVP() {
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	//OPENSSL_config(NULL);
}

void cleanup_EVP() {
	EVP_cleanup();
	ERR_free_strings();
}

void H(unsigned char k[16], View v, unsigned char r[4], unsigned char hash[SHA256_DIGEST_LENGTH]) {
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, k, 16);
	SHA256_Update(&ctx, &v, sizeof(v));
	SHA256_Update(&ctx, r, 4);
	SHA256_Final(hash, &ctx);
}


void H3(uint32_t y[8], a* as, int s, int* es) {

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, y, 32);
	SHA256_Update(&ctx, as, sizeof(a)*s);
	SHA256_Final(hash, &ctx);

	//Pick bits from hash
	int i = 0;
	int bitTracker = 0;
	while(i < s) {
		if(bitTracker >= SHA256_DIGEST_LENGTH*8) { //Generate new hash as we have run out of bits in the previous hash
			SHA256_Init(&ctx);
			SHA256_Update(&ctx, hash, sizeof(hash));
			SHA256_Final(hash, &ctx);
			bitTracker = 0;
		}

		int b1 = GETBIT(hash[bitTracker/8], bitTracker % 8);
		int b2 = GETBIT(hash[(bitTracker+1)/8], (bitTracker+1) % 8);
		if(b1 == 0) {
			if(b2 == 0) {
				es[i] = 0;
				bitTracker += 2;
				i++;
			} else {
				es[i] = 1;
				bitTracker += 2;
				i++;
			}
		} else {
			if(b2 == 0) {
				es[i] = 2;
				bitTracker += 2;
				i++;
			} else {
				bitTracker += 2;
			}
		}
	}

}

void output(View v, uint32_t* result) {
	memcpy(result, &v.y[ySize - 8], 32);
}

void reconstruct(uint32_t* y0, uint32_t* y1, uint32_t* y2, uint32_t* result) {
	for (int i = 0; i < 8; i++) {
		result[i] = y0[i] ^ y1[i] ^ y2[i];
	}
}

void mpc_XOR2(uint32_t x[2], uint32_t y[2], uint32_t z[2]) {
	z[0] = x[0] ^ y[0];
	z[1] = x[1] ^ y[1];
}

void mpc_NEGATE2(uint32_t x[2], uint32_t z[2]) {
	z[0] = ~x[0];
	z[1] = ~x[1];
}

omp_lock_t *locks;

void openmp_locking_callback(int mode, int type, char *file, int line)
{
  if (mode & CRYPTO_LOCK)
  {
    omp_set_lock(&locks[type]);
  }
  else
  {
    omp_unset_lock(&locks[type]);
  }
}


unsigned long openmp_thread_id(void)
{
  return (unsigned long)omp_get_thread_num();
}

void openmp_thread_setup(void)
{
  int i;

  locks = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(omp_lock_t));
  for (i=0; i<CRYPTO_num_locks(); i++)
  {
    omp_init_lock(&locks[i]);
  }

  CRYPTO_set_id_callback((unsigned long (*)())openmp_thread_id);
  CRYPTO_set_locking_callback((void (*)())openmp_locking_callback);
}

void openmp_thread_cleanup(void)
{
  int i;

  CRYPTO_set_id_callback(NULL);
  CRYPTO_set_locking_callback(NULL);
  for (i=0; i<CRYPTO_num_locks(); i++)
    omp_destroy_lock(&locks[i]);
  OPENSSL_free(locks);
}

// from Picnic Project
//
/* For an input bit b = 0 or 1, return the word of all b bits, i.e.,
 * extend(1) = 0xFFFFFFFFFFFFFFFF
 * extend(0) = 0x0000000000000000
 * Assumes inputs are always 0 or 1.  If this doesn't hold, add "& 1" to the
 * input.
 */
static uint32_t extend(uint8_t bit)
{
    return ~(bit - 1);
}


/* Get one bit from a byte array */
uint8_t getBit(const uint8_t* array, uint32_t bitNumber)
{
	return (array[bitNumber / 8] >> (7 - (bitNumber % 8))) & 0x01;
}

uint8_t getBit32(uint32_t value, uint32_t bitNumber)
{
	return (value>>(31-bitNumber))&0x01;
}

void setBit32(uint32_t * value, uint32_t bitNumber, uint8_t b)
{
	*value = (b&1)? (*value)|(1<<(31-bitNumber)) : (*value)&(~(1<<(31-bitNumber)));
}

uint8_t getParityFromWordArray(uint32_t * array, uint32_t size, uint32_t bitNumber)
{
	uint8_t parity = 0;

	for (int i=0;i<size;i++)
	{
		parity ^= getBit32(array[i],bitNumber);
	}
	return parity;
}


/* Get one bit from a 32-bit int array for all parties*/
uint32_t getBitFromWordArray(const uint32_t* array, uint32_t size, uint32_t bitNumber)
{
	if (size == 1)
	{
		return getBit32(array[0], bitNumber);
	}
	else
	{
		return getBit32(array[size-1], bitNumber) | (getBitFromWordArray(array,size-1,bitNumber)<<1);
	}
	
}

/* Set a specific bit in a byte array to a given value */
void setBit(uint8_t* bytes, uint32_t bitNumber, uint8_t val)
{
	bytes[bitNumber / 8] = (bytes[bitNumber >> 3]
				& ~(1 << (7 - (bitNumber % 8)))) | (val << (7 - (bitNumber % 8)));
}



static uint32_t parity32(uint32_t x)
{
	uint32_t y = x ^ (x >> 1);

	y ^= (y >> 2);
	y ^= (y >> 4);
	y ^= (y >> 8);
	y ^= (y >> 16);
	return y & 1;
}

static uint32_t int32ToWord(uint32_t x[NUM_PARTIES], int posn)
{
	uint32_t shares;

	for (size_t i = 0; i < NUM_PARTIES;i++) // NUM_PARTIES = 32 
	{
		uint8_t bit = getBit32(x[i],posn);
		setBit32(&shares,i,bit);
	}
	return shares;

}

static uint32_t tapesToWord(unsigned char randomness[NUM_PARTIES][rSize],int * randCount)
{
	uint32_t shares;

	for (size_t i = 0; i < NUM_PARTIES;i++) // NUM_PARTIES = 32 
	{
		uint8_t bit = getBit(randomness[i],*randCount);
		setBit((uint8_t*)&shares,i,bit);
	}
	*randCount += 1;

	return shares;  
}

int mpc_AND_verify(uint32_t x[2], uint32_t y[2], uint32_t z[2], View ve, View ve1, unsigned char randomness[2][rSize], int* randCount, int* countY) {
	uint32_t r[2] = { getRandom32(randomness[0], *randCount), getRandom32(randomness[1], *randCount) };
	*randCount += 4;

	uint32_t t = 0;

	t = (x[0] & y[1]) ^ (x[1] & y[0]) ^ (x[0] & y[0]) ^ r[0] ^ r[1];
	if(ve.y[*countY] != t) {
		return 1;
	}
	z[0] = t;
	z[1] = ve1.y[*countY];

	(*countY)++;
	return 0;
}


int mpc_ADD_verify(uint32_t x[2], uint32_t y[2], uint32_t z[2], View ve, View ve1, unsigned char randomness[2][rSize], int* randCount, int* countY) {
	uint32_t r[2] = { getRandom32(randomness[0], *randCount), getRandom32(randomness[1], *randCount) };
	*randCount += 4;

	uint8_t a[2], b[2];

	uint8_t t;

	for(int i=0;i<31;i++)
	{
		a[0]=GETBIT(x[0]^ve.y[*countY],i);
		a[1]=GETBIT(x[1]^ve1.y[*countY],i);

		b[0]=GETBIT(y[0]^ve.y[*countY],i);
		b[1]=GETBIT(y[1]^ve1.y[*countY],i);

		t = (a[0]&b[1]) ^ (a[1]&b[0]) ^ GETBIT(r[1],i);
		if(GETBIT(ve.y[*countY],i+1) != (t ^ (a[0]&b[0]) ^ GETBIT(ve.y[*countY],i) ^ GETBIT(r[0],i))) {
			return 1;
		}
	}

	z[0]=x[0]^y[0]^ve.y[*countY];
	z[1]=x[1]^y[1]^ve1.y[*countY];
	(*countY)++;
	return 0;
}

void mpc_RIGHTROTATE2(uint32_t x[], int i, uint32_t z[]) {
	z[0] = RIGHTROTATE(x[0], i);
	z[1] = RIGHTROTATE(x[1], i);
}

void mpc_RIGHTSHIFT2(uint32_t x[2], int i, uint32_t z[2]) {
	z[0] = x[0] >> i;
	z[1] = x[1] >> i;
}


int mpc_MAJ_verify(uint32_t a[2], uint32_t b[2], uint32_t c[2], uint32_t z[3], View ve, View ve1, unsigned char randomness[2][rSize], int* randCount, int* countY) {
	uint32_t t0[3];
	uint32_t t1[3];

	mpc_XOR2(a, b, t0);
	mpc_XOR2(a, c, t1);
	if(mpc_AND_verify(t0, t1, z, ve, ve1, randomness, randCount, countY) == 1) {
		return 1;
	}
	mpc_XOR2(z, a, z);
	return 0;
}

int mpc_CH_verify(uint32_t e[2], uint32_t f[2], uint32_t g[2], uint32_t z[2], View ve, View ve1, unsigned char randomness[2][rSize], int* randCount, int* countY) {

	uint32_t t0[3];
	mpc_XOR2(f,g,t0);
	if(mpc_AND_verify(e, t0, t0, ve, ve1, randomness, randCount, countY) == 1) {
		return 1;
	}
	mpc_XOR2(t0,g,z);


	return 0;
}

void mpc_RIGHTROTATE(uint32_t x[NUM_PARTIES], int j, uint32_t z[NUM_PARTIES]) {

	for (int i=0; i < NUM_PARTIES;i++)
		z[i] = RIGHTROTATE(x[i], j);
}

void mpc_RIGHTSHIFT(uint32_t x[NUM_PARTIES], int j, uint32_t z[NUM_PARTIES]) {
	for (int i=0; i < NUM_PARTIES;i++)
		z[i] = x[i] >> j;
}

void mpc_NEGATE(uint32_t x[NUM_PARTIES], uint32_t z[NUM_PARTIES]) 
{
	for (int i=0; i < NUM_PARTIES;i++)
		z[i] = ~x[i];
}

void mpc_XOR(uint32_t x[NUM_PARTIES], uint32_t y[NUM_PARTIES], uint32_t z[NUM_PARTIES]) 
{
	for (int i=0; i < NUM_PARTIES;i++)
		z[i] = x[i] ^ y[i];
}

int32_t aux_bit_AND(uint8_t mask_a, uint8_t mask_b, unsigned char randomness[NUM_PARTIES][rSize], int *randCount)
{
	uint32_t output_mask = tapesToWord(randomness,randCount);

	size_t lastParty = NUM_PARTIES-1;
	uint32_t and_helper = tapesToWord(randomness,randCount);
	setBit((uint8_t*)&and_helper,NUM_PARTIES-1,0);
	uint8_t aux_bit = (mask_a & mask_b) ^ parity32(and_helper);
	setBit(randomness[lastParty], *randCount-1,aux_bit);

	return output_mask;
} 	

void aux_AND(uint32_t x[NUM_PARTIES], uint32_t y[NUM_PARTIES], uint32_t z[NUM_PARTIES], unsigned char randomness[NUM_PARTIES][rSize], int* randCount) 
{
	uint8_t mask_a,mask_b;
	uint32_t output_mask; // NUM_PARTIES=32

	for (int i = 0; i < 32;i++) 
	{
		mask_a = getParityFromWordArray(x,NUM_PARTIES,i);  
		mask_b = getParityFromWordArray(y,NUM_PARTIES,i);  

		output_mask = aux_bit_AND(mask_a,mask_b,randomness,randCount);

		for (int j = (NUM_PARTIES-1); j >= 0 ; j--)
		{
			setBit32(&z[j],i,output_mask & 0x01);
			output_mask>>=1;
		}
	}


}

void aux_ADD(uint32_t x[NUM_PARTIES], uint32_t y[NUM_PARTIES], uint32_t z[NUM_PARTIES], unsigned char randomness[NUM_PARTIES][rSize], int* randCount) {

	uint32_t aANDb, prev_carry = 0;
	uint32_t carry[NUM_PARTIES] = {0};
	uint8_t mask_a, mask_b;

	// sum = x ^ y ^ c
	// carry = ((x ^ c) & (y ^ c)) ^ c
	memset(carry,0,sizeof(uint32_t)*NUM_PARTIES);
	for (int i = 31; i > 0; i--)
	{
		prev_carry = getBitFromWordArray(carry,NUM_PARTIES,i);
		mask_a = parity32(getBitFromWordArray(x,NUM_PARTIES,i) ^ prev_carry);  
		mask_b = parity32(getBitFromWordArray(y,NUM_PARTIES,i) ^ prev_carry);  

		aANDb = aux_bit_AND(mask_a,mask_b,randomness,randCount);
		aANDb ^= prev_carry;
		{
			for (int j = (NUM_PARTIES-1); j >= 0; j--)
			{
				setBit32(&carry[j],i-1,(aANDb & 0x01));
				aANDb>>=1;
			}
		}
	}

	for (int i=0;i<NUM_PARTIES;i++)
		z[i] = x[i]^y[i]^carry[i];


}

void aux_MAJ(uint32_t a[NUM_PARTIES], uint32_t b[NUM_PARTIES], uint32_t c[NUM_PARTIES], uint32_t z[NUM_PARTIES], unsigned char randomness[NUM_PARTIES][rSize], int* randCount) {
	uint32_t t0[NUM_PARTIES];
	uint32_t t1[NUM_PARTIES];

	mpc_XOR(a, b, t0);
	mpc_XOR(a, c, t1);
	aux_AND(t0, t1, z, randomness, randCount);
	mpc_XOR(z, a, z);
}


void aux_CH(uint32_t e[NUM_PARTIES], uint32_t f[NUM_PARTIES], uint32_t g[NUM_PARTIES], uint32_t z[NUM_PARTIES], unsigned char randomness[NUM_PARTIES][rSize], int* randCount) {
	uint32_t t0[NUM_PARTIES]; 

	//e & (f^g) ^ g
	mpc_XOR(f,g,t0);
	aux_AND(e,t0,t0, randomness, randCount);
	mpc_XOR(t0,g,z);

}


int computeAuxTape(unsigned char randomness[NUM_PARTIES][rSize],unsigned char shares[NUM_PARTIES][SHA256_INPUTS])
{
	int randCount = 0;

	uint32_t w[64][NUM_PARTIES];

	memset(w,0,sizeof(int32_t)*64*NUM_PARTIES);
	for (int i = 0; i < NUM_PARTIES; i++) {
		for (int j = 0; j < 16; j++) {
			w[j][i] = (shares[i][j * 4] << 24) | (shares[i][j * 4 + 1] << 16)
							| (shares[i][j * 4 + 2] << 8) | shares[i][j * 4 + 3];
		}
	}

	uint32_t s0[NUM_PARTIES], s1[NUM_PARTIES];
	uint32_t t0[NUM_PARTIES], t1[NUM_PARTIES];
	for (int j = 16; j < 64; j++) {
		//s0[i] = RIGHTROTATE(w[i][j-15],7) ^ RIGHTROTATE(w[i][j-15],18) ^ (w[i][j-15] >> 3);
		mpc_RIGHTROTATE(w[j-15], 7, t0);

		mpc_RIGHTROTATE(w[j-15], 18, t1);
		mpc_XOR(t0, t1, t0);
		mpc_RIGHTSHIFT(w[j-15], 3, t1);
		mpc_XOR(t0, t1, s0);

		//s1[i] = RIGHTROTATE(w[i][j-2],17) ^ RIGHTROTATE(w[i][j-2],19) ^ (w[i][j-2] >> 10);
		mpc_RIGHTROTATE(w[j-2], 17, t0);
		mpc_RIGHTROTATE(w[j-2], 19, t1);

		mpc_XOR(t0, t1, t0);
		mpc_RIGHTSHIFT(w[j-2], 10, t1);
		mpc_XOR(t0, t1, s1);
		//w[i][j] = w[i][j-16]+s0[i]+w[i][j-7]+s1[i];
		aux_ADD(w[j-16], s0, t1, randomness, &randCount);
		aux_ADD(w[j-7], t1, t1, randomness, &randCount);
		aux_ADD(t1, s1, w[j], randomness, &randCount);

	}

	uint32_t a[NUM_PARTIES];
	uint32_t b[NUM_PARTIES];
	uint32_t c[NUM_PARTIES];
	uint32_t d[NUM_PARTIES];
	uint32_t e[NUM_PARTIES];
	uint32_t f[NUM_PARTIES];
	uint32_t g[NUM_PARTIES];
	uint32_t h[NUM_PARTIES];
	for (int i = 0; i < NUM_PARTIES;i++)
	{
		a[i] = hA[0];
		b[i] = hA[1];
		c[i] = hA[2];
		d[i] = hA[3];
		e[i] = hA[4];
		f[i] = hA[5];
		g[i] = hA[6];
		h[i] = hA[7];
	}

	uint32_t temp1[NUM_PARTIES], temp2[NUM_PARTIES], temp3[NUM_PARTIES], maj[NUM_PARTIES];

	for (int i = 0; i < 64; i++) {
		//s1 = RIGHTROTATE(e,6) ^ RIGHTROTATE(e,11) ^ RIGHTROTATE(e,25);
		mpc_RIGHTROTATE(e, 6, t0);
		mpc_RIGHTROTATE(e, 11, t1);
		mpc_XOR(t0, t1, t0);

		mpc_RIGHTROTATE(e, 25, t1);
		mpc_XOR(t0, t1, s1);


		//ch = (e & f) ^ ((~e) & g);
		//temp1 = h + s1 + CH(e,f,g) + k[i]+w[i];

		//t0 = h + s1

		aux_ADD(h, s1, t0, randomness, &randCount);


		aux_CH(e, f, g, t1, randomness, &randCount);

		//t1 = t0 + t1 (h+s1+ch)
		aux_ADD(t0, t1, t1, randomness, &randCount);

		for (int j = 0; j < NUM_PARTIES; j++)
			temp3[j] = k[i];	
		aux_ADD(t1, temp3, t1, randomness, &randCount);

		aux_ADD(t1, w[i], temp1, randomness, &randCount);

		//s0 = RIGHTROTATE(a,2) ^ RIGHTROTATE(a,13) ^ RIGHTROTATE(a,22);
		mpc_RIGHTROTATE(a, 2, t0);
		mpc_RIGHTROTATE(a, 13, t1);
		mpc_XOR(t0, t1, t0);
		mpc_RIGHTROTATE(a, 22, t1);
		mpc_XOR(t0, t1, s0);


		aux_MAJ(a, b, c, maj, randomness, &randCount);

		//temp2 = s0+maj;
		aux_ADD(s0, maj, temp2, randomness, &randCount);

		memcpy(h, g, sizeof(uint32_t) * NUM_PARTIES);
		memcpy(g, f, sizeof(uint32_t) * NUM_PARTIES);
		memcpy(f, e, sizeof(uint32_t) * NUM_PARTIES);
		//e = d+temp1;
		aux_ADD(d, temp1, e, randomness, &randCount);
		memcpy(d, c, sizeof(uint32_t) * NUM_PARTIES);
		memcpy(c, b, sizeof(uint32_t) * NUM_PARTIES);
		memcpy(b, a, sizeof(uint32_t) * NUM_PARTIES);
		//a = temp1+temp2;

		aux_ADD(temp1, temp2, a, randomness, &randCount);
	}
	uint32_t hHa[8][NUM_PARTIES];
	for (int i = 0;i < 8;i++)
	{
		for (int j = 0;j < NUM_PARTIES;j++)
			hHa[i][j] = hA[i];
	}
	aux_ADD(hHa[0], a, hHa[0], randomness, &randCount);
	aux_ADD(hHa[1], b, hHa[1], randomness, &randCount);
	aux_ADD(hHa[2], c, hHa[2], randomness, &randCount);
	aux_ADD(hHa[3], d, hHa[3], randomness, &randCount);
	aux_ADD(hHa[4], e, hHa[4], randomness, &randCount);
	aux_ADD(hHa[5], f, hHa[5], randomness, &randCount);
	aux_ADD(hHa[6], g, hHa[6], randomness, &randCount);
	aux_ADD(hHa[7], h, hHa[7], randomness, &randCount);

printf("computeAuxTape: randCount %d\n",randCount);
	return 0;


}


int verify(a a, int e, z z) {
	unsigned char* hash = malloc(SHA256_DIGEST_LENGTH);
	H(z.ke, z.ve, z.re, hash);

	if (memcmp(a.h[e], hash, 32) != 0) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	H(z.ke1, z.ve1, z.re1, hash);
	if (memcmp(a.h[(e + 1) % 3], hash, 32) != 0) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	free(hash);

	uint32_t* result = malloc(32);
	output(z.ve, result);
	if (memcmp(a.yp[e], result, 32) != 0) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}

	output(z.ve1, result);
	if (memcmp(a.yp[(e + 1) % 3], result, 32) != 0) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}

	free(result);

	unsigned char randomness[2][rSize];
	getAllRandomness(z.ke, randomness[0]);
	getAllRandomness(z.ke1, randomness[1]);

	int* randCount = calloc(1, sizeof(int));
	int* countY = calloc(1, sizeof(int));

	uint32_t w[64][2];
	for (int j = 0; j < 16; j++) {
		w[j][0] = (z.ve.x[j * 4] << 24) | (z.ve.x[j * 4 + 1] << 16)
								| (z.ve.x[j * 4 + 2] << 8) | z.ve.x[j * 4 + 3];
		w[j][1] = (z.ve1.x[j * 4] << 24) | (z.ve1.x[j * 4 + 1] << 16)
								| (z.ve1.x[j * 4 + 2] << 8) | z.ve1.x[j * 4 + 3];
	}

	uint32_t s0[2], s1[2];
	uint32_t t0[2], t1[2];
	for (int j = 16; j < 64; j++) {
		//s0[i] = RIGHTROTATE(w[i][j-15],7) ^ RIGHTROTATE(w[i][j-15],18) ^ (w[i][j-15] >> 3);
		mpc_RIGHTROTATE2(w[j-15], 7, t0);
		mpc_RIGHTROTATE2(w[j-15], 18, t1);
		mpc_XOR2(t0, t1, t0);
		mpc_RIGHTSHIFT2(w[j-15], 3, t1);
		mpc_XOR2(t0, t1, s0);

		//s1[i] = RIGHTROTATE(w[i][j-2],17) ^ RIGHTROTATE(w[i][j-2],19) ^ (w[i][j-2] >> 10);
		mpc_RIGHTROTATE2(w[j-2], 17, t0);
		mpc_RIGHTROTATE2(w[j-2], 19, t1);
		mpc_XOR2(t0, t1, t0);
		mpc_RIGHTSHIFT2(w[j-2],10,t1);
		mpc_XOR2(t0, t1, s1);

		//w[i][j] = w[i][j-16]+s0[i]+w[i][j-7]+s1[i];

		if(mpc_ADD_verify(w[j-16], s0, t1, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, j);
#endif
			return 1;
		}


		if(mpc_ADD_verify(w[j-7], t1, t1, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, j);
#endif
			return 1;
		}
		if(mpc_ADD_verify(t1, s1, w[j], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, j);
#endif
			return 1;
		}

	}



	uint32_t va[2] = { hA[0],hA[0] };
	uint32_t vb[2] = { hA[1],hA[1] };
	uint32_t vc[2] = { hA[2],hA[2] };
	uint32_t vd[2] = { hA[3],hA[3] };
	uint32_t ve[2] = { hA[4],hA[4] };
	uint32_t vf[2] = { hA[5],hA[5] };
	uint32_t vg[2] = { hA[6],hA[6] };
	uint32_t vh[2] = { hA[7],hA[7] };
	uint32_t temp1[3], temp2[3], maj[3];
	for (int i = 0; i < 64; i++) {
		//s1 = RIGHTROTATE(e,6) ^ RIGHTROTATE(e,11) ^ RIGHTROTATE(e,25);
		mpc_RIGHTROTATE2(ve, 6, t0);
		mpc_RIGHTROTATE2(ve, 11, t1);
		mpc_XOR2(t0, t1, t0);
		mpc_RIGHTROTATE2(ve, 25, t1);
		mpc_XOR2(t0, t1, s1);




		//ch = (e & f) ^ ((~e) & g);
		//temp1 = h + s1 + CH(e,f,g) + k[i]+w[i];

		//t0 = h + s1

		if(mpc_ADD_verify(vh, s1, t0, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}



		if(mpc_CH_verify(ve, vf, vg, t1, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}

		//t1 = t0 + t1 (h+s1+ch)
		if(mpc_ADD_verify(t0, t1, t1, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}



		t0[0] = k[i];
		t0[1] = k[i];
		if(mpc_ADD_verify(t1, t0, t1, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}



		if(mpc_ADD_verify(t1, w[i], temp1, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}

		//s0 = RIGHTROTATE(a,2) ^ RIGHTROTATE(a,13) ^ RIGHTROTATE(a,22);
		mpc_RIGHTROTATE2(va, 2, t0);
		mpc_RIGHTROTATE2(va, 13, t1);
		mpc_XOR2(t0, t1, t0);
		mpc_RIGHTROTATE2(va, 22, t1);
		mpc_XOR2(t0, t1, s0);

		//maj = (a & (b ^ c)) ^ (b & c);
		//(a & b) ^ (a & c) ^ (b & c)

		if(mpc_MAJ_verify(va, vb, vc, maj, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}

		//temp2 = s0+maj;
		if(mpc_ADD_verify(s0, maj, temp2, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}



		memcpy(vh, vg, sizeof(uint32_t) * 2);
		memcpy(vg, vf, sizeof(uint32_t) * 2);
		memcpy(vf, ve, sizeof(uint32_t) * 2);
		//e = d+temp1;
		if(mpc_ADD_verify(vd, temp1, ve, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}

		memcpy(vd, vc, sizeof(uint32_t) * 2);
		memcpy(vc, vb, sizeof(uint32_t) * 2);
		memcpy(vb, va, sizeof(uint32_t) * 2);
		//a = temp1+temp2;

		if(mpc_ADD_verify(temp1, temp2, va, z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d, iteration %d", __LINE__, i);
#endif
			return 1;
		}
	}

	uint32_t hHa[8][3] = { { hA[0],hA[0],hA[0]  }, { hA[1],hA[1],hA[1] }, { hA[2],hA[2],hA[2] }, { hA[3],hA[3],hA[3] },
			{ hA[4],hA[4],hA[4] }, { hA[5],hA[5],hA[5] }, { hA[6],hA[6],hA[6] }, { hA[7],hA[7],hA[7] } };
	if(mpc_ADD_verify(hHa[0], va, hHa[0], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	if(mpc_ADD_verify(hHa[1], vb, hHa[1], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	if(mpc_ADD_verify(hHa[2], vc, hHa[2], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	if(mpc_ADD_verify(hHa[3], vd, hHa[3], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	if(mpc_ADD_verify(hHa[4], ve, hHa[4], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	if(mpc_ADD_verify(hHa[5], vf, hHa[5], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	if(mpc_ADD_verify(hHa[6], vg, hHa[6], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}
	if(mpc_ADD_verify(hHa[7], vh, hHa[7], z.ve, z.ve1, randomness, randCount, countY) == 1) {
#if VERBOSE
		printf("Failing at %d", __LINE__);
#endif
		return 1;
	}

	free(randCount);
	free(countY);

	return 0;
}



#endif /* SHARED_H_ */
