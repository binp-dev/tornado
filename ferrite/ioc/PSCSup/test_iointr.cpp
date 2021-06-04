   /* Demonstration of using custom allocation for waveformRecord buffers.
    *
    * Requires EPICS Base with the array field memory management patch
    * https://code.launchpad.net/~epics-core/epics-base/array-opt
    *
    * This example makes inefficient use of malloc() and
    * free().  This is done to make clear where new memory appears.
    * In reality a free list should be used.
    *
    * Also be aware that this example will use 100% of the time of one CPU core.
    * However, this will be spread across available cores.
    *
    * To use this example include the following in a DBD file:
    *
    * device(waveform,CONSTANT,devWfZeroCopy,"Zero Copy Demo")
    *
    * Also include a record instance
    *
    * record(waveform, "$(NAME)") {
    *  field(DTYP, "Zero Copy Demo")
    *  field(FTVL, "SHORT")
    *  field(NELM, "100")
    *  field(SCAN, "I/O Intr")
    * }
    */
   
#include <iostream>

   #include <errlog.h>
   #include <initHooks.h>
   #include <ellLib.h>
   #include <devSup.h>
   #include <dbDefs.h>
   #include <dbAccess.h>
   #include <cantProceed.h>
   #include <epicsTypes.h>
   #include <epicsMutex.h>
   #include <epicsEvent.h>
   #include <epicsThread.h>
   #include <menuFtype.h>
   #include <dbScan.h>
   
   #include <waveformRecord.h>
   
   static ELLLIST allPvt = ELLLIST_INIT;
   
   struct devicePvt {
       // ELLNODE node;
   
       /* synchronize access to this structure */
       epicsMutexId lock;
       /* wakeup the worker when another update is needed */
       epicsEventId wakeup;
       /* notify the scanner thread when another update is available */
       // IOSCANPVT scan;
   
       /* the next update */
       void *nextBuffer;
       epicsUInt32 maxbytes, numbytes;
   };
   
   static void startWorkers(initHookState);
   
   static long init(int phase)
   {
       if(phase!=0)
           return 0;
       initHookRegister(&startWorkers);
       return 0;
   }
   
   static long init_record(waveformRecord *prec)
   {
       struct devicePvt *priv;
       if(prec->ftvl!=menuFtypeSHORT) {
           errlogPrintf("%s.FTVL must be set to SHORT for this example\n", prec->name);
           return 0;
       }
   
       /* cleanup array allocated by record support.
        * Not necessary since we use calloc()/free(),
        * but needed when allocating in other ways.
        */
       free(prec->bptr);
       prec->bptr = callocMustSucceed(prec->nelm, dbValueSize(prec->ftvl), "first buf");
   
       priv = (devicePvt *)callocMustSucceed(1, sizeof(*priv), "init_record devWfZeroCopy");
       priv->lock = epicsMutexMustCreate();
       priv->wakeup = epicsEventMustCreate(epicsEventFull);
       scanIoInit(&scan);
       priv->maxbytes = prec->nelm*dbValueSize(prec->ftvl);
   
       // ellAdd(&allPvt, &priv->node);
   
       prec->dpvt = priv;
       return 0;
   }
   
   static void worker(void*);
   
   static void startWorkers(initHookState state)
   {
       // ELLNODE *cur;
       // /* Don't start worker threads until
       //  * it is safe to call scanIoRequest()
       //  */
       // if(state!=initHookAfterInterruptAccept)
       //     return;
       // for(cur=ellFirst(&allPvt); cur; cur=ellNext(cur))
       // {
           struct devicePvt *priv = CONTAINER(cur, struct devicePvt, node);
           epicsThreadMustCreate("wfworker",
                                 epicsThreadPriorityHigh,
                                 epicsThreadGetStackSize(epicsThreadStackSmall),
                                 &worker, NULL);
       // }
   }
   
   static void worker(void* raw)
   {
        std::cout << "  TEST IOINTR worker thread start" << std::endl << std::flush;
        std::cout << "      Thread id = " << pthread_self() << std::endl << std::flush;
       struct devicePvt *priv = (devicePvt *)raw;
       void *buf = NULL;
       epicsUInt32 nbytes = priv->maxbytes;
   
       while(1) {
   
           if(!buf) {
               /* allocate and initialize a new buffer for later (local) use */
               size_t i;
               epicsInt16 *ibuf;
               buf = callocMustSucceed(1, nbytes, "buffer");
               ibuf = (epicsInt16*)buf;
               for(i=0; i<nbytes/2; i++)
               {
                   ibuf[i] = rand();
               }
           }
   
           /* wait for Event signal when record is scanning 'I/O Intr',
            * and timeout when record is scanning periodic
            */
           if(epicsEventWaitWithTimeout(priv->wakeup, 1.0)==epicsEventError) {
               cantProceed("worker encountered an error waiting for wakeup\n");
           }
   
           epicsMutexMustLock(priv->lock);
   
           if(!priv->nextBuffer) {
               /* make the local buffer available to the read_wf function */
               priv->nextBuffer = buf;
               buf = NULL;
               priv->numbytes = priv->maxbytes;
               scanIoRequest(scan);
           }
   
           epicsMutexUnlock(priv->lock);
       }
   }
   
   static long get_iointr_info(int dir, dbCommon *prec, IOSCANPVT *scan)
   {
       struct devicePvt *priv = (devicePvt *)prec->dpvt;
       if(!priv)
           return 0;
       *scan = scan;
       /* wakeup the worker when this thread is placed in the I/O scan list */
       if(dir==0)
           epicsEventSignal(priv->wakeup);
       return 0;
   }
   
   static long read_wf(waveformRecord *prec)
   {
        // std::cout << "  TEST IOINTR read_wf()" << std::endl << std::flush;
        // std::cout << "      Thread id = " << pthread_self() << std::endl << std::flush;
       struct devicePvt *priv = (devicePvt *)prec->dpvt;
       if(!priv)
           return 0;
   
       if(priv->nextBuffer) {
           /* an update is available, so claim it. */
   
           if(prec->bptr)
               free(prec->bptr);
   
           prec->bptr = priv->nextBuffer; /* no memcpy! */
           priv->nextBuffer = NULL;
           prec->nord = priv->numbytes / dbValueSize(prec->ftvl);
   
           epicsEventSignal(priv->wakeup);
       }
   
       epicsMutexUnlock(priv->lock);
   
       assert(prec->bptr);
   
       return 0;
   }
   
   static
   struct dset5 {
       dset com;
       DEVSUPFUN read;
   } devWfZeroCopy = {
   {5, NULL,
    reinterpret_cast<DEVSUPFUN>(init),
    reinterpret_cast<DEVSUPFUN>(init_record),
    reinterpret_cast<DEVSUPFUN>(get_iointr_info)
   },
    reinterpret_cast<DEVSUPFUN>(read_wf)
   };
   
   #include <epicsExport.h>
   
   epicsExportAddress(dset, devWfZeroCopy);
