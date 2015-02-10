/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_task_pub.h
 *
 *  创建时间: 2014年12月16日10:15:56
 *  作    者: Larry
 *  描    述: 业务控制模块，群呼任务相关定义
 *  修改历史:
 */

#ifndef __SC_TASK_PUB_H__
#define __SC_TASK_PUB_H__
#if 0
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif
/* include public header files */

/* include private header files */

/* define marcos */

/* 检测一个TCB是有正常的Task和CustomID */
#define SC_TCB_HAS_VALID_OWNER(pstTCB)                        \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0                                \
    && (pstTCB)->ulTaskID != U32_BUTT                         \
    && (pstTCB)->ulCustomID != 0                              \
    && (pstTCB)->ulCustomID != U32_BUTT)

#define SC_CCB_HAS_VALID_OWNER(pstCCB)                        \
    ((pstCCB)                                                 \
    && (pstCCB)->ulTaskID != 0                                \
    && (pstCCB)->ulTaskID != U32_BUTT                         \
    && (pstCCB)->ulCustomID != 0                              \
    && (pstCCB)->ulCustomID != U32_BUTT)


#define SC_TCB_VALID(pstTCB)                                  \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0)

#define SC_CCB_IS_VALID(pstCCB)                               \
    ((pstCCB) && (pstCCB)->bValid)

#define SC_CCB_SET_STATUS(pstCCB, ulStatus)                   \
do                                                            \
{                                                             \
    if (DOS_ADDR_INVALID(pstCCB)                              \
        || ulStatus >= SC_CCB_BUTT)                           \
    {                                                         \
        break;                                                \
    }                                                         \
    pthread_mutex_lock(&(pstCCB)->mutexCCBLock);              \
    sc_call_trace((pstCCB), "CCB Status Change %s -> %s"      \
                , sc_ccb_get_status((pstCCB)->usStatus)       \
                , sc_ccb_get_status(ulStatus));               \
    (pstCCB)->usStatus = ulStatus;                            \
    pthread_mutex_unlock(&(pstCCB)->mutexCCBLock);            \
}while(0)


#define SC_TRACE_HTTPD                  (1<<1)
#define SC_TRACE_HTTP                   (1<<2)
#define SC_TRACE_TASK                   (1<<3)
#define SC_TRACE_SC                     (1<<4)
#define SC_TRACE_ACD                    (1<<5)
#define SC_TRACE_DIAL                   (1<<6)
#define SC_TRACE_FUNC                   (1<<7)
#define SC_TRACE_ALL                    0xFFFFFFFF

typedef enum tagSiteAccompanyingStatus{
    SC_SITE_ACCOM_DISABLE              = 0,       /* 分机随行，禁止 */
    SC_SITE_ACCOM_ENABLE,                         /* 分机随行，允许 */

    SC_SITE_ACCOM_BUTT                 = 255      /* 分机随行，非法值 */
}SC_SITE_ACCOM_STATUS_EN;


typedef enum tagSysStatus{
    SC_SYS_NORMAL                      = 3,
    SC_SYS_BUSY                        = 4,       /* 任务状态，系统忙，暂停呼叫量在80%以上的任务发起呼叫 */
    SC_SYS_ALERT,                                 /* 任务状态，系统忙，只允许高优先级客户，并且呼叫量在80%以下的客户发起呼叫 */
    SC_SYS_EMERG,                                 /* 任务状态，系统忙，暂停所有任务 */

    SC_SYS_BUTT                        = 255      /* 任务状态，非法值 */
}SC_SYS_STATUS_EN;

typedef enum tagTaskStatusInDB{
    SC_TASK_STATUS_DB_START            = 0,       /* 数据库中任务状态 */
    SC_TASK_STATUS_DB_STOP,                       /* 数据库中任务状态 */

    SC_TASK_STATUS_DB_BUTT
}SC_TASK_STATUS_DB_EN;

typedef enum tagTaskStatus{
    SC_TASK_INIT                       = 0,       /* 任务状态，初始化 */
    SC_TASK_WORKING,                              /* 任务状态，工作 */
    SC_TASK_STOP,                                 /* 任务状态，停止，不再发起呼叫，如果所有呼叫结束即将释放资源 */
    SC_TASK_PAUSED,                               /* 任务状态，暂停 */
    SC_TASK_SYS_BUSY,                             /* 任务状态，系统忙，暂停呼叫量在80%以上的任务发起呼叫 */
    SC_TASK_SYS_ALERT,                            /* 任务状态，系统忙，只允许高优先级客户，并且呼叫量在80%以下的客户发起呼叫 */
    SC_TASK_SYS_EMERG,                            /* 任务状态，系统忙，暂停所有任务 */

    SC_TASK_BUTT                       = 255      /* 任务状态，非法值 */
}SC_TASK_STATUS_EN;

