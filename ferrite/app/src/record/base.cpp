#include "base.hpp"


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
    raw()->dpvt = data;
}
const void *Record::private_data() const {
    return raw()->dpvt;
}
void *Record::private_data() {
    return raw()->dpvt;
}
