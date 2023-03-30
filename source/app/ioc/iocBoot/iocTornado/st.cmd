#!../../bin/linux-x86_64/Tornado

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase("dbd/Tornado.dbd", 0, 0)
Tornado_registerRecordDeviceDriver(pdbbase) 

## Load record instances
dbLoadRecords("db/Tornado.db", "PREFIX=tornado0:")

cd "${TOP}/iocBoot/${IOC}"
iocInit()
