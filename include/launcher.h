#ifndef 	LAUNCHER_H_
#define 	LAUNCHER_H_

#include <stdint.h>
#include <request.h>

/* Size of a single cache line. 64 bytes */
#define 	CACHE_LINE_SZ 		64

class launcher {
 protected:
        volatile uint64_t 	*_txns_executed;	/* # txns completed */
        uint64_t 		_num_requests;		/* # txns issued */

        /* Atomically increment the 64-bit int done_ptr points to */
        static void incr_done_txns(volatile uint64_t *done_ptr);

 public:
        launcher();
        ~launcher();        

        /* Returns the latest value of *_txns_executed */
        uint64_t read_txns_executed();
        
        /* Wait for outstanding requests to finish executing */
        void wait_outstanding();
        
        /* Execute a single request */
        virtual void execute_request(request *req);
};

#endif 		// LAUNCHER_H_
