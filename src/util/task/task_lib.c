/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  task_lib.c
 *
 *  Created on: 2014-11-6
 *      Author: Larry
 *        Todo: 封装系统相关的函数，提高可一致性性
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

/**
 * 函数: void dos_task_delay(U32 ulMsSec)
 * 功能: 使某一个线程睡眠 ulMsSec毫秒之后再执行
 * 参数: U32 ulMSEC睡眠的毫秒数
 * 返回值: void
 */
DLLEXPORT VOID dos_task_delay(U32 ulMSec)
{
    if (0 == ulMSec)
    {
        return;
    }

    usleep(ulMSec * 1000);
}

/**
 * 函数: void dos_clean_watchdog()
 * 功能: 清除看门狗
 * 参数: NULL
 * 返回值: void
 */
DLLEXPORT void dos_clean_watchdog()
{

}

DLLEXPORT U32 dos_random(U32 ulMax)
{

    srand((unsigned)time( NULL ));

    return rand() % ulMax;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


