/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  hb_server.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 本文件定义心跳服务器相关操作函数集
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos header files */
#include <dos.h>

#if INCLUDE_BH_SERVER

/* include system header file */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/* include private header file */
#include "heartbeat.h"
/* 定义系统最长的等待时间，如果超过该时间业务还未跑完，强制重启 */
#define MAX_WAIT_TIME   30
/* 定义系统最短等待时间 */
#define MIN_WAIT_TIME   2

/* define global varables */
/* 对等待退出标志做保护 */
static pthread_mutex_t      g_HBMutex = PTHREAD_MUTEX_INITIALIZER;

/* 线程id */
static pthread_t            g_pthIDHBTask;

/* 链接进程控制块 */
static PROCESS_INFO_ST      *g_pstProcessInfo[DOS_PROCESS_MAX_NUM];

/* 对端地址 */
static struct sockaddr_un   g_stServerAddr;  /* 对端地址 */

/* 服务socket */
static S32                  g_lSocket = -1;

/* 等待退出标志 */
static S32                  g_lHBTaskWaitingExit = 0;

/* 接收数据缓存 */
static S8                   g_szRecvBuf[MAX_BUFF_LENGTH];

/* 接收外部客户端消息线程 */
static pthread_t            g_pthSendMsgTask;


extern DB_HANDLE_ST *                g_pstCCDBHandle;
static U32 mon_restart_immediately();
static U32 mon_restart_fixed(U32 ulTimeStamp);
static U32 mon_restart_later();
extern S32 hb_send_msg(U8 * pszBuff,U32 ulBuffLen,struct sockaddr_un * pstAddr,U32 ulAddrLen,S32 lSocket);
extern U32 mon_system(S8 * pszCmd);
static U32 mon_restart_cycle_setting(U32 ulTime);
static U32 mon_restart_cycle_start();

/**
 * 函数: hb_get_max_timeout()
 * 功能: 获取最大超时时间
 * 参数:
 * 返回值
 */
S32 hb_get_max_timeout()
{
    U32 ulHBSendInterval = 0, ulHBMaxFailCnt;
    ulHBSendInterval = config_hh_get_send_interval();
    if (ulHBSendInterval < DEFAULT_HB_INTERVAL_MIN
        || ulHBSendInterval > DEFAULT_HB_INTERVAL_MAX)
    {
        ulHBSendInterval = DEFAULT_HB_INTERVAL_MIN;
    }

    ulHBMaxFailCnt = config_hb_get_max_fail_cnt();
    if (ulHBMaxFailCnt < DEFAULT_HB_FAIL_CNT_MIN
        || ulHBMaxFailCnt < DEFAULT_HB_FAIL_CNT_MAX)
    {
        ulHBMaxFailCnt = DEFAULT_HB_FAIL_CNT_MIN;
    }

    return ulHBSendInterval * ulHBMaxFailCnt;

}

VOID hb_recv_timeout(U64 uLParam)
{
    PROCESS_INFO_ST *pstProcessInfo = NULL;

    if (uLParam >= DOS_PROCESS_MAX_NUM)
    {
        hb_logr_warning("Heartbeat recv timeout, but with an error param \"%lu\".", uLParam);
        DOS_ASSERT(0);
        return;
    }

    pstProcessInfo = g_pstProcessInfo[uLParam];
    pstProcessInfo->ulHBFailCnt++;
    pstProcessInfo->hTmrHBTimeout = NULL;

    dos_printf("Process lost.");
}

