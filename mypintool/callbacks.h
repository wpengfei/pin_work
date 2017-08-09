#include "structs.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>


// Check if the two memory accesses protected by a same critical section.
bool is_protected_by_cs(memAccess first, memAccess second){
    assert(first.tid == second.tid);
    assert(first.time < second.time);
    assert(first.addr == second.addr);

    unsigned int i;
    for(i=0; i<csTable.size(); i++){
        if(first.time > csTable[i].st && first.time < csTable[i].ft
            && second.time > csTable[i].st && second.time < csTable[i].ft
            && first.tid == csTable[i].tid)
            return true;
    }

    return false;
}



void record_pattern(memAccess first, memAccess second, memAccess inter){

    /*here determine whether these three accesses are protected by locks
    *if not protected by locks (critical sections), then we record these accesses.
    *otherwitse, we record the lock and unlock of each access, 
    *which will be used to delay the access later on in the replay phase.
    */

    bool first_locked = false;
    bool second_locked = false;
    bool inter_locked = false;

    criticalSection first_cs = {0,0,0,0};
    criticalSection second_cs = {0,0,0,0};
    criticalSection inter_cs = {0,0,0,0};

    unsigned int i;
    //find the critical section for First
    for(i=0; i<csTable.size(); i++){
        if(first.time > csTable[i].st && first.time < csTable[i].ft
            && first.tid == csTable[i].tid){
            first_cs.tid = csTable[i].tid;
            first_cs.entryref = csTable[i].entryref;
            first_locked = true;
            break;
        }
            
    }
    //find the critical section for Second
    for(i=0; i<csTable.size(); i++){
        if(second.time > csTable[i].st && second.time < csTable[i].ft
            && second.tid == csTable[i].tid){
            second_cs.tid = csTable[i].tid;
            second_cs.entryref = csTable[i].entryref;
            second_locked = true;
            break;
        }
            
    }
    //find the critical section for Inter
    for(i=0; i<csTable.size(); i++){
        if(inter.time > csTable[i].st && inter.time < csTable[i].ft
            && inter.tid == csTable[i].tid){
            inter_cs.tid = csTable[i].tid;
            inter_cs.entryref = csTable[i].entryref;
            inter_locked = true;
            break;
        }
            
    }
    unsigned int locked;
    if(first_locked && second_locked && inter_locked ){
        locked = 1;
        fprintf(replay_log, "%u[%u]%x,%x; [%u]%x,%x; [%u]%x,%x\n",
            locked,first_cs.tid, first_cs.entryref, 0,
            second_cs.tid, second_cs.entryref, 0,
            inter_cs.tid, inter_cs.entryref, 0);
    }

    //printf("\033[01;31m[record_pattern]:first: 0x%x, second: 0x%x,inter: 0x%x,\n",first.inst, second.inst, interleave.inst);
    else if(!first_locked && !second_locked && !inter_locked ){
        locked = 0;
        fprintf(replay_log, "%u[%u]%x,%x; [%u]%x,%x; [%u]%x,%x\n",
            locked,first.tid, first.inst, first.addr,
            second.tid, second.inst, second.addr,
            inter.tid, inter.inst, inter.addr);
    }
    else
        assert(false);

}
//check whether the found interleaving could cause an atomicity-violation
//if it is buggy, then report immediatly,
void examine_interleaving(memAccess first, memAccess second, memAccess interleave){
    if(interleave.time > first.time && interleave.time < second.time){
        printf("\033[01;31m[examine_interleaving] Buggy interleaving. \033[0m\n");
        return;
    }
    // pattern 1:
    //
    //  T1     T2
    //          I
    //   A1
    //   ...
    //   A2
    //
    if(interleave.time < first.time){
        unsigned int i;
        // check whether a synchronization (e.g. barriar) pretents this pattern form an AV
        for(i=0; i<synchTable.size(); i++){
            if(synchTable[i].tid == interleave.tid 
                && synchTable[i].time > interleave.time
                && synchTable[i].time < first.time){

                printf("\033[22;36m[examine_interleaving] Synchronization exists, skip. \033[0m\n");
                print_synch(synchTable[i], "examine_interleaving");
                return;
            }
               
        }
        printf("\033[01;31m[examine_interleaving] No synch protection. Record potential buggy pattern for replay. \033[0m\n");
        record_pattern(first, second, interleave);
        return;
    }
    //pattern 2:
    //
    //  T1     T2
    //          
    //   A1
    //   ...
    //   A2
    //          I
    if(interleave.time > second.time){
        unsigned int i;
        // check whether a synchronization (e.g. barriar) pretents this pattern form an AV
        for(i=0; i<synchTable.size(); i++){
            if(synchTable[i].tid == interleave.tid 
                && synchTable[i].time < interleave.time
                && synchTable[i].time > second.time){

                printf("\033[22;36m[examine_interleaving] Synchronization exists, skip. \033[0m\n");
                print_synch(synchTable[i], "examine_interleaving");
                return;
            }
                
        }
        printf("\033[01;31m[examine_interleaving] No synch protection. Record potential buggy pattern for replay. \033[0m\n");
        record_pattern(first, second, interleave);
        return;
    }
}


