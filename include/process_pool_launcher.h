#ifndef 	PROCESS_POOL_LAUNCHER_H_
#define 	PROCESS_POOL_LAUNCHER_H_

#define 	RQST_BUF_SZ	(1<<10)	/* 1K per request */

#include <launcher.h>
#include <semaphore.h>
#include <request.h>

struct proc_state;

struct proc_mgr {
        sem_t 		*_done_sem;		/* # idle procs */
        sem_t 		*_free_list_sem;	/* locks free list below */
        proc_state	*_free_list;		/* free list of processes */
};

struct proc_state {
        request			*_request;		/* request location */
        sem_t 			*_proc_sem;		/* signals new request */
        proc_mgr		*_launcher_state;	/* global pool mgmt state */
        volatile uint64_t 	*_txns_executed;	/* ptr to txn executed counter */
        proc_state 		*_list_ptr;		/* links proc states */
};

class process_pool_launcher : public launcher {
 private:
        uint32_t 	_pool_sz;
        proc_mgr 	*_launcher_state;

        static void executor_fn(proc_state *st);/* executed by pooled procs */

 public:
        process_pool_launcher(uint32_t pool_sz);
        void execute_request(request *req);
};

#endif 		// PROCESS_POOL_LAUNCHER_H_
