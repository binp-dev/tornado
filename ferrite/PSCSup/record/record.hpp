#pragma once

#include <dbCommon.h>


class Record {
public:
    Record() = default;
    Record(const Record &record) = delete;
    Record &operator=(const Record &record) = delete;
    virtual ~Record() = default;
};
