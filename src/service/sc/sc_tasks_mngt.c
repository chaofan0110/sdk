/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_tasks_mngt.c
 *
 *  创建时间: 2014年12月16日10:24:40
 *  作    者: Larry
 *  描    述: 对群呼任务进行管理
 *  修改历史:
 */
#if 0
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif
/* include public header files */
#include <dos.h>
#include <sys/time.h>


/* include private header files */
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_http_api.h"


/* define global variable */
extern U32       g_ulTaskTraceAll;

SC_TASK_MNGT_ST   *g_pstTaskMngtInfo;

#if 1
/* declare functions */
static S32 sc_task_load_callback(VOID *pArg, S32 argc, S8 **argv, S8 **columnNames)
{
    SC_TASK_CB_ST *pstTCB;
    U32 ulIndex;
    U32 ulTaskID = U32_BUTT, ulCustomID = U32_BUTT;

    SC_TRACE_IN((U64)pArg, (U32)argc, (U64)argv, (U64)columnNames);

    for (ulIndex=0; ulIndex<argc; ulIndex++)
    {
        if (dos_strcmp(columnNames[ulIndex], "id") == 0)
        {
            if (dos_atoul(argv[ulIndex], &ulTaskID) < 0)
            {
                SC_TRACE_OUT();
                return DOS_FAIL;
            }
        }
        else if (dos_strcmp(columnNames[ulIndex], "customer_id") == 0)
        {
            if (dos_atoul(argv[ulIndex], &ulCustomID) < 0)
            {
                SC_TRACE_OUT();
                return DOS_FAIL;
            }
        }

        pstTCB = sc_tcb_alloc();
        if (pstTCB)
        {
            DOS_ASSERT(0);
            continue;
        }

        sc_task_set_owner(pstTCB, ulTaskID, ulCustomID);
    }

    return DOS_SUCC;
}

U32 sc_task_mngt_load_task()
{
    S8 szSqlQuery[128]= { 0 };
    U32 ulLength, ulResult;

    SC_TRACE_IN(0, 0, 0, 0);

    if (!g_pstTaskMngtInfo)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    ulLength = dos_snprintf(szSqlQuery
                , sizeof(szSqlQuery)
                , "SELECT tbl_calltask.id, tbl_calltask.customer_id from tbl_calltask WHERE tbl_calltask.status=%d"
                , SC_TASK_STATUS_DB_START);

    ulResult = db_query(g_pstTaskMngtInfo->pstDBHandle
                            , szSqlQuery
                            , sc_task_load_callback
                            , NULL
                            , NULL);

    if (ulResult != DOS_SUCC)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}
#endif

/* 启动一个已经暂停的任务 */
U32 sc_task_mngt_continue_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB_ST *pstTCB = NULL;
    SC_TRACE_IN((U32)ulTaskID, ulCustomID, 0, 0);

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_PAUSED)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_continue(pstTCB);

    SC_TRACE_OUT();
    return SC_HTTP_ERRNO_SUCC;
}


/* 暂停一个在运行的任务 */
U32 sc_task_mngt_pause_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB_ST *pstTCB = NULL;

    SC_TRACE_IN((U32)ulTaskID, 0, 0, 0);

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_pause(pstTCB);

    SC_TRACE_OUT();
    return SC_HTTP_ERRNO_SUCC;

}

