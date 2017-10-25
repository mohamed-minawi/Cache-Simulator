#include <iostream>
#include <iomanip>
#include <vector>
#include <stdlib.h>
#include <fstream>

using namespace std;
ofstream myfile("SA 16way 64kb cache-32b line.txt");
#define cout myfile

struct cashemem
{
	int validbit;
	int access;			// LFU
	int pos;			// FIFO
	int RScounter;		// LRU
	int tag;
};

#define		BlOCKSIZE		32
#define		DBG				1
#define		DRAM_SIZE		(64*1024*1024)
#define		CACHE_SIZE		(64*1024)
#define		LINENUM  CACHE_SIZE / BlOCKSIZE
#define		ITERATIONS		1000000
#define		NWAY			16

cashemem cachearray[LINENUM];

enum cacheResType { MISS = 0, HIT = 1 };
enum replacementpolicy {Random=0, LFU=1, LRU=2, FIFO=3};
int FAcounter = 0;
vector<int> order;

unsigned int m_w = 0xABABAB55;    /* must not be zero, nor 0x464fffff */
unsigned int m_z = 0x05080902;    /* must not be zero, nor 0x9068ffff */

unsigned int rand_()
{
	m_z = 36969 * (m_z & 65535) + (m_z >> 16);
	m_w = 18000 * (m_w & 65535) + (m_w >> 16);
	return (m_z << 16) + m_w;  /* 32-bit result */
}
unsigned int memGen1()
{
	static unsigned int addr = 0;
	return (addr++) % (DRAM_SIZE);
}
unsigned int memGen2()
{
	static unsigned int addr = 0;
	return  rand_() % (128 * 1024);
}
unsigned int memGen3()
{
	return rand_() % (DRAM_SIZE);
}
unsigned int memGen4()
{
	static unsigned int addr = 0;
	return (addr++) % (1024);
}
unsigned int memGen5()
{
	static unsigned int addr = 0;
	return (addr++) % (1024 * 64);
}
unsigned int memGen6()
{
	static unsigned int addr = 0;
	return (addr += 256) % (DRAM_SIZE);
}

void resetcache()
{
	for (int i = 0; i<LINENUM; i++)
	{
		cachearray[i].access = 0;
		cachearray[i].validbit = 0;
		cachearray[i].pos = -1;							// Resets cache 
		cachearray[i].tag = -1;
		cachearray[i].RScounter = -1;
	}
}
void erasefromqueue(int pos)
{
	for(int i=0;i<order.size();i++)
		{
		if (pos == order[i])
			order.erase(order.begin() + i); break;
		}
}

void RandomRP(int tag)
{
	int line;
	line = rand() % LINENUM;

	erasefromqueue(cachearray[line].pos);
	order.push_back(FAcounter);
																// Random Replacement Policy
	cachearray[line].tag = tag;
	cachearray[line].pos = FAcounter;
	cachearray[line].RScounter = FAcounter;
	cachearray[line].access = 1;
}
void LFURP(int tag)
{
	int value = 10000000000;
	int position;
	for (int i = 0; i < LINENUM; i++)
		if (value > cachearray[i].access)
			value= cachearray[i].access , position = i;

	erasefromqueue(cachearray[position].pos);					// Least Frequently Used Replacement Policy
	order.push_back(FAcounter);

	cachearray[position].tag = tag;
	cachearray[position].pos = FAcounter;
	cachearray[position].RScounter = FAcounter;
	cachearray[position].access = 1;
}
void LRURP(int tag)
{
	int value = 10000000000;
	int position;
	for (int i = 0; i < LINENUM; i++)
		if (value > cachearray[i].RScounter)
			value = cachearray[i].RScounter, position = i;			
															// Least Recenetly Used Replacement Policy
	erasefromqueue(cachearray[position].pos);
	order.push_back(FAcounter);

	cachearray[position].tag = tag;
	cachearray[position].pos = FAcounter;
	cachearray[position].RScounter = FAcounter;
	cachearray[position].access = 1;
}
void FIFORP(int tag)
{
	int popposition = order[0], position;
	order.erase(order.begin());

	for (int i = 0; i < LINENUM; i++)
		if (popposition == cachearray[i].pos)
			position = i;

	order.push_back(FAcounter);								// First in First out Replacement Policy

	cachearray[position].tag = tag;
	cachearray[position].pos = FAcounter;
	cachearray[position].RScounter = FAcounter;
	cachearray[position].access = 1;
}

cacheResType cacheSimDM(unsigned int addr)
{
	int offsetbits = log2(BlOCKSIZE);
	int indexbits = log2(LINENUM);
	int tag = (addr >> (offsetbits + indexbits));
	int index= (addr >> offsetbits) & (int(pow(2, indexbits)) - 1);
		
	if (cachearray[index].validbit == 1 && cachearray[index].tag == tag)
	{																// Direct Mapped Cache Simulation
		return HIT;
	}
	else
	{
		cachearray[index].validbit = 1;
		cachearray[index].tag = tag;
		return MISS;
	}

}
void printDM(int memgen)
{
	int inst = 0;
	cacheResType r;
	double hit = 0, miss = 0;
	unsigned int addr;

	resetcache();
	for (; inst<ITERATIONS; inst++)
	{
		switch (memgen)
		{
		case 1:		addr = memGen1(); break;
		case 2:		addr = memGen2(); break;
		case 3:		addr = memGen3(); break;
		case 4: 	addr = memGen4(); break;
		case 5:		addr = memGen5(); break;
		case 6:		addr = memGen6(); break;
		}
															// Direct Mapped Cache printing
		r = cacheSimDM(addr);
		if (r == MISS)
			miss++;
		else
			hit++;
	}

	cout << "HIT ratio = " << (hit / ITERATIONS) * 100 << endl;
	cout << "Miss ratio = " << (miss / ITERATIONS) * 100 << endl;

}

