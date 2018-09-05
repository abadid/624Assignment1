#include <request.h>
#include <cassert>
#include <algorithm>
#include <string.h>

#define      ROUNDUP(n, v)	((n) - 1 + (v) - ((n) - 1) % (v))

request::request(database *db, uint32_t nwrites, uint64_t *writeset, 
                 uint64_t *updates)
{
        _db = db;
        _nwrites = nwrites;
        _writeset = writeset;
        _updates = updates;
}

void request::set_database(database *db)
{
        _db = db;
}

size_t request::copy_size(request *req)
{
        size_t sz;
        uint32_t nfields;

        nfields = RECORD_SIZE / FIELD_SIZE;
        sz = sizeof(request);
        sz = ROUNDUP(sz, sizeof(uint64_t));
        return sz + sizeof(uint64_t)*(nfields + req->_nwrites);
}

void request::copy_request(char *buf, request *req)
{
        uint64_t offset;
        uint64_t *array_ptr;
        request *buf_req;
        uint32_t nfields;        

        buf_req = (request*)buf;

        /* Copy contents of request class */
        memcpy(buf, req, sizeof(request));
        
        /* Offset read/write set arrays appropriately */
        offset = ROUNDUP(sizeof(request), sizeof(uint64_t));
        assert(offset % sizeof(uint64_t) == 0);
        array_ptr = (uint64_t*)&buf[offset];        
        
        /* Copy writeset */
        memcpy(array_ptr, req->_writeset, sizeof(uint64_t)*req->_nwrites);
        buf_req->_writeset = array_ptr;
        
        /* Copy updates */
        nfields = RECORD_SIZE / FIELD_SIZE;
        array_ptr = (array_ptr + req->_nwrites);
        memcpy(array_ptr, req->_updates, sizeof(uint64_t*)*nfields);
        buf_req->_updates = array_ptr;
}

void request::do_write(char *record, uint64_t *updates)
{
        uint32_t nfields, i;
        uint64_t *rec_ptr;
        
        nfields = RECORD_SIZE / FIELD_SIZE;
        for (i = 0; i < nfields; ++i) {
                rec_ptr = (uint64_t*)(&record[i*FIELD_SIZE]);
                *rec_ptr += updates[i];
        }
        
}

/*
void request::do_read(char *record, char *buf)
{
        uint32_t nfields, i;
        uint64_t *buf_ptr, *rec_ptr;

        nfields = RECORD_SIZE / FIELD_SIZE;
        
        for (i = 0; i < nfields; ++i) {
                assert(i*FIELD_SIZE + sizeof(uint64_t) <= RECORD_SIZE);
                buf_ptr = (uint64_t*)(&buf[i*FIELD_SIZE]);
                rec_ptr = (uint64_t*)(&record[i*FIELD_SIZE]);
                *buf_ptr += *rec_ptr;
        }
}
*/

void request::execute()
{
        lock_records();
        txn();
        unlock_records();
}

void request::txn()
{
        assert(RECORD_SIZE % FIELD_SIZE == 0);
        uint32_t i;
        record *rec;
        
        for (i = 0; i < _nwrites; ++i) {
                if (i > 0)
                        assert(_writeset[i-1] != _writeset[i]);
                rec = _db->get_record(_writeset[i]);
                request::do_write(rec->_bytes, _updates);
        }

        /*
        memset(temp._bytes, 0x0, RECORD_SIZE);
        for (i = 0; i < _nreads; ++i) {
                rec = _db->get_record(_readset[i]);
                request::do_read(rec->_bytes, temp._bytes);                
        }
        
        for (i = 0; i < _nwrites; ++i) {
                rec = _db->get_record(_writeset[i]);
                request::do_write(rec->_bytes, temp._bytes);
        }
        */
}

void request::lock_records()
{
        uint32_t i;

        /* Ask students why you need to sort locks here */
        std::sort(_writeset, &_writeset[_nwrites]);
        for (i = 0; i < _nwrites; ++i) 
                _db->lock_record(_writeset[i]);
}

void request::unlock_records()
{
        uint32_t i;
        
        for (i = 0; i < _nwrites; ++i) 
                _db->unlock_record(_writeset[i]);
}
