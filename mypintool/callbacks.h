
void update_lastRead(UINT64 addr, record rec){

    map<UINT64, record>::iterator ite; 
    ite=lastRead.find(addr);
    if(ite==lastRead.end()){
        printf("[update_lastRead] Do not find 0x%llx, insert.\n", addr);
        lastRead.insert(pair<UINT64,record>(addr,rec));
    }
    else{
        printf("[update_lastRead] Already have 0x%llx, update.\n", addr);
        lastRead[addr]=rec;
    }

}

void update_lastWrite(UINT64 addr, record rec){

    map<UINT64, record>::iterator ite; 
    ite=lastWrite.find(addr);
    if(ite==lastWrite.end()){
        printf("[update_lastWrite] Do not find 0x%llx, insert.\n", addr);
        lastWrite.insert(pair<UINT64,record>(addr,rec));
    }
    else{
        printf("[update_lastWrite] Already have 0x%llx.\n", addr);
        if (lastWrite[addr].tid == rec.tid){
            lastWrite[addr]=rec;
            printf("[update_lastWrite] Same thread, update.\n");
        }
        else{
            printf("update_lastWrite] save WAW\n");
            edge e = {"WAW", lastWrite[addr], rec};
            result.push_back(e);
        }
    }

}

void check_war(UINT64 addr, record rec){
    map<UINT64, record>::iterator ite;
    ite=lastRead.find(addr);
    if(ite==lastRead.end()){
        printf("[check_war] Do not find 0x%llx from lastRead table, skip.\n", addr);
    }
    else{
        printf("[check_war] Find 0x%llx from lastRead table.\n", addr);
        if (lastRead[addr].tid == rec.tid){
            printf("[check_war] Same thread, skip.\n");
        }
        else{
            printf("check_war] save WAR\n");
            edge e = {"WAR", lastRead[addr], rec};
            result.push_back(e);
        }
    }
}


void check_raw(UINT64 addr, record rec){
    map<UINT64, record>::iterator ite;
    ite=lastWrite.find(addr);
    if(ite==lastWrite.end()){
        printf("[check_raw] Do not find 0x%llx from lastWrite table, skip.\n", addr);
    }
    else{
        printf("[check_raw] Find 0x%llx from lastWrite table.\n", addr);
        if (lastWrite[addr].tid == rec.tid){
            printf("[check_raw] Same thread, skip.\n");
        }
        else{
            printf("check_raw] save WAR\n");
            edge e = {"RAW", lastWrite[addr], rec};
            result.push_back(e);
        }
    }
}





// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, THREADID tid, VOID* rtnName)
{
    if((unsigned int)addr > STACK_LOWERBOUND)
        return;

    PIN_GetLock(&lock,tid+1);

    timestamp++;

    //fprintf(trace,"Thread %d read from address %p in rtn %s\n", tid, addr, (char*)rtnName);
    //outFile<<"Thread:"<<tid<<" [Read] from Addr:"<<addr<<"\tin rtn: "<<string((char*)rtnName)<<endl;
    //printf("[RecordMemRead] T: %d, ins: %p, add: %p, rtn: %s, time: %llu\n", tid, addr, ip, (char*)rtnName, timestamp);
    
    record rec={'R',(ADDRINT)ip, (ADDRINT)addr, tid, timestamp, string((char*)rtnName)};
    print_record(rec, "RecordMemRead");
    if (logging_start){
        records.push_back(rec); //add new record

        update_lastRead((ADDRINT)addr, rec);
        check_raw((ADDRINT)addr, rec); //check for RAW
    }
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
    //printf("[RecordMemWrite] T: %d, ins: %p, add: %p, rtn: %s, time: %llu\n", tid, addr, ip, (char*)rtnName, timestamp);
    
    record rec={'W',(ADDRINT)ip, (ADDRINT)addr, tid, timestamp, string((char*)rtnName)};
    print_record(rec, "RecordMemRead");
    if (logging_start){
        records.push_back(rec);//create new record 

        update_lastWrite((ADDRINT)addr, rec);//update lastWrite table and check for WAW
        check_war((ADDRINT)addr, rec);//then check for WAR
    }

    PIN_ReleaseLock(&lock);
}



VOID afterThreadCreate(THREADID threadid)
{

    PIN_GetLock(&lock, threadid+1);
    
    logging_start = true;
    printf("[afterThreadCreate] start logging\n");
    
    PIN_ReleaseLock(&lock);
    
}

VOID afterThreadLock(THREADID threadid, ADDRINT address)
{
    timestamp++;
    printf("[afterThreadLock] T %d Locked, time: %llu.\n", threadid, timestamp);
}
VOID beforeThreadUnLock(THREADID threadid, ADDRINT address)
{
    timestamp++;
    printf("[beforeThreadUnLock] T %d Unlocked, time: %llu.\n", threadid, timestamp);
}

VOID afterThreadBarrier(THREADID threadid)
{
    timestamp++;
    printf("[afterThreadBarrier] T %d Barrier, time: %llu.\n", threadid, timestamp);
}

VOID afterThreadCondWait(THREADID threadid)
{
    timestamp++;
    printf("[afterThreadCondWait] T %d Condwait, time: %llu.\n", threadid, timestamp);
}

VOID afterThreadCondTimedwait(THREADID threadid)
{
    timestamp++;
    printf("[afterThreadCondTimedwait] T %d CondTimewait, time: %llu.\n", threadid, timestamp);
}

VOID afterThreadSleep(THREADID threadid)
{
    timestamp++;
    printf("[afterThreadSleep] T %d sleep, time: %llu.\n", threadid, timestamp);
}


// This routine is executed each time malloc is called.
VOID BeforeMalloc( int size, THREADID threadid )
{

}