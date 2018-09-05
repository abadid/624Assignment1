#include <database.h>
#include <request.h>
#include <launcher.h>
#include <thread_launcher.h>
#include <thread_pool_launcher.h>
#include <process_launcher.h>
#include <process_pool_launcher.h>
#include <config.h>
#include <set>
#include <utils.h>
#include <iostream>
#include <fstream>
#include <perf_monitor.h>

#define 	TXN_SZ			50
#define 	LOW_DATABASE_SZ 	5000000
#define 	HIGH_DATABASE_SZ 	500
#define 	DRY_RUN_SZ		1000
#define 	NUM_REQS		2000000

const uint32_t rand_seed = 0xdeadbeef;
const char *output_file = "results.txt";

uint64_t gen_unique(uint64_t max, std::set<uint64_t> *seen)
{
        uint64_t gen;
        
        while (true) {
                gen = ((uint64_t)rand()) % max;
                if (seen->count(gen) == 0) {
                        seen->insert(gen);
                        break;
                }
        }
        assert(seen->count(gen) > 0);
        assert(gen < max);
        return gen;
}

request* generate_single_request(database *db)
{
        std::set<uint64_t> seen_keys;
        uint64_t *writeset, *updates;
        uint32_t i, nfields;
        request *rqst;
        
        seen_keys.clear();
        /* Generate writeset */
        writeset = (uint64_t*)malloc(sizeof(uint64_t)*TXN_SZ);
        for (i = 0; i < TXN_SZ; ++i) {
                writeset[i] = gen_unique(db->db_sz(), &seen_keys);
        }
        
        /* Generate updates */
        nfields = RECORD_SIZE / FIELD_SIZE;
        updates = (uint64_t*)malloc(sizeof(uint64_t)*nfields);
        for (i = 0; i < nfields; ++i) 
                updates[i] = (uint64_t)rand();
        
        /* Generate request */
        rqst = new request(db, TXN_SZ, writeset, updates);
        return rqst;
}

request** generate_requests(database *db, uint32_t num_requests)
{
        request **ret;
        uint32_t i;

        ret = (request**)malloc(sizeof(request*)*num_requests);
        for (i = 0; i < num_requests; ++i) 
                ret[i] = generate_single_request(db);
        return ret;
}

pthread_t* run_experiment(perf_monitor *monitor, launcher *lnchr, 
                          request ***requests, 
                          volatile uint64_t *done_flag)
{
        int err;
        uint32_t i;
        pthread_t *ret;

        ret = (pthread_t*)malloc(sizeof(pthread_t));
        
        /* Do dry run */
        for (i = 0; i < DRY_RUN_SZ; ++i) 
                lnchr->execute_request(requests[0][i]);
        lnchr->wait_outstanding();
        std::cerr << "Done dry run\n";

        /* 
         * Run the "real" experiment. We create a new thread to monitor the 
         * progress of requests. See include/perf_monitor.h and 
         * src/perf_monitor.cc. The perf_monitor thread samples the number of 
         * _txns_executed counter in the launcher class in one second 
         * intervals, and sets the done_flag below after 60 seconds. 
         */
        err = pthread_create(ret, NULL, perf_monitor::execute_thread, monitor);
        assert(err == 0);
        barrier();
        i = 0;
        while (true) {
                barrier();
                if (*done_flag != 0)
                        break;
                barrier();
                lnchr->execute_request(requests[1][i % NUM_REQS]);
                i += 1;
        }
        return ret;        
}

