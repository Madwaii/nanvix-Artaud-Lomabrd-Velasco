
#include <nanvix/const.h>
#include <nanvix/fs.h>
#include <nanvix/pm.h>
#include <errno.h>
#include <unistd.h>
#include <sys/sem.h>

#define SMAX 64


EXTERN int sys_kill(pid_t pid, int sig);


typedef struct Sem{
    int counter;
    struct process *tabWait[64];
    unsigned  key;
} SEM;


SEM tabSem[64];

unsigned int curkey=1;

SEM* create (int n){

    int i=0;
    while(tabSem[i].key!=0){
        i++;
    }
    if(i<64){
        tabSem[i].key=curkey;
        tabSem[i].counter=n;
        curkey++;
        return &tabSem[i];
    }
    else{
        return NULL;
    }


}

void down(SEM *sem){
    if(sem->counter > 0){
        sem->counter--;
    }
    else{
        sleep(sem->tabWait,5);
    }
}

void up(SEM *sem){
    if(sem->counter==0){
        if(sem->tabWait[0]!=NULL){
            wakeup(sem->tabWait);
        }
        else{
            sem->counter++;
        }
    }

}

void destroy(SEM *sem){
    for(int i=0;i<64;i++){
        sem->tabWait[i]=NULL;
    }
    sem->key=0;
}

int sys_semget(unsigned key){
    int i=0;
    while(i<64){
        if(tabSem[i].key==key){
            return i;
        }
        i++;
    }
    SEM* sem= create(1);
    sem->key=key;
    int ID=0;
    while(tabSem[ID].key!=key){
        ID++;
    }
    return ID;
}

int sys_semctl(int semid, int cmd, int val){
    switch(cmd){
        case GETVAL:
        return tabSem[semid].counter;
        case SETVAL:
        tabSem[semid].counter=val;
        return 0;
        case IPC_RMID:
        destroy(&tabSem[semid]);
        return 0;
        default:
        break;
    }
    return -1;
}

int sys_semop(int semid, int op){
    if(op>0){
        up(&tabSem[semid]);
        return 0;
    }
    if(op<0){
        down(&tabSem[semid]);
        return 0;
    }
    else{
        return -1;
    }
}