/**
 * 函数：S32 bh_process_list(U32 ulIndex, S32 argc, S8 **argv)
 * 功能：命令行回调函数，打印客户端列表
 * 参数：
 *      U32 ulIndex：客户端标示
 *      S32 argc：参数个数
 *      S8 **argv：参数集
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 bh_process_list(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 i, ulTotal, ulActive;
    S8 szBuff[258];

    snprintf(szBuff, sizeof(szBuff)
                    , "\r\n%-32s%-16s%-10s%-10s%-10s\r\n"
                    , "Name"
                    , "Version"
                    , "Status"
                    , "Total HB"
                    , "Fail HB");
    cli_out_string(ulIndex, szBuff);

    ulTotal = 0;
    ulActive = 0;
    for (i=0; i<DOS_PROCESS_MAX_NUM; i++)
    {
        if (g_pstProcessInfo[i]->ulVilad)
        {
            snprintf(szBuff, sizeof(szBuff)
                                , "%-32s%-16s%-10s%-10d%-10d\r\n"
                                , g_pstProcessInfo[i]->szProcessName
                                , g_pstProcessInfo[i]->szProcessVersion
                                , g_pstProcessInfo[i]->ulActive ? "Active" : "Inactive"
                                , g_pstProcessInfo[i]->ulHBCnt
                                , g_pstProcessInfo[i]->ulHBFailCnt);
            cli_out_string(ulIndex, szBuff);
            ulTotal++;
        }
    }

    snprintf(szBuff, sizeof(szBuff)
                        , "-----------------------------------------------------------------------------------------\r\n");
    cli_out_string(ulIndex, szBuff);
    snprintf(szBuff, sizeof(szBuff)
                            , "Total:%d,   Active:%d \r\n"
                            , ulTotal, ulActive);
    cli_out_string(ulIndex, szBuff);

    return 0;
}


/**
 * 函数：VOID hb_process_lost(PROCESS_INFO_ST *pstProcessInfo)
 * 功能：发现客户端心跳断开
 * 参数：
 *      PROCESS_INFO_ST *pstProcessInfo: 进程控制块
 * 返回值：
 *      成功返回0，失败返回－1
 */
VOID hb_process_lost(PROCESS_INFO_ST *pstProcessInfo)
{
    S32 lTreatmentNo;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return;
    }

    lTreatmentNo = config_hb_get_treatment();
    pstProcessInfo->ulActive = DOS_FALSE;
    if (lTreatmentNo < 0)
    {
        lTreatmentNo = 0;
    }

    hb_logr_error("Process \"%s\" lost. Treate as %d", pstProcessInfo->szProcessName, lTreatmentNo);
}


/**
 * 函数：PROCESS_INFO_ST *hb_alloc_process()
 * 功能：分配进程控制块函数
 * 参数：
 * 返回值：成功返回进程控制块，失败返回－1
 */
PROCESS_INFO_ST *hb_alloc_process()
{
    S32 i;

    for (i=0; i<DOS_PROCESS_MAX_NUM; i++)
    {
        if (!g_pstProcessInfo[i]->ulVilad)
        {
            break;
        }
    }

    if (DOS_PROCESS_MAX_NUM <= i)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return g_pstProcessInfo[i];
}

/**
 * 函数：PROCESS_INFO_ST *hb_alloc_process()
 * 功能：分配进程控制块函数
 * 参数：
 * 返回值：成功返回进程控制块，失败返回－1
 */
PROCESS_INFO_ST *hb_find_process(HEARTBEAT_DATA_ST *pstHBData)
{
    S32 i;

    for (i=0; i<DOS_PROCESS_MAX_NUM; i++)
    {
        if (dos_strcmp(pstHBData->szProcessName, g_pstProcessInfo[i]->szProcessName) == 0
                && dos_strcmp(pstHBData->szProcessVersion, g_pstProcessInfo[i]->szProcessVersion) == 0)
        {
            return g_pstProcessInfo[i];
        }
    }

    return NULL;
}

