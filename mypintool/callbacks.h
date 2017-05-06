#include "structs.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>

void update_lastRead(UINT64 addr, record rec){

    map<UINT64, record>::iterator ite; 
    ite=lastRead.find(addr);
    if(ite==lastRead.end()){
        printf("\033[22;36m[update_lastRead] New read address 0x%llx, insert.\033[0m\n", addr);
        lastRead.insert(pair<UINT64,record>(addr,rec));
    }
    else{
        printf("\033[22;36m[update_lastRead] Already have 0x%llx, update.\033[0m\n", addr);
        lastRead[addr]=rec;
    }

}

void update_lastWrite(UINT64 addr, record rec){

    map<UINT64, record>::iterator ite; 
    ite=lastWrite.find(addr);
    if(ite==lastWrite.end()){
        printf("\033[22;36m[update_lastWrite] New write address 0x%llx, insert.\033[0m\n", addr);
        lastWrite.insert(pair<UINT64,record>(addr,rec));
    }
    else{
        printf("\033[22;36m[update_lastWrite] Already have 0x%llx.\033[0m\n", addr);
        if (lastWrite[addr].tid == rec.tid){
            lastWrite[addr]=rec;
            printf("\033[22;36m[update_lastWrite] Same thread, update.\033[0m\n");
        }
        else{
            if(lastWrite[addr].isLocked == 'L' || rec.isLocked == 'L'){
                printf("\033[22;36m[update_lastWrite] Both locked.\033[0m\n");
            }
            else{
                printf("\033[22;36m[update_lastWrite] save WAW.\033[0m\n");
                edge e = {"WAW", lastWrite[addr], rec};
                result.push_back(e);
            }
        }
    }

}

void check_war(UINT64 addr, record rec){
    map<UINT64, record>::iterator ite;
    ite=lastRead.find(addr);
    if(ite==lastRead.end()){
        printf("\033[22;36m[check_war] Do not find 0x%llx from lastRead table, skip.\033[0m\n", addr);
    }
    else{
        printf("\033[22;36m[check_war] Find 0x%llx from lastRead table.\033[0m\n", addr);
        if (lastRead[addr].tid == rec.tid){
            printf("\033[22;36m[check_war] Same thread, skip.\033[0m\n");
        }
        else{
            if(lastRead[addr].isLocked == 'L' || rec.isLocked == 'L'){
                printf("\033[22;36m[check_war] Both locked.\033[0m\n");
            }
            else{
                printf("\033[22;36m[check_war] save WAR.\033[0m\n");
                edge e = {"WAR", lastRead[addr], rec};
                result.push_back(e);
            }
        }
    }
}


void check_raw(UINT64 addr, record rec){
    map<UINT64, record>::iterator ite;
    ite=lastWrite.find(addr);
    if(ite==lastWrite.end()){
        printf("\033[22;36m[check_raw] Do not find 0x%llx from lastWrite table, skip.\033[0m\n", addr);
    }
    else{
        printf("\033[22;36m[check_raw] Find 0x%llx from lastWrite table.\033[0m\n", addr);
        if (lastWrite[addr].tid == rec.tid){
            printf("\033[22;36m[check_raw] Same thread, skip.\033[0m\n");
        }
        else{
            if(lastWrite[addr].isLocked == 'L' || rec.isLocked == 'L'){
                printf("\033[22;36m[check_raw] Both locked.\033[0m\n");
            }
            else{
                printf("\033[22;36m[check_raw] save RAW.\033[0m\n");
                edge e = {"RAW", lastWrite[addr], rec};
                result.push_back(e);
            }
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
    char l;
    if(locksets[tid].size()>0)
        l = 'L';
    else
        l = 'U';

    record rec={'R',(ADDRINT)ip, (ADDRINT)addr, tid, timestamp, string((char*)rtnName), l};
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
    
    char l;
    if(locksets[tid].size()>0)
        l = 'L';
    else
        l = 'U';

    record rec={'W',(ADDRINT)ip, (ADDRINT)addr, tid, timestamp, string((char*)rtnName), l};
    print_record(rec, "RecordMemWrite");

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
    printf("\033[01;34m[afterThreadCreate] start logging\033[0m\n");
    
    PIN_ReleaseLock(&lock);
    
}

VOID afterThreadLock(THREADID threadid, ADDRINT address)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadLock] T %d Locked, time: %llu, addr: 0x%x.\033[0m\n", threadid, timestamp, address);

    mlock l = {threadid, timestamp, address};

    locksets[threadid].push_back(l);
    printf("\033[01;33m[ThreadLock] lockset size %d\033[0m\n", locksets[threadid].size());

    PIN_ReleaseLock(&lock);
}
VOID beforeThreadUnLock(THREADID threadid, ADDRINT address)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadUnLock] T %d Unlocked, time: %llu, addr: 0x%x.\033[0m\n", threadid, timestamp, address);

 
    
    unsigned int i;
    for (i=0; i<locksets[threadid].size(); i++){
        if(locksets[threadid][i].addr == address){
            locksets[threadid].erase(locksets[threadid].begin()+i);
            printf("\033[01;33m[ThreadUnLock] lockset size %d\033[0m\n", locksets[threadid].size());
        }
    }

    
    
    PIN_ReleaseLock(&lock);
}

VOID afterThreadBarrier(THREADID threadid)
{
    timestamp++;
    printf("\033[01;33m[ThreadBarrier] T %d Barrier, time: %llu.\033[0m\n", threadid, timestamp);
}

VOID afterThreadCondWait(THREADID threadid)
{
    timestamp++;
    printf("\033[01;33m[ThreadCondWait] T %d Condwait, time: %llu.\033[0m\n", threadid, timestamp);
}

VOID afterThreadCondTimedwait(THREADID threadid)
{
    timestamp++;
    printf("\033[01;33m[ThreadCondTimedwait] T %d CondTimewait, time: %llu.\033[0m\n", threadid, timestamp);
}

VOID afterThreadSleep(THREADID threadid)
{
    timestamp++;
    printf("\033[01;33m[ThreadSleep] T %d sleep, time: %llu.\033[0m\n", threadid, timestamp);
}


// This routine is executed each time malloc is called.
VOID BeforeMalloc( int size, THREADID threadid )
{

}