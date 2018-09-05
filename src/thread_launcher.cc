#include <thread_launcher.h>
#include <stdlib.h>
#include <utils.h>
#include <cassert>

thread_launcher::thread_launcher(int max_outstanding)
        : launcher()
{
        /* 
         * _done_sem ensures that the number of outstanding requests never 
         * exceeds max_outstanding. The process_launcher class' _done_sem field 
         * serves the exact same purpose.
         */
        sem_init(&_done_sem, INTRA_PROC_SEM, max_outstanding - 1);
        
        /* 
         * _targ_list_sem is used as a mutex lock. It is used to protect the 
         * value of _targ_list, which is a linked-list of thread_args 
         * corresponding to threads (or requests) that have finished executing.
         * 
         * _targ_list is initialized to NULL.
         */
        sem_init(&_targ_list_sem, INTRA_PROC_SEM, 1);
        _targ_list = NULL;
}

void thread_launcher::execute_request(request *req)
{
        pthread_t *thread;
        thread_arg *arg, *temp;
        int err;

        /* 
         * Track the number of requests issued. launcher::execute_request is 
         * called from every sub-class that derives from launcher.  
         */
        launcher::execute_request(req);
        
        /* 
         * Create a new thread to assign the request to. 
         */
        thread = (pthread_t*)malloc(sizeof(pthread_t));
        
        /* Setup the thread's thread_arg struct. */
        arg = gen_thread_arg(req, thread, &_done_sem, &_targ_list_sem, 
                             &_targ_list, _txns_executed);
        
        /* Create a thread to execute the request */
        err = pthread_create(thread, NULL, thread_launcher::executor_fn, arg);
        assert(err == 0);

        /* 
         * Wait until the value of _done_sem exceeds 0. This serves the same 
         * function as waiting on _done_sem in process_launcher.
         */
        sem_wait(&_done_sem);
        
        /*
         * _targ_list is a linked-list of thread_args corresponding to requests 
         * that have finished executing. Free up the memory we allocated for 
         * these args.
         * 
         * Note that _targ_list_sem performs a very different function from that
         * of _done_sem. We use _targ_list_sem as a mutual exclusion lock to 
         * protect _targ_list. It's value is initialized to 1 (in the 
         * thread_launcher constructor), which guarantees that only a single 
         * thread can ever execute code between calls to sem_wait and sem_post. 
         */
        sem_wait(&_targ_list_sem);
        arg = _targ_list;
        _targ_list = NULL;
        sem_post(&_targ_list_sem);        
        while (arg != NULL) {
                temp = arg;
                arg = arg->_link;
                pthread_join(*temp->_thread_id, NULL);
                free(temp->_thread_id);
                free(temp);
        }
}

/*
 * executor_fn is executed by each thread created to execute a request. 
 */
void* thread_launcher::executor_fn(void *arg)
{
        thread_arg *targ;
        request *req;
        
        targ = (thread_arg*)arg;
        req = targ->_request;
        
        /* Execute the request */
        req->execute();
        
        /* 
         * Return the thread_arg back to the launcher thread by linking it into 
         * _targ_list. As in thread_launcher::execute_request, _targ_list_sem 
         * is used as a mutex lock to protect _targ_list from concurrent 
         * modifications.
         */
        sem_wait(targ->_targ_list_sem);
        targ->_link = *targ->_targ_list;
        *targ->_targ_list = targ;
        sem_post(targ->_targ_list_sem);
        
        /* 
         * Notify the launcher thread that the request has finished executing. 
         * Recall that _done_sem is used to enforce the requirement that the 
         * number of outstanding requests does not exceed _max_outstanding.
         */          
        sem_post(targ->_done_sem);        
        
        /* Atomically increment the number of executed transactions */
        fetch_and_increment(targ->_txns_executed);
        return NULL;
}

/* 
 * Allocate and intialize a thread_arg struct. This function is simply allows 
 * the caller to succinctly initialize a thread_arg.
 */
thread_arg* thread_launcher::gen_thread_arg(request *req, pthread_t *thread_id, 
                                            sem_t *done_sem, 
                                            sem_t *targ_list_sem,
                                            thread_arg **targ_list, 
                                            volatile uint64_t *txns_executed)
{
        thread_arg *arg;
        
        arg = (thread_arg*)malloc(sizeof(thread_arg));
        arg->_request = req;
        arg->_thread_id = thread_id;
        arg->_done_sem = done_sem;
        arg->_targ_list_sem = targ_list_sem;
        arg->_targ_list = targ_list;
        arg->_link = NULL;
        arg->_txns_executed = txns_executed;
        return arg;
}