// find a write from a remote thread that could interleave the local pair
void find_interleave_write(memAccess first, memAccess second){
    unsigned int i, j;

    for(i=0; i<threadExisted; i++){
        if(i != first.tid){
            for(j=0; j<maTable[i].size(); j++){
                if(maTable[i][j].op == 'W' && maTable[i][j].addr == first.addr){
                    printf("\033[22;36m[find_interleave_write] Find remote write to the same address.\033[0m\n");
                    print_ma(maTable[i][j],"find_interleave_write");
                    examine_interleaving(first, second, maTable[i][j]);
                }
            }
        }
    }

}

// find a read from a remote thread that could interleave the local pair
void find_interleave_read(memAccess first, memAccess second){
    unsigned int i, j;

    for(i=0; i<threadExisted; i++){
        if(i != first.tid){
            for(j=0; j<maTable[i].size(); j++){
                if(maTable[i][j].op == 'R' && maTable[i][j].addr == first.addr){
                    printf("\033[22;36m[find_interleave_read] Find remote read to the same address.\033[0m\n");
                    print_ma(maTable[i][j],"find_interleave_read");
                    examine_interleaving(first, second, maTable[i][j]);
                }
            }
        }
    }

}

//check whether an atomic pair is interleaved by a remote access
//we have 4 patterns that can form a atomicity-violation
void check_pair(memAccess first, memAccess second){
    
    if(is_protected_by_cs(first, second)){
        //printf("\033[22;36m[check_pair] Access pair protected by the same lock .\033[0m\n");
        return;
    }
    
    printf("\033[22;36m[check_pair] Find unprotected access pair.\033[0m\n");
    print_ma(first, "check_pair");
    print_ma(second, "check_pair");

    if(first.op == 'R' && second.op == 'R'){
        find_interleave_write(first, second);
    }
    if(first.op == 'R' && second.op == 'W'){
        find_interleave_write(first, second);
    }
    if(first.op == 'W' && second.op == 'R'){
        find_interleave_write(first, second);
    }
    if(first.op == 'W' && second.op == 'W'){
        find_interleave_read(first, second);
    }

}



// find memory access pairs from the same thread accessing the same address
void find_same_address_accesses(){
    bool found = false;
    unsigned int i,j,k;
    for(i=0; i<threadExisted; i++){
        for(j=0; j<maTable[i].size(); j++){
            for(k=j+1; k<maTable[i].size(); k++){
                if(maTable[i][j].addr == maTable[i][k].addr){
                    check_pair(maTable[i][j], maTable[i][k]);
                    found = true;
                }
            }
        }
    }

    if(!found)
        printf("\033[22;36m[find_same_address_accesses] Did not find same address pair. \033[0m\n");

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


    memAccess ma={'R',(ADDRINT)ip, (ADDRINT)addr, tid, timestamp, string((char*)rtnName)};
    print_ma(ma, "RecordMemRead");

    if (logging_start){
        maTable[tid].push_back(ma); //add new record

        //update_reads((ADDRINT)addr, rec);
        //check_raw((ADDRINT)addr, rec); //check for RAW
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
    
    memAccess ma={'W',(ADDRINT)ip, (ADDRINT)addr, tid, timestamp, string((char*)rtnName)};
    print_ma(ma,  "RecordMemWrite");

    if (logging_start){
        maTable[tid].push_back(ma); //add new record

        //update_writes((ADDRINT)addr, rec);//update lastWrite table and check for WAW
        //check_war((ADDRINT)addr, rec);//then check for WAR
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

VOID beforeThreadLock(THREADID threadid, VOID * ip, ADDRINT entryref)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadLock] T %d Locked, time: %d, ip: 0x%x, entryref: 0x%x.\033[0m\n", 
        threadid, timestamp, (ADDRESS)ip, entryref);

    criticalSection cs = {threadid, entryref, timestamp, 0}; //use 0 by default when a critical section is not finished

    csTable.push_back(cs);

    PIN_ReleaseLock(&lock);
}
VOID beforeThreadUnLock(THREADID threadid, VOID * ip, ADDRINT entryref)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadUnLock] T %d Unlocked, time: %d, ip: 0x%x, entryref: 0x%x.\033[0m\n", 
        threadid, timestamp, (ADDRESS)ip, entryref);

    unsigned int i;
    for (i=0; i<csTable.size(); i++){
        if(csTable[i].entryref == entryref && csTable[i].tid == threadid && csTable[i].ft == 0){
            csTable[i].ft = timestamp;//finish the critical section
        }
    }
  
    PIN_ReleaseLock(&lock);
}

VOID afterThreadBarrier(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);

    timestamp++;
    printf("\033[01;33m[ThreadBarrier] T %d Barrier, time: %d.\033[0m\n", threadid, timestamp);

    synch s = {"barrier", threadid, timestamp};
    synchTable.push_back(s);

    PIN_ReleaseLock(&lock);
}

VOID afterThreadCondWait(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadCondWait] T %d Condwait, time: %d.\033[0m\n", threadid, timestamp);
    
    synch s = {"condwait", threadid, timestamp};
    synchTable.push_back(s);

    PIN_ReleaseLock(&lock);
}

VOID afterThreadCondTimedwait(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadCondTimedwait] T %d CondTimewait, time: %d.\033[0m\n", threadid, timestamp);

    synch s = {"condtimewait", threadid, timestamp};
    synchTable.push_back(s);

    PIN_ReleaseLock(&lock);
}

VOID afterThreadSleep(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    timestamp++;
    printf("\033[01;33m[ThreadSleep] T %d sleep, time: %d.\033[0m\n", threadid, timestamp);

    synch s = {"sleep", threadid, timestamp};
    synchTable.push_back(s);

    PIN_ReleaseLock(&lock);
}

