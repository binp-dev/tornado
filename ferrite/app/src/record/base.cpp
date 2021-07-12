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

void Record::set_callback(callback_function callback) {
    CALLBACK *callback_struct_ptr = (CALLBACK *)callocMustSucceed(
        1, 
        sizeof(CALLBACK), 
        "Can't allocate memory for CALLBACK"
    );
    callbackSetCallback(callback, callback_struct_ptr);
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