/**
 *  函数：S32 hb_mod_msg_proc(VOID *pMsg, U32 uiLen, struct sockaddr_un *pstServerAddr,S32 lSocket)
 *  功能：该函数对收到的监控进程的数据进行分发
 *  参数：
 *      VOID *pMsg：消息数据
 *      U32 uiLen：消息长度
 *      struct sockaddr_un *pstServerAddr: 对端地址
 *      S32 lSocket：链接socket
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_msg_proc(VOID *pMsg, U32 uiLen, struct sockaddr_un *pstServerAddr, S32 lAddrLen, S32 lSocket)
{
    HEARTBEAT_DATA_ST stHBData;
    PROCESS_INFO_ST *pstProcessInfo;
    S32 lResult = -1;

    if (!pMsg || uiLen < sizeof(HEARTBEAT_DATA_ST))
    {
        DOS_ASSERT(0);
        hb_logr_warning("%s", "Heartbeat server recv invalid msg.");
        return -1;
    }

    if (!pstServerAddr)
    {
        DOS_ASSERT(0);
        hb_logr_warning("%s", "Heartbeat server recv msg with unknow address.");
        return -1;
    }

    memcpy((VOID *)&stHBData, pMsg, sizeof(HEARTBEAT_DATA_ST));

    /* 查找进程, 如果没有，就要添加一个 */
    pstProcessInfo = hb_find_process(&stHBData);
    if (!pstProcessInfo)
    {
        hb_logr_info("%s", "New process register, alloc ccb.");
        pstProcessInfo = hb_alloc_process(&stHBData);

        pstProcessInfo->ulVilad = DOS_TRUE;
        pstProcessInfo->ulActive = DOS_TRUE;
        pstProcessInfo->ulHBCnt = 0;
        pstProcessInfo->ulStatus = PROCESS_HB_INIT;
        pstProcessInfo->hTmrHBTimeout = NULL;


        dos_strncpy(pstProcessInfo->szProcessName, stHBData.szProcessName, sizeof(pstProcessInfo->szProcessName));
        pstProcessInfo->szProcessName[sizeof(pstProcessInfo->szProcessName) - 1] = '\0';

        dos_strncpy(pstProcessInfo->szProcessVersion, stHBData.szProcessVersion, sizeof(pstProcessInfo->szProcessVersion));
        pstProcessInfo->szProcessVersion[sizeof(pstProcessInfo->szProcessVersion) -1] = '\0';
    }

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        hb_logr_warning("%s", "Cannot alloc process info block.");
        return -1;
    }

    /* 有可能对方重连了，所以要更新一下 */
    pstProcessInfo->lSocket = lSocket;
    dos_memcpy(&pstProcessInfo->stPeerAddr, pstServerAddr, sizeof(struct sockaddr_un));
    pstProcessInfo->ulPeerAddrLen = lAddrLen;

    hb_logr_debug("Heartbeat server recv msg. Cmd:%d, Length:%d, Process name:%s, Process version:%s, ID:%d, %p"
                    , stHBData.ulCommand, uiLen
                    , pstProcessInfo->szProcessName
                    , pstProcessInfo->szProcessVersion
                    , pstProcessInfo->ulProcessCBNo
                    , pstProcessInfo);

    /* 分发消息 */
    switch (stHBData.ulCommand)
    {
        case HEARTBEAT_DATA_REG:
            lResult = hb_reg_proc(pstProcessInfo);
            break;
        case HEARTBEAT_DATA_REG_RESPONSE:
            DOS_ASSERT(0);
            lResult = -1;
            break;
        case HEARTBEAT_DATA_UNREG:
            lResult = hb_unreg_proc(pstProcessInfo);
            break;
        case HEARTBEAT_DATA_UNREG_RESPONSE:
            DOS_ASSERT(0);
            lResult = -1;
            break;
        case HEARTBEAT_DATA_HB:
            lResult = hb_heartbeat_proc(pstProcessInfo);
            break;
        case HEARTBEAT_WARNING_SEND:
            lResult = hb_recv_external_warning(pMsg);
            break;
        case HEARTBEAT_WARNING_SEND_RESPONSE:
            DOS_ASSERT(0);
            break;
        case HEARTBEAT_SYS_REBOOT_RESPONSE:
            pstProcessInfo->bRecvRebootRsp = DOS_TRUE;
            break;
        default:
            DOS_ASSERT(0);
            lResult = -1;
            break;
    }

    return lResult;
}



/**
 *  函数：VOID *heartbeat_task(VOID *ptr)
 *  功能：心跳住线程函数
 *  参数：
 *  返回值：成功返回0.失败返回－1
 */
VOID *heartbeat_task(VOID *ptr)
{
    S32 lRet, lMaxFd;
    fd_set stFdset;
    struct timeval stTimeout={1, 0};
    struct sockaddr_un stServerAddr;
    socklen_t lAddrLen;

    if (g_lSocket < 0)
    {
        DOS_ASSERT(0);
        pthread_exit(0);
    }

    while (1)
    {
        FD_ZERO(&stFdset);
        FD_SET(g_lSocket, &stFdset);
        lMaxFd = g_lSocket + 1;
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;

        lRet = select(lMaxFd, &stFdset, NULL, NULL, &stTimeout);
        if (-1 == lRet)
        {
            perror("Error happened when select return.");
            g_lSocket = -1;
            hb_logr_warning("%s", "Exception occurred in heartbeat client tast.");
            break;
        }
        else if (0 == lRet)
        {
            continue;
        }

        g_szRecvBuf[0] = '\0';
        lAddrLen = sizeof(stServerAddr);
        lRet = recvfrom(g_lSocket
                , g_szRecvBuf
                , sizeof(g_szRecvBuf)
                , 0
                , (struct sockaddr *)&stServerAddr
                , &lAddrLen);
        if (lRet < 0)
        {
            g_lSocket = -1;
            DOS_ASSERT(0);
            break;
        }

        hb_msg_proc(g_szRecvBuf, lRet, &stServerAddr, lAddrLen, g_lSocket);
    }

    DOS_ASSERT(0);
    hb_logr_warning("%s", "Heartbeat server will be exited. The recv socket failed.s");

    return 0;
}


