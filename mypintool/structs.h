#include "pin.H"
#include <vector>
#include <map>
#include <assert.h>

#define STACK_LOWERBOUND 0x40000000  //base address to load shared libraries in Linux x86
//0xbfffffff 
#define USER_UPPERBOUND  0x15000000
#define USER_LOWERBOUND  0x08048000 //start address for Unix/Linux x86 program 

#define MAX_THREAD_NUM 10

//ofstream outFile;
FILE * trace;
PIN_LOCK lock;


//global varibles
UINT32 threadNum = 0; //number of running threads
bool logging_start = false; //start logging the memory accessing when at least two thread exist.
UINT64 timestamp = 1; //a global timer to mark the ordering of the operations.

#define LIB_RTN_NAME_SIZE 15
string LIB_RTN_NAME[] = {
  "_start",
  ".plt",
  "__libc_csu_init",
  "__i686.get_pc_thunk.bx",
  "_init",
  "frame_dummy",
  "__do_global_ctors_aux",
  "_fini",
  "__do_global_dtors_aux",
  ".text",
  "malloc",
  "calloc",
  "__libc_memalign",
  "bsearch",
  "cfree"
}; //routines from the lib, we skip memory accesses from these routines.

// memory access record
struct record{
	char op; // R or W
	UINT64 inst; //instruction
	UINT64 addr; //address to read or write
	UINT32 tid; //thread id
	UINT64 time; //time
	string rtn; //name of the routine
  char isLocked; // L or U, if this memory access protected by a lock
};
vector<record> records;

typedef vector<record> records_vector;

/* A table records all the previous reads to each memory address
* A records vector maps to each address*/
map<UINT64,records_vector> prevReads;  

/* A table records only the last write to each memory address,
* Here only a single record struct maps to each address.
* Because reads do not change the memory, all the reads could pair with 
* a later write to form a WAR, but writes change the memory, only the last
* write can pair with a later read to RAW. Or pair with a later write to WAW.*/
map<UINT64,record> lastWrite; 
  


struct edge{
	string type; //RAW,WAR,WAW
	record first;
	record second;
};

vector<edge> result; //potential racy edges


struct mlock{ //mutex_lock
  UINT32 tid; // the thread that add this lock
  UINT64 time; // when this lock was added
  UINT64 addr; // entry address of the lock, used to distinguish the locks
};

typedef vector<mlock>  lockset; // a lockset for each thread.

lockset locksets[MAX_THREAD_NUM]; // an array to hold the locksets of all the threads


struct synch{
  string type; //"condwait","condtimewait","sleep","barrier"
  UINT32 tid; //the thread used this synchronization.
  UINT64 time; //timestamp

};

vector<synch> synchs; //records all the synchronizations used.

// Below are helper functions used

void print_record(record rec, string func){

	printf("[%s] Tid %d, %c, addr: 0x%llx, inst: 0x%llx, rtn: %s, time: %llu , isLocked: %c\n",
		func.c_str(), rec.tid, rec.op, rec.addr, rec.inst, rec.rtn.c_str(), rec.time, rec.isLocked);
}

void print_edge(edge e){
	printf("--> %s\n", e.type.c_str());
	print_record(e.first, "-->");
	print_record(e.second, "-->");
}

bool isLibRtnName(string name){
  for(int i=0;i<LIB_RTN_NAME_SIZE;++i){
    if(name==LIB_RTN_NAME[i])
      return true;
  }
  return false;
}
