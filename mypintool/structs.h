#include "pin.H"
#include <vector>
#include <map>

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
UINT64 timestamp = 0; // a global timer to mark the ordering of the operations.

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
};
vector<record> records;//starts from vector[0]
UINT64 recordNum; //records number

map<UINT64,record> lastRead;  // a table records the last read to each memory address
map<UINT64,record> lastWrite; // a table records the last write to each memory address


struct edge{
	string type; //RAW,WAR,WAW
	record first;
	record second;
};

vector<edge> result; //potential racy edges


struct lock{
  UINT32 tid; // the thread that add this lock
  UINT64 time; // when this lock was added
};


void print_record(record rec, string func){

	printf("[%s] Tid %d, %c, addr: 0x%llx, inst: 0x%llx, rtn: %s, time: %llu \n",
		func.c_str(), rec.tid, rec.op, rec.addr, rec.inst, rec.rtn.c_str(), rec.time);
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