S32 hb_read_mod_config()
{
    return 0;
}

/**
 *  函数：S32 heartbeat_init()
 *  功能：初始化心跳模块
 *  参数：
 *  返回值：成功返回0.失败返回－1
 */
S32 heartbeat_init()
{
    U32 i;
    S32 lRet;
    g_lHBTaskWaitingExit = 0;
    S8 szBuffSockPath[256] = { 0 };
    S8 szBuffCMD[256];
    PROCESS_INFO_ST *pstProcessMem = NULL;

    /* 初始化进程控制块 */
    pstProcessMem = (PROCESS_INFO_ST *)dos_smem_alloc(sizeof(PROCESS_INFO_ST) * DOS_PROCESS_MAX_NUM);
    dos_memzero((VOID*)pstProcessMem, sizeof(PROCESS_INFO_ST) * DOS_PROCESS_MAX_NUM);
    for (i = 0; i < DOS_PROCESS_MAX_NUM; i++)
    {
        g_pstProcessInfo[i] = pstProcessMem + i;
        g_pstProcessInfo[i]->ulVilad = DOS_FALSE;
        g_pstProcessInfo[i]->ulActive = DOS_FALSE;
        g_pstProcessInfo[i]->ulProcessCBNo = i;
        g_pstProcessInfo[i]->ulHBCnt = 0;
        g_pstProcessInfo[i]->ulHBFailCnt = 0;
        g_pstProcessInfo[i]->ulPeerAddrLen = 0;
        g_pstProcessInfo[i]->ulStatus = PROCESS_HB_BUTT;
        g_pstProcessInfo[i]->hTmrHBTimeout = NULL;
        g_pstProcessInfo[i]->bRecvRebootRsp = DOS_FALSE;
    }

    if (DOS_SUCC == config_hb_init())
    {
        S32 ulRes;

        for (i = 0; i < DOS_PROCESS_MAX_NUM; i++)
        {
            ulRes = config_hb_get_process_list(i
                    , g_pstProcessInfo[i]->szProcessName
                    , sizeof(g_pstProcessInfo[i]->szProcessName)
                    , g_pstProcessInfo[i]->szProcessVersion
                    , sizeof(g_pstProcessInfo[i]->szProcessVersion));
            if (ulRes < 0)
            {
                g_pstProcessInfo[i]->szProcessName[0] = '\0';
                g_pstProcessInfo[i]->szProcessVersion[0] = '\0';
            }
            else
            {
                g_pstProcessInfo[i]->ulVilad = DOS_TRUE;
            }
        }

        config_hb_deinit();
    }

    /* 初始化socket */
    g_lSocket = -1;
    g_lSocket = socket (AF_UNIX, SOCK_DGRAM, 0);
    if (g_lSocket < 0)
    {
        perror ("create socket failed");
        return -1;
    }

    snprintf(szBuffCMD, sizeof(szBuffCMD), "mkdir -p %s/var/run/socket", dos_get_sys_root_path());
    system(szBuffCMD);
    snprintf(szBuffSockPath, sizeof(szBuffSockPath), "%s/var/run/socket/moniter.sock"
                , dos_get_sys_root_path());
    unlink(szBuffSockPath);

    dos_memzero(&g_stServerAddr, sizeof(g_stServerAddr));
    g_stServerAddr.sun_family = AF_UNIX;
    dos_strcpy(g_stServerAddr.sun_path, szBuffSockPath);
    S32 lLen = offsetof (struct sockaddr_un, sun_path) + strlen(szBuffSockPath);
    lRet = bind(g_lSocket, (struct sockaddr*)&g_stServerAddr, lLen);
    if(lRet < 0)
    {
        hb_logr_error("Cannot bind server socket.(%d)\n", errno);
        close(g_lSocket);
        return DOS_FAIL;
    }

    return 0;
}


