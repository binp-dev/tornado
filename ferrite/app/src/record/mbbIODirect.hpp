#pragma once

#include <epicsTypes.h>
#include <mbbiDirectRecord.h>
#include <mbboDirectRecord.h>

#include "base.hpp"

template <typename mbbIODirectType>
class MbbIODirect : public Record {
public:
    virtual ~MbbIODirect() override = default;

    epicsUInt32 raw_value() { return raw()->rval; };
    epicsUInt32 raw_value() const { return raw()->rval; };
    void set_raw_value(epicsUInt32 rval) { raw()->rval = rval; }

    epicsInt32 value() { return raw()->val; };
    epicsInt32 value() const { return raw()->val; };

protected:
    explicit MbbIODirect(mbbIODirectType *raw) : Record((dbCommon *)raw) {}
    
    mbbIODirectType *raw() { 
        return (mbbIODirectType *)Record::raw(); 
    }

    const mbbIODirectType *raw() const { 
    	return (const mbbIODirectType *)Record::raw(); 
   }
};


class MbbiDirect final : public MbbIODirect<mbbiDirectRecord> {
public:
	explicit MbbiDirect(mbbiDirectRecord *raw);
    virtual ~MbbiDirect() override = default;
};


class MbbiDirectHandler final : public Handler {
public:
    MbbiDirectHandler(dbCommon *raw_record);
    MbbiDirectHandler(
        dbCommon *raw_record, 
        bool asyn_process
    );
    virtual ~MbbiDirectHandler() override = default;

    MbbiDirectHandler(const MbbiDirectHandler &) = delete;
    MbbiDirectHandler &operator=(const MbbiDirectHandler &) = delete;
    MbbiDirectHandler(MbbiDirectHandler &&) = default;
    MbbiDirectHandler &operator=(MbbiDirectHandler &&) = default;

    virtual void readwrite() override;
};


class MbboDirect final : public MbbIODirect<mbboDirectRecord> {
public:
    explicit MbboDirect(mbboDirectRecord *raw);
    virtual ~MbboDirect() override = default;
};


class MbboDirectHandler final : public Handler {
public:
    MbboDirectHandler(dbCommon *raw_record);
    MbboDirectHandler(
        dbCommon *raw_record, 
        bool asyn_process
    );
    virtual ~MbboDirectHandler() override = default;

    MbboDirectHandler(const MbboDirectHandler &) = delete;
    MbboDirectHandler &operator=(const MbboDirectHandler &) = delete;
    MbboDirectHandler(MbboDirectHandler &&) = default;
    MbboDirectHandler &operator=(MbboDirectHandler &&) = default;

    virtual void readwrite() override;
};