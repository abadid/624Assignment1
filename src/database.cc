#include <database.h>
#include <cassert>
#include <utils.h>
#include <cstddef>
#include <cstdlib>
#include <string.h>
#include <iostream>

size_t database::db_sz()
{
        return (size_t)_num_records;
}

void database::compare(database *db1, database *db2)
{
        assert(db1->db_sz() == db2->db_sz());
        
        uint32_t i;
        record *record1, *record2;
        bool cmp;

        for (i = 0; i < db1->db_sz(); ++i) {
                record1 = db1->get_record(i);
                record2 = db2->get_record(i);
                cmp = memcmp(record1->_bytes, record2->_bytes, RECORD_SIZE);
                if (cmp != 0) {
                        std::cerr << "Database records do not match!\n";
                        assert(false);
                }
        }
}

void database::copy(database *copy_to, database *copy_from)
{
        assert(copy_to->db_sz() == copy_from->db_sz());
        
        uint32_t i;
        record *to_rec, *from_rec;
        for (i = 0; i < copy_to->db_sz(); ++i) {
                to_rec = copy_to->get_record(i);
                from_rec = copy_from->get_record(i);
                memcpy(to_rec->_bytes, from_rec->_bytes, RECORD_SIZE);
        }
}

database* database::create(uint32_t nrecords, bool multi_process)
{
        assert(RECORD_SIZE % FIELD_SIZE == 0);

        database *db_mem;
        record *record_mem; 
        sem_t *sem_mem;
        uint32_t i;
        int sem_flag;
        //        uint64_t *field_ptr;

        /* 
         * Allocate memory to database class, records, and their semaphores. 
         * 
         * Memory is allocated as anonymous mmap'ed buffers. Allocating as anon 
         * mmap'ed buffers allows the allocator (this process) to share memory 
         * with child processes.
         */
        db_mem = (database*)mmap(NULL, sizeof(database), PROT_FLAGS, MAP_FLAGS, 0, 0);
        assert(db_mem != MAP_FAILED);
        record_mem = (record*)mmap(NULL, sizeof(record)*nrecords, PROT_FLAGS, MAP_FLAGS, 0, 0);
        assert(record_mem != MAP_FAILED);
        sem_mem = (sem_t*)mmap(NULL, sizeof(sem_t)*nrecords, PROT_FLAGS, MAP_FLAGS, 0, 0);
        assert(sem_mem != MAP_FAILED);
                
        /*
         * If semaphores are used for inter-process synchronization, sem_flag is 1. 
         * If semaphores are used for inter-thread synchronization, sem_flag is 0.
         */
        if (multi_process)
                sem_flag = 1;
        else 
                sem_flag = 0;

        /* 
         * Initialize records and their associated semaphores. 
         * 
         * Each semaphore is initailized to 1. Setting its value to 1 allows it 
         * to function as a mutex lock. 
         */ 
        for (i = 0; i < nrecords; ++i) {
                sem_init(&sem_mem[i], sem_flag, 1);
                init_record((char*)&record_mem[i]);
        }
        
        /* Initialize db class state */
        db_mem->_num_records = nrecords;
        db_mem->_records = record_mem;
        db_mem->_locks = sem_mem;

        return db_mem;
}

/*
 * Dispose of the memory associated with the database.
 */
void database::destroy(database *db)
{
        uint32_t i;
        int err;
        
        /* OS needs to be explicitly notified of semaphores */
        for (i = 0; i < db->_num_records; ++i) {
                err = sem_destroy(&db->_locks[i]);
                assert(err == 0);
        }
        
        err = munmap(db->_locks, sizeof(sem_t)*db->_num_records);
        assert(err == 0);
        err = munmap(db->_records, sizeof(record)*db->_num_records);
        assert(err == 0);
        err = munmap(db, sizeof(database));
        assert(err == 0);
}

/* 
 * Initialize a buffer corresponding to a single record. 
 * 
 * The buffer's size is RECORD_SIZE. Each field is of size FIELD_SIZE.
 */
void database::init_record(char *buf)
{
        uint32_t i, nfields;
        uint64_t *field_ptr;

        nfields = RECORD_SIZE / FIELD_SIZE;
        for (i = 0; i < nfields; ++i) {
                field_ptr = (uint64_t*)(&buf[FIELD_SIZE*i]);
                *field_ptr = (uint64_t)rand();
        }        
}

/* 
 * Obtain mutually exclusive access to a record. Recall that each record's 
 * semaphore is initialized to 1. Thus, only a single process/thread at a time 
 * is allowed to obtain the semaphore. 
 */
void database::lock_record(uint64_t key)
{
        assert(key < (uint64_t)_num_records);
        sem_wait(&_locks[key]);
}

/* Relinquish mutually exclusive access to a record. */
void database::unlock_record(uint64_t key)
{
        assert(key < (uint64_t)_num_records);
        sem_post(&_locks[key]);
}

/* Return a reference to a record */
record* database::get_record(uint64_t key)
{
        return &_records[key];
}