/**
 *  函数：S32 heartbead_start()
 *  功能：启动心跳线程
 *  参数：
 *  返回值：成功返回0.失败返回－1
 */
S32 heartbeat_start()
{
    S32 iResult = 0;

    iResult = pthread_create(&g_pthIDHBTask, NULL, heartbeat_task, NULL);
    if (iResult < 0)
    {
        return -1;
    }

    iResult = pthread_create(&g_pthSendMsgTask, NULL, hb_external_warning_proc, NULL);
    if (iResult < 0)
    {
        return -1;
    }

    //pthread_join(g_pthIDHBTask, NULL);
    return 0;
}

/**
 *  函数：S32 heartbead_stop()
 *  功能：停止心跳线程
 *  参数：
 *  返回值：成功返回0.失败返回－1
 */
S32 heartbeat_stop()
{
    pthread_mutex_lock(&g_HBMutex);
    g_lHBTaskWaitingExit = 1;
    pthread_mutex_unlock(&g_HBMutex);

    pthread_join(g_pthIDHBTask, NULL);

    return 0;
}


/**
 *  函数：U32 mon_restart_immediately()
 *  功能：立刻系统重启
 *  返回值：成功返回DOS_SUCC.失败返回DOS_FAIL
 */
static U32 mon_restart_immediately()
{
    S8  szReboot[32] = {0};

    logr_alert("System will be restart after %u minutes.", MIN_WAIT_TIME);

    dos_snprintf(szReboot, sizeof(szReboot), "shutdown -r %u &", MIN_WAIT_TIME);
    mon_system(szReboot);

    return DOS_SUCC;
}

static U32 mon_cancel_restart()
{
    S8  szReboot[32] = {0};

    dos_snprintf(szReboot, sizeof(szReboot), "shutdown -c &", MIN_WAIT_TIME);
    mon_system(szReboot);

    return DOS_SUCC;
}

/**
 *  函数：U32 mon_restart_fixed(U32 ulTimeStamp)
 *  功能：指定时间系统重启
 *  参数:
         U32 ulTimeStamp  重启时间戳
 *  返回值：成功返回DOS_SUCC.失败返回DOS_FAIL
 */
static U32 mon_restart_fixed(U32 ulTimeStamp)
{
    time_t ulCurTimeStamp = time(0);
    U32 ulTimeDiff = U32_BUTT;
    S8  szReboot[32] = {0};

    if (ulTimeStamp <= ulCurTimeStamp)
    {
        DOS_ASSERT(0);
        hb_logr_debug("Your TimeStamp is %u, but current timestamp is %u. Please check the time.", ulTimeStamp, ulCurTimeStamp);
        return DOS_FAIL;
    }

    ulTimeDiff = ulTimeStamp - ulCurTimeStamp;
    /* 将秒转换为分钟 */
    ulTimeDiff = (ulTimeDiff + ulTimeDiff % 60)/60;
    /* 默认至少给2分钟时间 */
    if (ulTimeDiff < MIN_WAIT_TIME)
    {
        ulTimeDiff = MIN_WAIT_TIME;
    }

    logr_alert("System will be restart after %u minutes.", ulTimeDiff);

    dos_snprintf(szReboot, sizeof(szReboot), "shutdown -r %u &", ulTimeDiff);

    mon_system(szReboot);

    return DOS_SUCC;
}

/**
 *  函数：U32 mon_restart_later()
 *  功能：稍后系统重启(即没有业务的时候)
 *  返回值：成功返回DOS_SUCC.失败返回DOS_FAIL
 */
static U32 mon_restart_later()
{
    U32  ulStartTime = time(0);
    U32  ulCurTime, ulIndex = 0;
    BOOL bCanReboot = DOS_TRUE;

    while (1)
    {
        ulCurTime = time(0);
        if (ulCurTime - ulStartTime >= MAX_WAIT_TIME * 60)
        {
            mon_system("/sbin/reboot &");
            break;
        }

        for (ulIndex = 0; ulIndex < DOS_PROCESS_MAX_NUM; ++ulIndex)
        {
            if (g_pstProcessInfo[ulIndex]->ulVilad == DOS_TRUE
                && g_pstProcessInfo[ulIndex]->ulActive == DOS_TRUE
                && g_pstProcessInfo[ulIndex]->bRecvRebootRsp == DOS_FALSE)
            {
                bCanReboot = DOS_FALSE;
                break;
            }
        }

        if (bCanReboot)
        {
            mon_system("/sbin/reboot &");
            break;
        }
        else
        {
            /* 每隔5秒钟去检查一次 */
            sleep(5);
        }
    }

    return DOS_SUCC;
}

