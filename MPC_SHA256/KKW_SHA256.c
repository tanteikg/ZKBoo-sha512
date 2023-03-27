/*
 ============================================================================
 Name        : MPC_SHA256.c
 Author      : Sobuno
 Version     : 0.1
 Description : MPC SHA256 for one block only
 ============================================================================
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "KKW_shared.h"
#include "omp.h"


#define CH(e,f,g) ((e & f) ^ ((~e) & g))


int totalRandom = 0;
int totalSha = 0;
int totalSS = 0;
int totalHash = 0;

void Compute_RAND(unsigned char * output, int size, unsigned char * seed, int seedLen)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	char * tempptr = output;

	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, &seedLen, sizeof(int));
	SHA256_Update(&ctx, seed, seedLen);
	SHA256_Update(&ctx, &size, sizeof(int));
        SHA256_Final(hash, &ctx);
	while (size > 0)
	{
		SHA256_Init(&ctx);
		SHA256_Update(&ctx, &seedLen, sizeof(int));
		SHA256_Update(&ctx, seed, seedLen);
		SHA256_Update(&ctx, hash, sizeof(hash));
        	SHA256_Final(hash, &ctx);
		if (size >= SHA256_DIGEST_LENGTH)
		{
			memcpy(tempptr,hash,SHA256_DIGEST_LENGTH);
			tempptr += SHA256_DIGEST_LENGTH;
		}
		else
			memcpy(tempptr,hash,size);
		size -= SHA256_DIGEST_LENGTH;
	}
}



uint32_t rand32() {
	uint32_t x;
	x = rand() & 0xff;
	x |= (rand() & 0xff) << 8;
	x |= (rand() & 0xff) << 16;
	x |= (rand() & 0xff) << 24;

	return x;
}

void printbits(uint32_t n) {
	if (n) {
		printbits(n >> 1);
		printf("%d", n & 1);
	}

}

/*

void mpc_XOR(uint32_t x[3], uint32_t y[3], uint32_t z[3]) {
	z[0] = x[0] ^ y[0];
	z[1] = x[1] ^ y[1];
	z[2] = x[2] ^ y[2];
}

*/