typedef enum tagTaskPriority{
    SC_TASK_PRI_LOW                       = 0,    /* 任务优先级，低优先级 */
    SC_TASK_PRI_NORMAL,                           /* 任务优先级，正常优先级 */
    SC_TASK_PRI_HIGHT,                            /* 任务优先级，高优先级 */
}SC_TASK_PRI_EN;

typedef enum tagCallNumType{
    SC_CALL_NUM_TYPE_NORMOAL              = 0,       /* 号码类型，正常的号码 */
    SC_CALL_NUM_TYPE_EXPR,                           /* 号码类型，正则表达式 */

    SC_CALL_NUM_TYPE_BUTT                 = 255      /* 号码类型，非法值 */
}SC_CALL_NUM_TYPE_EN;


typedef enum tagCCBStatus
{
    SC_CCB_IDEL                           = 0,     /* CCB状态，空闲状态 */
    SC_CCB_INIT,
    SC_CCB_AUTH,                                   /* CCB状态，呼叫递交状态 */
    SC_CCB_EXEC,                                   /* CCB状态，呼叫接收状态 */
    SC_CCB_REPORT,                                 /* CCB状态，呼叫接续状态 */
    SC_CCB_RELEASE,                                /* CCB状态，呼叫释放状态 */

    SC_CCB_BUTT
}SC_CCB_STATUS_EN;

typedef struct tagCallerQueryNode{
    U16        usNo;                              /* 编号 */
    U8         bValid;
    U8         bTraceON;                          /* 是否跟踪 */

    U32        ulIndexInDB;                       /* 数据库中的ID */

    S8         szNumber[SC_TEL_NUMBER_LENGTH];    /* 号码缓存 */
}SC_CALLER_QUERY_NODE_ST;

/* define structs */
typedef struct tagTelNumQueryNode
{
	list_t     stLink;                            /* 队列链表节点 */

    U32        ulIndex;                           /* 数据库中的ID */

    U8         ucTraceON;                         /* 是否跟踪 */
	U8         ucCalleeType;                      /* 被叫号码类型， refer to enum SC_CALL_NUM_TYPE_EN */
    U8         aucRes[2];

	S8         szNumber[SC_TEL_NUMBER_LENGTH];    /* 号码缓存 */
}SC_TEL_NUM_QUERY_NODE_ST;

typedef struct tagSiteQueryNode
{
    U16        usSCBNo;
    U16        usRec;

    U32        bValid:1;
    U32        bRecord:1;
    U32        bTraceON:1;
    U32        bAllowAccompanying:1;              /* 是否允许分机随行 refer to SC_SITE_ACCOM_STATUS_EN */
    U32        ulRes1:28;

	U32        ulStatus;                          /* 坐席状态 refer to SC_SITE_STATUS_EN */
    U32        ulSiteID;                          /* 坐席数据库编号 */

    S8         szUserID[SC_TEL_NUMBER_LENGTH];    /* SIP User ID */
    S8         szExtension[SC_TEL_NUMBER_LENGTH]; /* 分机号 */
    S8         szEmpNo[SC_EMP_NUMBER_LENGTH];     /* 号码缓存 */
}SC_SITE_QUERY_NODE_ST;

typedef struct tagTaskAllowPeriod{
    U8         ucValid;
    U8         ucWeekMask;                        /* 周控制，使用位操作，第0位为星期天 */
    U8         ucHourBegin;                       /* 开始时间，小时 */
    U8         ucMinuteBegin;                     /* 开始时间，分钟 */

    U8         ucSecondBegin;                     /* 开始时间，秒 */
    U8         ucHourEnd;                         /* 结束时间，小时 */
    U8         ucMinuteEnd;                       /* 结束时间，分钟 */
    U8         ucSecondEnd;                       /* 结束时间，秒 */

    U32        ulRes;
}SC_TASK_ALLOW_PERIOD_ST;


