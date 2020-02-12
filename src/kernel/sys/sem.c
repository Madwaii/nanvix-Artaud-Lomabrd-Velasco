
#include <nanvix/const.h>
#include <nanvix/fs.h>
#include <nanvix/pm.h>
#include <errno.h>
#include <unistd.h>
#include <sys/sem.h>

#define SMAX 64


EXTERN int sys_kill(pid_t pid, int sig);


typedef struct
{
    unsigned key;
    int counter;
    int id;
    struct process** processes;
} SEM;

PUBLIC SEM *slist[SMAX] = {NULL};

int idlibre () {
    for (int i =0; i<SMAX; i++) {
        if (slist[i]==NULL) {
            return i;
        }
    }
    return -1;
}
SEM create(int n){
    int id = idlibre();
    SEM sem;
    sem.counter=n;
    struct process* lp[64];
    sem.processes=lp;
    sem.id=id;
    sem.key=-42;
    slist[id]=&sem;
    return sem;
}

PUBLIC void down(SEM s){
    disable_interrupts();
    if (s.counter>0) {
        s.counter--;
    }
    else {
        sleep(s.processes, 1);
    }
    enable_interrupts();
}

PUBLIC void up(SEM s){
    disable_interrupts();
    if (s.processes != NULL && s.counter==0 ) {
        struct process *p = s.processes[0];
        int i =0;
        while(p->next!= NULL){
            p = p->next;
            i++;
        }
        struct process **l = &s.processes[i];
        wakeup(l);
        s.processes[i-1]->next = NULL;
    }
    else {
        s.counter++;
    }
    enable_interrupts();
}

PUBLIC void destroy(SEM s){
    struct process *p = s.processes[0];
    while(p->next!= NULL){
        p = p->next;
        sys_kill(p->pid, 9);
    }
    slist[s.id]=NULL;
}

PUBLIC extern int sys_semget(unsigned key ) {
    SEM s = create(0);
    s.key = key;
    return s.id;
}

PUBLIC extern int sys_semctl(int semid, int cmd, int val){
    SEM s = *slist[semid];
    if (cmd == GETVAL) {
        return s.counter;
    }
    else if (cmd == SETVAL) {
        s.counter = val;
        return 0;
    }
    else if (cmd == IPC_RMID) {
        destroy(s);
        return 0;
    }
    return -1;
}

PUBLIC extern int sys_semop(int semid, int op){
    SEM s = *slist[semid];
    if (op <0) {
        down(s);
    } else {
        up(s);
    }
    return 0;
}