cacheResType cacheSimFA(unsigned int addr, replacementpolicy RP)
{
	int offsetbits = log2(BlOCKSIZE);
	int tag = addr >> offsetbits;
	
	for (int i = 0; i < LINENUM; i++)
	{
		if (cachearray[i].validbit == 1 && cachearray[i].tag == tag)
		{
			cachearray[i].access++;											// Fully Associative Cache Simulation
			cachearray[i].RScounter = FAcounter;
			return HIT;
		}
	}
	if (FAcounter < LINENUM)
	{
		cachearray[FAcounter].tag = tag;
		cachearray[FAcounter].validbit = 1;
		cachearray[FAcounter].access++;
		cachearray[FAcounter].pos = FAcounter;
		cachearray[FAcounter].RScounter = FAcounter;
		order.push_back(FAcounter);
		FAcounter++;
	}
	else
	{
		switch (RP)
		{
		case Random: RandomRP(tag);
			break;
		case LFU: LFURP(tag);
			break;
		case LRU: LRURP(tag);
			break;
		case FIFO: FIFORP(tag);
			break;
		
		}

	}
	return MISS;
}
void printFA(int memgen, replacementpolicy RP)
{
	int inst = 0;
	cacheResType r;
	double hit = 0, miss = 0;
	unsigned int addr;

	resetcache();
	for (; inst<ITERATIONS; inst++)
	{
		switch (memgen)
		{
		case 1:		addr = memGen1(); break;
		case 2:		addr = memGen2(); break;
		case 3:		addr = memGen3(); break;
		case 4: 	addr = memGen4(); break;
		case 5:		addr = memGen5(); break;							// Fully Associative Cache printing
		case 6:		addr = memGen6(); break;
		}

		r = cacheSimFA(addr, RP);
		if (r == MISS)
			miss++;
		else
			hit++;
	}

	cout << "HIT ratio = " << (hit / ITERATIONS) * 100 << endl;
	cout << "Miss ratio = " << (miss / ITERATIONS) * 100 << endl;

}

cacheResType cacheSimSA(unsigned int addr)
{
	int offsetbits = log2(BlOCKSIZE);
	int setbits = log2(LINENUM/NWAY);
	int tag = (addr >> (offsetbits + setbits));
	int set = (addr >> offsetbits) & (int(pow(2, setbits)) - 1);

	int setposition = NWAY*(set-1);

	for (int i = 0; i < NWAY; i++)									// Set Associative Cache Simulation
	{																
		if (cachearray[setposition+i].validbit == 1 && cachearray[setposition+i].tag == tag)
		{
			return HIT;

		}
	}

	int line = rand()% NWAY + setposition;
	cachearray[line].tag = tag;
	cachearray[line].validbit = 1;
	return MISS;
}
void printSA(int memgen)
{
	int inst = 0;
	cacheResType r;
	double hit = 0, miss = 0;
	unsigned int addr;

	resetcache();
	for (; inst<ITERATIONS; inst++)
	{
		switch (memgen)
		{
		case 1:		addr = memGen1(); break;
		case 2:		addr = memGen2(); break;
		case 3:		addr = memGen3(); break;					// Fully Associative Cache printing
		case 4: 	addr = memGen4(); break;
		case 5:		addr = memGen5(); break;
		case 6:		addr = memGen6(); break;
		}

		r = cacheSimSA(addr);
		if (r == MISS)
			miss++;
		else
			hit++;
	}

	cout << "HIT ratio = " << (hit / ITERATIONS) * 100 << endl;
	cout << "Miss ratio = " << (miss / ITERATIONS) * 100 << endl;

}

int main()
{
	cout << "Set Associative Cache 16-way: \n";

	cout << endl << "Memory Generator 1: \n";
	printSA(1);
	FAcounter = 0;
	order.clear();
	resetcache();

	cout << endl << "Memory Generator 2: \n";	
	printSA(2);
	FAcounter = 0;
	order.clear();
	resetcache();

	cout << endl << "Memory Generator 3: \n";
	printSA(3);
	FAcounter = 0;
	order.clear();
	resetcache();
															// User Chooses the functions for the required cache scheme
	cout << endl << "Memory Generator 4: \n";
	printSA(4);
	FAcounter = 0;
	order.clear();
	resetcache();

	cout << endl << "Memory Generator 5: \n";
	printSA(5);
	FAcounter = 0;
	order.clear();
	resetcache();

	cout << endl << "Memory Generator 6: \n";
	printSA(6);
	FAcounter = 0;
	order.clear();
	resetcache();

	cout << endl;
	
	system("pause");
}