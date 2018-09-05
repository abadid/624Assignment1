#ifndef 	THREAD_LAUNCHER_H_
#define 	THREAD_LAUNCHER_H_

#include <launcher.h>
#include <deque>
#include <pthread.h>

/* 
 * thread_args are used to communicate requests and coordination info between 
 * the launcher thread and request execution threads.  
 */
struct thread_arg {
        request			*_request;	/* ptr to request to execute */
        pthread_t 		*_thread_id;	/* thread id of the execution thread */
        sem_t			*_done_sem;	/* ptr to launcher's _done_sem semaphore */
        sem_t 			*_targ_list_sem;/* ptr to launcher's _targ_list_sem semaphore */
        volatile uint64_t 	*_txns_executed;/* ptr to txns executed counter */
        thread_arg 		*_link;		/* links together entries in _targ_list */
        thread_arg		**_targ_list;   /* points to launcher's list of executed requests */
};

class thread_launcher : public launcher {
 private:
        sem_t		_done_sem;	/* Ensures that the max outstanding requests stays bounded */
        sem_t		_targ_list_sem;	/* Protects the list of executed thread_args below */
        thread_arg	*_targ_list;	/* Linked-list of executed thread_args */
        
        /* Function executed by a spawned thread */
        static void* executor_fn(void *arg);

        /* Convenience function to intialize a thread_arg */
        static thread_arg* gen_thread_arg(request *req, pthread_t *thread_id, 
                                          sem_t *outstanding_sem, 
                                          sem_t *done_sem,
                                          thread_arg **done_requests,
                                          volatile uint64_t *txns_executed);

 public:
        thread_launcher(int num_outstanding);
        
        /* Execute a new request */
        void execute_request(request *req);
};

#endif 		// THREAD_LAUNCHER_H_
