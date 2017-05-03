#include "pin.H"

#define STACK_LOWERBOUND 0x40000000
#define USER_LOWERBOUND  0x08048000
#define USER_UPPERBOUND  0x15000000

//ofstream outFile;
FILE * trace;
PIN_LOCK lock;


//global varibles
UINT32 threadNum = 0;
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