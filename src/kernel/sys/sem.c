
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
int create(int n){
    int id = idlibre();
    SEM sem;
    sem.counter=n;
    struct process* lp[64];
    sem.processes=lp;
    sem.id=id;
    sem.key=-42;
    slist[id]=&sem;
    return sem.id;
}

PUBLIC void down(SEM s){
    int i =s.counter;
    i++;
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

PUBLIC int sys_semget(unsigned key ) {
    for (int i=0; i<SMAX; i++) {
        if (slist[i] !=NULL) {
            if (slist[i]->key==key) return i;
        }
    }
    int id = create(0);
    slist[id]->key = key;;
    return id;
}

PUBLIC int sys_semctl(int semid, int cmd, int val){
    if (cmd == GETVAL) {
        return slist[semid]->counter;
    }
    else if (cmd == SETVAL) {
        slist[semid]->counter = val;
        return 0;
    }
    else if (cmd == IPC_RMID) {
        destroy(*slist[semid]);
        return 0;
    }
    return -1;
}

PUBLIC int sys_semop(int semid, int op){
    int i =semid;
    i++;
    if (op <0) {
        down(*slist[semid]);
    } else {
        up(*slist[semid]);
    }
    return 0;
}
