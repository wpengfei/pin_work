#include "structs.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>

/* Check whether any sychronizations lie between two memory accesses.
* Check by the timestamp, if sychroniztions exist, then return true.
*/
bool synch_exists(record first, record second){
    assert(first.time < second.time);
    unsigned int i=0;

    for(i=0; i<synchs.size(); i++){
        if(synchs[i].tid == second.tid && synchs[i].time < second.time && synchs[i].time > first.time)
            return true;
    } 
    return false;

}


void update_reads(UINT64 addr, record rec){

    map<UINT64, records_vector>::iterator ite; 
    ite=prevReads.find(addr);
    if(ite==prevReads.end()){
        printf("\033[22;36m[update_lastRead] New read address 0x%llx, insert.\033[0m\n", addr);
        records_vector rv;
        rv.push_back(rec);
        prevReads.insert(pair<UINT64,records_vector>(addr,rv));
    }
    else{
        printf("\033[22;36m[update_lastRead] Already have 0x%llx, update.\033[0m\n", addr);
        prevReads[addr].push_back(rec);
    }

}

void update_writes(UINT64 addr, record rec){

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
                printf("\033[22;36m[update_lastWrite] locked.\033[0m\n");
            }
            else{
                if(!synch_exists(lastWrite[addr],rec)){
                    printf("\033[22;36m[update_lastWrite] save WAW.\033[0m\n");
                    edge e = {"WAW", lastWrite[addr], rec};
                    result.push_back(e);
                }
                else{
                    printf("\033[22;36m[update_lastWrite] synch exists.\033[0m\n");
                }
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
                printf("\033[22;36m[check_raw] locked.\033[0m\n");
            }
            else{
                if(!synch_exists(lastWrite[addr],rec)){
                    printf("\033[22;36m[check_raw] save RAW.\033[0m\n");
                    edge e = {"RAW", lastWrite[addr], rec};
                    result.push_back(e);
                }
                else{
                     printf("\033[22;36m[check_raw] synch exists.\033[0m\n");
                }
            }
        }
    }
}

void check_war(UINT64 addr, record rec){
    map<UINT64, records_vector>::iterator ite;
    ite=prevReads.find(addr);
    if(ite==prevReads.end()){
        printf("\033[22;36m[check_war] Do not find 0x%llx from prevReads table, skip.\033[0m\n", addr);
    }
    else{
        printf("\033[22;36m[check_war] Find 0x%llx from prevReads table.\033[0m\n", addr);
        unsigned int i;
        /*check all the previous reads whether can form war*/
        for(i=0; i<prevReads[addr].size(); i++){
            if (prevReads[addr][i].tid == rec.tid){
                printf("\033[22;36m[check_war] Same thread, skip.\033[0m\n");
            }
            else{
                if(prevReads[addr][i].isLocked == 'L' || rec.isLocked == 'L'){
                    printf("\033[22;36m[check_war] locked.\033[0m\n");
                }
                else{
                    if(!synch_exists(prevReads[addr][i],rec)){
                        printf("\033[22;36m[check_war] save WAR.\033[0m\n");
                        edge e = {"WAR", prevReads[addr][i], rec};
                        result.push_back(e);
                    }
                    else{
                        printf("\033[22;36m[check_war] synch exists.\033[0m\n");
                    }
                }
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

        update_reads((ADDRINT)addr, rec);
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

        update_writes((ADDRINT)addr, rec);//update lastWrite table and check for WAW
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
    PIN_GetLock(&lock, threadid+1);

    timestamp++;
    printf("\033[01;33m[ThreadBarrier] T %d Barrier, time: %llu.\033[0m\n", threadid, timestamp);

    synch s = {"barrier", threadid, timestamp};
    synchs.push_back(s);

    PIN_ReleaseLock(&lock);
}

VOID afterThreadCondWait(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadCondWait] T %d Condwait, time: %llu.\033[0m\n", threadid, timestamp);
    
    synch s = {"condwait", threadid, timestamp};
    synchs.push_back(s);

    PIN_ReleaseLock(&lock);
}

VOID afterThreadCondTimedwait(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadCondTimedwait] T %d CondTimewait, time: %llu.\033[0m\n", threadid, timestamp);

    synch s = {"condtimewait", threadid, timestamp};
    synchs.push_back(s);

    PIN_ReleaseLock(&lock);
}

VOID afterThreadSleep(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadSleep] T %d sleep, time: %llu.\033[0m\n", threadid, timestamp);

    synch s = {"sleep", threadid, timestamp};
    synchs.push_back(s);

    PIN_ReleaseLock(&lock);
}


// This routine is executed each time malloc is called.
VOID BeforeMalloc( int size, THREADID threadid )
{

}