void run_test(expt_config conf)
{
        uint32_t test_db_sz, num_requests, i;
        database *db_test, *db_simple;
        bool multi_process;
        request **reqs;
        launcher *test;
        
        /* Use a small db to guarantee conflicts */
        test_db_sz = 500;
        multi_process = (conf._type == PROCESS_POOL || conf._type == PROCESS);
        
        /* Create database */
        db_test = database::create(test_db_sz, multi_process);
        db_simple = database::create(test_db_sz, multi_process);
        database::copy(db_simple, db_test);
        
        /* Gen requests */
        num_requests = 10000;
        reqs = generate_requests(db_test, num_requests);
        
        /* Create launcher */
        switch (conf._type) {
        case PROCESS_POOL:
                test = new process_pool_launcher(conf._pool_size);
                break;
        case PROCESS:
                test = new process_launcher(conf._max_outstanding);
                break;
        case THREAD:
                test = new thread_launcher(conf._max_outstanding);
                break;
        case THREAD_POOL:
                test = new thread_pool_launcher(conf._pool_size);
                break;
        default:
                assert(false);	/* Shouldn't get here */        
        }
        
        /* Run test */
        for (i = 0; i < num_requests; ++i) 
                test->execute_request(reqs[i]);
        test->wait_outstanding();
        
        /* Switch database */
        for (i = 0; i < num_requests; ++i) 
                reqs[i]->set_database(db_simple);

        /* Run sequential test */
        for (i = 0; i < num_requests; ++i) 
                reqs[i]->execute();
        
        /* Compare databases to ensure they're the same */
        database::compare(db_test, db_simple);
        std::cerr << "Test passed!\n";
}

void write_results(expt_config conf, double *results)
{
        double throughput;
        std::ofstream result_file;
        uint32_t i;

        throughput = 0;
        for (i = 0; i < EXPT_LEN; ++i) 
                throughput += results[i];

        throughput = throughput / (EXPT_LEN*1.0);

        std::cerr << "Throughput: " <<  throughput << "\n";
        result_file.open(output_file, std::ios::app | std::ios::out);
        
        switch (conf._type) {
        case PROCESS_POOL:
                result_file << "process_pool ";
                result_file << "pool_size:" << conf._pool_size << " ";
                break;
        case PROCESS:
                result_file << "process ";
                result_file << "max_oustanding:" << conf._max_outstanding << " ";
                break;
        case THREAD_POOL:
                result_file << "thread_pool ";
                result_file << "pool_size:" << conf._pool_size << " ";
                break;
        case THREAD:
                result_file << "thread ";
                result_file << "max_outstanding:" << conf._max_outstanding << " ";
                break;
        default:
                assert(false);
        }
        
        result_file << "throughput:" << throughput << " ";        
        if (conf._contention == false) 
                result_file << "low_contention ";
        else
                result_file << "high_contention ";
        result_file << "\n";
        result_file.close();
}

int main(int argc, char **argv)
{
        database *db;
        expt_config conf(argc, argv);
        bool multi_process;
        request **txns[2];
        launcher *lnchr;
        volatile uint64_t done;
        double *results;
        perf_monitor *monitor;
        pthread_t *monitor_thread;
        uint64_t db_sz;

        srand(rand_seed);

        if (conf._test == true) { 
                run_test(conf);
                return 0;
        }

        /* Initialize database */
        multi_process = (conf._type == PROCESS || conf._type == PROCESS_POOL);
        if (conf._contention == true)
                db_sz = HIGH_DATABASE_SZ;
        else
                db_sz = LOW_DATABASE_SZ;
        db = database::create(db_sz, multi_process);
        
        /* Generate requests to process */
        txns[0] = generate_requests(db, DRY_RUN_SZ);
        txns[1] = generate_requests(db, NUM_REQS);
        
        /* Initialize the appropriate launcher */
        if (conf._type == PROCESS) 
                lnchr = new process_launcher(conf._max_outstanding);
        else if (conf._type == PROCESS_POOL)
                lnchr = new process_pool_launcher(conf._pool_size);
        else if (conf._type == THREAD)
                lnchr = new thread_launcher(conf._max_outstanding);
        else if (conf._type == THREAD_POOL)
                lnchr = new thread_pool_launcher(conf._pool_size);
        else 
                assert(false);

        /* Measure throughput, and report results */
        results = (double*)malloc(sizeof(double)*EXPT_LEN);
        done = 0;
        barrier();
        monitor = new perf_monitor(results, &done, lnchr);
        monitor_thread = run_experiment(monitor, lnchr, txns, &done);
        free(monitor_thread);
        write_results(conf, results);
}