/* 呼叫控制块 */
typedef struct tagSCCCB{
    U16       usCCBNo;                            /* 编号 */
    U8        ucTerminationFlag;                  /* 业务终止标志 */
    U8        ucTerminationCause;                 /* 业务终止原因 */

    U32       bValid:1;                           /* 是否合法 */
    U32       bTraceNo:1;                         /* 是否跟踪 */
    U32       bBanlanceWarning:1;                 /* 是否余额告警 */
    U32       bNeedConnSite:1;                    /* 接通后是否需要接通坐席 */
    U32       ulRes:28;

    U16       usTCBNo;                            /* 任务控制块编号ID */
    U16       usSiteNo;                           /* 坐席编号 */
    U16       usCallerNo;                         /* 主叫号码编号 */
    U16       usStatus;                           /* 呼叫控制块编号，refer to SC_CCB_STATUS_EN */

    U8        aucServiceType[SC_MAX_SRV_TYPE_PRE_LEG];        /* 业务类型 */
    U32       ulCurrentSrvInd;                                /* 当前空闲的业务类型索引 */

    U32       ulCustomID;                         /* 当前呼叫属于哪个客户 */
    U32       ulAgentID;                          /* 当前呼叫属于哪个客户 */
    U32       ulTaskID;                           /* 当前任务ID */
    U32       ulTrunkID;                          /* 中继ID */
    U32       ulAuthToken;                        /* 与计费模块的关联字段 */
    U32       ulCallDuration;                     /* 呼叫时长，防止吊死用，每次心跳时更新 */
    U32       ulOtherLegID;                       /* 另外一个leg的CCB编号 */

    U16       usHoldCnt;                          /* 被hold的次数 */
    U16       usHoldTotalTime;                    /* 被hold的总时长 */
    U32       ulLastHoldTimetamp;                 /* 上次hold是的时间戳，解除hold的时候值零 */

    S8        szCallerNum[SC_TEL_NUMBER_LENGTH];  /* 主叫号码 */
    S8        szCalleeNum[SC_TEL_NUMBER_LENGTH];  /* 被叫号码 */
    S8        szANINum[SC_TEL_NUMBER_LENGTH];     /* 被叫号码 */
    S8        szDialNum[SC_TEL_NUMBER_LENGTH];    /* 用户拨号 */
    S8        szSiteNum[SC_TEL_NUMBER_LENGTH];    /* 坐席号码 */
    S8        szUUID[SC_MAX_UUID_LENGTH];         /* Leg-A UUID */

    U32       ulBSMsgNo;                          /* 和BS通讯使用的资源号 */

    sem_t     semCCBSyn;                          /* 用于同步的CCB */
    pthread_mutex_t mutexCCBLock;                 /* 保护CCB的锁 */
}SC_CCB_ST;

/* 定义CCBhash表 */
typedef struct tagCCBHashNode
{
    HASH_NODE_S     stNode;                       /* hash链表节点 */

    S8              szUUID[SC_MAX_UUID_LENGTH];   /* UUID */
    SC_CCB_ST       *pstCCB;                      /* CCB指针 */

    sem_t           semCCBSyn;
}SC_CCB_HASH_NODE_ST;


typedef struct tagTaskCB
{
    U16        usTCBNo;                           /* 编号 */
    U8         ucValid;                           /* 是否被使用 */
    U8         ucTaskStatus;                      /* 任务状态 refer to SC_TASK_STATUS_EN */
    U8         ucPriority;                        /* 任务优先级 */
    U8         ucAudioPlayCnt;                    /* 语言播放次数 */
    U8         bTraceON;                          /* 是否跟踪 */
    U8         bTraceCallON;                      /* 是否跟踪呼叫 */

    pthread_t  pthID;                             /* 线程ID */
    pthread_mutex_t  mutexTaskList;               /* 保护任务队列使用的互斥量 */

    U32        ulTaskID;                          /* 呼叫任务ID */
    U32        ulCustomID;                        /* 呼叫任务所属 */
    U32        ulConcurrency;                     /* 当前并发数 */

    U16        usSiteCount;                       /* 坐席数量 */
    U16        usCallerCount;                     /* 当前主叫号码数量 */
    U32        ulLastCalleeIndex;                 /* 用于数据分页 */

    list_t     stCalleeNumQuery;                  /* 被叫号码缓存 refer to struct tagTelNumQueryNode */
    S8         szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN];      /* 语言文件文件名 */
    SC_CALLER_QUERY_NODE_ST *pstCallerNumQuery;    /* 主叫号码缓存 refer to struct tagTelNumQueryNode */
    SC_SITE_QUERY_NODE_ST   *pstSiteQuery;         /* 主叫号码缓存 refer to struct tagSiteQueryNode*/
    SC_TASK_ALLOW_PERIOD_ST astPeriod[SC_MAX_PERIOD_NUM];   /* 任务执行时间段 */

    /* 统计相关 */
    U32        ulTotalCall;                       /* 总呼叫数 */
    U32        ulCallFailed;                      /* 呼叫失败数 */
    U32        ulCallConnected;                   /* 呼叫接通数 */
}SC_TASK_CB_ST;


typedef struct tagTaskCtrlCMD{
    list_t      stLink;
    U32         ulTaskID;                         /* 任务ID */
    U32         ulCMD;                            /* 命令字 */
    U32         ulCustomID;                       /* 客户ID */
    U32         ulAction;                         /* action */
    U32         ulCMDSeq;                         /* 请求序号 */
    U32         ulCMDErrCode;                     /* 错误码 */
    S8          *pszErrMSG;                       /* 错误信息 */
    sem_t       semCMDExecNotify;                 /* 命令执行完毕通知使用的信号量 */
}SC_TASK_CTRL_CMD_ST;

