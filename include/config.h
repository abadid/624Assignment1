#ifndef 	CONFIG_H_
#define 	CONFIG_H_

#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <unordered_map>
#include <cassert>

static struct option long_options[] = {
        {"max_outstanding", required_argument, NULL, 0},
        {"pool_size", required_argument, NULL, 1},
        {"exp_type", required_argument, NULL, 2},
        {"test", no_argument, NULL, 3},
        {"contention", no_argument, NULL, 4},
        { NULL, no_argument, NULL, 5},
};

enum exec_model {
        PROCESS_POOL = 0,
        PROCESS,
        THREAD_POOL,
        THREAD,
};

enum option_code {
        MAX_OUTSTANDING = 0,
        POOL_SIZE = 1,
        EXP_TYPE = 2,
        TEST = 3,
        CONTENTION = 4,
};

class expt_config {
 private:
        std::unordered_map<int, char*> _arg_map;

        /* parse cmd line args, and put them into _arg_map */
        void read_args(int argc, char **argv) {
                int index = -1;
                while (getopt_long_only(argc, argv, "", long_options, &index) != -1) {
                        
                        /* correct argument */
                        if (index != -1 && _arg_map.count(index) == 0) { 
                                _arg_map[index] = optarg;
                        } else if (index == -1) { /* unknown argument */
                                std::cerr << "Error. Unknown argument.\n";
                                exit(-1);
                        } else { /* duplicate argument */
                                std::cerr << "Error. Duplicate argument.\n";
                                exit(-1);
                        }
                        index = -1;
                }
        }
        
        void general_error() {
                std::cerr << "Required parameters:\n";
                std::cerr << "--" << long_options[EXP_TYPE].name << "\n";
        }

        void init_config() {
                
                if (_arg_map.count(EXP_TYPE) == 0) {
                        std::cerr << "Error. Missing some required arguments.\n";
                        general_error();
                        exit(0);
                }

                _type = (exec_model)atoi(_arg_map[EXP_TYPE]);
                if (atoi(_arg_map[EXP_TYPE]) < 0 || atoi(_arg_map[EXP_TYPE]) > 3) {
                        std::cerr << "Error. exp_type param must be between 0 and 3.\n";
                        std::cerr << "0 -- PROCESS_POOL\n1 -- PROCESS/REQUEST\n2 -- THREAD_POOL\n3 -- THREAD/REQUEST\n";
                        exit(0);
                }

                switch(atoi(_arg_map[EXP_TYPE])) {
                case 0:
                        _type = PROCESS_POOL;                        
                        break;
                case 1:
                        _type = PROCESS;
                        break;
                case 2:
                        _type = THREAD_POOL;
                        break;
                case 3:
                        _type = THREAD;
                        break;
                default:
                        assert(false);	/* Shouldn't get here */
                }
                if (_type == PROCESS_POOL || _type == THREAD_POOL) {
                        if (_arg_map.count(POOL_SIZE) == 0) {
                                std::cerr << "--pool_size argument required for ";
                                std::cerr << "process pool, and thread pool experiments\n";
                                exit(0);
                        } else if (_arg_map.count(MAX_OUTSTANDING) != 0) {
                                std::cerr << "--max_oustanding argument ignored for ";
                                std::cerr << "process pool, and thread pool experiments\n";
                                exit(0);
                        } else {
                                _pool_size = atoi(_arg_map[POOL_SIZE]);
                        }
                } else if (_type == THREAD || _type == PROCESS) {
                        if (_arg_map.count(MAX_OUTSTANDING) == 0) {
                                std::cerr << "--max_outstanding argument required for ";
                                std::cerr << "process/request and thread/request experiments.\n";
                                exit(0);
                        } else if (_arg_map.count(POOL_SIZE) != 0) {
                                std::cerr << "--pool_size argument ignored for ";
                                std::cerr << "process/request and thread/request experiments.\n";
                                exit(0);
                        } else {
                                _max_outstanding = atoi(_arg_map[MAX_OUTSTANDING]);
                        }
                } else {
                        assert(false); 	/* Shouldn't get here */
                }                
                _test = (_arg_map.count(TEST) > 0);
                _contention = (_arg_map.count(CONTENTION) > 0);
        }

 public:
        int _max_outstanding;
        int _pool_size;
        exec_model _type;
        bool _test;
        bool _contention;

        expt_config(int argc, char **argv) {
                read_args(argc, argv);
                init_config();
        }
};

#endif 		// CONFIG_H_

