/*******************************************************
                          cache.cc
********************************************************/
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
  //*******************//
   //initialize your counters here//
   //*******************//

   reads = readMisses  = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
   cache_to_cache_transactions= memory_transactions= interventions = invalidations = flushes = busrdxes = 0;
   busrd = busrdx = 0;
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::MSI_Access(int processor, int num_processors, ulong addr,uchar op, int protocol, Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order among cache ways, updated on every cache access*/
	busrd = busrdx = busupgr = busupd = 0;
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(!(line))										//miss
	{
		memory_transactions++;
		
		cacheLine *newline = fillLine(addr);

		//Change state and bus signal
		if (op == 'w')
		{
			writeMisses++;
			newline->setFlags(M);						
			if (protocol == 0){
				busrdx = 1;busrdxes++;
			}
		}
		
		if (op == 'r') 
		{
			readMisses++;
			if (protocol == 0) busrd = 1;
		}
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);

		//busRdx and memory transaction for shared to modified 
		if ((line->getFlags() == S) && (op == 'w'))   
		{
			busrdx = 1;
			busrdxes+=1;
			memory_transactions+=1;
			line->setFlags(M);
		}

	}

	//Effect of bus commands on other caches
	for (int i = 0; i < num_processors; i++)
	{
		if (i != processor)
		{
			if(busrd) cache[i]->MSI_Bus_Read(addr);
			if(busrdx) cache[i]->Bus_ReadX(addr);
		}
	}

}
void Cache::MESI_Access(int processor, int num_processors, ulong addr,uchar op, int protocol , Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order among cache ways, updated on every cache access*/
	busrd = busrdx = busupgr = busupd = 0;
        	
	if(op == 'w') 
		writes++;
	else          
		reads++;
	
	cacheLine * line = findLine(addr);
	if(!(line))									//miss
	{
		if(another_copy) 
			cache_to_cache_transactions+=1;
		else 									
			memory_transactions+=1;
		
		cacheLine *newline = fillLine(addr);
		
		//Change state and issue bus signal
		if (op == 'w')
		{
			writeMisses++;
			newline->setFlags(M);			
			busrdx = 1;
			busrdxes+=1;
		}
		
		if (op == 'r') 
		{
			readMisses ++;
			busrd = 1;
			if (another_copy ) 
				newline->setFlags(S);
			else 
				newline->setFlags(E);
		}
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if ((line->getFlags() == S || line->getFlags()==E) && (op == 'w')){
			if(line->getFlags()==S){
				busupgr = 1;							//issue bus upgrade
				line->setFlags(M) ;						//set to modified
			}
			else{
				line->setFlags(M) ;						//just set to modified			
			}
		}
	}

	//Effect of bus signals on other caches
	for (int i = 0; i < num_processors; i++)
	{
		if (i != processor){
			if (busrd) cache[i]->MESI_Bus_Read(addr);
			if(busrdx) cache[i]->Bus_ReadX(addr);
			if(busupgr) cache[i]->Bus_Upgrade(addr);
		}
	}
}
void Cache::Dragon_Access(int processor, int num_processors, ulong addr,uchar op, int protocol, Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
	busrd = busrdx = busupgr = busupd = 0;
        	
	if(op == 'w') 
		writes++;
	else          
		reads++;
	
	cacheLine * line = findLine(addr);
	if(!(line))									//miss
	{
		memory_transactions++;
		cacheLine *newline = fillLine(addr);
		
		//Change state & bus signals
		if (op == 'w')
		{
			writeMisses++;
			newline->setFlags(M);					
			busrd = 1;
			if (another_copy )
			{
				newline->setFlags(SM);			//set to shared modified if another copy exists
				busupd = 1;						//issue bus upgrade
			}
			else
			{
				newline->setFlags(M);			//set to modified
			}
		}
		
		if (op == 'r') 
		{	
			readMisses++;
			if (another_copy )
			{
				newline->setFlags(SC);		//
				busrd = 1;
			}
			else
			{
				newline->setFlags(E);
				busrd = 1;
			}
		}
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
			if (op == 'w')
			{
				if (line->getFlags() == E)
				{
					line->setFlags(M);//MODIFIED
				}
				if (line->getFlags() == SC)
				{
					if (another_copy )
					{
						line->setFlags(SM);
						busupd = 1;
					}
					else
					{
						line->setFlags(M);
						busupd = 1;
					}
				}
				if (line->getFlags() == SM)
				{
					if (another_copy )
						busupd = 1;
					else
					{
						line->setFlags(M);
						busupd = 1;
					}
				}
			}
	}

	//Effect of bus signals on other caches
	for (int i = 0; i < num_processors; i++)
	{
		if (i != processor)
		{
		if(busrd) cache[i]->Dragon_Bus_Read(addr);
		if(busupd)cache[i]->Bus_Update(addr);
		}
	}

}