U32 g_ulCycleRestartTime     = 0;
U32 g_ulCycleRestartRunning  = 0;
pthread_t  g_pthCycleRestartRunning;

static void * mon_restart_cycle_task(VOID *ptr)
{
    U32 ulTime = 0;
    U32 ulType = 0;
    U32 ulWeekDay = 0;
    U32 ulMonthDay = 0;
    U32 ulHour = 0;
    U32 ulMin = 0;
    U32 ulCurrentTime;
    struct tm stTime;

    g_ulCycleRestartRunning = DOS_TRUE;
    logr_alert("Restart task has been started.");

    while (1)
    {
        dos_task_delay(1000 * 60);

        if (!g_ulCycleRestartRunning)
        {
            goto proc_fail;
        }

        if (ulTime != g_ulCycleRestartTime)
        {
            ulTime = g_ulCycleRestartTime;
            ulType = (ulTime >> 24) & 0xFF;
            ulWeekDay = (ulTime >> 16) & 0xFF;
            ulMonthDay = (ulTime >> 16) & 0xFF;
            ulHour = (ulTime >> 8) & 0xFF;
            ulMin = (ulTime) & 0xFF;
        }

        ulCurrentTime = time(NULL);
        dos_get_localtime_struct(ulCurrentTime, &stTime);
        switch (ulType)
        {
            case MON_SYS_RESTART_SUB_TYPE_DAILY:
                if (stTime.tm_hour == ulHour && stTime.tm_min == ulMin)
                {
                    logr_alert("Daily restart time fired. Time: %02d:%02d", stTime.tm_hour, stTime.tm_min);
                    goto proc_restart;
                }
                break;

            case MON_SYS_RESTART_SUB_TYPE_WEEKLY:
                if (stTime.tm_hour == ulHour && stTime.tm_min == ulMin && ulWeekDay == stTime.tm_wday)
                {
                    logr_alert("Daily restart time fired. Time: %02d:%02d Week: %d", stTime.tm_hour, stTime.tm_min, stTime.tm_wday);
                    goto proc_restart;
                }

                break;

            case MON_SYS_RESTART_SUB_TYPE_MONTHLY:
                if (stTime.tm_hour == ulHour && stTime.tm_min == ulMin && ulMonthDay == stTime.tm_mday)
                {
                    logr_alert("Daily restart time fired. Time: %02d:%02d Day: %d", stTime.tm_hour, stTime.tm_min, stTime.tm_mday);
                    goto proc_restart;
                }

                break;

            default:
                goto proc_fail;
        }
    }

proc_restart:
    logr_alert("Cycle restart timer fired.");

    mon_restart_immediately();

    g_ulCycleRestartRunning = DOS_FALSE;

    return NULL;

proc_fail:
    g_ulCycleRestartRunning = DOS_FALSE;
    logr_alert("Restart task has been exited.");
    return NULL;
}

U32 mon_restart_cycle_setting(U32 ulTime)
{
    g_ulCycleRestartTime = ulTime;

    return DOS_SUCC;
}

static U32 mon_restart_cycle_init()
{
    return DOS_SUCC;
}

U32 mon_restart_cycle_start()
{
    if (!g_ulCycleRestartRunning)
    {
        mon_restart_cycle_init();
        pthread_create(&g_pthCycleRestartRunning, NULL, mon_restart_cycle_task, NULL);
    }

    return DOS_SUCC;
}

static U32 mon_restart_cycle_stop()
{
    g_ulCycleRestartRunning = DOS_FALSE;
    return DOS_SUCC;
}


/**
 *  函数：U32 mon_restart_system(U32 ulStyle, U32 ulTimeStamp)
 *  功能：系统重启
 *  参数：
 *        U32 ulStyle       --重启方式，参见MON_SYS_RESTART_TYPE
 *        U32 ulTimeStamp   --重启时刻的时间戳
 *  返回值：成功返回DOS_SUCC.失败返回DOS_FAIL
 */
