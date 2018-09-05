#include <utils.h>
#include <process_launcher.h>
#include <cstddef>
#include <climits>
#include <cassert>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <signal.h>

process_launcher::process_launcher(int max_outstanding)
        : launcher()
{
        assert(max_outstanding < INT_MAX && max_outstanding > 0);
        
        int err;
        struct sigaction sigchild_action;
        
        /*
         * Prevent children turning into zombies. When a child exits, we want 
         * the operating system to immediately free any resources allocated to 
         * the child. Doing this allows us to avoid waiting on a child to free 
         * resources. 
         */
        sigchild_action.sa_handler = SIG_IGN;
        sigemptyset(&sigchild_action.sa_mask);
        sigchild_action.sa_flags = 0;
        err = sigaction(SIGCHLD, &sigchild_action, 0);
        assert(err == 0);

        /* Initialize fields */
        _max_outstanding = max_outstanding;
        
        /* 
         * Map _done_sem into a memory segment that is shared by this 
         * process, and any subsequent forked processes. _done_sem must be 
         * shared between this process and its children. We use _done_sem to 
         * control the maximum number of in-flight processes (as specified in 
         * _max_outstanding). Note that the call to mmap only _allocates_ memory
         * for _done_sem. We still need to intialize _done_sem appropriately. 
         */ 
        _done_sem = (sem_t*)mmap(NULL, sizeof(sem_t), PROT_FLAGS, MAP_FLAGS, 0, 0);
        
        /* 
         * Initialize _done_sem. The second argument, INTER_PROC_SEM, indicates 
         * that the semaphore is used for inter-process synchronization 
         * (see include/utils.h). The third argument (max_outstanding - 1) 
         * specifies the intial value of the semaphore. 
         */

        sem_init(_done_sem, INTER_PROC_SEM, max_outstanding-1);
}

void process_launcher::execute_request(request *req)
{
        /* Child process identifier */
        pid_t pid;        
        
        /* 
         * Track the number of requests issued. launcher::execute_request is 
         * called from every sub-class that derives from launcher.  
         */
        launcher::execute_request(req);
        
        /* fork() creates a new child process. 
         * 
         * fork() returns 0 to the child process (the code-path corresponding to
         * the "if" branch). 
         * 
         * fork() returns a non-zero pid to the parent process (the code-path 
         * corresponding to the "else" branch).
         */
        if ((pid = fork()) == 0) { /* This code is executed in the child */


                /* 
                 * Execute the request. We treat requests' "execute" function 
                 * as a black-box. Just make sure it's called at the appropriate
                 * place. 
                 */ 
                req->execute();

                /* Atomically increment the number of executed transactions */
                fetch_and_increment(this->_txns_executed);
                
                /* 
                 * sem_post increments the value of _done_sem, which tells the
                 * parent process that an outstanding request has finished 
                 * executing 
                 */
                sem_post(_done_sem);
                exit(0);
        } else { /* This code is executed in the parent */
                
                /* 
                 * Assert that the fork() call did not fail. fork() returns a 
                 * negative pid on an error. 
                 */
                assert(pid != -1);

                /* 
                 * sem_wait waits until the value of _done_sem is greater than 
                 * 0, and then decrements its value by 1. This guarantees that 
                 * the number of outstanding requests is always at most 
                 * _max_outstanding. Recall that _done_sem's value  was 
                 * initialized to (_max_outstanding - 1). 
                 */
                sem_wait(_done_sem);
        }
}
