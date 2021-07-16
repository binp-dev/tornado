#pragma once

#include <string>
#include <utility>

#include <dbScan.h>


/*
Namespace with functions for work witch EPICS scan mode "I/O Intr". This functions is 
used to initialize scan lists from different DeviceSupport and start worker threads
in which events associated with the corresponding scan list should be checked.
*/
namespace iointr {


/*
Init EPICS scan list and save associated name.
If scan list with that name already exist, then nothing happend. 
*/
void init_scan_list(const std::string &list_name);

IOSCANPVT &get_scan_list(const std::string &list_name);

void init_iointr_scan_lists();

void start_scan_list_worker_thread(std::string scan_list_name);


} // namespace iointr