typedef struct tagTaskMngtInfo{
    pthread_t            pthID;                   /* 线程ID */
    pthread_mutex_t      mutexCMDList;            /* 保护命令队列使用的互斥量 */
    pthread_mutex_t      mutexTCBList;            /* 保护任务控制块使用的互斥量 */
    pthread_mutex_t      mutexCallList;           /* 保护呼叫控制块使用的互斥量 */
    pthread_mutex_t      mutexCallHash;           /* 保护呼叫控制块使用的互斥量 */
    pthread_cond_t       condCMDList;             /* 命令队列数据到达通知条件量 */
    U32                  blWaitingExitFlag;       /* 等待退出标示 */

    list_t               stCMDList;               /* 命令队列(节点由HTTP Server创建，有HTTP Server释放) */
    SC_CCB_ST            *pstCallCCBList;         /* 呼叫控制块列表 (需要hash表存储) */
    HASH_TABLE_S         *pstCallCCBHash;         /* 呼叫控制块的hash索引 */
    SC_TASK_CB_ST        *pstTaskList;            /* 任务列表 refer to struct tagTaskCB*/
    U32                  ulTaskCount;             /* 当前正在执行的任务数 */

    U32                  ulMaxCall;               /* 历史最大呼叫并发数 */
    DB_HANDLE_ST         *pstDBHandle;            /* 数据库句柄 */

    SC_SYS_STATUS_EN     enSystemStatus;          /* 系统状态 */
}SC_TASK_MNGT_ST;


/* declare functions */
SC_CCB_ST *sc_ccb_alloc();
VOID sc_ccb_free(SC_CCB_ST *pstCCB);
U32 sc_ccb_init(SC_CCB_ST *pstCCB);
U32 sc_call_set_owner(SC_CCB_ST *pstCCB, U32  ulTaskID, U32 ulCustomID);
U32 sc_call_set_trunk(SC_CCB_ST *pstCCB, U32 ulTrunkID);
U32 sc_call_set_auth_token(SC_CCB_ST *pstCCB, U32 ulToken);
SC_TASK_CB_ST *sc_tcb_find_by_taskid(U32 ulTaskID);
SC_CCB_ST *sc_ccb_get(U32 ulIndex);

SC_TASK_CB_ST *sc_tcb_alloc();
VOID sc_tcb_free(SC_TASK_CB_ST *pstTCB);
U32 sc_tcb_init(SC_TASK_CB_ST *pstTCB);
VOID sc_task_set_owner(SC_TASK_CB_ST *pstTCB, U32 ulTaskID, U32 ulCustomID);
VOID sc_task_set_current_call_cnt(SC_TASK_CB_ST *pstTCB, U32 ulCurrentCall);
U32 sc_task_get_current_call_cnt(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_site(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_caller(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_callee(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_period(SC_TASK_CB_ST *pstTCB);
U32 sc_task_update_stat(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_time(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_status(SC_TASK_CB_ST *pstTCB);
U32 sc_task_get_call_interval(SC_TASK_CB_ST *pstTCB);
U32 sc_task_set_recall(SC_TASK_CB_ST *pstTCB);
U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_audio_playcnt(U32 ulTCBNo);
U32 sc_task_get_timeout_for_noanswer(U32 ulTCBNo);
U32 sc_dialer_add_call(SC_CCB_ST *pstCCB);
VOID sc_call_trace(SC_CCB_ST *pstCCB, const S8 *szFormat, ...);
U32 sc_task_callee_set_recall(SC_TASK_CB_ST *pstTCB, U32 ulIndex);
U32 sc_task_load_audio(SC_TASK_CB_ST *pstTCB);
BOOL sc_ccb_is_valid(SC_CCB_ST *pstCCB);
U32 sc_task_init(SC_TASK_CB_ST *pstTCB);
BOOL sc_call_check_service(SC_CCB_ST *pstCCB, U32 ulService);

U32 sc_task_continue(SC_TASK_CB_ST *pstTCB);
U32 sc_task_pause(SC_TASK_CB_ST *pstTCB);
U32 sc_task_start(SC_TASK_CB_ST *pstTCB);
U32 sc_task_stop(SC_TASK_CB_ST *pstTCB);
S8 *sc_ccb_get_status(U32 ulStatus);
U32 sc_ccb_hash_tables_add(S8 *pszUUID, SC_CCB_ST *pstCCB);
U32 sc_ccb_hash_tables_delete(S8 *pszUUID);
SC_CCB_ST *sc_ccb_hash_tables_find(S8 *pszUUID);
U32 sc_ccb_syn_post(S8 *pszUUID);
U32 sc_ccb_syn_wait(S8 *pszUUID);

SC_SYS_STATUS_EN sc_check_sys_stat();


#if 0
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif

#endif /* __SC_TASK_PUB_H__ */

