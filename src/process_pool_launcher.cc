#include <process_pool_launcher.h>
#include <utils.h>
#include <stdlib.h>
#include <cassert>
#include <unistd.h>
#include <iostream>

process_pool_launcher::process_pool_launcher(uint32_t nprocs)
        : launcher()
{
        uint32_t i;
        sem_t *temp_sem;
        char *req_bufs;
        sem_t *proc_sems;
        proc_state *pstates;

        /* 
         * Setup launcher state. Initialize the launcher's proc_mgr struct. 
         */
        _launcher_state = (proc_mgr*)mmap(NULL, sizeof(proc_mgr), PROT_FLAGS, MAP_FLAGS, 0, 0);
        temp_sem = (sem_t*)mmap(NULL, 2*sizeof(sem_t), PROT_FLAGS, MAP_FLAGS, 0, 0);
        
        /*
         * _done_sem is used to track the number of idle processes. 
         * process_pool_launcher's use of _done_sem is similar to 
         * process_launcher's use of _done_sem to control the number of 
         * outstanding processes
         */
        _launcher_state->_done_sem = temp_sem;
        
        /* 
         * YOUR CODE HERE
         * 
         * Initialize _launcher_state->_done_sem with an appropriate starting 
         * value using sem_init.
         * 
         * Hint: Since _done_sem is shared among multiple processes, its second 
         * argument must be set appropriately.
         */


        /* 
         * _free_list_sem is used as a mutex lock. It protects the free-list of 
         * idle processes from concurrent modifications. 
         */
        _launcher_state->_free_list_sem = &temp_sem[1];  
        
        /* 
         * YOUR CODE HERE
         * 
         * Initialize _free_list_sem with an appropriate starting value using 
         * sem_init.
         * 
         * Hint: Since _free_list_sem is shared among multiple processes, its 
         * second argument must be set appropriately.
         */

        
        /* 
         * Setup request buffers. Each process in the pool has its own private 
         * request buffer. In order to assign a request to a process the 
         * launcher process must copy the request into the process' request 
         * buffer.
         */
        req_bufs = (char*)mmap((NULL), nprocs*RQST_BUF_SZ, PROT_FLAGS, MAP_FLAGS, 0, 0);
        
        /* 
         * Each process in the pool has a corresponding semaphore which 
         * is used to signal the process to begin executing a new request. 
         * We initialize the value of this semaphore to 0.
         */
        proc_sems = (sem_t*)mmap(NULL, nprocs*sizeof(sem_t), PROT_FLAGS, MAP_FLAGS, 0, 0);
        for (i = 0; i < nprocs; ++i) 
                sem_init(&proc_sems[i], INTER_PROC_SEM, 0);

        /* 
         * Setup proc_states for each process in the pool. proc_states are 
         * linked via the _list_ptr field. The free-list of proc_states is 
         * stored in _launcher_state->_free_list. 
         */
        pstates = (proc_state*)mmap(NULL, sizeof(proc_state)*nprocs, PROT_FLAGS, MAP_FLAGS, 0, 0);
        for (i = 0; i < nprocs; ++i) {
                pstates[i]._request = (request*)&req_bufs[i*RQST_BUF_SZ];
                pstates[i]._proc_sem = &proc_sems[i];
                pstates[i]._launcher_state = _launcher_state;
                pstates[i]._txns_executed = _txns_executed;
                pstates[i]._list_ptr = &pstates[i+1];
        }
        pstates[i-1]._list_ptr = NULL;
        _launcher_state->_free_list = pstates;

        /* 
         * YOUR CODE HERE
         * 
         * Launch the processes in the process pool. 
         */
}

void process_pool_launcher::executor_fn(proc_state *st)
{
        while (true) {
                /*
                 * YOUR CODE HERE
                 * 
                 * Wait for a new request, and execute it. After executing the 
                 * request return proc_state to the launcher's proc_state 
                 * free-list. Remember to signal/wait the appropriate _proc_sem,
                 * and _done_sem.
                 * 
                 * When your code is ready, remove the assert(false) statement 
                 * below.
                 */
                assert(false);
           
                st->_request->execute();
                fetch_and_increment(st->_txns_executed);

        }
        exit(0);
}


void process_pool_launcher::execute_request(request *req)
{
        proc_state *st;
        launcher::execute_request(req);

        st = NULL;
        /* 
         * YOUR CODE HERE
         * 
         * Find an idle process from _free_list, copy the request into the 
         * process' request buffer, and execute the request on the idle process.
         * 
         * Hint: Use the process' _proc_sem, and the launcher's _done_sem to 
         * initiate a new request on the idle process, and ensure that there 
         * exist idle processes.
         */

        /* Copy request into proc's request buffer */
        assert(request::copy_size(req) <= RQST_BUF_SZ);
        assert(st != NULL);
        request::copy_request((char*)st->_request, req);
}
