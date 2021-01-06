#pragma once

#include <dbCommon.h>


class Record {
private:
    dbCommon *raw_;

public:
    inline explicit Record(dbCommon *raw) : raw_(raw) {}
    virtual ~Record() = default;

    Record(const Record &) = delete;
    Record &operator=(const Record &) = delete;
    Record(Record &&) = delete;
    Record &operator=(Record &&) = delete;

    const char *name() const;

//protected:
public:
    const dbCommon *raw() const;
    dbCommon *raw();

    void set_private_data(void *data);
    const void *private_data() const;
    void *private_data();
};

class Handler {
public:
    Handler() = default;
    virtual ~Handler() = default;

    Handler(const Handler &) = delete;
    Handler &operator=(const Handler &) = delete;
    Handler(Handler &&) = default;
    Handler &operator=(Handler &&) = default;
};
