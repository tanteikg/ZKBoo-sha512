/*
 ============================================================================
 Name        : MPC_SHA256_VERIFIER.c
 Author      : Sobuno
 Version     : 0.1
 Description : Verifies a proof for SHA-256 generated by MPC_SHA256.c
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


int isOnline(int es[NUM_ROUNDS], int round)
{
	return es[round];
}	

int main(int argc, char * argv[]) {
	setbuf(stdout, NULL);
	init_EVP();
	openmp_thread_setup();
	
	z kkwProof;
	FILE *file;
	static int once = 0;

	if (argc !=2)
	{
		printf("Usage: %s <proof file name>\n",argv[0]);
		return -1;
	}

	file = fopen(argv[1], "rb");
	if (!file) {
		printf("Unable to open file %s!\n",argv[1]);
	}
	fread(&kkwProof, sizeof(z), 1, file);
	fclose(file);

	int es[NUM_ROUNDS];
	memset(es,0,NUM_ROUNDS*sizeof(int));
	H3(kkwProof.H, NUM_ONLINE, es);

	unsigned char keys[NUM_ROUNDS][NUM_PARTIES][16];
	unsigned char rsseed[20];
	unsigned char rs[NUM_ROUNDS][NUM_PARTIES][4];
	unsigned char shares[NUM_ROUNDS][NUM_PARTIES][SHA256_INPUTS];
	unsigned char randomness[NUM_ROUNDS][NUM_PARTIES][rSize];
	memset(randomness,0,NUM_ROUNDS*NUM_PARTIES*rSize);

	memset(shares,0,NUM_ROUNDS*NUM_PARTIES*SHA256_INPUTS);
	memcpy(&rsseed[4],kkwProof.rsseed,16);
	int roundctr = 0;
	int partyctr = 0;
	int onlinectr = 0;
	for (int j = 0; j < NUM_ROUNDS; j++)
	{
		memcpy((unsigned char *)rsseed,&j,sizeof(int));
		Compute_RAND((unsigned char *)rs[j],NUM_PARTIES*4,rsseed,20);
		if (!isOnline(es,j))
		{
			Compute_RAND((unsigned char *)keys[j], NUM_PARTIES*16,kkwProof.masterkeys[roundctr++],16);

			for (int k = 0; k < NUM_PARTIES; k++)
			{
				Compute_RAND((unsigned char *)&(shares[j][k]),SHA256_INPUTS,(unsigned char *)keys[j][k],16);
				getAllRandomness(keys[j][k], randomness[j][k]);
			}
			computeAuxTape(randomness[j],shares[j]);
		}
		else
		{
			partyctr = 0;
			for (int k = 0; k < NUM_PARTIES;k++)
			{
				if ((k+1) != es[j])
				{
					memcpy((unsigned char *)keys[j][k],kkwProof.keys[onlinectr][partyctr++],16);
					Compute_RAND((unsigned char *)&(shares[j][k]),SHA256_INPUTS,(unsigned char *)keys[j][k],16);
					getAllRandomness(keys[j][k], randomness[j][k]);
				}
				else
				{
					// ?? online how
					memset(randomness[j][k],0,rSize);
				}

			}
			memcpy(randomness[j][NUM_PARTIES-1],kkwProof.auxBits[onlinectr],rSize);
			onlinectr++;
		}
	}
	SHA256_CTX ctx,hctx,H1ctx,H2ctx;
	unsigned char H1hash[SHA256_DIGEST_LENGTH];
	unsigned char H2hash[SHA256_DIGEST_LENGTH];
	unsigned char temphash1[SHA256_DIGEST_LENGTH];
	unsigned char temphash2[SHA256_DIGEST_LENGTH];
	unsigned char masked_result[SHA256_DIGEST_LENGTH];
	unsigned char party_result[NUM_PARTIES][SHA256_DIGEST_LENGTH];
	View localViews[NUM_ONLINE][NUM_PARTIES];
	memset(localViews,0,NUM_ONLINE*NUM_PARTIES*sizeof(View));

	roundctr = 0;

	SHA256_Init(&H1ctx);
	for (int k = 0; k<NUM_ROUNDS;k++)
	{
		if (!isOnline(es,k))
		{
			SHA256_Init(&hctx);
			for (int j = 0; j < NUM_PARTIES; j++)
			{
				SHA256_Init(&ctx);
				SHA256_Update(&ctx, keys[k][j], 16);
				if (j == (NUM_PARTIES-1))
				{
/*
					size_t pos = 0;
					memset(auxBits,0,rSize/8+1);
                               	 // need to include aux tape
    					for (int i = 1; i < rSize; i+=2)
       					{
						uint8_t auxBit = getBit(randomness[k][j],i);
						setBit(auxBits,pos,auxBit);
						pos++;
					}
*/
             				SHA256_Update(&ctx, randomness[k][NUM_PARTIES-1], rSize);
				}
				SHA256_Update(&ctx, rs[k][j], 4);
				SHA256_Final(temphash1, &ctx);
				SHA256_Update(&hctx,temphash1,SHA256_DIGEST_LENGTH);
  			}
			SHA256_Final(temphash2,&hctx);
			SHA256_Update(&H1ctx, temphash2, SHA256_DIGEST_LENGTH);
		}
		else
		{
			SHA256_Init(&hctx);
			for (int j = 0; j < NUM_PARTIES; j++)
			{
				if ((j+1) != es[k])
				{
					SHA256_Init(&ctx);
					SHA256_Update(&ctx, keys[k][j], 16);
					if (j == (NUM_PARTIES-1))
					{
             					SHA256_Update(&ctx, kkwProof.auxBits[roundctr], rSize);
					}
					SHA256_Update(&ctx, rs[k][j], 4);
					SHA256_Final(temphash1, &ctx);
				}
				else
				{
					memcpy(temphash1,kkwProof.com[roundctr],SHA256_DIGEST_LENGTH);
				}
				SHA256_Update(&hctx,temphash1,SHA256_DIGEST_LENGTH);
			}
			SHA256_Final(temphash2,&hctx);
			SHA256_Update(&H1ctx, temphash2, SHA256_DIGEST_LENGTH);
			roundctr++;
		}
	}
	SHA256_Final(H1hash,&H1ctx);

	SHA256_Init(&H2ctx);
	roundctr = 0;
	onlinectr = 0;
	for (int k=0; k < NUM_ROUNDS; k++)
	{
		int countY = 0;
		if (!isOnline(es,k))
		{
			SHA256_Update(&H2ctx,kkwProof.H2[roundctr++],SHA256_DIGEST_LENGTH);
		}
		else
		{
			SHA256_Init(&hctx);
			SHA256_Update(&hctx,kkwProof.maskedInput[onlinectr],SHA256_INPUTS);
			memcpy(&localViews[onlinectr][es[k]-1],&kkwProof.views[onlinectr],sizeof(View));
			mpc_sha256(masked_result,kkwProof.maskedInput[onlinectr],shares[k],NULL,es[k]-1,randomness[k],localViews[onlinectr],party_result,&countY);
			if (0)
			{
				printf("mpc round %d verification of hash: ",k);
				for (int j=0;j<SHA256_DIGEST_LENGTH;j++)
				{
					unsigned char temp = masked_result[j];
					for (int i=0;i<NUM_PARTIES;i++)
					{
						temp ^= party_result[i][j];
					}
					printf("%02X",temp);
				}
				printf("\n");
		//		once = 1;
			}
			SHA256_Update(&hctx,masked_result,SHA256_DIGEST_LENGTH);
			for (int j = 0; j < 32; j++)
				SHA256_Update(&hctx, localViews[onlinectr][j].y,ySize*4);
			SHA256_Update(&hctx,rs[k],NUM_PARTIES*4);
			SHA256_Final(temphash1,&hctx);
			SHA256_Update(&H2ctx,temphash1,SHA256_DIGEST_LENGTH);

			onlinectr++;
		}
	}
	SHA256_Final(H2hash,&H2ctx);

	SHA256_Init(&hctx);
	SHA256_Update(&hctx,H1hash,SHA256_DIGEST_LENGTH);
	SHA256_Update(&hctx,H2hash,SHA256_DIGEST_LENGTH);
	SHA256_Final(temphash1,&hctx);

	if (memcmp(temphash1,kkwProof.H,SHA256_DIGEST_LENGTH))
	{
		printf("Error: Hash does not match\n");
		return -1;
	}		
	else
	{
		printf("Received pre-image proof for hash : ");
		for (int j = 0; j<SHA256_DIGEST_LENGTH;j++)
		{
			unsigned char temp = masked_result[j];
			for (int i=0;i<NUM_PARTIES;i++)
			{
				temp ^= party_result[i][j];
			}
			printf("%02X",temp);
		}
		printf("\n");		
	}
	
	
	openmp_thread_cleanup();
	cleanup_EVP();
	return EXIT_SUCCESS;
}