/* 启动一个新的任务 */
U32 sc_task_mngt_start_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB_ST *pstTCB = NULL;
    SC_TRACE_IN((U32)ulTaskID, 0, 0, 0);

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_alloc();
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    sc_task_set_owner(pstTCB, ulTaskID, ulCustomID);

    if (sc_task_init(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_tcb_free(pstTCB);

        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    if (sc_task_start(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_tcb_free(pstTCB);

        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    SC_TRACE_OUT();
    return SC_HTTP_ERRNO_SUCC;
}


/* 停止一个在运行的任务 */
U32 sc_task_mngt_stop_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB_ST *pstTCB = NULL;

    SC_TRACE_IN(ulTaskID, ulCustomID, 0, 0);

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_stop(pstTCB);

    SC_TRACE_OUT();
    return SC_HTTP_ERRNO_SUCC;
}


U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD)
{
    SC_TRACE_IN((U64)pstCMD, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstCMD))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCMDList);
    dos_list_add_head(&g_pstTaskMngtInfo->stCMDList, &pstCMD->stLink);
    pthread_cond_signal(&g_pstTaskMngtInfo->condCMDList);
    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCMDList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD)
{
    list_t                *pstListNode;
    SC_TASK_CTRL_CMD_ST   *pstCMDNode;
    U32                    ulResult;

    SC_TRACE_IN((U64)pstCMD, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstCMD))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCMDList);
    pstListNode = &g_pstTaskMngtInfo->stCMDList;
    while (1)
    {
        pstListNode = dos_list_work(&g_pstTaskMngtInfo->stCMDList, pstListNode);
        if (!pstListNode)
        {
            break;
        }

        pstCMDNode = dos_list_entry(pstListNode, SC_TASK_CTRL_CMD_ST, stLink);

        sc_logr_debug("Work list, Current Node Addr: %p, (%p)", pstCMDNode, pstCMD);
        if (pstCMDNode && pstCMDNode == pstCMD)
        {
            break;
        }
    }

    if (pstCMDNode && pstCMDNode == pstCMD)
    {
        dos_list_del(pstListNode);

        ulResult = DOS_SUCC;
    }
    else
    {
        DOS_ASSERT(0);
        ulResult = DOS_FAIL;
    }

    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCMDList);

    SC_TRACE_OUT();
    return ulResult;
}


VOID sc_task_mngt_cmd_process(SC_TASK_CTRL_CMD_ST *pstCMD)
{
    SC_TRACE_IN((U64)pstCMD, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstCMD))
    {
        SC_TRACE_OUT();
        return;
    }

    sc_logr_debug("Process CMD. CMD: %d, Action:%d, Task: %d, CustomID: %d"
                    , pstCMD->ulCMD, pstCMD->ulAction, pstCMD->ulTaskID, pstCMD->ulCustomID);

    switch (pstCMD->ulCMD)
    {
        case SC_API_CMD_TASK_CTRL:
        {
            switch (pstCMD->ulAction)
            {
                case SC_API_CMD_ACTION_START:
                {
                    pstCMD->ulCMDErrCode = sc_task_mngt_start_task(pstCMD->ulTaskID, pstCMD->ulCustomID);
                    break;
                }
                case SC_API_CMD_ACTION_STOP:
                {
                    pstCMD->ulCMDErrCode = sc_task_mngt_stop_task(pstCMD->ulTaskID, pstCMD->ulCustomID);
                    break;
                }
                case SC_API_CMD_ACTION_CONTINUE:
                {
                    pstCMD->ulCMDErrCode = sc_task_mngt_continue_task(pstCMD->ulTaskID, pstCMD->ulCustomID);
                    break;
                }
                case SC_API_CMD_ACTION_PAUSE:
                {
                    pstCMD->ulCMDErrCode = sc_task_mngt_pause_task(pstCMD->ulTaskID, pstCMD->ulCustomID);
                    break;
                }
                default:
                {
                    sc_logr_notice("CMD templately not support. CMD:%d, ACTION: %d", pstCMD->ulCMD, pstCMD->ulAction);
                    DOS_ASSERT(0);
                    break;
                }
            }

            break;
        }
        case SC_API_CMD_CALL_CTRL:
        {
            switch (pstCMD->ulAction)
            {
                case SC_API_CMD_ACTION_MONITORING:
                {
                    break;
                }
                case SC_API_CMD_ACTION_HUNGUP:
                {
                    break;
                }
                default:
                {
                    sc_logr_notice("CMD templately not support. CMD:%d, ACTION: %d", pstCMD->ulCMD, pstCMD->ulAction);
                    DOS_ASSERT(0);
                    break;
                }
            }

            break;
        }
        default:
        {
            sc_logr_notice("CMD templately not support. CMD:%d, ACTION: %d", pstCMD->ulCMD, pstCMD->ulAction);

            DOS_ASSERT(0);
            break;
        }
    }

    sc_logr_debug("CMD Process finished. CMD:%d , Action: %d, ErrCode:%d"
                    , pstCMD->ulCMD, pstCMD->ulAction, pstCMD->ulCMDErrCode);
}