//Bus functions

void Cache::MSI_Bus_Read(ulong addr)
{
		cacheLine *line = findLine(addr);
		if (line)
		{               
			if (line->getFlags() == M)
			{
				writeBacks++;
				memory_transactions++;
				flushes++;
				line->setFlags(S);
				interventions++;
			}
		}
}
void Cache::MESI_Bus_Read(ulong addr)
{
		cacheLine *line = findLine(addr);
		if (line)
		{               
			if (line->getFlags() == M)
			{
				writeBacks++;
				memory_transactions++;
				flushes++;
				line->setFlags(S);
				interventions++;
				return;
			}
			if (line->getFlags() == E)
			{
				line->setFlags(S);
				interventions++;
			}
		}
}
void Cache::Dragon_Bus_Read(ulong addr)
{
		cacheLine *line = findLine(addr);
		if (line)
		{               
			if (line->getFlags() == M)
			{
				line->setFlags(SM);
				interventions++;
			}
			if (line->getFlags() == E)
			{
				line->setFlags(SC);
				interventions++;
			}

			if (line->getFlags() == SM)
			{
				flushes++;
			}
		}
}
void Cache::Bus_ReadX(ulong addr)
{
//if(busrdx){
	cacheLine * line = findLine(addr);
	if (line)
	{
		if (line->getFlags() == M)
		{
			writeBacks++;
			memory_transactions++;
			flushes++;
		}
		line->invalidate();
		invalidations++;
	}
//}
}
void Cache::Bus_Upgrade(ulong addr)
{
//if(busupgrade){
	cacheLine *line = findLine(addr);
	if (line)
	{
		if (line->getFlags() == S)
		{
			line->invalidate();
			invalidations++;
		}
	}
//}
}

void Cache::Bus_Update(ulong addr)
{
//if(busupdate){
	cacheLine *line = findLine(addr);
	if (line)
	{
		if (line->getFlags() == SM)
			line->setFlags(SC);
	}
//}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if ((victim->getFlags() == M) || (victim->getFlags() == SM))
   {
	   writeBack(addr);
   }
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(S);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(int i)
{ 
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
	printf("============ Simulation results (Cache %d) ============\n", i);
	printf("01. number of reads: 				%lu\n", getReads());
	printf("02. number of read misses: 			%lu\n", getRM());
	printf("03. number of writes: 				%lu\n", getWrites());
	printf("04. number of write misses:			%lu\n", getWM());
	printf("05. total miss rate: 				%.2f%%\n", (double)(getRM() + getWM()) * 100 / (getReads() + getWrites()));
	printf("06. number of writebacks: 			%lu\n", getWB());
	printf("07. number of cache-to-cache transfers: 	%lu\n", getC2C());
	printf("08. number of memory transactions: 		%lu\n", memory_transactions);
	printf("09. number of interventions: 			%lu\n", getInterventions());
	printf("10. number of invalidations: 			%lu\n", getInvalidations());
	printf("11. number of flushes: 				%lu\n", getFlushes());
	printf("12. number of BusRdX: 				%lu\n", getBusRdX());
}



