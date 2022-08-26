#!../../bin/linux-x86_64/Fer

#- You may have to change Fer to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase("dbd/Fer.dbd",0,0)
Fer_registerRecordDeviceDriver(pdbbase) 

## Load record instances
dbLoadRecords("db/devFer.db")

cd "${TOP}/iocBoot/${IOC}"
iocInit()

## Start any sequence programs
#seq sncFer