VOID *sc_task_mngt_runtime(VOID *ptr)
{
    struct timespec       stTimeout;
    U32                   ulTaskIndex;
    list_t                *pstListNode;
    SC_TASK_CTRL_CMD_ST   *pstCMD;

    SC_TRACE_IN(0, 0, 0, 0);

    while(1)
    {
        pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCMDList);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_pstTaskMngtInfo->condCMDList, &g_pstTaskMngtInfo->mutexCMDList, &stTimeout);

        /* 处理命令队列 */
        if (!dos_list_is_empty(&g_pstTaskMngtInfo->stCMDList))
        {
            pstListNode = &g_pstTaskMngtInfo->stCMDList;
            while (1)
            {
                if (dos_list_is_empty(&g_pstTaskMngtInfo->stCMDList))
                {
                    break;
                }

                pstListNode = dos_list_work(&g_pstTaskMngtInfo->stCMDList, pstListNode);
                if (!pstListNode)
                {
                    break;
                }

                pstCMD = dos_list_entry(pstListNode, SC_TASK_CTRL_CMD_ST, stLink);
                if (!pstCMD)
                {
                    break;
                }

                sc_task_mngt_cmd_process(pstCMD);

                sem_post(&pstCMD->semCMDExecNotify);
            }
        }

        /* 处理系统状态 */
        g_pstTaskMngtInfo->enSystemStatus = sc_check_sys_stat();

        /* 处理退出标志 */
        if (g_pstTaskMngtInfo->blWaitingExitFlag)
        {
            pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCMDList);
            break;
        }

        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCMDList);
    }

    for (ulTaskIndex=0; ulTaskIndex<SC_MAX_TASK_NUM; ulTaskIndex++)
    {
        if (g_pstTaskMngtInfo->pstTaskList[ulTaskIndex].ucValid)
        {
            sc_task_stop(&g_pstTaskMngtInfo->pstTaskList[ulTaskIndex]);
        }
    }

    SC_TRACE_OUT();

    return NULL;
}

U32 sc_task_mngt_start()
{
    SC_TASK_CB_ST    *pstTCB = NULL;
    U32              ulIndex;

    SC_TRACE_IN(0, 0, 0, 0);

    pthread_create(&g_pstTaskMngtInfo->pthID, NULL, sc_task_mngt_runtime, NULL);

    if (!g_pstTaskMngtInfo->ulTaskCount)
    {
        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    for (ulIndex=0; ulIndex<SC_MAX_TASK_NUM; ulIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulIndex];

        if (pstTCB->ucValid
            && SC_TCB_HAS_VALID_OWNER(pstTCB))
        {
            if (sc_task_init(pstTCB) != DOS_SUCC)
            {
                SC_TASK_TRACE(pstTCB, "%s", "Task init fail");
                sc_logr_notice("Task init fail. Custom ID: %d, Task ID: %d", pstTCB->ulCustomID, pstTCB->ulTaskID);

                sc_tcb_free(pstTCB);
                continue;
            }

            if (sc_task_start(pstTCB) != DOS_SUCC)
            {
                SC_TASK_TRACE(pstTCB, "%s", "Task init fail");
                sc_logr_notice("Task start fail. Custom ID: %d, Task ID: %d", pstTCB->ulCustomID, pstTCB->ulTaskID);

                sc_tcb_free(pstTCB);
                continue;
            }
        }
    }

    return DOS_SUCC;
}

