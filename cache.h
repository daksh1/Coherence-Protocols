/*******************************************************
                          cache.h
********************************************************/


#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum{
	INVALID = 0,
	S,     //Valid or Shared
	M,     //Dirty or Modified
	E,     //Exclusive
	SM,   //Shared Modified
	SC    //Shared CLean
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq;     
 
public:
   cacheLine()            { tag = 0; Flags = 0; }
   ulong getTag()         { return tag; }
   ulong getFlags()			{ return Flags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(ulong flags)			{  Flags = flags;}
   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }//useful function
   bool isValid()         { return ((Flags) != INVALID); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;

   //Coherence counters
   ulong interventions, invalidations;
   ulong memory_transactions; 
   ulong cache_to_cache_transactions; 
   ulong flushes;
   ulong busrdxes;
   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  

   //Bus Signals
   int busrd, busrdx, busupgr, busupd;
   bool another_copy;
  
    Cache(int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM(){return readMisses;} 
   ulong getWM(){return writeMisses;} 
   ulong getReads(){return reads;}
   ulong getWrites(){return writes;}
   ulong getWB(){return writeBacks;}
   ulong getC2C(){ return cache_to_cache_transactions; }
   ulong getInterventions(){ return interventions; }
   ulong getInvalidations(){ return invalidations; }
   ulong getFlushes(){ return flushes; }
   ulong getBusRdX(){ return busrdxes; }


   void writeBack(ulong) {  writeBacks++; memory_transactions++;}
   void MSI_Access(int, int, ulong,uchar, int, Cache **);
   void MESI_Access(int, int, ulong,uchar, int, Cache **);
   void Dragon_Access(int, int, ulong,uchar, int, Cache **);
   void printStats(int i);
   void updateLRU(cacheLine *);

   //functions to handle bus transactions
   void MSI_Bus_Read(ulong);
   void MESI_Bus_Read(ulong);
   void Dragon_Bus_Read(ulong);
   void Bus_ReadX(ulong);
   void Bus_Upgrade(ulong);
   void Bus_Update(ulong);
   
};

#endif
