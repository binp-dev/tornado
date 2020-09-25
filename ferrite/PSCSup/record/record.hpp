#pragma once

#include <dbCommon.h>


class Record {
private:
    dbCommon *_raw;

public:
    Record(dbCommon *raw) : _raw(raw) {};
    virtual ~Record() = default;

    Record(const Record &) = delete;
    Record &operator=(const Record &) = delete;

    const dbCommon *raw() const {
        return _raw;
    }
    dbCommon *raw() {
        return _raw;
    }

    const char *name() const {
        return raw()->name;
    }

    void set_private_data(void *data) {
        raw()->dpvt = data;
    }
    const void *private_data() const {
        return raw()->dpvt;
    }
    void *private_data() {
        return raw()->dpvt;
    }
};

class Handler {
public:
    Handler() = default;
    virtual ~Handler() = default;

    Handler(const Handler &) = delete;
    Handler &operator=(const Handler &) = delete;
};
