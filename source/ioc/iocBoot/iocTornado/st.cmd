#!../../bin/linux-x86_64/Tornado

#- You may have to change Tornado to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase("dbd/Tornado.dbd",0,0)
Tornado_registerRecordDeviceDriver(pdbbase) 

## Load record instances
dbLoadRecords("db/Tornado.db")

cd "${TOP}/iocBoot/${IOC}"
iocInit()

## Start any sequence programs
#seq sncTornado
