#include "mcs.h"

int mcs_trylock(mcs_lock *L, mcs_qnode_ptr I) {
    I->next=NULL;
    if (CAS_PTR(L, NULL, I)==NULL) return 0;
    return 1;

}

void mcs_acquire(mcs_lock *L, mcs_qnode_ptr I) 
{
    I->next = NULL;
    mcs_qnode_ptr pred = (mcs_qnode*) SWAP_PTR((volatile void*) L, (void*) I);
    if (pred == NULL) 		/* lock was free */
        return;
    I->waiting = 1; // word on which to spin
    MEM_BARRIER;
    pred->next = I; // make pred point to me

    while (I->waiting != 0)
    {
        PAUSE;
    }

}

void mcs_release(mcs_lock *L, mcs_qnode_ptr I) 
{
    mcs_qnode_ptr succ;
    if (!(succ = I->next)) /* I seem to have no succ. */
    { 
        /* try to fix global pointer */
        if (CAS_PTR(L, I, NULL) == I)
            return;
        do {
            succ = I->next;
            PAUSE;
        } while (!succ); // wait for successor
    }
    succ->waiting = 0;
}

int is_free_mcs(mcs_lock *L ){
    if ((*L) == NULL) return 1;
    return 0;
}

/*
    Methods for easy lock array manipulation
 */

mcs_global_params* init_mcs_array_global(uint32_t num_locks) {
    DPRINT("Global mcs lock initialization\n");
    DPRINT("sizeof(mcs_qnode) is %lu\n",sizeof(mcs_qnode));
    uint32_t i;
    mcs_global_params* the_locks = (mcs_global_params*)malloc(num_locks * sizeof(mcs_global_params));
    for (i=0;i<num_locks;i++) {
        the_locks[i].the_lock=(mcs_lock*)malloc(sizeof(mcs_lock));
        *(the_locks[i].the_lock)=0;
    }
    return the_locks;
}


mcs_qnode** init_mcs_array_local(uint32_t thread_num, uint32_t num_locks) {
    set_cpu(thread_num);

    //init its qnodes
    uint32_t i;
    mcs_qnode** the_qnodes = (mcs_qnode**)malloc(num_locks * sizeof(mcs_qnode*));
    for (i=0;i<num_locks;i++) {
        the_qnodes[i]=(mcs_qnode*)malloc(sizeof(mcs_qnode));
    }
    return the_qnodes;

}

void end_mcs_array_local(mcs_qnode** the_qnodes, uint32_t size) {
    uint32_t i;
    for (i = 0; i < size; i++) {
        free(the_qnodes[i]);
    }
    free(the_qnodes);
}

void end_mcs_array_global(mcs_global_params* the_locks, uint32_t size) {
    uint32_t i;
    for (i = 0; i < size; i++) {
        free(the_locks[i].the_lock);
    }
    free(the_locks); 
}

mcs_global_params init_mcs_global() {
    DPRINT("Global mcs lock initialization\n");
    DPRINT("sizeof(mcs_qnode) is %lu\n",sizeof(mcs_qnode));
    mcs_global_params the_lock;
    the_lock.the_lock=(mcs_lock*)malloc(sizeof(mcs_lock));
    *(the_lock.the_lock)=0;
    return the_lock;
}


mcs_qnode* init_mcs_local(uint32_t thread_num) {
    set_cpu(thread_num);

    mcs_qnode*  the_qnodes=(mcs_qnode*)malloc(sizeof(mcs_qnode));
    return the_qnodes;

}

void end_mcs_local(mcs_qnode* the_qnodes) {
    free(the_qnodes);
}

void end_mcs_global(mcs_global_params the_locks) {
    free(the_locks.the_lock);
}

