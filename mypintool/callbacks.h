/*This file has the customized callback functions for the instrumentation*/



// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, THREADID tid, VOID* rtnName)
{
    if((unsigned int)addr > STACK_LOWERBOUND)
        return;

    PIN_GetLock(&lock,tid+1);

    timestamp++;

    //fprintf(trace,"Thread %d read from address %p in rtn %s\n", tid, addr, (char*)rtnName);
    //outFile<<"Thread:"<<tid<<" [Read] from Addr:"<<addr<<"\tin rtn: "<<string((char*)rtnName)<<endl;
    printf("[RecordMemRead] Thread: %d, instruction: %p, address: %p, rtn %s\n", tid, addr, ip, (char*)rtnName);
    record rec={'R',(ADDRINT)ip, (ADDRINT)addr, tid, timestamp, string((char*)rtnName)}; //create new record
    records.push_back(rec); 
    recordNum++;
    PIN_ReleaseLock(&lock);
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, THREADID tid, VOID* rtnName)
{
    if((unsigned int)addr > STACK_LOWERBOUND)
        return;

    PIN_GetLock(&lock,tid+1);
    
    timestamp++;

    //fprintf(trace,"Thread %d write from address %p in rtn %s\n", tid, addr, (char*)rtnName);
    printf("[RecordMemWrite] Thread: %d, instruction: %p, address: %p, rtn %s\n", tid, addr, ip, (char*)rtnName);
    record rec={'W',(ADDRINT)ip, (ADDRINT)addr, tid, timestamp, string((char*)rtnName)}; //create new record
    records.push_back(rec); 
    recordNum++;

    PIN_ReleaseLock(&lock);
}

// This routine is executed each time malloc is called.
VOID BeforeMalloc( int size, THREADID threadid )
{
    /*
    PIN_GetLock(&lock, threadid+1);
    outFile <<"thread "<<threadid<<" entered malloc("<<size<<")\n";
    PIN_ReleaseLock(&lock);
    */
}