U32 sc_task_mngt_init()
{
    SC_TASK_CB_ST   *pstTCB = NULL;
    SC_CCB_ST       *pstCCB = NULL;
    U32             ulIndex    = 0;
    U16             usDBPort;
    S8              szDBHost[DB_MAX_STR_LEN] = {0, };
    S8              szDBUsername[DB_MAX_STR_LEN] = {0, };
    S8              szDBPassword[DB_MAX_STR_LEN] = {0, };
    S8              szDBName[DB_MAX_STR_LEN] = {0, };

    SC_TRACE_IN(0, 0, 0, 0);

    if (config_get_db_host(szDBHost, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (config_get_db_user(szDBUsername, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (config_get_db_password(szDBPassword, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    usDBPort = config_get_db_port();
    if (0 == usDBPort || U16_BUTT == usDBPort)
    {
        usDBPort = 3306;
    }

    if (config_get_db_dbname(szDBName, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 申请管理任务的相关变量 */
    g_pstTaskMngtInfo = (SC_TASK_MNGT_ST *)dos_dmem_alloc(sizeof(SC_TASK_MNGT_ST));
    if (!g_pstTaskMngtInfo)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    dos_memzero(g_pstTaskMngtInfo, sizeof(SC_TASK_MNGT_ST));

    /* 申请数据库句柄的内存 */
    g_pstTaskMngtInfo->pstDBHandle = db_create(DB_TYPE_MYSQL);
    if (!g_pstTaskMngtInfo->pstDBHandle)
    {
        DOS_ASSERT(0);

        dos_smem_free(g_pstTaskMngtInfo);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_strncpy(g_pstTaskMngtInfo->pstDBHandle->szHost, szDBHost, sizeof(g_pstTaskMngtInfo->pstDBHandle->szHost));
    g_pstTaskMngtInfo->pstDBHandle->szHost[sizeof(g_pstTaskMngtInfo->pstDBHandle->szHost) - 1] = '\0';

    dos_strncpy(g_pstTaskMngtInfo->pstDBHandle->szUsername, szDBUsername, sizeof(g_pstTaskMngtInfo->pstDBHandle->szUsername));
    g_pstTaskMngtInfo->pstDBHandle->szUsername[sizeof(g_pstTaskMngtInfo->pstDBHandle->szUsername) - 1] = '\0';

    dos_strncpy(g_pstTaskMngtInfo->pstDBHandle->szPassword, szDBPassword, sizeof(g_pstTaskMngtInfo->pstDBHandle->szPassword));
    g_pstTaskMngtInfo->pstDBHandle->szPassword[sizeof(g_pstTaskMngtInfo->pstDBHandle->szPassword) - 1] = '\0';

    dos_strncpy(g_pstTaskMngtInfo->pstDBHandle->szDBName, szDBName, sizeof(g_pstTaskMngtInfo->pstDBHandle->szDBName));
    g_pstTaskMngtInfo->pstDBHandle->szDBName[sizeof(g_pstTaskMngtInfo->pstDBHandle->szDBName) - 1] = '\0';

    g_pstTaskMngtInfo->pstDBHandle->usPort = usDBPort;


    /* 连接数据库 */
    if (db_open(g_pstTaskMngtInfo->pstDBHandle) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        db_destroy(&g_pstTaskMngtInfo->pstDBHandle);
        dos_smem_free(g_pstTaskMngtInfo);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 初始化其他全局变量 */
    pthread_mutex_init(&g_pstTaskMngtInfo->mutexCMDList, NULL);
    pthread_mutex_init(&g_pstTaskMngtInfo->mutexTCBList, NULL);
    pthread_mutex_init(&g_pstTaskMngtInfo->mutexCallList, NULL);
    pthread_mutex_init(&g_pstTaskMngtInfo->mutexCallHash, NULL);
    pthread_cond_init(&g_pstTaskMngtInfo->condCMDList, NULL);
    dos_list_init(&g_pstTaskMngtInfo->stCMDList);

    g_pstTaskMngtInfo->pstCallCCBHash = hash_create_table(SC_MAX_CCB_HASH_NUM, NULL);
    if (!g_pstTaskMngtInfo->pstCallCCBHash)
    {
        DOS_ASSERT(0);

        db_close(g_pstTaskMngtInfo->pstDBHandle);
        db_destroy(&g_pstTaskMngtInfo->pstDBHandle);

        dos_smem_free(g_pstTaskMngtInfo);
        g_pstTaskMngtInfo = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;

    }

    /* 初始化呼叫控制块和任务控制块 */
    g_pstTaskMngtInfo->pstTaskList = (SC_TASK_CB_ST *)dos_smem_alloc(sizeof(SC_TASK_CB_ST) * SC_MAX_TASK_NUM);
    g_pstTaskMngtInfo->pstCallCCBList = (SC_CCB_ST *)dos_smem_alloc(sizeof(SC_CCB_ST) * SC_MAX_CCB_NUM);
    if (!g_pstTaskMngtInfo->pstTaskList || !g_pstTaskMngtInfo->pstCallCCBList)
    {
        DOS_ASSERT(0);

        if (g_pstTaskMngtInfo->pstTaskList)
        {
            dos_smem_free(g_pstTaskMngtInfo->pstTaskList);
            g_pstTaskMngtInfo->pstTaskList = NULL;
        }

        if (g_pstTaskMngtInfo->pstCallCCBList)
        {
            dos_smem_free(g_pstTaskMngtInfo->pstCallCCBList);
            g_pstTaskMngtInfo->pstCallCCBList = NULL;
        }

        db_close(g_pstTaskMngtInfo->pstDBHandle);
        db_destroy(&g_pstTaskMngtInfo->pstDBHandle);

        dos_smem_free(g_pstTaskMngtInfo);
        g_pstTaskMngtInfo = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(g_pstTaskMngtInfo->pstCallCCBList, sizeof(SC_CCB_ST) * SC_MAX_CCB_NUM);
    for (ulIndex = 0; ulIndex < SC_MAX_CCB_NUM; ulIndex++)
    {
        pstCCB = &g_pstTaskMngtInfo->pstCallCCBList[ulIndex];
        pthread_mutex_init(&pstCCB->mutexCCBLock, NULL);
        pthread_mutex_lock(&pstCCB->mutexCCBLock);
        pstCCB->usCCBNo = (U16)ulIndex;
        sc_ccb_init(pstCCB);
        pthread_mutex_unlock(&pstCCB->mutexCCBLock);
    }

    dos_memzero(g_pstTaskMngtInfo->pstTaskList, sizeof(SC_TASK_CB_ST) * SC_MAX_TASK_NUM);
    for (ulIndex = 0; ulIndex < SC_MAX_TASK_NUM; ulIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulIndex];
        pstTCB->usTCBNo = ulIndex;
        pthread_mutex_init(&pstTCB->mutexTaskList, NULL);
        pstTCB->ucTaskStatus = SC_TASK_BUTT;
        pstTCB->ulTaskID = U32_BUTT;
        pstTCB->ulCustomID = U32_BUTT;

        dos_list_init(&pstTCB->stCalleeNumQuery);
    }

    g_pstTaskMngtInfo->ulTaskCount = 0;
    g_pstTaskMngtInfo->ulMaxCall = 0;
    g_pstTaskMngtInfo->blWaitingExitFlag = 0;

    sc_task_mngt_load_task();

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_task_mngt_shutdown()
{
    g_pstTaskMngtInfo->blWaitingExitFlag = 1;

    return DOS_SUCC;
}

#if 0
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
