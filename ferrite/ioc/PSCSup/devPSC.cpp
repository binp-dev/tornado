#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExit.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <aaoRecord.h>

#include "framework.hpp"

void init(void) {
    std::cout << "init" << std::endl;
    framework_init_device();
}

long record_aao_init(aaoRecord *raw) {
    Aao record(raw);
    std::cout << "record_aao_init: " << record.name() << std::endl;

    std::unique_ptr<AaoHandler> handler = framework_record_init_dac(record);
    if (!bool(handler)) {
        std::cerr << "framework_record_init_dac returned NULL" << std::endl;
        epicsExit(1);
    }
    record.set_private_data((void *)handler.release());
    return 0;
}
long record_aao_get_ioint_info(int cmd, aaoRecord *raw, IOSCANPVT *ppvt) {
    std::cout << "record_aao_get_ioint_info: " << raw->name << std::endl;
    std::cerr << "unimplemented" << std::endl;
    return 0;
}
long record_aao_write(aaoRecord *raw) {
    Aao record(raw);
    std::cout << "record_aao_write: " << record.name() << std::endl;

    // FIXME: Check result
    ((AaoHandler *)record.private_data())->readwrite();
    return 0;
}

struct AaoRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_aao;
};

struct AaoRecordCallbacks aao_record_handler = {
    5,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_aao_init),
    reinterpret_cast<DEVSUPFUN>(record_aao_get_ioint_info),
    reinterpret_cast<DEVSUPFUN>(record_aao_write)
};

epicsExportAddress(dset, aao_record_handler);

epicsExportRegistrar(init);