void mpc_AND(uint32_t x[NUM_PARTIES], uint32_t y[NUM_PARTIES], uint32_t z[NUM_PARTIES], unsigned char randomness[NUM_PARTIES][rSize], int* randCount, View views[NUM_PARTIES], int* countY) 
{
	uint32_t mask_a, mask_b, mask_z;

	for (int i = 0; i < NUM_PARTIES;i++)
		z[i] = getRandom32(randomness[i],*randCount;
	*randCount+=32;

	for (int i=0;i < 32;i++)
	{
		uint32_t and_helper = tapesToWord(randomness,randCount);
		mask_a = int32ToWord(x,i);
		mask_b = int32ToWord(y,i);
		mask_z = int32ToWord(z,i);

	}

	views[0].y[*countY] = z[0];
	views[1].y[*countY] = z[1];
	views[2].y[*countY] = z[2];
	(*countY)++;
}

void mpc_ADD(uint32_t x[NUM_PARTIES], uint32_t y[NUM_PARTIES], uint32_t z[NUM_PARTIES], unsigned char randomness[NUM_PARTIES][rSize], int* randCount, View views[NUM_PARTIES], int* countY) {
	uint32_t c[NUM_PARTIES] = { 0 };
	uint32_t r[NUM_PARTIES] = { getRandom32(randomness[0], *randCount), getRandom32(randomness[1], *randCount), getRandom32(randomness[2], *randCount)};
	*randCount += 4;

	uint8_t a[NUM_PARTIES], b[NUM_PARTIES];

	uint8_t t;

	for(int i=0;i<31;i++)
	{
		a[0]=GETBIT(x[0]^c[0],i);
		a[1]=GETBIT(x[1]^c[1],i);
		a[2]=GETBIT(x[2]^c[2],i);

		b[0]=GETBIT(y[0]^c[0],i);
		b[1]=GETBIT(y[1]^c[1],i);
		b[2]=GETBIT(y[2]^c[2],i);

		t = (a[0]&b[1]) ^ (a[1]&b[0]) ^ GETBIT(r[1],i);
		SETBIT(c[0],i+1, t ^ (a[0]&b[0]) ^ GETBIT(c[0],i) ^ GETBIT(r[0],i));

		t = (a[1]&b[2]) ^ (a[2]&b[1]) ^ GETBIT(r[2],i);
		SETBIT(c[1],i+1, t ^ (a[1]&b[1]) ^ GETBIT(c[1],i) ^ GETBIT(r[1],i));

		t = (a[2]&b[0]) ^ (a[0]&b[2]) ^ GETBIT(r[0],i);
		SETBIT(c[2],i+1, t ^ (a[2]&b[2]) ^ GETBIT(c[2],i) ^ GETBIT(r[2],i));


	}
	z[0]=x[0]^y[0]^c[0];
	z[1]=x[1]^y[1]^c[1];
	z[2]=x[2]^y[2]^c[2];

	views[0].y[*countY] = c[0];
	views[1].y[*countY] = c[1];
	views[2].y[*countY] = c[2];
	*countY += 1;


}



void mpc_MAJ(uint32_t a[NUM_PARTIES], uint32_t b[NUM_PARTIES], uint32_t c[NUM_PARTIES], uint32_t z[NUM_PARTIES], unsigned char randomness[NUM_PARTIES][rSize], int* randCount, View views[NUM_PARTIES], int* countY) {
	uint32_t t0[NUM_PARTIES];
	uint32_t t1[NUM_PARTIES];

	mpc_XOR(a, b, t0);
	mpc_XOR(a, c, t1);
	mpc_AND(t0, t1, z, randomness, randCount, views, countY);
	mpc_XOR(z, a, z);
}


void mpc_CH(uint32_t e[NUM_PARTIES], uint32_t f[NUM_PARTIES], uint32_t g[NUM_PARTIES], uint32_t z[NUM_PARTIES], unsigned char randomness[NUM_PARTIES][rSize], int* randCount, View views[NUM_PARTIES], int* countY) {
	uint32_t t0[NUM_PARTIES];

	//e & (f^g) ^ g
	mpc_XOR(f,g,t0);
	mpc_AND(e,t0,t0, randomness, randCount, views, countY);
	mpc_XOR(t0,g,z);

}



int mpc_sha256(unsigned char results[NUM_PARTIES][SHA256_DIGEST_LENGTH], unsigned char shares[NUM_PARTIES][SHA256_INPUTS], unsigned char * inputs, int numBytes, unsigned char randomness[NUM_PARTIES][rSize], View views[NUM_PARTIES], int* countY) 
{

	if (numBytes > 55)
	{	
		printf("Input too long, aborting!");
		return -1;
	}

	int randCount=0;;

	uint32_t w[64][NUM_PARTIES];

	for (int i = 0; i < NUM_PARTIES; i++) {
		for (int j = 0; j < numBytes;j++)
			shares[i][j] ^= inputs[j];
		shares[i][numBytes] ^= 0x80;
		//Last 8 chars used for storing length of input without padding, in big-endian.
		//Since we only care for one block, we are safe with just using last 9 bits and 0'ing the rest

		//chunk[60] = numBits >> 24;
		//chunk[61] = numBits >> 16;
		shares[i][62] ^= numBytes >> 5;
		shares[i][63] ^= (numBytes * 8)&0xFF;
		memcpy(views[i].x, shares[i], 64);

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

		mpc_ADD(w[j-16], s0, t1, randomness, &randCount, views, countY);
		mpc_ADD(w[j-7], t1, t1, randomness, &randCount, views, countY);
		mpc_ADD(t1, s1, w[j], randomness, &randCount, views, countY);

	}
	uint32_t a[NUM_PARTIES];
	uint32_t b[NUM_PARTIES];
	uint32_t c[NUM_PARTIES];
	uint32_t d[NUM_PARTIES];
	uint32_t e[NUM_PARTIES];
	uint32_t f[NUM_PARTIES];
	uint32_t g[NUM_PARTIES];
	uint32_t h[NUM_PARTIES];
	for (int i = 0;i < NUM_PARTIES;i++)
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
	uint32_t temp1[NUM_PARTIES], temp2[NUM_PARTIES], maj[NUM_PARTIES];
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

		mpc_ADD(h, s1, t0, randomness, &randCount, views,countY);


		mpc_CH(e, f, g, t1, randomness, &randCount, views, countY);

		//t1 = t0 + t1 (h+s1+ch)
		mpc_ADD(t0, t1, t1, randomness, &randCount, views, countY);

		{
			uint32_t temp3[NUM_PARTIES];
			for (int j = 0; j < NUM_PARTIES;j++)
				temp3[j] = k[i];
			mpc_ADDK(t1,temp3, t1, randomness, &randCount, views, countY);
		}

		mpc_ADD(t1, w[i], temp1, randomness, &randCount, views, countY);

		//s0 = RIGHTROTATE(a,2) ^ RIGHTROTATE(a,13) ^ RIGHTROTATE(a,22);
		mpc_RIGHTROTATE(a, 2, t0);
		mpc_RIGHTROTATE(a, 13, t1);
		mpc_XOR(t0, t1, t0);
		mpc_RIGHTROTATE(a, 22, t1);
		mpc_XOR(t0, t1, s0);


		mpc_MAJ(a, b, c, maj, randomness, &randCount, views, countY);

		//temp2 = s0+maj;
		mpc_ADD(s0, maj, temp2, randomness, &randCount, views, countY);

		memcpy(h, g, sizeof(uint32_t) * 3);
		memcpy(g, f, sizeof(uint32_t) * 3);
		memcpy(f, e, sizeof(uint32_t) * 3);
		//e = d+temp1;
		mpc_ADD(d, temp1, e, randomness, &randCount, views, countY);
		memcpy(d, c, sizeof(uint32_t) * 3);
		memcpy(c, b, sizeof(uint32_t) * 3);
		memcpy(b, a, sizeof(uint32_t) * 3);
		//a = temp1+temp2;

		mpc_ADD(temp1, temp2, a, randomness, &randCount, views, countY);
	}
	uint32_t hHa[8][NUM_PARTIES];
	for (int i = 0;i < 8;i++)
	{
		for (int j = 0;j < NUM_PARTIES;j++)
			hHa[i][j] = hA[i];
	}
	mpc_ADD(hHa[0], a, hHa[0], randomness, &randCount, views, countY);
	mpc_ADD(hHa[1], b, hHa[1], randomness, &randCount, views, countY);
	mpc_ADD(hHa[2], c, hHa[2], randomness, &randCount, views, countY);
	mpc_ADD(hHa[3], d, hHa[3], randomness, &randCount, views, countY);
	mpc_ADD(hHa[4], e, hHa[4], randomness, &randCount, views, countY);
	mpc_ADD(hHa[5], f, hHa[5], randomness, &randCount, views, countY);
	mpc_ADD(hHa[6], g, hHa[6], randomness, &randCount, views, countY);
	mpc_ADD(hHa[7], h, hHa[7], randomness, &randCount, views, countY);

	for (int i = 0; i < 8; i++) {
		mpc_RIGHTSHIFT(hHa[i], 24, t0);
		for (int j = 0;j< NUM_PARTIES;j++)
			results[j][i * 4] = t0[j];
		mpc_RIGHTSHIFT(hHa[i], 16, t0);
		for (int j = 0;j< NUM_PARTIES;j++)
			results[j][i * 4 + 1] = t0[j];
		mpc_RIGHTSHIFT(hHa[i], 8, t0);
		for (int j = 0;j< NUM_PARTIES;j++)
			results[j][i * 4 + 2] = t0[j];

		for (int j = 0;j< NUM_PARTIES;j++)
			results[j][i * 4 + 3] = hHa[i][j];
	}

	return 0;
}


