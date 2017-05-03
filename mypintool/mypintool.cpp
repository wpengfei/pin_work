#include "definations.h"
#include "callbacks.h"
#include "helperfuncs.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>



// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    if(INS_Address(ins) > USER_UPPERBOUND) 
        return; 
    if(INS_Address(ins) < USER_LOWERBOUND) 
        return;
    if(INS_IsBranchOrCall(ins)) 
        return;
    
    RTN rtn = INS_Rtn(ins);  
    if(isLibRtnName(RTN_Name(rtn))) 
        return;

    if (INS_IsMemoryRead(ins)){
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead, IARG_INST_PTR, IARG_MEMORYREAD_EA, 
          IARG_THREAD_ID, IARG_PTR, (VOID*)RTN_Name(rtn).c_str(), IARG_END);
    }

    
    if(INS_IsMemoryWrite(ins)){
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,IARG_INST_PTR,IARG_MEMORYWRITE_EA, 
          IARG_THREAD_ID, IARG_PTR, (VOID*)RTN_Name(rtn).c_str(), IARG_END);
    }

}

// Pin calls this function every time a new rtn is executed
VOID Routine(RTN rtn, VOID *v)
{
    
    string rtn_name = RTN_Name(rtn);
/*
    PIN_GetLock(&lock,PIN_ThreadId()+1);

    if(rtn_name=="pthread_create"||rtn_name=="pthread_create"){
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadCreate,IARG_END);
        //RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadCreate, IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_mutex_lock"||rtn_name=="pthread_mutex_lock"){
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadLock,IARG_FUNCARG_ENTRYPOINT_VALUE, 0,IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_mutex_unlock"||rtn_name=="pthread_mutex_unlock"){
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadUnLock,IARG_FUNCARG_ENTRYPOINT_VALUE, 0,IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_join"||rtn_name=="pthread_join"){
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadJoin, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadJoin, IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_cond_wait"||rtn_name=="pthread_cond_wait"){
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadCondWait, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadCondWait, IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_cond_timedwait"||rtn_name=="pthread_cond_timedwait"){
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadCondTimedwait, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadCondTimedwait, IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="sleep"||rtn_name=="usleep"||rtn_name=="__sleep"||rtn_name=="__usleep"){
    //we consider thread sleep as it likes thread wait 
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadSleep, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadSleep, IARG_END);
        RTN_Close(rtn);
    }

    PIN_ReleaseLock(&lock);
    //ADDRINT rtn_addr = RTN_Address(rtn);
    
    //outFile <<"Routine name: "<<rtn_name.c_str()<<"\t Addr:"<<rtn_addr<<endl;
    
*/    

}



// This routine is executed for each image.
VOID ImageLoad(IMG img, VOID *)
{
    //cerr << "image load" << endl;
    RTN rtn = RTN_FindByName(img, "malloc");
    if ( RTN_Valid(rtn))
    {
        RTN_Open(rtn);
       
        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(BeforeMalloc),
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_THREAD_ID, IARG_END);

        RTN_Close(rtn);
    }
}

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    // Note:  threadid+1 is used as an argument to the PIN_GetLock()
    //        routine as a debugging aid.  This is the value that
    //        the lock is set to, so it must be non-zero.
    PIN_GetLock(&lock, threadid+1);
    threadNum++;
    printf("[ThreadStart] Thread %d created, total number is %d\n", threadid, threadNum);
    if (threadNum >= 2){
        logging_start = true;
        printf("[ThreadStart] start logging\n");
    }
    PIN_ReleaseLock(&lock);
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    PIN_GetLock(&lock, threadid+1);
    threadNum--;
    printf("[ThreadFini] Thread %d joined, total number is %d\n", threadid, threadNum);
    if (threadNum < 2){
        logging_start = false;
        printf("[ThreadFini] stop logging\n");
    }
    PIN_ReleaseLock(&lock);
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    fprintf(trace, "#eof\n");
    fclose(trace);

    //outFile<<"#eof"<< endl;
    //outFile.close();

    UINT32 i = 0;
    for(i=0; i<recordNum; i++){
        printf("[Fini] Thread: %d, op:%c, address: 0x%llx, instruction: 0x%llx, rtn %s, time: %llu\n", 
            records[i].tid, records[i].op,records[i].addr, records[i].inst, records[i].rtn.c_str(), records[i].time);
    }

}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");

    cerr << "This Pintool counts the number of times a routine is executed" << endl;
    cerr << "and the number of instructions executed in a routine" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    trace = fopen("pinatrace.out", "w");

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    //OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register ImageLoad to be called when each image is loaded.
    IMG_AddInstrumentFunction(ImageLoad, 0);

    // Register Routine to be called to instrument rtn
    RTN_AddInstrumentFunction(Routine, 0);

    // Register Analysis routines to be called when a thread begins/ends
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}