#include <launcher.h>
#include <utils.h>
#include <cassert>
#include <stddef.h>
#include <string.h>

launcher::launcher()
{
        _txns_executed = (volatile uint64_t*)mmap(NULL, CACHE_LINE_SZ, PROT_FLAGS, MAP_FLAGS, 0, 0);
        assert((void*)_txns_executed != MAP_FAILED);
        memset((void*)_txns_executed, 0x0, CACHE_LINE_SZ);
        assert(*_txns_executed == 0);
        _num_requests = 0;
}

launcher::~launcher()
{
        int err;
        err = munmap((void*)_txns_executed, CACHE_LINE_SZ);
        assert(err == 0);
}

uint64_t launcher::read_txns_executed()
{
        uint64_t num_executed;
        barrier();
        num_executed = *_txns_executed;
        barrier();
        return num_executed;
}

void launcher::incr_done_txns(volatile uint64_t *done_ptr)
{
        fetch_and_increment(done_ptr);
}

void launcher::execute_request(__attribute__((unused)) request *req)
{
        _num_requests += 1;
}

void launcher::wait_outstanding()
{
        while (true) {
                barrier();
                if (*_txns_executed == _num_requests)
                        break;
                barrier();
                asm volatile("pause;":::);
        }
}