int writeToFile(char filename[], void* data, int size, int numItems) {
	FILE *file;

	file = fopen(filename, "wb");
	if (!file) {
		printf("Unable to open file!");
		return 1;
	}
	fwrite(data, size, numItems, file);
	fclose(file);
	return 0;
}




int secretShare(unsigned char* input, int numBytes, unsigned char output[3][numBytes]) {
	if(RAND_bytes(output[0], numBytes) != 1) {
		printf("RAND_bytes failed crypto, aborting\n");
	}
	if(RAND_bytes(output[1], numBytes) != 1) {
		printf("RAND_bytes failed crypto, aborting\n");
	}
	for (int j = 0; j < numBytes; j++) {
		output[2][j] = input[j] ^ output[0][j] ^ output[1][j];
	}
	return 0;
}



z prove(int e, unsigned char keys[NUM_PARTIES][16], View views[NUM_PARTIES]) {
	z z;
	memcpy(z.ke, keys[e], 16);
	memcpy(z.ke1, keys[(e + 1) % 3], 16);
	z.ve = views[e];
	z.ve1 = views[(e + 1) % 3];
/*
	memcpy(z.re, rs[e],4);
	memcpy(z.re1, rs[(e + 1) % 3],4);
*/

	return z;
}

int main(int argc, char * argv[]) 
{
	printf("hello world\n");
//	setbuf(stdout, NULL);
	srand((unsigned) time(NULL));
	init_EVP();
	openmp_thread_setup();

	printf("Enter the string to be hashed (Max 55 characters): ");
	char userInput[56]; //55 is max length as we only support 447 bits = 55.875 bytes
	memset(userInput,0,sizeof(userInput));
	fgets(userInput, sizeof(userInput)-1, stdin);
	
	int i = strlen(userInput)-1; 
	printf("String length: %d\n", i);
	
	//printf("Iterations of SHA: %d\n", NUM_ROUNDS);

	unsigned char input[SHA256_INPUTS]; // 512 bits
	memset(input,0,sizeof(input));
	for(int j = 0; j<i; j++) {
		input[j] = userInput[j];
	}
	unsigned char rs[NUM_ROUNDS][NUM_PARTIES][4];
	unsigned char keys[NUM_ROUNDS][NUM_PARTIES][16];
	a as[NUM_ROUNDS];

        //Generating keys
	Compute_RAND((unsigned char *)keys, NUM_ROUNDS*NUM_PARTIES*16,input,strlen(userInput));  
        //Sharing secrets
	unsigned char shares[NUM_ROUNDS][NUM_PARTIES][SHA256_INPUTS];
	for (int j=0;j<NUM_ROUNDS;j++)
	{
		Compute_RAND((unsigned char *)&(shares[j]),NUM_PARTIES*SHA256_INPUTS,(unsigned char *)keys[j],NUM_PARTIES*16);
	}

        //Generating randomness
	unsigned char randomness[NUM_ROUNDS][NUM_PARTIES][rSize];

//	#pragma omp parallel for
	for(int k=0; k<NUM_ROUNDS; k++) {
		for(int j = 0; j<NUM_PARTIES; j++) {
			getAllRandomness(keys[k][j], randomness[k][j]);
		}
	}
	//compute AUX Tape
	unsigned char com[NUM_PARTIES][SHA256_DIGEST_LENGTH];
	unsigned char commitedH[SHA256_DIGEST_LENGTH];
	SHA256_CTX ctx;
	for (int k = 0; k<NUM_ROUNDS;k++)
	{
		
		computeAuxTape(randomness[k],shares[k]);
		SHA256_Init(&ctx);
		SHA256_Update(&ctx, randomness[k], rSize);
		SHA256_Update(&ctx, shares[k], 16);
		SHA256_Final(com[k],&ctx);
	}


	//Running MPC-SHA2 online
	unsigned char result[NUM_ROUNDS][NUM_PARTIES][SHA256_DIGEST_LENGTH];
	View localViews[NUM_ROUNDS][NUM_PARTIES];
//	#pragma omp parallel for
	for(int k=0; k<NUM_ROUNDS; k++) {
		int countY = 0;
		mpc_sha256(result[k],shares[k],input, i, randomness[k], localViews[k],&countY);
		for(int i = 0; i<8; i++) 
		{
			for (int j = 0;j < NUM_PARTIES;j++)
			{
				localViews[k][j].y[countY] = (result[k][j][i * 4] << 24) | (result[k][j][i * 4 + 1] << 16)
								| (result[k][j][i * 4 + 2] << 8) | result[k][j][i * 4 + 3];
				countY += 1;
			}
		}
		for (int j = 0;j < NUM_PARTIES;j++)
		{
			memcpy(as[k].yp[j], result[k][j], 32);
		}
	}
/*
	//Committing
//	#pragma omp parallel for
	for(int k=0; k<NUM_ROUNDS; k++) {
		unsigned char hash1[SHA256_DIGEST_LENGTH];
		H(keys[k][0], localViews[k][0], rs[k][0], hash1);
		memcpy(as[k].h[0], &hash1, 32);
		H(keys[k][1], localViews[k][1], rs[k][1], hash1);
		memcpy(as[k].h[1], &hash1, 32);
		H(keys[k][2], localViews[k][2], rs[k][2], hash1);
		memcpy(as[k].h[2], hash1, 32);
	}
				

	//Generating E
	int es[NUM_ROUNDS];
	uint32_t finalHash[8];
	for (int j = 0; j < 8; j++) {
		finalHash[j] = as[0].yp[0][j]^as[0].yp[1][j]^as[0].yp[2][j];
	}
	H3(finalHash, as, NUM_ROUNDS, es);


	//Packing Z
	z* zs = malloc(sizeof(z)*NUM_ROUNDS);

//	#pragma omp parallel for
	for(int i = 0; i<NUM_ROUNDS; i++) {
		zs[i] = prove(es[i],keys[i],localViews[i]);
	}
	
	
	//Writing to file
	FILE *file;

	char outputFile[3*sizeof(int) + 8];
	sprintf(outputFile, "out%i.bin", NUM_ROUNDS);
	file = fopen(outputFile, "wb");
	if (!file) {
		printf("Unable to open file!");
		return 1;
	}
	fwrite(as, sizeof(a), NUM_ROUNDS, file);
	fwrite(zs, sizeof(z), NUM_ROUNDS, file);

	fclose(file);

	free(zs);


	int sumOfParts = 0;

	printf("Proof output to file %s", outputFile);
*/

	openmp_thread_cleanup();
	cleanup_EVP();
	return EXIT_SUCCESS;
}
