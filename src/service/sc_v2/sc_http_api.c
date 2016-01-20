/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_api_process.c
 *
 *  创建时间: 2014年12月16日10:18:20
 *  作    者: Larry
 *  描    述: 处理HTTP请求，并将HTTP请求中所携带的命令分发到各个处理模块
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/* include public header files */
#include <dos.h>
#include <esl.h>

/* include private header files */
#include "sc_def.h"
#include "sc_res.h"
#include "sc_debug.h"
#include "sc_httpd.h"
#include "sc_http_api.h"


/* global parameters */
SC_HTTP_REQ_REG_TABLE_SC g_pstHttpReqRegTable[] =
{
    {"reload",                   sc_http_api_reload_xml},
    {"task-ctrl",                sc_http_api_task_ctrl},
    {"verify",                   sc_http_api_num_verify},
    {"callctrl",                 sc_http_api_call_ctrl},
    {"agent-status",             sc_http_api_agent_action},
    {"agent-call-ctrl",          sc_http_api_agent_call_ctrl},
    {"agent",                    sc_http_api_agent},
    {"agentgrp",                 sc_http_api_agent_grp},
    {"gateway",                  sc_http_api_gateway_action},
    {"sip",                      sc_http_api_sip_action},
    {"route",                    sc_http_api_route_action},
    {"gw_group",                 sc_http_api_gw_group_action},
    {"did",                      sc_http_api_did_action},
    {"black",                    sc_http_api_black_action},
    {"caller",                   sc_http_api_caller_action},
    {"callergrp",                sc_http_api_callergrp_action},
    {"callerset",                sc_http_api_callerset_action},
    {"eix",                      sc_http_api_eix_action},
    {"numlmt",                   sc_http_api_numlmt_action},
    {"numtransform",             sc_http_api_numtransform_action},
    {"customer",                 sc_http_api_customer_action},
    {"demo",                     sc_http_api_demo_action},
    {"serv-ctrl",                sc_http_api_serv_ctrl_action},

    {"",                         NULL}
};


http_req_handle sc_http_api_find(S8 *pszName)
{
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pszName) || '\0' == pszName[0])
    {
        return NULL;
    }

    for (ulIndex=0; ulIndex<(sizeof(g_pstHttpReqRegTable)/sizeof(SC_HTTP_REQ_REG_TABLE_SC)); ulIndex++)
    {
        if (g_pstHttpReqRegTable[ulIndex].pszRequest
            && dos_strcmp(g_pstHttpReqRegTable[ulIndex].pszRequest, pszName) == 0
            && g_pstHttpReqRegTable[ulIndex].callback)
        {
            return g_pstHttpReqRegTable[ulIndex].callback;
        }
    }

    return NULL;
}


/* declare functions */
/**
 * 函数: S8 *sc_http_api_get_value(list_t *pstParamList, S8 *pszKey)
 * 功能:
 *      从http请求控制块pstParamList中获取相应字段pszKey的值
 * 参数:
 *      list_t *pstParamList: 参数列表
 *      S8 *pszKey: 参数名
 * 返回值: 如果找到相关参数，就返回该参数的首指针，否则返回NULL
 */
static S8 *sc_http_api_get_value(list_t *pstParamList, S8 *pszKey)
{
    SC_API_PARAMS_ST *pstParamListNode = NULL;
    list_t           *pstLineNode = pstParamList;

    if (!pstParamList || !pszKey)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    while (1)
    {
        if (dos_list_is_empty(pstParamList))
        {
            break;
        }

        pstLineNode = dos_list_work(pstParamList, pstLineNode);
        if (!pstLineNode)
        {
            break;
        }

        pstParamListNode = dos_list_entry(pstLineNode, SC_API_PARAMS_ST, stList);

        if (pstParamListNode
            && 0 == dos_strcmp(pstParamListNode->pszStringName, pszKey))
        {
            return pstParamListNode->pszStringVal;
        }
    }

    return NULL;
}

U32 sc_http_api_reload_xml(list_t *pstArgv)
{
    U32 ulCustomID, ulAction, ulGatewayID, ulDialplan;
    S8  *pszCustomID = NULL, *pszAction = NULL, *pszGatewayID = NULL, *pszDialplan = NULL;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);

        return DOS_FALSE;
    }

    pszCustomID = sc_http_api_get_value(pstArgv, "userid");
    if (!pszCustomID)
    {
        ulCustomID = U32_BUTT;

        goto invalid_params;
    }
    else
    {
        if (dos_atoul(pszCustomID, &ulCustomID) < 0)
        {
            ulCustomID = U32_BUTT;
        }
    }

    pszAction = sc_http_api_get_value(pstArgv, "action");
    if (!pszAction)
    {
        ulAction = SC_API_CMD_ACTION_BUTT;
    }
    else
    {
        ulAction = SC_API_CMD_ACTION_BUTT;

        if (0 == dos_strncmp(pszAction, "pause", dos_strlen("pause")))
        {
            ulAction = SC_API_CMD_ACTION_PAUSE;
        }
        else if (0 == dos_strncmp(pszAction, "monitoring", dos_strlen("monitoring")))
        {
            ulAction = SC_API_CMD_ACTION_MONITORING;
        }
        else if (0 == dos_strncmp(pszAction, "hungup", dos_strlen("hungup")))
        {
            ulAction = SC_API_CMD_ACTION_HUNGUP;
        }
    }

    pszGatewayID = sc_http_api_get_value(pstArgv, "gateway");
    if (!pszGatewayID)
    {
        ulGatewayID = U32_BUTT;
    }
    else
    {
        if (dos_atoul(pszGatewayID, &ulGatewayID) < 0)
        {
            ulGatewayID = U32_BUTT;
        }
    }

    pszDialplan = sc_http_api_get_value(pstArgv, "dialplan");
    if (!pszDialplan)
    {
        ulDialplan = U32_BUTT;
    }
    else
    {
        if (dos_atoul(pszDialplan, &ulDialplan) < 0)
        {
            ulDialplan = U32_BUTT;
        }
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;

}

