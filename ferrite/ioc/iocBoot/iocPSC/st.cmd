< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/PSC.dbd"
PSC_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadRecords("db/devPSC.db")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=alex"
