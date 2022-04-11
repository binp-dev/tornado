#pragma once

#include <FreeRTOS.h>

#include <common/config.h>
#include <ipp.h>


#define TASK_STACK_SIZE 256

// clang-format off
#define SYNC_TASK_PRIORITY         tskIDLE_PRIORITY + 5
#define CONTROL_TASK_PRIORITY      tskIDLE_PRIORITY + 4
#define RPMSG_SEND_TASK_PRIORITY   tskIDLE_PRIORITY + 3
#define RPMSG_RECV_TASK_PRIORITY   tskIDLE_PRIORITY + 2
#define STATISTICS_TASK_PRIORITY   tskIDLE_PRIORITY + 1
// clang-format on