U32 mon_restart_system(U32 ulStyle, U32 ulTimeStamp, U32 ulStatus)
{
    U32 ulRet = U32_BUTT;
    HEARTBEAT_DATA_ST stData;
    U32 ulIndex = 0;

    hb_logr_notice("Request restart. Type: %u, Time: %X", ulStyle, ulTimeStamp);

    /* 为每一个进程进行一次通知 */
    for (ulIndex = 0; ulIndex < DOS_PROCESS_MAX_NUM; ulIndex++)
    {
        if (!g_pstProcessInfo[ulIndex]->ulVilad
            || !g_pstProcessInfo[ulIndex]->ulActive)
        {
            continue;
        }

        dos_memzero((VOID*)&stData, sizeof(stData));
        stData.ulCommand = HEARTBEAT_SYS_REBOOT;
        dos_snprintf(stData.szProcessName, sizeof(stData.szProcessName), "%s", g_pstProcessInfo[ulIndex]->szProcessName);
        dos_snprintf(stData.szProcessVersion, sizeof(stData.szProcessVersion), "%s", g_pstProcessInfo[ulIndex]->szProcessVersion);
        hb_send_msg((U8 *)&stData, sizeof(HEARTBEAT_DATA_ST)
                        , &g_pstProcessInfo[ulIndex]->stPeerAddr
                        , g_pstProcessInfo[ulIndex]->ulPeerAddrLen
                        , g_pstProcessInfo[ulIndex]->lSocket);
    }

    switch (ulStyle)
    {
        /* 立即重启 */
        case MON_SYS_RESTART_IMMEDIATELY:
        {
            ulRet = mon_restart_immediately();
            if (DOS_SUCC != ulRet)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            break;
        }
        /* 指定时间重启 */
        case MON_SYS_RESTART_FIXED:
        {
            if (ulStatus)
            {
                ulRet = mon_restart_fixed(ulTimeStamp);
                if (DOS_SUCC != ulRet)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
            }
            else
            {
                ulRet = mon_cancel_restart();
            }
            break;
        }
        /* 稍后重启(没有业务跑的时候重启) */
        case MON_SYS_RESTART_LATER:
        {
            ulRet = mon_restart_later();
            if (DOS_SUCC != ulRet)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            break;
        }
        case MON_SYS_RESTART_CYCLE:
        {
            if (ulStatus)
            {
                mon_restart_cycle_setting(ulTimeStamp);
                mon_restart_cycle_start();
            }
            else
            {
                mon_restart_cycle_stop();
            }

            break;
        }
        default:
            break;
    }
    hb_logr_debug("Restart System SUCC.(Style:%u, TimeStamp:%u).", ulStyle, ulTimeStamp);

    return DOS_SUCC;
}

S32 mon_init_restart_task_cb(VOID* data, S32 lFieldCnt, S8** pszValue, S8** pszFiald)
{
    if (DOS_ADDR_INVALID(pszFiald) || DOS_ADDR_INVALID(pszValue) || DOS_ADDR_INVALID(data))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (lFieldCnt < 2
        || DOS_ADDR_INVALID(pszFiald[0])
        || DOS_ADDR_INVALID(pszFiald[1]))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszValue[0])
        || DOS_ADDR_INVALID(pszValue[1]))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return dos_atoul(pszValue[1], (U32 *)data);
}

U32 mon_start_restart_task()
{
    S8 szBuffer[128];
    U32 ulStatus = 0;
    U32 ulTime = 0;
    U32 ulRet;

    dos_snprintf(szBuffer, sizeof(szBuffer), "SELECT parameter_name,parameter_value FROM tbl_parameters WHERE id=16");
    ulRet = db_query(g_pstCCDBHandle, szBuffer, mon_init_restart_task_cb, &ulStatus, NULL);

    if (ulRet != DOS_SUCC)
    {
        return ulRet;
    }

    if (ulStatus)
    {
        dos_snprintf(szBuffer, sizeof(szBuffer), "SELECT parameter_name,parameter_value FROM tbl_parameters WHERE id=15");
        ulRet = db_query(g_pstCCDBHandle, szBuffer, mon_init_restart_task_cb, &ulTime, NULL);

        if (ulRet != DOS_SUCC)
        {
            return ulRet;
        }

        logr_notice("Restart task found. Time: %u", ulTime);

        mon_restart_cycle_setting(ulTime);
        mon_restart_cycle_start();
    }

    return DOS_SUCC;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

