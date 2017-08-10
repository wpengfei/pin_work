
#include "callbacks.h"


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

    PIN_GetLock(&lock,PIN_ThreadId()+1);

    if(rtn_name=="pthread_create"||rtn_name=="pthread_create"){
        RTN_Open(rtn);
        //RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadCreate,IARG_THREAD_ID, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadCreate, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_join"||rtn_name=="pthread_join"){
        RTN_Open(rtn);
        //RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadJoin, IARG_THREAD_ID, IARG_END);
        //RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadJoin, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtn); 
    } else if(rtn_name=="__pthread_mutex_lock"||rtn_name=="pthread_mutex_lock"){ //,IARG_FUNCARG_CALLSITE_REFERENCE,IARG_FUNCARG_CALLSITE_VALUE
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadLock, IARG_THREAD_ID, IARG_INST_PTR, IARG_FUNCARG_CALLSITE_VALUE, 0,IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_mutex_unlock"||rtn_name=="pthread_mutex_unlock"){//IARG_FUNCARG_ENTRYPOINT_REFERENCE, IARG_FUNCARG_ENTRYPOINT_VALUE
        RTN_Open(rtn);
        RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadUnLock, IARG_THREAD_ID, IARG_INST_PTR, IARG_FUNCARG_CALLSITE_VALUE, 0,IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_barrier_wait"||rtn_name=="pthread_barrier_wait"){
        RTN_Open(rtn);
        //RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadCondWait, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadBarrier, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_cond_wait"||rtn_name=="pthread_cond_wait"){
        RTN_Open(rtn);
        //RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadCondWait, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadCondWait, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="__pthread_cond_timedwait"||rtn_name=="pthread_cond_timedwait"){
        RTN_Open(rtn);
        //RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadCondTimedwait, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadCondTimedwait, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtn);
    } else if(rtn_name=="sleep"||rtn_name=="usleep"||rtn_name=="__sleep"||rtn_name=="__usleep"){
    //we consider thread sleep as it likes thread wait 
        RTN_Open(rtn);
        //RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)beforeThreadSleep, IARG_END);
        RTN_InsertCall(rtn,IPOINT_AFTER,(AFUNPTR)afterThreadSleep, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtn);
    }

    PIN_ReleaseLock(&lock);
    
}





VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{

   
    PIN_GetLock(&lock, threadid+1);

    threadNum++;
    threadExisted++;
    if (threadNum == 1)
        printf("\033[01;34m[ThreadStart] Main Thread T 0 created, total number is %d\033[0m\n", threadNum);
    else
        printf("\033[01;34m[ThreadStart] Thread %d created, total number is %d\033[0m\n", threadid, threadNum);
    
     
    PIN_ReleaseLock(&lock);
    
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    
    PIN_GetLock(&lock, threadid+1);
    threadNum--;
    if (threadNum > 1)
    {
        printf("\033[01;34m[ThreadFini] Thread %d joined, total number is %d\033[0m\n", threadid, threadNum);
    }
    else if (threadNum == 1)
    {
        printf("\033[01;34m[ThreadFini] Thread %d joined, total number is %d, stop logging.\033[0m\n", threadid, threadNum);
        logging_start = false;
    }
    else
        printf("\033[01;34m[ThreadFini] Main Thread joined, total number is %d\033[0m\n", threadNum);
  
    PIN_ReleaseLock(&lock);
    
}

/* If a critical section does not protect any memory access from the same thread, 
    it is an empty cs, we should remove it*/
bool is_cs_empty(vector<criticalSection>::iterator it){
    unsigned int i = 0;
    for(i=0; i < maTable[it->tid].size(); i++){
        if (maTable[it->tid][i].time > it->st && maTable[it->tid][i].time < it->ft){
            return false;
        }
    }

    return true;
}
// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application


    unsigned int i = 0;
    unsigned int j = 0;

    //remove empty critical sections.
    vector<criticalSection>::iterator it;
    
    for(it=csTable.begin();it!=csTable.end();){
        if(it->st + 1 == it->ft){
            it = csTable.erase(it);       
        }
        else if(is_cs_empty(it)){
            it = csTable.erase(it);
        }
        else{
             ++it;
        }
    }

    printf("----------------------------Memory access\n");
    for(i=0; i<threadExisted; i++){
        printf("Thread %d: \n",i);
        for(j=0; j<maTable[i].size(); j++){
            print_ma(maTable[i][j], "Fini");
        }
    }
    
    printf("----------------------------Critical sections\n");
    for(i=0; i<csTable.size(); i++){
        print_cs(csTable[i],"Fini");
    }
    printf("----------------------------Synchronizations\n");
    for(i=0; i<synchTable.size(); i++){
        print_synch(synchTable[i], "Fini");
    }
    printf("============================Analysis\n");
    find_same_address_accesses();

    fprintf(replay_log, "#eof\n");
    fclose(replay_log);
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

    replay_log = fopen("pattern_for_replay.log", "w");

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    //OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);



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