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

typedef unsigned int ADDRESS;
typedef unsigned int TIME;
typedef unsigned int COUNT;


//global varibles
COUNT threadNum = 0; //number of running threads
COUNT threadExisted = 0; //used to mark how many threads existed once.
bool logging_start = false; //start logging the memory accessing when at least two threads exist.

/* A global timer to mark the ordering of the operations. 
*Use 0 to indicate a default value, which is meaningless.*/
TIME timestamp = 1; 



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
struct memAccess{
	char op; // R or W
	ADDRESS inst; //instruction
	ADDRESS addr; //address to read or write
	COUNT tid; //thread id
	TIME time; //time
	string rtn; //name of the routine
};
typedef vector<memAccess> ma_vector; //memory access table

ma_vector maTable[MAX_THREAD_NUM]; // A two dimension table for at most MAX_THREAD_NUM threads.



struct criticalSection{
  COUNT tid; // the thread that holds this critical section
  ADDRESS lockAddr; //entry address of the lock
  TIME st; //start time
  TIME ft; //finish time
};

vector<criticalSection> csTable; //critical section table



struct synch{
  string type; //"condwait","condtimewait","sleep","barrier"
  COUNT tid; //the thread used this synchronization.
  TIME time; //timestamp
};

vector<synch> synchTable; //records all the synchronizations used.

//map<UINT64,records_vector> prevReads; 




// Below are helper functions used

void print_ma(memAccess ma, string func){

	printf("[%s] Tid %d, %c, addr: 0x%x, inst: 0x%x, rtn: %s, time: %d \n",
		func.c_str(), ma.tid, ma.op, ma.addr, ma.inst, ma.rtn.c_str(), ma.time);
}
void print_cs(criticalSection cs, string func){

  printf("[%s] Tid %d, lockaddr: 0x%x, stime: %d, ftime:%d \n",
    func.c_str(), cs.tid, cs.lockAddr, cs.st, cs.ft);
}
void print_synch(synch sy, string func){

  printf("[%s] Tid %d, type: %s, time: %d\n",
    func.c_str(), sy.tid, sy.type.c_str(), sy.time);
}



bool isLibRtnName(string name){
  for(int i=0;i<LIB_RTN_NAME_SIZE;++i){
    if(name==LIB_RTN_NAME[i])
      return true;
  }
  return false;
}
