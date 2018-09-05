#ifndef 	REQUEST_H_
#define		REQUEST_H_

#include <database.h>
#include <stdint.h>
#include <vector>


class request {
 private:
        request();
        request(const request&);
        request& operator=(const request&);

 protected:
        database *_db;
        uint32_t _nwrites;
        uint64_t *_writeset;
        uint64_t *_updates;

        static void do_write(char *record, uint64_t *updates);

        void lock_records();
        void txn();
        void unlock_records();

 public:
        request(database *db, uint32_t nwrites, uint64_t *writeset, 
                uint64_t *updates);

        static void copy_request(char *buf, request *req);
        static size_t copy_size(request *req);
        void execute();
        void set_database(database *db);
};

#endif 		// REQUEST_H_
