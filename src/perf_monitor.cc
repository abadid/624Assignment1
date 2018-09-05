#include <perf_monitor.h>
#include <unistd.h>
#include <utils.h>
#include <cassert>

perf_monitor::perf_monitor(double *results, volatile uint64_t *done, 
                           launcher *lnchr)
{
        _results = results;
        _done = done;
        _lnchr = lnchr;
}

timespec perf_monitor::diff_time(timespec end, timespec start)
{
        timespec temp;
        if ((end.tv_nsec - start.tv_nsec) < 0) {
                temp.tv_sec = end.tv_sec - start.tv_sec - 1;
                temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
        } else {
                temp.tv_sec = end.tv_sec-start.tv_sec;
                temp.tv_nsec = end.tv_nsec-start.tv_nsec;
        }
        return temp;
}


void* perf_monitor::execute_thread(void *arg)
{
        perf_monitor *monitor;
        monitor = (perf_monitor*)arg;
        monitor->run();
        return NULL;
}

double perf_monitor::timespec_seconds(timespec t)
{
        double elapsed_sec;
        elapsed_sec =
                t.tv_sec + t.tv_nsec/1000000000.0;
        return elapsed_sec;
}

void perf_monitor::run()
{
        assert(*_done == 0);
        uint64_t total_executed, interval_executed;
        timespec now, interval;
        uint32_t i;
        double elapsed_sec;
        
        _prev_txns_elapsed = _lnchr->read_txns_executed();
        clock_gettime(CLOCK_REALTIME, &_prev_time_elapsed);
        barrier();
        for (i = 0; i < EXPT_LEN; ++i) {
                sleep(1);
                total_executed = _lnchr->read_txns_executed();
                clock_gettime(CLOCK_REALTIME, &now);
                barrier();
                interval = diff_time(now, _prev_time_elapsed);
                interval_executed = total_executed - _prev_txns_elapsed;
                elapsed_sec = timespec_seconds(interval);
                _results[i] = interval_executed / elapsed_sec;
                _prev_txns_elapsed = total_executed;
                _prev_time_elapsed = now;
        }
        assert(*_done == 0);
        fetch_and_increment(_done);
}
