#ifndef 		THREAD_POOL_LAUNCHER_H_
#define 		THREAD_POOL_LAUNCHER_H_

#include <launcher.h>
#include <semaphore.h>
#include <pthread.h>

struct thread_state {        
        request	       		*_req;		/* request to process */
        sem_t 			_thread_sem;	/* notify thread of a request */
        sem_t			*_done_sem;	/* # idle threads */
        pthread_t		*_thread_id;	/* identifier of current thread */
        sem_t 			*_list_sem;	/* lock thread free list */
        thread_state		**_free_list;	/* ptr to launcher's free list */
        volatile uint64_t 	*_txns_executed;/* ptr to txn executed counter */
        thread_state		*_list_ptr;	/* thread_state free list link */
};

class thread_pool_launcher : public launcher {
 private:
        uint32_t 	_pool_sz;	/* pool size */	
        sem_t 		_done_sem; 	/* # idle threads */
        sem_t 		_free_list_sem;	/* free list lock */
        thread_state 	*_free_list;	/* free list of idle threads */
        
        /* Executed by threads in the thread pool */
        static void* executor_fn(void *arg);

 public:
        thread_pool_launcher(int pool_sz);
        void execute_request(request* req);
        
};

#endif 			// THREAD_POOL_LAUNCHER_H_
