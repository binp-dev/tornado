#include <cantProceed.h>
#include <dbAccess.h>

#include "base.hpp"

//===========================
//  Record
//===========================

const char *Record::name() const {
    return raw()->name;
}

const dbCommon *Record::raw() const {
    return raw_;
}
dbCommon *Record::raw() {
    return raw_;
}

void Record::set_private_data(void *data) {
    dptr_struct()->data = data;
}
const void *Record::private_data() const {
    return dptr_struct()->data;

}
void *Record::private_data() {
    return dptr_struct()->data;
}

bool Record::pact() { 
    return raw()->pact ? true : false;
}

bool Record::pact() const { 
    return raw()->pact ? true : false;
}

void Record::set_pact(bool pact) {
    raw()->pact = pact ? TRUE : FALSE;
}

struct typed_rset *Record::rset() {
    return static_cast<struct typed_rset *>(raw()->rset);
}

void Record::scan_lock() {
    dbScanLock(raw());
}
void Record::scan_unlock() {
    dbScanUnlock(raw());
}

void Record::process_record() {
    (*rset()->process)(raw());
}

// void Record::set_callback(std::function<void(CALLBACK *)> callback) {
//     callback_func_ptr *function_ptr = callback.target<void(*)(CALLBACK *)>();

//     CALLBACK *callback_struct_ptr = (CALLBACK *)callocMustSucceed(
//         1, 
//         sizeof(CALLBACK), 
//         "Can't allocate memory for CALLBACK"
//     );
//     callbackSetCallback(*function_ptr, callback_struct_ptr);
//     callbackSetUser(raw(), callback_struct_ptr);
//     callbackSetPriority(priorityLow, callback_struct_ptr);

//     dptr_struct()->callback_struct_ptr = callback_struct_ptr;
// }

void Record::set_callback(std::function<callback_func_t> callback) {
    callback_func_t **function_ptr = callback.target<callback_func_t *>();

    CALLBACK *callback_struct_ptr = (CALLBACK *)callocMustSucceed(
        1, 
        sizeof(CALLBACK), 
        "Can't allocate memory for CALLBACK"
    );
    callbackSetCallback(*function_ptr, callback_struct_ptr);
    callbackSetUser(raw(), callback_struct_ptr);
    callbackSetPriority(priorityLow, callback_struct_ptr);

    dptr_struct()->callback_struct_ptr = callback_struct_ptr;
}

void Record::request_callback() {
    CALLBACK *callback_struct_ptr = dptr_struct()->callback_struct_ptr;
    if (callback_struct_ptr == nullptr) { return; }
    callbackRequest(callback_struct_ptr);
}

Record::PrivateData *Record::dptr_struct() {
    return static_cast<Record::PrivateData *>(raw()->dpvt);
}

Record::PrivateData *Record::dptr_struct() const {
    return static_cast<Record::PrivateData *>(raw()->dpvt);
}

//===========================
//  Handler
//===========================
Handler::Handler(dbCommon *raw_record) : asyn_process(false), raw_record_(raw_record) {}

Handler::Handler(dbCommon *raw_record, bool asyn_process)
: asyn_process(asyn_process), raw_record_(raw_record) {}

// void mbboDirect_record_write_callback(CALLBACK *callback_struct_pointer) {
//     struct dbCommon *record_pointer;
//     // Line below doesn't work, because in C++ cast result is not lvalue
//     // callbackGetUser((void *)(record_pointer), callback_struct_pointer);
//     record_pointer = (dbCommon *)callback_struct_pointer->user;

//     MbboDirect mbbo_direct_record((mbboDirectRecord *)record_pointer);
// #ifdef RECORD_DEBUG
//     std::cout << mbbo_direct_record.name() << 
//     " mbboDirect_record_write_callback() Thread id = " << 
//     pthread_self() << std::endl << std::flush;
// #endif

//     mbbo_direct_record.scan_lock();
//     mbbo_direct_record.write();
//     mbbo_direct_record.process_record();
//     mbbo_direct_record.scan_unlock();
// }


void Handler::epics_read_write() {
    Record record(raw_record_);
    if (asyn_process) {
        if (record.pact() != true) {
            record.set_pact(true);
            record.request_callback();
        }
    } else {
        read_write_func();
    }
}

//===========================
//  ReadHandler
//===========================
ReadHandler::ReadHandler(dbCommon *raw_record) : Handler(raw_record) {}

ReadHandler::ReadHandler(dbCommon *raw_record, bool asyn_process)
: Handler(raw_record, asyn_process) {}

//===========================
//  WriteHandler
//===========================
WriteHandler::WriteHandler(dbCommon *raw_record) : Handler(raw_record) {}

WriteHandler::WriteHandler(dbCommon *raw_record, bool asyn_process)
: Handler(raw_record, asyn_process) {}