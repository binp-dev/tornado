#!../../bin/linux-x86_64/Tornado

#< /opt/env.cmd
< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase("dbd/Tornado.dbd", 0, 0)
Tornado_registerRecordDeviceDriver(pdbbase) 

epicsEnvSet("PREFIX","tornado0:")
## Load record instances
dbLoadTemplate("db/ai.substitutions", "PREFIX=${PREFIX}")
dbLoadRecords("db/ao.db", "PREFIX=${PREFIX}")
dbLoadRecords("db/di.db", "PREFIX=${PREFIX}")
dbLoadRecords("db/do.db", "PREFIX=${PREFIX}")
dbLoadRecords("db/debug.db", "PREFIX=${PREFIX}")

cd "${TOP}/iocBoot/${IOC}"
iocInit()
