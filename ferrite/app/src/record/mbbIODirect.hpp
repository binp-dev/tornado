#pragma once

#include <epicsTypes.h>
#include <mbbiDirectRecord.h>
#include <mbboDirectRecord.h>

#include "base.hpp"

template <typename mbbDirectType>
class MbbIODirect : public Record {
public:
    virtual ~MbbIODirect() override = default;

    unsigned long raw_value() { return raw()->rval; };
    unsigned long raw_value() const { return raw()->rval; };
    void set_raw_value(unsigned long rval) { raw()->rval = rval; }

    long value() { return raw()->val; };
    long value() const { return raw()->val; };

protected:
    explicit MbbIODirect(mbbDirectType *raw) : Record((dbCommon *)raw) {}

    mbbDirectType *raw() { return (mbbDirectType *)Record::raw(); }
    const mbbDirectType *raw() const { 
    	return (const mbbDirectType *)Record::raw(); 
   }
};


class MbbiDirect final : 
	public MbbIODirect<mbbiDirectRecord>,
	public InputRecord {
public:
	explicit MbbiDirect(mbbiDirectRecord *raw);
    virtual ~MbbiDirect() override = default;

    virtual void read() override;
};


class MbboDirect final : 
    public MbbIODirect<mbboDirectRecord>,
    public OutputRecord {
public:
    explicit MbboDirect(mbboDirectRecord *raw);
    virtual ~MbboDirect() override = default;

    virtual void write() override;
};

//---------------------------------------------------

class MbboHandler final : public WriteHandler {
public:
    MbboHandler(dbCommon *raw_record);
    MbboHandler(
        dbCommon *raw_record, 
        std::function<callback_func_t> write_callback
    );
    virtual ~MbboHandler() override = default;

    MbboHandler(const MbboHandler &) = delete;
    MbboHandler &operator=(const MbboHandler &) = delete;
    MbboHandler(MbboHandler &&) = default;
    MbboHandler &operator=(MbboHandler &&) = default;

    virtual void write() override;
};