#pragma once

#include <epicsTypes.h>
#include <mbbiDirectRecord.h>
#include <mbboDirectRecord.h>

#include "base.hpp"

template <typename mbbIODirectType>
class MbbIODirect : public Record {
public:
    virtual ~MbbIODirect() override = default;

    unsigned long raw_value() { return raw()->rval; };
    unsigned long raw_value() const { return raw()->rval; };
    void set_raw_value(unsigned long rval) { raw()->rval = rval; }

    long value() { return raw()->val; };
    long value() const { return raw()->val; };

// protected:
    explicit MbbIODirect(mbbIODirectType *raw) : Record((dbCommon *)raw) {}
    
    mbbIODirectType *raw() { 
        return (mbbIODirectType *)Record::raw(); 
    }

    const mbbIODirectType *raw() const { 
    	return (const mbbIODirectType *)Record::raw(); 
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


// class MbboDirect final : 
//     public MbbIODirect<mbboDirectRecord>,
//     public OutputRecord {
// public:
//     explicit MbboDirect(mbboDirectRecord *raw);
//     virtual ~MbboDirect() override = default;

//     virtual void write() override;
// };

//---------------------------------------------------

class MbboDirectHandler final : public WriteHandler {
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

    virtual void write() override;
};