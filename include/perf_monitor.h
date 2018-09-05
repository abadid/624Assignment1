#ifndef 	PERF_MONITOR_H_
#define 	PERF_MONITOR_H_

#include <time.h>
#include <launcher.h>

#define 	EXPT_LEN 	60

class perf_monitor {
 private:
        volatile uint64_t 	*_done;
        launcher 		*_lnchr;
        double 			*_results;        
        uint64_t		_prev_txns_elapsed;
        timespec 		_prev_time_elapsed;

	static timespec diff_time(timespec end, timespec begin);
        static double timespec_seconds(timespec t);
        void run();        

 public:
        perf_monitor(double *results, volatile uint64_t *done, launcher *lnchr);
        static void* execute_thread(void *arg);        
};

#endif		// PERF_MONITOR_H_