U32 sc_http_api_task_ctrl(list_t *pstArgv)
{
    S8 *pszCustomID = NULL;
    S8 *pszTaskID = NULL;
    S8 *pszAction = NULL;
    U32 ulCustomID, ulTaskID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);

        return DOS_FALSE;
    }


    pszCustomID = sc_http_api_get_value(pstArgv, "userid");
    if (!pszCustomID || '\0' == pszCustomID[0])
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }
    if (dos_atoul(pszCustomID, &ulCustomID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    pszTaskID = sc_http_api_get_value(pstArgv, "task");
    if (!pszTaskID || '\0' == pszTaskID[0])
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }
    if (dos_atoul(pszTaskID, &ulTaskID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    pszAction = sc_http_api_get_value(pstArgv, "action");
    if (!pszAction || '\0' == pszAction[0])
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }
    if (dos_strcmp(pszAction, "add") == 0)
    {
        ulAction = SC_API_CMD_ACTION_ADD;
    }
    else if (dos_strcmp(pszAction, "delete") == 0)
    {
        ulAction = SC_API_CMD_ACTION_DELETE;
    }
    else if (dos_strcmp(pszAction, "start") == 0)
    {
        ulAction = SC_API_CMD_ACTION_START;
    }
    else if (dos_strcmp(pszAction, "stop") == 0)
    {
        ulAction = SC_API_CMD_ACTION_STOP;
    }
    else if (dos_strcmp(pszAction, "pause") == 0)
    {
        ulAction = SC_API_CMD_ACTION_PAUSE;
    }
    else if (dos_strcmp(pszAction, "continue") == 0)
    {
        ulAction = SC_API_CMD_ACTION_CONTINUE;
    }
    else if (dos_strcmp(pszAction, "update") == 0)
    {
        ulAction = SC_API_CMD_ACTION_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    return sc_task_mngt_cmd_proc(ulAction, ulCustomID, ulTaskID);

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}


U32 sc_http_api_callerset_action(list_t *pstArgv)
{
    S8  *pszCallerSetID = NULL, *pszAction = NULL;
    U32 ulSettingID = U32_BUTT, ulAction = U32_BUTT;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取主叫设定ID */
    pszCallerSetID = sc_http_api_get_value(pstArgv, "callerset_id");
    pszAction = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszCallerSetID)
        || DOS_ADDR_INVALID(pszAction))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_HTTP_API), "%s", "CallerGrp Data is Synchronizing");
    }
    else
    {
        if (dos_atoul(pszCallerSetID, &ulSettingID) < 0)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_HTTP_API), "%s", "Invalid parameters.");
            goto invalid_params;
        }
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_SET_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_SET_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_SET_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (sc_caller_setting_update_proc(ulAction, ulSettingID) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_HTTP_API), "Update Caller Setting FAIL.(ulAction:%u, ulID:%u)"
                        , ulAction, ulSettingID);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }


    return SC_HTTP_ERRNO_SUCC;
invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_callergrp_action(list_t *pstArgv)
{
    S8  *pszCallerGrpID = NULL, *pszAction = NULL;
    U32 ulCallerGrpID = U32_BUTT, ulAction = U32_BUTT;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取主叫号码组的id */
    pszCallerGrpID = sc_http_api_get_value(pstArgv, "callergrp_id");
    pszAction = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszCallerGrpID)
        || DOS_ADDR_INVALID(pszAction))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_HTTP_API), "%s", "CallerGrp Data is Synchronizing");
    }
    else
    {
        if (dos_atoul(pszCallerGrpID, &ulCallerGrpID) < 0)
        {
            DOS_ASSERT(0);
            goto invalid_params;
        }
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_GRP_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_GRP_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_GRP_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (sc_caller_group_update_proc(ulAction, ulCallerGrpID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;
invalid_params:
    return SC_HTTP_ERRNO_INVALID_PARAM;
}


/**
 * 函数: U32 sc_http_api_gateway(SC_HTTP_CLIENT_CB_S *pstClient)
 * 功能:
 *      网关配置文件控制
 * 参数:
 *      SC_HTTP_CLIENT_CB_S *pstClient: 客户端指针
 * 返回值: 成功则返回DOS_TRUE,否则返回DOS_FALSE
 */
U32 sc_http_api_gateway_action(list_t *pstArgv)
{
    S8   *pszGateWayID = NULL, *pszAction = NULL;
    U32  ulGatewayID = U32_BUTT, ulAction = U32_BUTT;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取网关id */
    pszGateWayID = sc_http_api_get_value(pstArgv, "gateway_id");
    /* 获取动作 */
    pszAction = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszGateWayID)
        || DOS_ADDR_INVALID(pszAction))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_HTTP_API), "%s", "Gateway data is synchronizing...");
    }
    else
    {
        if (dos_atoul(pszGateWayID, &ulGatewayID) < 0)
        {
            DOS_ASSERT(0);
            goto invalid_params;
        }
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_GATEWAY_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_GATEWAY_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_GATEWAY_UPDATE;
    }
    else if (0 == dos_strnicmp(pszAction, "sync", dos_strlen("sync")))
    {
        /* http://127.0.0.1/gateway?action=sync */
        ulAction = SC_API_CMD_ACTION_GATEWAY_SYNC;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (sc_gateway_update_proc(ulAction, ulGatewayID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_sip_action(list_t *pstArgv)
{
    S8  *pszSipID = NULL, *pszAction = NULL, *pszCustomerID = NULL;
    U32 ulSipID, ulAction, ulCustomerID;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取sip账户id */
    pszSipID = sc_http_api_get_value(pstArgv, "sip_id");
    /* 获取SIP账户所属客户ID */
    pszCustomerID = sc_http_api_get_value(pstArgv, "customer_id");
    /* 获取动作 */
    pszAction = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszSipID)
        || DOS_ADDR_INVALID(pszCustomerID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (dos_atoul(pszCustomerID, &ulCustomerID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszSipID, &ulSipID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
       ulAction = SC_API_CMD_ACTION_SIP_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
       ulAction = SC_API_CMD_ACTION_SIP_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
       ulAction = SC_API_CMD_ACTION_SIP_UPDATE;
    }
    else
    {
       DOS_ASSERT(0);
       goto invalid_params;
    }

    if (sc_sip_account_update_proc(ulAction, ulSipID, ulCustomerID) != DOS_SUCC)
    {
       DOS_ASSERT(0);
       return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return DOS_SUCC;

invalid_params:

   return SC_HTTP_ERRNO_INVALID_PARAM;
}


U32 sc_http_api_num_verify(list_t *pstArgv)
{
    S8 *pszCustomer = NULL;
    S8 *pszCallee   = NULL;
    S8 *pszPassword = NULL;
    S8 *pszPlaycnt = NULL;
    U32 ulCustomer = U32_BUTT;
    U32 ulPlayCnt  = 0;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    pszCustomer = sc_http_api_get_value(pstArgv, "customer");
    pszCallee = sc_http_api_get_value(pstArgv, "callee");
    pszPassword = sc_http_api_get_value(pstArgv, "verify_num");
    pszPlaycnt = sc_http_api_get_value(pstArgv, "play_cnt");
    if (DOS_ADDR_INVALID(pszCustomer)
        || DOS_ADDR_INVALID(pszCallee)
        || DOS_ADDR_INVALID(pszPassword))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (DOS_ADDR_INVALID(pszPlaycnt)
        || dos_atoul(pszPlaycnt, &ulPlayCnt) < 0
        || ulPlayCnt < SC_NUM_VERIFY_TIME_MIN
        || ulPlayCnt > SC_NUM_VERIFY_TIME_MAX)
    {
        ulPlayCnt = SC_NUM_VERIFY_TIME;
    }

    if (dos_atoul(pszCustomer, &ulCustomer) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (sc_voice_verify_proc(ulCustomer, pszCallee, pszPassword, ulPlayCnt) != DOS_SUCC)
    {
        return SC_HTTP_ERRNO_SERVER_ERROR;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:
    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_agent_call_ctrl(list_t *pstArgv)
{
    S8 *pszAction       = NULL;
    S8 *pszCustomerID   = NULL;
    S8 *pszAgent        = NULL;
    S8 *pszNumber       = NULL;
    S8 *pszAgentCalled  = NULL;
    S8 *pszType         = NULL;
    U32 ulType          = 0;
    U32 ulAgent         = U32_BUTT;
    U32 ulCustomer      = U32_BUTT;
    U32 ulAction        = SC_ACTION_AGENT_BUTT;
    U32 ulAgentCalled   = 0;
    U32 ulTaskID        = U32_BUTT;
    U32 ulRet           = DOS_FAIL;
    SC_AGENT_NODE_ST  *pstAgentNode = NULL;

    pszAction      = sc_http_api_get_value(pstArgv, "action");
    pszCustomerID  = sc_http_api_get_value(pstArgv, "customer");
    pszAgent       = sc_http_api_get_value(pstArgv, "agent");
    pszAgentCalled = sc_http_api_get_value(pstArgv, "agent_id");
    pszType        = sc_http_api_get_value(pstArgv, "type");
    pszNumber      = sc_http_api_get_value(pstArgv, "number");

    if (DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (DOS_ADDR_INVALID(pszAgent)
        || '\0' == pszAgent[0]
        || dos_atoul(pszAgent, &ulAgent) < 0)
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (DOS_ADDR_INVALID(pszCustomerID)
        || '\0' == pszCustomerID[0]
        || dos_atoul(pszCustomerID, &ulCustomer) < 0)
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (dos_strncmp(pszAction, "call",sizeof("call")) == 0)
    {
        ulAction = SC_ACTION_AGENT_CALL;
    }
    else if (dos_strncmp(pszAction, "hangup",sizeof("hangup")) == 0)
    {
        ulAction = SC_ACTION_AGENT_HANGUP;
    }
    else if (dos_strncmp(pszAction, "hold",sizeof("hold")) == 0)
    {
        ulAction = SC_ACTION_AGENT_HOLD;
    }
    else if (dos_strncmp(pszAction, "unhold",sizeof("unhold")) == 0)
    {
        ulAction = SC_ACTION_AGENT_UNHOLD;
    }
    else if (dos_strncmp(pszAction, "transfer",sizeof("transfer")) == 0)
    {
        ulAction = SC_ACTION_AGENT_TRANSFER;
    }
    else if (dos_strncmp(pszAction, "atransfer",sizeof("atransfer")) == 0)
    {
        ulAction = SC_ACTION_AGENT_ATRANSFER;
    }

    if (ulAction >= SC_ACTION_AGENT_BUTT)
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    switch (ulAction)
    {
        case SC_ACTION_AGENT_CALL:
            if (DOS_ADDR_INVALID(pszType))
            {
                DOS_ASSERT(0);

                return SC_HTTP_ERRNO_INVALID_REQUEST;
            }

            ulType = pszType[0] - '0';
            if (ulType >= SC_ACTION_CALL_TYPE_BUTT)
            {
                DOS_ASSERT(0);

                return SC_HTTP_ERRNO_INVALID_REQUEST;
            }

            switch (ulType)
            {
                case SC_ACTION_CALL_TYPE_CLICK:
                case SC_ACTION_CALL_TYPE_CALL_NUM:
                    if (DOS_ADDR_INVALID(pszNumber))
                    {
                        DOS_ASSERT(0);

                        return SC_HTTP_ERRNO_INVALID_REQUEST;
                    }

                    /* 判断被叫号码是否是坐席的工号，如果是，则呼叫这个坐席 */
                    pstAgentNode = sc_agent_get_by_emp_num(ulCustomer, pszNumber);
                    if (DOS_ADDR_VALID(pstAgentNode) && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
                    {
                        ulRet = sc_call_ctrl_call_agent(ulAgent, pstAgentNode);
                        break;
                    }

                    /* 判断被叫号码是否是分机号，如果是同一个客户的分机号，就呼叫这个分机 */
                    else if (sc_sip_account_get_customer(pszNumber) == ulCustomer)
                    {
                        ulRet = sc_call_ctrl_call_sip(ulAgent, pszNumber);
                        break;
                    }

                    /* 从系统呼出 */
                    else
                    {
                        ulRet = sc_call_ctrl_call_out(ulAgent, ulTaskID, pszNumber);
                        break;
                    }

                case SC_ACTION_CALL_TYPE_CALL_AGENT:
                    if (DOS_ADDR_INVALID(pszAgentCalled) || dos_atoul(pszAgentCalled, &ulAgentCalled) < 0)
                    {
                        DOS_ASSERT(0);

                        return SC_HTTP_ERRNO_INVALID_REQUEST;
                    }
                    pstAgentNode = sc_agent_get_by_id(ulAgentCalled);
                    if (DOS_ADDR_INVALID(pstAgentNode) || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
                    {
                        ulRet = sc_call_ctrl_call_agent(ulAgent, pstAgentNode);
                    }
                    break;

                default:
                    ulRet = DOS_FAIL;
            }
            break;

        case SC_ACTION_AGENT_TRANSFER:
        case SC_ACTION_AGENT_ATRANSFER:
            if (DOS_ADDR_INVALID(pszAgentCalled) || dos_atoul(pszAgentCalled, &ulAgentCalled) < 0)
            {
                DOS_ASSERT(0);

                return SC_HTTP_ERRNO_INVALID_REQUEST;
            }

            ulRet = sc_call_ctrl_transfer(ulAgent, ulAgentCalled, (SC_ACTION_AGENT_ATRANSFER == ulAction));

            break;

        case SC_ACTION_AGENT_HOLD:
            ulRet = sc_call_ctrl_hold(ulAgent, DOS_TRUE);
            break;

        case SC_ACTION_AGENT_UNHOLD:
            ulRet = sc_call_ctrl_unhold(ulAgent);
            break;

        case SC_ACTION_AGENT_HANGUP:
            ulRet = sc_call_ctrl_hangup(ulAgent);
            break;

        default:
            ulRet = DOS_FAIL;
    }

    if (ulRet != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }
    else
    {
        return SC_HTTP_ERRNO_SUCC;
    }
}

U32 sc_http_api_call_ctrl(list_t *pstArgv)
{
    S8 *pszAction       = NULL;
    S8 *pszCustomerID   = NULL;
    S8 *pszAgentID      = NULL;
    S8 *pszCallee       = NULL;
    S8 *pszFlag         = NULL;
    S8 *pszTaskID       = NULL;
    S8 *pszAgentCallee  = NULL;
    S8 *pszType         = NULL;
    U32 ulAction        = SC_API_CALLCTRL_BUTT;
    U32 ulAgent         = 0;
    U32 ulCustomer      = 0;
    U32 ulFlag          = 0;
    U32 ulTaskID        = 0;
    U32 ulCalleeAgentID = 0;
    U32 ulType          = 0;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);

        goto invalid_request;
    }

    pszAction = sc_http_api_get_value(pstArgv, "action");
    pszAgentID = sc_http_api_get_value(pstArgv, "agent");
    pszCustomerID = sc_http_api_get_value(pstArgv, "customerid");
    if (DOS_ADDR_INVALID(pszAction)
        || DOS_ADDR_INVALID(pszAgentID)
        || DOS_ADDR_INVALID(pszCustomerID))
    {
        DOS_ASSERT(0);
        goto invalid_request;
    }

    if (dos_atoul(pszAgentID, &ulAgent) < 0
        || dos_atoul(pszCustomerID, &ulCustomer) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_request;
    }

    if (dos_strnicmp(pszAction, "make", dos_strlen("make")) == 0)
    {
        /*
            flag :  0 -- 点击呼叫客户
                    1 -- 输入号码呼叫
                    2 -- 呼叫坐席
        */
        pszFlag = sc_http_api_get_value(pstArgv, "flag");
        if (DOS_ADDR_INVALID(pszFlag) || dos_atoul(pszFlag, &ulFlag) < 0)
        {
            DOS_ASSERT(0);

            goto invalid_request;
        }

        /* 呼叫坐席时，Callee为空 */
        pszCallee = sc_http_api_get_value(pstArgv, "callee");
        if (DOS_ADDR_INVALID(pszCallee) || ('\0' == pszCallee[0] && ulFlag != 2))
        {
            DOS_ASSERT(0);

            goto invalid_request;
        }

        pszTaskID = sc_http_api_get_value(pstArgv, "task");
        if (DOS_ADDR_INVALID(pszTaskID)
            || dos_atoul(pszTaskID, &ulTaskID) < 0)
        {
            ulTaskID = 0;
        }

        pszAgentCallee = sc_http_api_get_value(pstArgv, "agent_called");
        if (DOS_ADDR_INVALID(pszAgentCallee) || dos_atoul(pszAgentCallee, &ulCalleeAgentID) < 0)
        {
            if (ulFlag == 2)
            {
                DOS_ASSERT(0);

                goto invalid_request;
            }
            else
            {
                ulCalleeAgentID = U32_BUTT;
            }
        }

        ulAction = SC_API_MAKE_CALL;
    }
    else if (dos_strnicmp(pszAction, "hangup", dos_strlen("hangup")) == 0)
    {
        ulAction = SC_API_HANGUP_CALL;
    }
    else if (dos_strnicmp(pszAction, "record", dos_strlen("record")) == 0)
    {
        ulAction = SC_API_RECORD;
    }
    else if (dos_strnicmp(pszAction, "whispers", dos_strlen("whispers")) == 0)
    {
        ulAction = SC_API_WHISPERS;

        pszType = sc_http_api_get_value(pstArgv, "type");
        if (DOS_ADDR_INVALID(pszType)
            || dos_atoul(pszType, &ulType) < 0)
        {
            DOS_ASSERT(0);

            goto invalid_request;
        }

        pszCallee = sc_http_api_get_value(pstArgv, "number");
        if (DOS_ADDR_INVALID(pszCallee) || '\0' == pszCallee[0])
        {
            DOS_ASSERT(0);

            goto invalid_request;
        }
    }
    else if (dos_strnicmp(pszAction, "intercept", dos_strlen("intercept")) == 0)
    {
        ulAction = SC_API_INTERCEPT;

        pszType = sc_http_api_get_value(pstArgv, "type");
        if (DOS_ADDR_INVALID(pszType)
            || dos_atoul(pszType, &ulType) < 0)
        {
            DOS_ASSERT(0);

            goto invalid_request;
        }

        pszCallee = sc_http_api_get_value(pstArgv, "number");
        if (DOS_ADDR_INVALID(pszCallee) || '\0' == pszCallee[0])
        {
            DOS_ASSERT(0);

            goto invalid_request;
        }

    }
    else if (dos_strnicmp(pszAction, "transfer", dos_strlen("transfer")) == 0)
    {
        pszCallee = sc_http_api_get_value(pstArgv, "callee");
        if (DOS_ADDR_INVALID(pszCallee) || '\0' == pszCallee[0])
        {
            DOS_ASSERT(0);

            goto invalid_request;
        }

        pszFlag = sc_http_api_get_value(pstArgv, "flag");
        if (DOS_ADDR_VALID(pszFlag)
            && '1' == pszFlag[0])
        {
            ulAction = SC_API_TRANSFOR_ATTAND;
        }
        else
        {
            ulAction = SC_API_TRANSFOR_BLIND;
        }
    }
    else if (dos_strnicmp(pszAction, "conference", dos_strlen("conference")) == 0)
    {
        pszCallee = sc_http_api_get_value(pstArgv, "callee");
        if (DOS_ADDR_INVALID(pszCallee) || '\0' == pszCallee[0])
        {
            DOS_ASSERT(0);

            goto invalid_request;
        }

        ulAction = SC_API_CONFERENCE;
    }
    else if (dos_strnicmp(pszAction, "hold", dos_strlen("hold")) == 0)
    {
        ulAction = SC_API_HOLD;
    }
    else if (dos_strnicmp(pszAction, "unhold", dos_strlen("unhold")) == 0)
    {
        ulAction = SC_API_UNHOLD;
    }
    else
    {
        DOS_ASSERT(0);

        goto invalid_request;
    }

    if (ulAction >= SC_API_CALLCTRL_BUTT)
    {
        DOS_ASSERT(0);

        goto invalid_request;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_HTTP_API), "Recv HTTP API CMD. CMD: %u, Action: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomer, ulTaskID
                    , pszCallee ? pszCallee : "");

    return sc_call_ctrl_proc(ulAction, ulTaskID, ulAgent, ulCustomer, ulType, pszCallee, ulFlag, ulCalleeAgentID);

invalid_request:
    return SC_HTTP_ERRNO_INVALID_REQUEST;
}

U32 sc_http_api_agent_grp(list_t *pstArgv)
{
    S8 *pszAgentGrpID = NULL;
    S8 *pszAction     = NULL;
    U32 ulAgentGrpID = U32_BUTT, ulAction = U32_BUTT;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    pszAgentGrpID = sc_http_api_get_value(pstArgv, "agentgrp_id");
    pszAction  = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszAgentGrpID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszAgentGrpID, &ulAgentGrpID) < 0)
    {
       DOS_ASSERT(0);
       goto invalid_params;
    }


    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_AGENTGREP_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_AGENTGREP_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_AGENTGREP_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (sc_agent_group_http_update_proc(ulAction, ulAgentGrpID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;

}

U32 sc_http_api_agent_action(list_t *pstArgv)
{
    S8 *pszAgent   = NULL;
    S8 *pszAction  = NULL;
    S8 *pszCustomer= NULL;
    U32 ulAgent        = U32_BUTT;
    U32 ulCustomer = U32_BUTT;
    U32 ulAction   = SC_ACTION_AGENT_BUTT;

    pszAction = sc_http_api_get_value(pstArgv, "action");
    pszAgent  = sc_http_api_get_value(pstArgv, "agent");
    pszCustomer = sc_http_api_get_value(pstArgv, "customer");

    if (DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (DOS_ADDR_INVALID(pszAgent)
        || '\0' == pszAgent[0]
        || dos_atoul(pszAgent, &ulAgent) < 0)
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (DOS_ADDR_INVALID(pszCustomer)
        || '\0' == pszCustomer[0]
        || dos_atoul(pszCustomer, &ulCustomer) < 0)
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (dos_strncmp(pszAction, "busy", sizeof("busy")) == 0)
    {
        ulAction = SC_ACTION_AGENT_BUSY;
    }
    else if (dos_strncmp(pszAction, "idle", sizeof("idle")) == 0)
    {
        ulAction = SC_ACTION_AGENT_IDLE;
    }
    else if (dos_strncmp(pszAction, "rest", sizeof("rest")) == 0)
    {
        ulAction = SC_ACTION_AGENT_REST;
    }
    else if (dos_strncmp(pszAction, "signin",sizeof("signin")) == 0)
    {
        ulAction = SC_ACTION_AGENT_SIGNIN;
    }
    else if (dos_strncmp(pszAction, "signout", sizeof("signout")) == 0)
    {
        ulAction = SC_ACTION_AGENT_SIGNOUT;
    }
    else if (dos_strncmp(pszAction, "login",sizeof("login")) == 0)
    {
        ulAction = SC_ACTION_AGENT_LOGIN;
    }
    else if (dos_strncmp(pszAction, "logout",sizeof("logout")) == 0)
    {
        ulAction = SC_ACTION_AGENT_LOGOUT;
    }
    else if (dos_strncmp(pszAction, "foffline",sizeof("foffline")) == 0)
    {
        ulAction = SC_ACTION_AGENT_FORCE_OFFLINE;
    }
    else if (dos_strncmp(pszAction, "query",sizeof("query")) == 0)
    {
        ulAction = SC_ACTION_AGENT_QUERY;
    }

    if (SC_ACTION_AGENT_BUTT == ulAction)
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (sc_agent_status_update(ulAction, ulAgent, OPERATING_TYPE_WEB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;
}

U32 sc_http_api_agent(list_t *pstArgv)
{
    S8 *pszAgentID    = NULL;
    S8 *pszUserID     = NULL;
    S8 *pszAction     = NULL;
    U32 ulAgentID = U32_BUTT, ulAction = U32_BUTT;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    pszAgentID = sc_http_api_get_value(pstArgv, "agent_id");
    pszUserID = sc_http_api_get_value(pstArgv, "sip_userid");
    pszAction = sc_http_api_get_value(pstArgv, "action");
    if (DOS_ADDR_INVALID(pszAgentID)
        || DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (dos_atoul(pszAgentID, &ulAgentID) < 0)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (dos_strncmp(pszAction, "signin", sizeof("signin")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_SIGNIN;
    }
    else if (dos_strncmp(pszAction, "signout", sizeof("signout")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_SIGNOUT;
    }
    else if (dos_strncmp(pszAction, "login", sizeof("login")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_ONLINE;
    }
    else if (dos_strncmp(pszAction, "logout",sizeof("logout")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_OFFLINE;
    }
    else if (dos_strncmp(pszAction, "update",sizeof("update")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_UPDATE;
    }
    else if (dos_strncmp(pszAction, "add",sizeof("add")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_ADD;
    }
    else if (dos_strncmp(pszAction, "delete",sizeof("delete")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_DELETE;
    }
    else if (dos_strncmp(pszAction, "idle",sizeof("idle")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_EN_QUEUE;
    }
    else if (dos_strncmp(pszAction, "busy",sizeof("busy")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_DN_QUEUE;
    }
    else if (dos_strncmp(pszAction, "query",sizeof("query")) == 0) {
        ulAction = SC_API_CMD_ACTION_QUERY;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_HTTP_API), "Recv HTTP API CMD. Action: %u, AgentID: %u, sip_userid: %s"
                    , ulAction, ulAgentID, pszUserID);

    if (sc_agent_http_update_proc(ulAction, ulAgentID, pszUserID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;
}

U32 sc_http_api_route_action(list_t *pstArgv)
{
    S8   *pszRouteID = NULL, *pszAction = NULL;
    U32  ulAction = U32_BUTT, ulRouteID = U32_BUTT;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    pszRouteID = sc_http_api_get_value(pstArgv, "route_id");
    pszAction  = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszRouteID)
        || DOS_ADDR_INVALID(pszAction))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_HTTP_API), "%s", "Route data is synchronizing...");
    }
    else
    {
        if (dos_atoul(pszRouteID, &ulRouteID) < 0)
        {
           DOS_ASSERT(0);
           goto invalid_params;
        }
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_ROUTE_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_ROUTE_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_ROUTE_UPDATE;
    }
    else if (0 == dos_strnicmp(pszAction, "sync", dos_strlen("sync")))
    {
        ulAction = SC_API_CMD_ACTION_ROUTE_SYNC;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (sc_route_update_proc(ulAction, ulRouteID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:
    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_gw_group_action(list_t *pstArgv)
{
    S8   *pszGwGroupID = NULL, *pszAction = NULL;
    U32   ulGwGroupID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    pszGwGroupID = sc_http_api_get_value(pstArgv, "gw_group_id");
    pszAction    = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszGwGroupID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszGwGroupID, &ulGwGroupID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_GW_GROUP_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_GW_GROUP_UPDATE;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_GW_GROUP_DELETE;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (sc_gateway_group_update_proc(ulAction, ulGwGroupID) != DOS_SUCC)
    {
       DOS_ASSERT(0);
       return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;

}

U32 sc_http_api_did_action(list_t *pstArgv)
{
    S8  *pszDidID = NULL, *pszAction = NULL;
    U32  ulDidID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    pszDidID  = sc_http_api_get_value(pstArgv, "did_id");
    pszAction = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszDidID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszDidID, &ulDidID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_DID_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_DID_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_DID_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (sc_did_update_proc(ulAction, ulDidID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:
    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_black_action(list_t *pstArgv)
{
    S8  *pszBlackID = NULL, *pszAction = NULL;
    U32  ulAction, ulBlackID;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    pszBlackID = sc_http_api_get_value(pstArgv, "black_id");
    pszAction  = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszBlackID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszBlackID, &ulBlackID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_BLACK_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_BLACK_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_BLACK_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_HTTP_API), "Recv HTTP API CMD. CMD: %u, Action: %u, ulBlackID: %u"
                    , ulAction, ulBlackID);

    if (sc_black_list_update_proc(ulAction, ulBlackID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_caller_action(list_t *pstArgv)
{
    S8  *pszCallerID = NULL, *pszAction = NULL;
    U32  ulCallerID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取主叫号码 */
    pszCallerID = sc_http_api_get_value(pstArgv, "caller_id");
    pszAction   = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszCallerID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszCallerID, &ulCallerID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    if (DOS_SUCC != sc_caller_update_proc(ulAction, ulCallerID))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;

}

U32 sc_http_api_eix_action(list_t *pstArgv)
{
    S8  *pszEixID = NULL, *pszAction = NULL;
    U32  ulEixID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取主叫号码 */
    pszEixID = sc_http_api_get_value(pstArgv, "eix_id");
    pszAction = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszEixID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszEixID, &ulEixID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    if (DOS_SUCC != sc_eix_dev_update_proc(ulAction, ulEixID))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }


    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;

}

U32 sc_http_api_serv_ctrl_action(list_t *pstArgv)
{
    S8  *pszID = NULL, *pszAction = NULL;
    U32  ulID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    pszID       = sc_http_api_get_value(pstArgv, "id");
    pszAction   = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszID, &ulID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    if (DOS_SUCC != sc_serv_ctrl_update_proc(ulAction, ulID))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }


    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_numlmt_action(list_t *pstArgv)
{
    S8  *pszNumlmtID = NULL, *pszAction = NULL;
    U32  ulNumlmtID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取主叫号码 */
    pszNumlmtID = sc_http_api_get_value(pstArgv, "lmt_id");
    pszAction   = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszNumlmtID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszNumlmtID, &ulNumlmtID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    if (DOS_SUCC != sc_number_lmt_update_proc(ulAction, ulNumlmtID))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_numtransform_action(list_t *pstArgv)
{
    S8  *pszNumTransID = NULL, *pszAction = NULL;
    U32  ulNumTransID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取主叫号码 */
    pszNumTransID = sc_http_api_get_value(pstArgv, "transform_id");
    pszAction   = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszNumTransID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszNumTransID, &ulNumTransID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    if (DOS_SUCC != sc_transform_update_proc(ulAction, ulNumTransID))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }


    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_customer_action(list_t *pstArgv)
{
    S8  *pszCustomerID = NULL, *pszAction = NULL;
    U32  ulCustomerID, ulAction;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取主叫号码 */
    pszCustomerID = sc_http_api_get_value(pstArgv, "customer_id");
    pszAction   = sc_http_api_get_value(pstArgv, "action");

    if (DOS_ADDR_INVALID(pszCustomerID)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszCustomerID, &ulCustomerID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "add", dos_strlen("add")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_ADD;
    }
    else if (0 == dos_strnicmp(pszAction, "delete", dos_strlen("delete")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_DELETE;
    }
    else if (0 == dos_strnicmp(pszAction, "update", dos_strlen("update")))
    {
        ulAction = SC_API_CMD_ACTION_CALLER_UPDATE;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    if (DOS_SUCC != sc_customer_update_proc(ulAction, ulCustomerID))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}

U32 sc_http_api_demo_action(list_t *pstArgv)
{
    S8  *pszCustomerID = NULL, *pszAction = NULL, *pszCallee = NULL, *pszAgentNum = NULL, *pszAgentID = NULL;
    U32  ulCustomerID, ulAction, ulAgentID;

    if (DOS_ADDR_INVALID(pstArgv))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    /* 获取主叫号码 */
    pszCustomerID = sc_http_api_get_value(pstArgv, "customer_id");
    pszAction   = sc_http_api_get_value(pstArgv, "action");
    pszCallee   = sc_http_api_get_value(pstArgv, "callee");
    pszAgentID   = sc_http_api_get_value(pstArgv, "agent_id");

    /* 坐席的手机号 */
    pszAgentNum   = sc_http_api_get_value(pstArgv, "agent_num");

    if (DOS_ADDR_INVALID(pszCustomerID)
        || DOS_ADDR_INVALID(pszAction)
        || DOS_ADDR_INVALID(pszCallee)
        || DOS_ADDR_INVALID(pszAgentNum)
        || DOS_ADDR_INVALID(pszAgentID))
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszCustomerID, &ulCustomerID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (dos_atoul(pszAgentID, &ulAgentID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    if (0 == dos_strnicmp(pszAction, "demo_groupcall", dos_strlen("demo_groupcall")))
    {
        /* 群呼任务演示 */
        ulAction = SC_API_CMD_ACTION_DEMO_TASK;
    }
    else if (0 == dos_strnicmp(pszAction, "demo_previewcall", dos_strlen("demo_previewcall")))
    {
        /* 预览外呼演示 */
        ulAction = SC_API_CMD_ACTION_DEMO_PREVIEW;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_DEMO_TASK:
            sc_demo_task(ulCustomerID, pszCallee, pszAgentNum, ulAgentID);
            break;
        case SC_API_CMD_ACTION_DEMO_PREVIEW:
            sc_demo_preview(ulCustomerID, pszCallee, pszAgentNum, ulAgentID);
            break;
        default:
            break;
    }


    return SC_HTTP_ERRNO_SUCC;

invalid_params:

    return SC_HTTP_ERRNO_INVALID_PARAM;
}


U32 sc_http_api_process(SC_HTTP_CLIENT_CB_S *pstClient)
{
    S8        *pStart = NULL, *pEnd = NULL;
    S8        *pszKeyWord[SC_API_PARAMS_MAX_NUM] = { 0 };
    S8        *pWord = NULL, *pValue = NULL;
    S8        szReqBuffer[SC_HTTP_REQ_MAX_LEN] = { 0 };
    S8        szReqLine[1024] = { 0 };
    S32       lKeyCnt = 0, lParamIndex = 0;
    U32       ulRet;
    list_t    *pstParamListNode;
    SC_API_PARAMS_ST *pstParamsList;
    http_req_handle cb;

    if (!pstClient)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    if (!pstClient->ulValid)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    if (!pstClient->stDataBuff.ulLength || !pstClient->stDataBuff.pszBuff)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_HTTP_API), "HTTP Request: %s", pstClient->stDataBuff.pszBuff);

    /* 获取请求的文件 */
    pStart = dos_strstr(pstClient->stDataBuff.pszBuff, "GET /");
    if (!pStart)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    pStart += dos_strlen("GET /");
    pEnd = dos_strstr(pStart, "?");
    if (!pStart || !pEnd
        || pEnd <= pStart)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    dos_strncpy(szReqBuffer, pStart, pEnd-pStart);
    szReqBuffer[pEnd - pStart] = '\0';
    if ('\0' == szReqBuffer[0])
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    /* 获取请求行参数 */
    pStart = dos_strstr(pstClient->stDataBuff.pszBuff, "?");
    pEnd = dos_strstr(pstClient->stDataBuff.pszBuff, " HTTP/1.");
    if (!pStart || !pEnd)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    /* 获取请求参数 */
    pStart += dos_strlen("?");
    if (pEnd - pStart >= sizeof(szReqLine))
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    dos_strncpy(szReqLine, pStart, pEnd - pStart);
    szReqLine[pEnd - pStart] = '\0';

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_HTTP_API), "HTTP Request Line: %s?%s", szReqBuffer, szReqLine);

    /* 获取 key=value 字符串 */
    lKeyCnt = 0;
    pWord = strtok(szReqLine, "&");
    while (pWord)
    {
        pszKeyWord[lKeyCnt] = dos_dmem_alloc(dos_strlen(pWord) + 1);
        if (!pszKeyWord[lKeyCnt])
        {
            logr_warning("%s", "Alloc fail.");
            break;
        }

        dos_strcpy(pszKeyWord[lKeyCnt], pWord);
        lKeyCnt++;
        pWord = strtok(NULL, "&");
        if (NULL == pWord)
        {
            break;
        }
    }

    if (lKeyCnt<= 0)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_HTTP_API), "%s", "Start parse the http request.");

    /* 解析key=value，并将结果存入链表 */
    dos_list_init(&pstClient->stParamList);
    for (lParamIndex=0; lParamIndex<lKeyCnt; lParamIndex++)
    {
        if(!pszKeyWord[lParamIndex])
        {
            continue;
        }

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_HTTP_API), "Process Token: %s", pszKeyWord[lParamIndex]);

        pWord = dos_strstr(pszKeyWord[lParamIndex], "=");
        pValue = pWord;
        if (!pValue)
        {
            continue;
        }
        pValue++;
        if (!pValue)
        {
            continue;
        }
        *pWord = '\0';

        pstParamsList = (SC_API_PARAMS_ST *)dos_dmem_alloc(sizeof(SC_API_PARAMS_ST));
        if (!pstParamsList)
        {
            DOS_ASSERT(0);
            continue;
        }

        /*
         * pValue 指向的是 pszKeyWord[lParamIndex]所在地址段的某个地址，
         * 而pszKeyWord[lParamIndex]是动态申请的内存，所以这里就不用重新申请内存了
         */
        pstParamsList->pszStringName = pszKeyWord[lParamIndex];
        pstParamsList->pszStringVal = pValue;

        dos_list_add_tail(&(pstClient->stParamList), &pstParamsList->stList);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_HTTP_API), "%s", "Parse the http request finished.");

    ulRet = SC_HTTP_ERRNO_INVALID_REQUEST;
    cb = sc_http_api_find(szReqBuffer);
    if (cb)
    {
        ulRet = cb(&(pstClient->stParamList));
    }


    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "HTTP Request process finished. Return code: %d", ulRet);

    while (1)
    {
        if (dos_list_is_empty(&pstClient->stParamList))
        {
            break;
        }

        pstParamListNode = dos_list_fetch(&pstClient->stParamList);
        if (!pstParamListNode)
        {
            break;
        }

        pstParamsList = dos_list_entry(pstParamListNode, SC_API_PARAMS_ST, stList);
        if (!pstParamsList)
        {
            continue;
        }

        pstParamsList->pszStringName = NULL;
        pstParamsList->pszStringVal = NULL;
        dos_dmem_free(pstParamsList);
        pstParamsList = NULL;
    }

    for (lParamIndex=0; lParamIndex<lKeyCnt; lParamIndex++)
    {
        dos_dmem_free(pszKeyWord[lParamIndex]);
        pszKeyWord[lParamIndex] = NULL;
    }

    pstClient->ulErrCode = ulRet;
    pstClient->ulResponseCode = SC_HTTP_OK;
    return DOS_SUCC;

cmd_prase_fail1:
    pstClient->ulErrCode = SC_HTTP_ERRNO_INVALID_REQUEST;
    pstClient->ulResponseCode = SC_HTTP_OK;
    return DOS_FAIL;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

