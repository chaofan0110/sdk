/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_event_process.c
 *
 *  创建时间: 2015年1月8日20:28:04
 *  作    者: Larry
 *  描    述: 处理FS核心发过来的各种事件
 *  修改历史:
 */

#ifndef _SC_EVENT_PROCESS_H__
#define _SC_EVENT_PROCESS_H__

#define SC_EP_EVENT_LIST \
            "CHANNEL_CREATE " \
            "CHANNEL_DESTROY " \
            "CHANNEL_HANGUP " \
            "CHANNEL_HANGUP_COMPLETE " \
            "CHANNEL_ANSWER " \
            "DTMF " \
            "CHANNEL_PROGRESS " \
            "CHANNEL_PROGRESS_MEDIA " \
            "SESSION_HEARTBEAT "


enum {
    SC_SERV_OUTBOUND_CALL           = 0,   /* 由FS向外部(包括SIP、PSTN)发起的呼叫 */
    SC_SERV_INBOUND_CALL            = 1,   /* 由外部(包括SIP、PSTN)向FS发起的呼叫 */
    SC_SERV_INTERNAL_CALL           = 2,   /* FS和SIP终端之间的呼叫 */
    SC_SERV_EXTERNAL_CALL           = 3,   /* FS和PSTN之间的呼叫 */

    SC_SERV_AUTO_DIALING            = 4,   /* 自动拨号外乎，比例呼叫 */
    SC_SERV_PREVIEW_DIALING         = 5,   /* 预览外乎 */
    SC_SERV_PREDICTIVE_DIALING      = 6,   /* 预测外乎 */

    SC_SERV_RECORDING               = 7,   /* 录音业务 */
    SC_SERV_FORWORD_CFB             = 8,   /* 忙转业务 */
    SC_SERV_FORWORD_CFU             = 9,   /* 无条件转业务 */
    SC_SERV_FORWORD_CFNR            = 10,   /* 无应答转业务 */
    SC_SERV_BLIND_TRANSFER          = 11,  /* 忙转业务 */
    SC_SERV_ATTEND_TRANSFER         = 12,  /* 协商转业务 */

    SC_SERV_PICK_UP                 = 13,  /* 代答业务 */        /* ** */
    SC_SERV_CONFERENCE              = 14,  /* 会议 */
    SC_SERV_VOICE_MAIL_RECORD       = 15,  /* 语言邮箱 */
    SC_SERV_VOICE_MAIL_GET          = 16,  /* 语言邮箱 */

    SC_SERV_SMS_RECV                = 17,  /* 接收短信 */
    SC_SERV_SMS_SEND                = 18,  /* 发送短信 */
    SC_SERV_MMS_RECV                = 19,  /* 接收彩信 */
    SC_SERV_MMS_SNED                = 20,  /* 发送彩信 */

    SC_SERV_FAX                     = 21,  /* 传真业务 */

    SC_SERV_NUMBER_VIRFY            = 22,  /* 号码审核业务 */
    SC_SERV_INGROUP_CALL            = 23,  /* 组内呼叫 */        /* * */
    SC_SERV_OUTGROUP_CALL           = 24,  /* 组外呼叫 */        /* *9 */
    SC_SERV_HOTLINE_CALL            = 25,  /* 热线呼叫 */
    SC_SERV_SITE_SIGNIN             = 26,  /* 坐席签入呼叫 */    /* *2 */

    SC_SERV_WARNING                 = 27,  /* 告警信息 */
    SC_SERV_QUERY_IP                = 28,  /* 查IP信息 */        /* *158 */
    SC_SERV_QUERY_EXTENTION         = 29,  /* 查分机信息 */      /* *114 */
    SC_SERV_QUERY_AMOUNT            = 30,  /* 查余额业务 */      /* *199 */

    SC_SERV_BUTT                    = 255
};

enum {
    SC_NUM_TYPE_USERID              = 0,   /* 号码为SIP User ID */
    SC_NUM_TYPE_EXTENSION,                 /* 号码为分机号 */
    SC_NUM_TYPE_OUTLINE,                   /* 号码为长号 */

    SC_NUM_TYPE_BUTT
};

/* 定义CCBhash表 */
typedef struct tagHMapCCBCalleNode
{
    HASH_NODE_S     stNode;                       /* hash链表节点 */

    S8              szCllee[SC_TEL_NUMBER_LENGTH]; /* 电话号码 */
    S8              szCller[SC_TEL_NUMBER_LENGTH]; /* 电话号码 */
    SC_CCB_ST       *pstCCB;                      /* CCB指针 */
}SC_HMAP_CCB_CALLEE_NODE_ST;


#endif /* end of _SC_EVENT_PROCESS_H__ */

