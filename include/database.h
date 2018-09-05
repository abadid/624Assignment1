#ifndef 	DATABASE_H_
#define 	DATABASE_H_

#define 	RECORD_SIZE 	1000
#define 	FIELD_SIZE 	100

#include <stdint.h>
#include <semaphore.h>

struct record {
        char _bytes[RECORD_SIZE];
};

class database {
 private:
        /* Disable constructors */
        database();
        database(const database&);
        database& operator=(const database&);

 protected:        
        uint32_t 	_num_records;
        record 		*_records;
        sem_t 		*_locks;

        //        static void init_records(uint32_t nrecords);
        static void init_record(char *buf);

 public:
        static database* create(uint32_t nrecords, bool multi_process);
        static void destroy(database *db);
        static void compare(database *db1, database *db2);
        static void copy(database *copy_to, database *copy_from);

        record* get_record(uint64_t key);
        void lock_record(uint64_t key);
        void unlock_record(uint64_t key);
        size_t db_sz();
};

#endif 		// DATABASE_H_
