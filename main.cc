/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include "cache.h"

using namespace std;

int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

	// print configuration details

	printf("===== 506 Personal information =====\n");  
	printf("Name: Daksh Kandpal\n");
	printf("Unity ID: Dkandpa\n");
	printf("ECE492 Students? NO\n");
	printf("===== 506 SMP Simulator configuration =====\n");
	printf("L1_SIZE:		%d\n", cache_size);
	printf("L1_ASSOC:		%d\n", cache_assoc);
	printf("L1_BLOCKSIZE:		%d\n", blk_size);
	printf("NUMBER OF PROCESSORS:	%d\n", num_processors);
	if (protocol == 0) printf("COHERENCE PROTOCOL:	MSI\n");
	else if (protocol == 1) printf("COHERENCE PROTOCOL:	MESI\n");
	else if (protocol == 2) printf("COHERENCE PROTOCOL:	Dragon\n");
	else {printf("Wrong COHERENCE PROTOCOL!\n");exit(0);}
	printf("TRACE FILE: 	trace/%s\n", fname);

    //create an array of caches here
	Cache **cache = (Cache **)malloc(num_processors);
	for (int i = 0; i < num_processors; i++)
	{
		cache[i] = new Cache(cache_size, cache_assoc, blk_size);
	}
	//*********************************************//	

	pFile = fopen (fname,"r");
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//
	int current_processor_number;
	char operation;
	int address;

	while (!feof(pFile))
	{
		fscanf(pFile, "%d", &current_processor_number);
		fscanf(pFile, " ");
		operation = fgetc(pFile);
		fscanf(pFile, "%x", &address);
		fscanf(pFile, "\n");
		
		//does copy exist for MESI and Dragon protocols
		cache[current_processor_number]->another_copy = 0;
		for (int i = 0; i < num_processors; i++)
			if ((i != current_processor_number)&&(cache[i]->findLine(address)))//if other caches can find same blk in the same addr.
			{
				cache[current_processor_number]->another_copy = 1;
			}

		//Access Current Cache
		if (protocol==0) cache[current_processor_number]->MSI_Access(current_processor_number, num_processors, address,operation, protocol, cache);
		if (protocol==1) cache[current_processor_number]->MESI_Access(current_processor_number, num_processors, address,operation, protocol, cache);
		if (protocol==2) cache[current_processor_number]->Dragon_Access(current_processor_number, num_processors, address,operation, protocol, cache);
	}
	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//
	for (int i = 0; i < num_processors; i++)
	{
		cache[i]->printStats(i);
	}
}
