#ifndef 	PROCESS_LAUNCHER_H_
#define 	PROCESS_LAUNCHER_H_

#include <launcher.h>
#include <semaphore.h>

/*
 * process_launcher implements the process-per-request execution model. 
 */
class process_launcher : public launcher {
 private:
        
        /* indicates the max # outstanding requests */
        int	_max_outstanding;	

        /* 
         * semaphore guarantees that max outstanding requests never exceeds 
         * _max_outstanding.
         */
        sem_t 	*_done_sem;		

 public:

        process_launcher(int max_outstanding);
        
        /* Run a single request */
        void execute_request(request *req);
};

#endif		// PROCESS_LAUNCHER_H_
