/**
 * @file : sc_event_fsm.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ҵ�����ģ�����ҵ��״̬��ʵ��
 *
 * @date: 2016��1��13��
 * @arthur: Larry
 */
#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_pub.h"
#include "bs_pub.h"
#include "sc_res.h"
#include "sc_hint.h"
#include "sc_debug.h"
#include "sc_pub.h"
#include "sc_bs.h"

extern SC_ACCESS_CODE_LIST_ST astSCAccessList[];

U32 sc_access_balance_enquiry(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    return sc_send_balance_query2bs(pstSCB, pstLegCB);
}

U32 sc_access_hungup(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    U32                 ulOtherLegNo    = U32_BUTT;
    SC_LEG_CB           *pstOtherLeg    = NULL;

    if (pstSCB->stCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stCall.ulCallingLegNo)
        {
            ulOtherLegNo = pstSCB->stCall.ulCalleeLegNo;
        }
        else
        {
            ulOtherLegNo = pstSCB->stCall.ulCallingLegNo;
        }
    }
    else if (pstSCB->stPreviewCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stPreviewCall.ulCallingLegNo)
        {
            ulOtherLegNo = pstSCB->stPreviewCall.ulCalleeLegNo;
        }
    }
    else if (pstSCB->stAutoCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stAutoCall.ulCallingLegNo)
        {
            ulOtherLegNo = pstSCB->stAutoCall.ulCalleeLegNo;
        }
    }
    else if (pstSCB->stTransfer.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stTransfer.ulPublishLegNo)
        {
            ulOtherLegNo = pstSCB->stTransfer.ulSubLegNo;
        }
    }

    pstOtherLeg = sc_lcb_get(ulOtherLegNo);
    if (DOS_ADDR_INVALID(pstOtherLeg))
    {
        return DOS_SUCC;
    }

    sc_req_hungup(pstSCB->ulSCBNo, ulOtherLegNo, CC_ERR_NORMAL_CLEAR);

    return DOS_SUCC;
}

/** ת��
*/
U32 sc_access_transfer(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    S8                  pszEmpNum[SC_NUM_LENGTH]    = {0};
    SC_AGENT_NODE_ST    *pstAgentNode               = NULL;
    U32                 ulRet                       = DOS_FAIL;
    SC_LEG_CB           *pstPublishLeg              = NULL;
    SC_CALL_TRANSFER_ST stTransfer;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ���Ҫת�ӵ���ϯ�Ĺ��� */
    if (dos_sscanf(pstSCB->stAccessCode.szDialCache, "*%*[^*]*%[^#]s", pszEmpNum) != 1)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "POTS, format error : %s", pstSCB->stAccessCode.szDialCache);

        return DOS_FAIL;
    }

    /* ���ݹ����ҵ���ϯ */
    pstAgentNode = sc_agent_get_by_emp_num(pstSCB->ulCustomerID, pszEmpNum);
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "POTS, Can not find agent. customer id(%u), empNum(%s)", pstSCB->ulCustomerID, pszEmpNum);

        return DOS_FAIL;
    }

    /* �ж���ϯ��״̬ */
    if (!SC_ACD_SITE_IS_USEABLE(pstAgentNode->pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "The agent is not useable.(Agent %u)", pstAgentNode->pstAgentInfo->ulAgentID);

        return DOS_FAIL;
    }

    if (pstSCB->stTransfer.stSCBTag.bValid)
    {
        /* ˵���Ѿ�����һ��ת��ҵ�񣬽�ת��ҵ�񿽱��� stTransfer �У���ʼ�� pstSCB->stTransfer */
        stTransfer = pstSCB->stTransfer;

        pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_IDEL;
        pstSCB->stTransfer.ulType = U32_BUTT;
        pstSCB->stTransfer.ulSubLegNo = U32_BUTT;
        pstSCB->stTransfer.ulPublishLegNo = U32_BUTT;
        pstSCB->stTransfer.ulNotifyLegNo = U32_BUTT;
        pstSCB->stTransfer.ulSubAgentID = 0;
        pstSCB->stTransfer.ulPublishAgentID = 0;
        pstSCB->stTransfer.ulNotifyAgentID = 0;
    }
    else
    {
        stTransfer.stSCBTag.bValid = DOS_FALSE;
    }

    pstSCB->stTransfer.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stTransfer.ulNotifyLegNo = pstLegCB->ulCBNo;
    switch (pstSCB->stAccessCode.ulSrvType)
    {
        case SC_ACCESS_AGENT_ONLINE:
            pstSCB->stTransfer.ulType = SC_ACCESS_BLIND_TRANSFER;
            break;
        default:
            pstSCB->stTransfer.ulType = SC_ACCESS_ATTENDED_TRANSFER;
            break;
    }

    pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_IDEL;
    pstSCB->stTransfer.ulPublishAgentID = pstAgentNode->pstAgentInfo->ulAgentID;

    /* ��֮ǰ��ҵ���п�����Ϣ��ת��ҵ���� */
    if (pstSCB->stCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stCall.ulCallingLegNo)
        {
            pstSCB->stTransfer.ulSubLegNo = pstSCB->stCall.ulCalleeLegNo;
            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
            {
                pstSCB->stTransfer.ulSubAgentID = pstSCB->stCall.pstAgentCallee->pstAgentInfo->ulAgentID;
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
            {
                pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stCall.pstAgentCalling->pstAgentInfo->ulAgentID;
            }
        }
        else
        {
            pstSCB->stTransfer.ulSubLegNo = pstSCB->stCall.ulCallingLegNo;
            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
            {
                pstSCB->stTransfer.ulSubAgentID = pstSCB->stCall.pstAgentCalling->pstAgentInfo->ulAgentID;
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
            {
                pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stCall.pstAgentCallee->pstAgentInfo->ulAgentID;
            }
        }
    }
    else if (pstSCB->stPreviewCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stPreviewCall.ulCallingLegNo)
        {
            pstSCB->stTransfer.ulSubLegNo = pstSCB->stPreviewCall.ulCalleeLegNo;
            pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stPreviewCall.ulAgentID;
        }
        else
        {
            pstSCB->stTransfer.ulSubLegNo = pstSCB->stPreviewCall.ulCallingLegNo;
            pstSCB->stTransfer.ulSubAgentID = pstSCB->stPreviewCall.ulAgentID;
        }
    }
    else if (pstSCB->stAutoCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stAutoCall.ulCallingLegNo)
        {
            pstSCB->stTransfer.ulSubLegNo = pstSCB->stAutoCall.ulCalleeLegNo;
            pstSCB->stTransfer.ulSubAgentID = pstSCB->stAutoCall.ulAgentID;
        }
        else
        {
            pstSCB->stTransfer.ulSubLegNo = pstSCB->stAutoCall.ulCallingLegNo;
            pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stAutoCall.ulAgentID;
        }
    }
    else if (stTransfer.stSCBTag.bValid)
    {
        /* ֮ǰ���ڵ���ת��ҵ�� */
        if (pstLegCB->ulCBNo == stTransfer.ulPublishLegNo)
        {
            pstSCB->stTransfer.ulSubLegNo = stTransfer.ulSubLegNo;
            pstSCB->stTransfer.ulSubAgentID = stTransfer.ulSubAgentID;
            pstSCB->stTransfer.ulNotifyAgentID = stTransfer.ulPublishAgentID;
        }
        else
        {
            pstSCB->stTransfer.ulSubLegNo = stTransfer.ulPublishLegNo;
            pstSCB->stTransfer.ulSubAgentID = stTransfer.ulPublishAgentID;
            pstSCB->stTransfer.ulNotifyAgentID = stTransfer.ulSubAgentID;
        }
    }

    if (pstSCB->stTransfer.ulSubLegNo == U32_BUTT)
    {
        /* û���ҵ� */
        DOS_ASSERT(0);
        goto proc_fail;
    }

    /* �ж���ϯ�Ƿ��ǳ�ǩ */
    if (pstAgentNode->pstAgentInfo->bConnected
        && pstAgentNode->pstAgentInfo->ulLegNo < SC_LEG_CB_SIZE)
    {
        pstPublishLeg = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
        if (DOS_ADDR_VALID(pstPublishLeg) && pstPublishLeg->ulIndSCBNo != U32_BUTT)
        {
            /* ��ǩ */
            pstSCB->stTransfer.ulPublishLegNo = pstAgentNode->pstAgentInfo->ulLegNo;
        }
    }

    if (pstSCB->stTransfer.ulPublishLegNo == U32_BUTT)
    {
        pstPublishLeg = sc_lcb_alloc();
        if (DOS_ADDR_INVALID(pstPublishLeg))
        {
            DOS_ASSERT(0);
            goto proc_fail;
        }

        /* �����к��� */
        pstPublishLeg->stCall.bValid = DOS_TRUE;
        pstPublishLeg->ulSCBNo = pstSCB->ulSCBNo;
        /* �����к��� */
        ulRet = sc_caller_setting_select_number(pstAgentNode->pstAgentInfo->ulCustomerID, pstAgentNode->pstAgentInfo->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstPublishLeg->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
        if (ulRet != DOS_SUCC)
        {
            /* TODO û���ҵ����к��룬������ */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "Agent signin customID(%u) get caller number FAIL by agent(%u)", pstAgentNode->pstAgentInfo->ulCustomerID, pstAgentNode->pstAgentInfo->ulAgentID);
            pstLegCB->ulSCBNo = pstSCB->ulSCBNo;
            sc_lcb_free(pstPublishLeg);

            goto proc_fail;
        }

        switch (pstAgentNode->pstAgentInfo->ucBindType)
        {
            case AGENT_BIND_SIP:
                dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szUserID);
                pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;

                break;

            case AGENT_BIND_TELE:
                dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTelePhone);
                pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);

                break;

            case AGENT_BIND_MOBILE:
                dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szMobile);
                pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                break;

            case AGENT_BIND_TT_NUMBER:
                dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTTNumber);
                pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
                break;

            default:
                break;
        }
        pstPublishLeg->stCall.stNumInfo.szOriginalCallee[sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee)-1] = '\0';

        pstSCB->stTransfer.ulPublishLegNo = pstPublishLeg->ulCBNo;
    }

    /* ҵ���л���ֱ���� ת��ҵ�� �滻����һ��ҵ�� */
    pstSCB->pstServiceList[0] = &pstSCB->stTransfer.stSCBTag;

    sc_scb_set_service(pstSCB, BS_SERV_CALL_TRANSFER);
    pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_AUTH;
    ulRet = sc_send_usr_auth2bs(pstSCB, pstLegCB);
    if (ulRet != DOS_SUCC)
    {
        /* ������ */
        goto proc_fail;
    }

    return DOS_SUCC;

proc_fail:

    return DOS_FAIL;
}

U32 sc_access_agent_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    SC_AGENT_NODE_ST    *pstAgent = NULL;
    U32                 ulResult  = DOS_FAIL;
    BOOL                bPlayRes  = DOS_TRUE;
    U32                 ulKey;

    if (pstSCB->stAccessCode.ulAgentID == 0)
    {
        /* �������к�������ϯ */
        if (SC_LEG_PEER_INBOUND_INTERNAL == pstLegCB->stCall.ucPeerType)
        {
            pstAgent = sc_agent_get_by_sip_acc(pstLegCB->stCall.stNumInfo.szOriginalCalling);
        }
        else
        {
            pstAgent = sc_agent_get_by_tt_num(pstLegCB->stCall.stNumInfo.szOriginalCalling);
        }
    }
    else
    {
        pstAgent = sc_agent_get_by_id(pstSCB->stAccessCode.ulAgentID);
    }

    if (DOS_ADDR_INVALID(pstAgent) || DOS_ADDR_INVALID(pstAgent->pstAgentInfo))
    {
        /* û���ҵ���ϯ������ʾ�� */
        ulResult = DOS_FAIL;
    }
    else
    {
        switch (pstSCB->stAccessCode.ulSrvType)
        {
            case SC_ACCESS_AGENT_ONLINE:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_LOGIN, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_AGENT_OFFLINE:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_LOGOUT, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_AGENT_EN_QUEUE:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_IDLE, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_AGENT_DN_QUEUE:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_BUSY, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_AGENT_SIGNIN:
                ulResult = sc_agent_access_set_sigin(pstAgent, pstSCB, pstLegCB);
                if (ulResult == DOS_SUCC)
                {
                    sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                    bPlayRes = DOS_FALSE;
                }
                break;
            case SC_ACCESS_AGENT_SIGNOUT:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_SIGNOUT, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_MARK_CUSTOMER:
                if (pstAgent->pstAgentInfo->szLastCustomerNum[0] == '\0')
                {
                    ulResult = DOS_FAIL;
                    break;
                }

                if (1 != dos_sscanf(pstLegCB->stCall.stNumInfo.szOriginalCallee+dos_strlen(astSCAccessList[SC_ACCESS_MARK_CUSTOMER].szCodeFormat), "%d", &ulKey) && ulKey <= 9)
                {
                    /* ��ʽ���� */
                    ulResult = DOS_FAIL;
                    break;
                }

                /* �޸������к��� */
                dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), "*%u#", pstLegCB->stCall.stNumInfo.szOriginalCallee);
                dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCalling), pstAgent->pstAgentInfo->szLastCustomerNum);

                dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstLegCB->stCall.stNumInfo.szRealCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
                dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstLegCB->stCall.stNumInfo.szRealCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

                dos_snprintf(pstLegCB->stCall.stNumInfo.szCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
                dos_snprintf(pstLegCB->stCall.stNumInfo.szCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

                /* ��ǿͻ� */
                ulResult = sc_agent_marker_update_req(pstSCB->ulCustomerID, pstAgent->pstAgentInfo->ulAgentID, ulKey, pstAgent->pstAgentInfo->szLastCustomerNum);
                break;
        }
    }

    if (DOS_SUCC != ulResult)
    {
        sc_req_play_sound(pstSCB->ulSCBNo, pstLegCB->ulCBNo, SC_SND_SET_FAIL, 1, 0, 0);
    }
    else if (bPlayRes)
    {
        sc_req_play_sound(pstSCB->ulSCBNo, pstLegCB->ulCBNo, SC_SND_SET_SUCC, 1, 0, 0);
    }

    return ulResult;
}

U32 sc_call_access_code(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB, S8 *szNum, BOOL bIsSecondDial)
{
    U32     i                        = 0;
    S8      szDealNum[SC_NUM_LENGTH] = {0,};

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstCallingLegCB)
        || DOS_ADDR_INVALID(szNum))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (!bIsSecondDial)
    {
        /* ���ݷֻ��ţ���ȡcustomer */
        if (SC_LEG_PEER_INBOUND_INTERNAL == pstCallingLegCB->stCall.ucPeerType)
        {
            pstSCB->ulCustomerID = sc_sip_account_get_customer(pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
        }
        else if (SC_LEG_PEER_INBOUND == pstCallingLegCB->stCall.ucPeerType)
        {
            /* external������TT�ź���Ŷ */
            pstSCB->ulCustomerID = sc_did_get_custom(pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
        }
    }

    if (pstSCB->ulCustomerID == U32_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ACTIVE;
    pstSCB->ulCurrentSrv++;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;
    pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
    pstSCB->stAccessCode.ulLegNo = pstCallingLegCB->ulCBNo;

    dos_strncpy(szDealNum, szNum, SC_NUM_LENGTH-1);
    if (szDealNum[dos_strlen(szDealNum)] == '#')
    {
        szDealNum[dos_strlen(szDealNum)] = '\0';
    }

    /* �ж�ҵ�� */
    for (i=0; i<SC_ACCESS_BUTT; i++)
    {
        if (!astSCAccessList[i].bValid)
        {
            continue;
        }

        if ((bIsSecondDial && !astSCAccessList[i].bSupportDirectDial)
            || (!bIsSecondDial && !astSCAccessList[i].bSupportDirectDial))
        {
            continue;
        }

        if (astSCAccessList[i].bExactMatch)
        {
            if (dos_strcmp(szDealNum, astSCAccessList[i].szCodeFormat) == 0)
            {
                break;
            }
        }
        else
        {
            if (dos_strncmp(szDealNum, astSCAccessList[i].szCodeFormat, dos_strlen(astSCAccessList[i].szCodeFormat)) == 0)
            {
                break;
            }
        }
    }

    if (i >= SC_ACCESS_BUTT)
    {
        /* û���Ҹ�ƥ��� */
        return DOS_FAIL;
    }

    pstSCB->stAccessCode.ulSrvType = astSCAccessList[i].ulType;
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_ACTIVE;
    if (DOS_ADDR_VALID(astSCAccessList[i].fn_init))
    {
        astSCAccessList[i].fn_init(pstSCB, pstCallingLegCB);
    }

    /* �˳� ������ҵ�� */
    pstSCB->stAccessCode.stSCBTag.bWaitingExit = DOS_TRUE;

    return DOS_SUCC;
}


/**
 * ��������ҵ���к��н�����Ϣ
 *
 * @param SC_MSG_TAG_ST *pstMsg
 * @param SC_SRV_CB *pstSCB
 *
 * @return �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB   *pstCallingLegCB = NULL;
    SC_MSG_EVT_CALL_ST  *pstCallSetup;
    U32         ulCallSrc        = U32_BUTT;
    U32         ulCallDst        = U32_BUTT;
    U32         ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_CALL_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the call setup msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
            break;

        case SC_CALL_PORC:
            /*  ����ط����Ǻ��� */
            pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallingLegCB))
            {
                sc_trace_scb(pstSCB, "Cannot find the LCB. (%s)", pstSCB->stCall.ulCallingLegNo);

                goto proc_fail;
            }

            /* ����ǽ�����ҵ����Ҫ���������ҵ�� */
            if ('*' == pstCallingLegCB->stCall.stNumInfo.szOriginalCallee[0])
            {
                pstCallingLegCB->stCall.stTimeInfo.ulAnswerTime = pstCallingLegCB->stCall.stTimeInfo.ulStartTime;
                if (SC_LEG_PEER_INBOUND_INTERNAL == pstCallingLegCB->stCall.ucPeerType
                    || SC_LEG_PEER_INBOUND_TT == pstCallingLegCB->stCall.ucPeerType)
                {
                    sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                }
                else
                {
                    sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                }

                ulRet = sc_call_access_code(pstSCB, pstCallingLegCB, pstCallingLegCB->stCall.stNumInfo.szOriginalCallee, DOS_FALSE);
                goto proc_finished;
            }

            ulCallSrc = sc_leg_get_source(pstSCB, pstCallingLegCB);
            /* һ��Ҫͨ�����аѿͻ���Ϣ�����������������ڽ�����  */
            if (U32_BUTT == pstSCB->ulCustomerID)
            {
                ulRet = sc_req_hungup_with_sound(U32_BUTT, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_NO_SERV_RIGHTS);
                goto proc_finished;
            }

            ulCallDst = sc_leg_get_destination(pstSCB, pstCallingLegCB);

            sc_trace_scb(pstSCB, "Get call source and dest. Customer: %u, Source: %d, Dest: %d", pstSCB->ulCustomerID, ulCallSrc, ulCallDst);

            /* ���浽scb�У�������õ� */
            pstSCB->stCall.ulCallSrc = ulCallSrc;
            pstSCB->stCall.ulCallDst = ulCallDst;

            /* ���ֺ��� */
            if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_PSTN == ulCallDst)
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH;

                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
                {
                    sc_agent_stat(SC_AGENT_STAT_CALL, pstSCB->stCall.pstAgentCalling->pstAgentInfo, 0, 0);
                }

                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);

                ulRet = sc_send_usr_auth2bs(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            /* ���� */
            else if (SC_DIRECTION_PSTN == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH;

                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling))
                {
                    sc_agent_stat(SC_AGENT_STAT_CALL, pstSCB->stCall.pstAgentCalling->pstAgentInfo, 0, 0);
                }

                sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);

                ulRet = sc_send_usr_auth2bs(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            /* �ڲ����� */
            else if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

                sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);

                ulRet = sc_internal_call_process(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            else
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;

                ulRet = sc_req_hungup_with_sound(U32_BUTT, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_NO_SERV_RIGHTS);
                goto proc_finished;
            }
            break;

        case SC_CALL_AUTH:
            break;
        case SC_CALL_EXEC:
            /* ����ط�Ӧ���Ǳ��е�LEG������, ����һ�±��е�LEG */
            pstSCB->stCall.ulCalleeLegNo = pstCallSetup->ulLegNo;
            break;

        case SC_CALL_ALERTING:
            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            ulRet = DOS_FAIL;
            sc_trace_scb(pstSCB, "Invalid status.%u", pstSCB->stCall.stSCBTag.usStatus);
            break;
    }

proc_finished:
    return DOS_SUCC;

proc_fail:
    sc_scb_free(pstSCB);
    pstSCB = NULL;
    return DOS_FAIL;
}

U32 sc_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    SC_LEG_CB                  *pstCalleeLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;
    pstLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */
        sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_BS_HEAD + pstAuthRsp->stMsgTag.usInterErr);
        return DOS_SUCC;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_AUTH:
            //pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;
            if (pstAuthRsp->ucBalanceWarning)
            {
                /* TODO */
                return sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, SC_SND_LOW_BALANCE, 1, 0, 0);
            }

            if (SC_DIRECTION_PSTN == pstSCB->stCall.ulCallSrc && SC_DIRECTION_SIP == pstSCB->stCall.ulCallDst)
            {
                /* ��ֺ��� */
                ulRet = sc_incoming_call_proc(pstSCB, pstLegCB);
            }
            else
            {
                ulRet = sc_outgoing_call_process(pstSCB, pstLegCB);
            }
            break;
         case SC_CALL_AUTH2:
            /* ���б��� */
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

            pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeLegCB))
            {
                return DOS_FAIL;
            }
            ulRet = sc_make_call2pstn(pstSCB, pstCalleeLegCB);
            break;
         default:
            break;
    }

    return ulRet;
}


U32 sc_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_ST *pstEvent = NULL;
    SC_LEG_CB             *pstCalleeLegCB = NULL;
    SC_LEG_CB             *pstCallingLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "process alerting msg. calling leg: %u, callee leg: %u, status : %u"
                        , pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.stSCBTag.usStatus);

    pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCalleeLegCB) || DOS_ADDR_INVALID(pstCallingLegCB))
    {
        sc_trace_scb(pstSCB, "alerting with only one leg.");
        return DOS_SUCC;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PORC:
        case SC_CALL_AUTH:
        case SC_CALL_AUTH2:
        case SC_CALL_EXEC:
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ALERTING;

            if (pstEvent->ulLegNo == pstSCB->stCall.ulCalleeLegNo)
            {
                if (pstEvent->ulWithMedia)
                {
                    pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
                    if (DOS_ADDR_VALID(pstCalleeLegCB))
                    {
                        pstCalleeLegCB->stCall.bEarlyMedia = DOS_TRUE;

                        if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
                        {
                            sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                            goto proc_fail;
                        }
                    }
                }
                else
                {
                    if (sc_req_ringback(pstSCB->ulSCBNo
                                            , pstSCB->stCall.ulCallingLegNo
                                            , pstCallingLegCB->stCall.bEarlyMedia) != DOS_SUCC)
                    {
                        sc_trace_scb(pstSCB, "Send ringback request fail.");
                    }
                }
            }

            break;

        case SC_CALL_ALERTING:
            /* ����LEG״̬�任 */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_EVENT), "Calling has been ringback.");
            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            break;
    }

    return DOS_SUCC;

proc_fail:
    /* ����ط����ùҶϺ��У�����ý������飬��Ӱ��� */
    return DOS_FAIL;
}

U32 sc_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB             *pstCalleeLegCB = NULL;
    SC_LEG_CB             *pstCallingLegCB = NULL;
    SC_LEG_CB             *pstRecordLegCB = NULL;
    SC_MSG_EVT_ANSWER_ST  *pstEvtAnswer   = NULL;
    SC_MSG_CMD_RECORD_ST  stRecordRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PORC:
        case SC_CALL_AUTH:
        case SC_CALL_AUTH2:
        case SC_CALL_EXEC:
        case SC_CALL_ALERTING:
            pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

            pstSCB->stCall.ulCalleeLegNo = pstEvtAnswer->ulLegNo;

            /* Ӧ������ */
            sc_req_answer_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo);

            /* ���������� */
            pstCalleeLegCB = sc_lcb_get(pstEvtAnswer->ulLegNo);
            if (DOS_ADDR_VALID(pstCalleeLegCB)
                && !pstCalleeLegCB->stCall.bEarlyMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto proc_fail;
                }

                /* �ж��Ƿ���Ҫ¼�� */
                if (pstCalleeLegCB->stRecord.bValid)
                {
                    pstRecordLegCB = pstCalleeLegCB;
                }
                else
                {
                    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
                    if (DOS_ADDR_VALID(pstCallingLegCB) && pstCallingLegCB->stRecord.bValid)
                    {
                        pstRecordLegCB = pstCallingLegCB;
                    }
                }

                if (DOS_ADDR_VALID(pstRecordLegCB))
                {
                    stRecordRsp.stMsgTag.ulMsgType = SC_CMD_RECORD;
                    stRecordRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stRecordRsp.stMsgTag.usInterErr = 0;
                    stRecordRsp.ulSCBNo = pstSCB->ulSCBNo;
                    stRecordRsp.ulLegNo = pstRecordLegCB->ulCBNo;

                    if (sc_send_cmd_record(&stRecordRsp.stMsgTag) != DOS_SUCC)
                    {
                        sc_log(SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Send record cmd FAIL! SCBNo : %u", pstSCB->ulSCBNo);
                    }
                }
            }

            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ACTIVE;
            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_EVENT), "Calling has been answered");
            break;
    }

    return DOS_SUCC;
proc_fail:
    /* �Ҷ����� */

    return DOS_FAIL;
}

U32 sc_call_bridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* DO nothing */
    return DOS_SUCC;
}

U32 sc_call_unbridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* DO nothing */
    return DOS_SUCC;
}

U32 sc_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST *pstHungup     = NULL;
    SC_LEG_CB            *pstCallee     = NULL;
    SC_LEG_CB            *pstCalling    = NULL;
    SC_LEG_CB            *pstHungupLeg  = NULL;
    SC_LEG_CB            *pstOtherLeg   = NULL;
    SC_AGENT_NODE_ST     *pstAgentCall  = NULL;
    S32                  i              = 0;
    S32                  lRes           = DOS_FAIL;

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHungup) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Leg %u has hungup. Legs:%u-%u, status : %u", pstHungup->ulLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PORC:
        case SC_CALL_AUTH:
            /* ��û�к��б��У����йҶ��ˣ�ֱ���ͷž����� */
            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_CALL_EXEC:
            /* ����ط�Ӧ���Ǳ����쳣�� */
            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    sc_lcb_free(pstCalling);
                    pstCalling = NULL;

                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                }
            }

            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;
            break;

        case SC_CALL_ALERTING:
        case SC_CALL_ACTIVE:
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallee)
                || DOS_ADDR_INVALID(pstCalling))
            {
                if (DOS_ADDR_VALID(pstCalling))
                {
                    /* ��������ҵ��� */
                    sc_send_billing_stop2bs(pstSCB, pstCalling, NULL);
                    sc_lcb_free(pstCalling);
                    sc_scb_free(pstSCB);
                    break;
                }

                /* ������ */
            }

            if (pstSCB->stCall.ulCallingLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalling;
            }
            else
            {
                pstHungupLeg = pstCallee;
            }

            /* ���ɻ��� */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            sc_send_billing_stop2bs(pstSCB, pstHungupLeg, NULL);

            /* �����˵������leg��OK */
            /*
              * ��Ҫ�����Ƿ�ǩ�����⣬�����/����LEG����ǩ�ˣ���Ҫ����SCB��������LEG�ҵ��µ�SCB��
              * ���򣬽���Ҫ��ǩ��LEG��Ϊ��ǰҵ����ƿ������LEG���Ҷ�����һ��LEG
              * ������Ҫ�����ͻ����
              */
            /* release ʱ���϶�����һ��leg hungup�ˣ����ڵ�leg��Ҫ�ͷŵ����ж���һ���ǲ�����ϯ��ǩ�����������Ҫ�Ҷ� */

            if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCallee;
                pstOtherLeg = pstCalling;
                pstAgentCall = pstSCB->stCall.pstAgentCalling;
            }
            else
            {
                pstHungupLeg = pstCalling;
                pstOtherLeg = pstCallee;
                pstAgentCall = pstSCB->stCall.pstAgentCallee;
            }

            /* �ж��Ƿ���Ҫ���У��ͻ���ǡ�1���ǿͻ�һ���ȹҶϵ�(���������У��ͻ�ֻ����PSTN����ϯֻ����SIP) */
            if ((pstHungupLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND
                || pstHungupLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0)
            {
                /* �ͻ���� */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* �ǳ�ǩʱ��Ҫ����ϯ��Ӧ��leg�Ľ���ʱ�䣬��ֵ����ʼʱ�䣬����ǻ���ʱʹ�� */
                    pstOtherLeg->stCall.stTimeInfo.ulAnswerTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                    for (i=0; i<SC_MAX_SERVICE_TYPE; i++)
                    {
                        pstSCB->aucServType[i] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* ���ͻ��ĺ����Ϊ���к��� */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* �޸���ϯ״̬Ϊ proc������ ��Ǳ����� */
                if (pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
                {
                    sc_agent_set_proc(pstAgentCall->pstAgentInfo, OPERATING_TYPE_PHONE);
                }
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;

                /* ������ʱ�� */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stCall.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stCall.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_PROCESS;

                break;
            }

            /* ����Ҫ�ͻ���� */
            sc_lcb_free(pstHungupLeg);
            pstHungupLeg = NULL;

            if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stCall.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stCall.ulCallingLegNo = U32_BUTT;
            }

            /* �޸���ϯ��״̬ */
            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo)
                && pstSCB->stCall.pstAgentCallee->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
            {
                sc_agent_set_idle(pstSCB->stCall.pstAgentCallee->pstAgentInfo, OPERATING_TYPE_PHONE);
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo)
                && pstSCB->stCall.pstAgentCalling->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
            {
                sc_agent_set_idle(pstSCB->stCall.pstAgentCalling->pstAgentInfo, OPERATING_TYPE_PHONE);
            }

            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* ��ǩ���������� */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* �ͷŵ� SCB */
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else
            {
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;
            }

            break;

        case SC_CALL_PROCESS:
            /* �޸���ϯ״̬, ����LEG���Ҷ��� */
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_CALL_RELEASE:
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        default:
            DOS_ASSERT(0);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Leg %u has hunguped. ", pstHungup->ulLegNo);

    return DOS_SUCC;
}

U32 sc_call_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST  *pstHold = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstHold->blIsHold)
    {
        /* ����Ǳ�HOLD�ģ���Ҫ����HOLDҵ��Ŷ */
        pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
        pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
        pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;

        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

        /* ��HOLD�� ���Ų����� */
        /* ��HOLD�Է� ���ź��б����� */
    }
    else
    {
        /* ����Ǳ�UNHOLD�ģ��Ѿ�û��HOLDҵ���ˣ������������оͺ� */
        if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
        {
            sc_trace_scb(pstSCB, "Bridge call when early media fail.");

            sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, CC_ERR_SC_SERV_NOT_EXIST);
            sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_SERV_NOT_EXIST);
            return DOS_FAIL;
        }
    }

    return DOS_SUCC;
}

U32 sc_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF = NULL;
    SC_LEG_CB             *pstLCB =  NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* �������й��̣�DTMFֻ���ǿͻ���ǣ�����û��ҵ����, ����ֻ�е�ǰLEGΪ��ϯʱ����Ҫ */
    if (pstDTMF->ulLegNo == pstSCB->stCall.ulCalleeLegNo)
    {
        if (DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCallee))
        {
            return DOS_SUCC;
        }
    }

    /* ���� ������ ҵ�� */
    if (!pstSCB->stAccessCode.stSCBTag.bValid)
    {
        /* ����������ҵ�� */
        if (pstDTMF->cDTMFVal != '*'
            || pstDTMF->cDTMFVal != '#' )
        {
            /* ��һ���ַ����� '*' ���� '#' ������  */
            return DOS_SUCC;
        }

        pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stAccessCode.szDialCache[0] = '\0';
        pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
        pstSCB->stAccessCode.ulAgentID = pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulAgentID;
        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;
    }

    /* �����ͻ���� */
    if (dos_strlen(pstLCB->stCall.stNumInfo.szDial) == 3)
    {
        /* ��һλ��*, �ڶ�λ��һ������, ����λΪ*����# */
        if ('*' == pstLCB->stCall.stNumInfo.szDial[0]
            && (pstLCB->stCall.stNumInfo.szDial[1] >= '0'  && pstLCB->stCall.stNumInfo.szDial[1] < '9')
            && ('*' == pstLCB->stCall.stNumInfo.szDial[2]  || '#' == pstLCB->stCall.stNumInfo.szDial[2]))
        {
            /* ��¼�ͻ���� */
            sc_trace_scb(pstSCB, "Mark custom. %d", pstLCB->stCall.stNumInfo.szDial[1]);
            return DOS_SUCC;
        }
    }

    /* ��ϯǿ�ƹҶκ��� */
    if (dos_strnicmp(pstLCB->stCall.stNumInfo.szDial, "##", SC_NUM_LENGTH) == 0
        || dos_strnicmp(pstLCB->stCall.stNumInfo.szDial, "**", SC_NUM_LENGTH) == 0)
    {
        /* �Ҷ϶Զ� */
        if (pstLCB->ulCBNo == pstSCB->stCall.ulCalleeLegNo)
        {
            return sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR);
        }
        else
        {
            return sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, CC_ERR_NORMAL_CLEAR);
        }
    }

    return DOS_SUCC;
}

U32 sc_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_CMD_RECORD_ST *pstRecord = NULL;
    SC_LEG_CB            *pstLCB    = NULL;

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        return DOS_FAIL;
    }

    /* ����¼������ */
    sc_send_billing_stop2bs_record(pstSCB, pstLCB);
    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);

    return DOS_SUCC;
}

U32 sc_call_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstCallingLegCB = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstCallSetup    = NULL;
    U32                    ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_PLAYBACK_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the call playback stop msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
            break;

        case SC_CALL_PORC:
            break;

        case SC_CALL_AUTH:
            break;

        case SC_CALL_EXEC:
            ulRet = sc_outgoing_call_process(pstSCB, pstCallingLegCB);
            break;

        case SC_CALL_TONE:
            /* ��ϯ��ǩʱ������ϯ����ʾ������ */
            //sc_req_answer_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo);
            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                return DOS_FAIL;
            }
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ACTIVE;
            break;

        case SC_CALL_ALERTING:
            break;
        case SC_CALL_ACTIVE:
            break;
        case SC_CALL_PROCESS:
            break;

        case SC_CALL_RELEASE:
            break;

        default:
            ulRet = DOS_FAIL;
            sc_trace_scb(pstSCB, "Invalid status.%u", pstSCB->stCall.stSCBTag.usStatus);
            break;
    }

    return DOS_SUCC;
}

U32 sc_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;
    U32                 ulRet               = DOS_FAIL;
    SC_LEG_CB           *pstCallingLegCB    = NULL;
    U32                 ulErrCode           = CC_ERR_NO_REASON;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call queue event. status : %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_AUTH:
            if (SC_LEAVE_CALL_QUE_TIMEOUT == pstMsg->usInterErr)
            {
                /* ������г�ʱ */
            }
            else if (SC_LEAVE_CALL_QUE_SUCC == pstMsg->usInterErr)
            {
                if (DOS_ADDR_INVALID(pstEvtCall->pstAgentNode))
                {
                    /* ���� */
                }
                else
                {
                    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
                    /* ������ϯ */
                    ulRet = sc_agent_call_by_id(pstSCB, pstCallingLegCB, pstEvtCall->pstAgentNode->pstAgentInfo->ulAgentID, &ulErrCode);
                }
            }
        default:
            break;

     }

    sc_trace_scb(pstSCB, "Proccessed call queue event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO ʧ�ܵĴ��� */
    }

    return ulRet;

}

U32 sc_preview_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing Preview auth event.");

    pstLCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_AUTH:
            switch (pstLCB->stCall.ucPeerType)
            {
                case SC_LEG_PEER_OUTBOUND:
                    ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_TT:
                    ulRet = sc_make_call2eix(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_INTERNAL:
                    ulRet = sc_make_call2sip(pstSCB, pstLCB);
                    break;

                default:
                    sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                    goto process_fail;
                    break;
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_EXEC;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
        case SC_PREVIEW_CALL_ALERTING:
        case SC_PREVIEW_CALL_ACTIVE:
        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
        case SC_PREVIEW_CALL_CONNECTED:
        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed Preview auth event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

process_fail:
    if (DOS_ADDR_VALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;
    }

    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    return DOS_FAIL;
}

U32 sc_preview_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstCallingCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call setup event event.");

    pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto unauth_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
        case SC_PREVIEW_CALL_ALERTING:
            /* Ǩ��״̬��proc */
            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_PORC;
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_ACTIVE:
            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTING;
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
        case SC_PREVIEW_CALL_CONNECTED:
        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

unauth_proc:
    return DOS_FAIL;

fail_proc:
    return DOS_FAIL;

}


U32 sc_preview_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstCallingCB = NULL;
    SC_LEG_CB    *pstCalleeCB  = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call setup event event.");

    pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            ulRet = DOS_FAIL;
            goto unauth_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
        case SC_PREVIEW_CALL_ALERTING:
            /* ��ϯ��֮ͨ��Ĵ��� */
            /* 1. ����PSTN�ĺ��� */
            /* 2. Ǩ��״̬��CONNTECTING */
            pstCalleeCB = sc_lcb_alloc();
            if (DOS_ADDR_INVALID(pstCalleeCB))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc lcb fail");
                goto fail_proc;
            }

            pstCalleeCB->stCall.bValid = DOS_TRUE;
            pstCalleeCB->stCall.ucStatus = SC_LEG_INIT;
            pstCalleeCB->ulSCBNo = pstSCB->ulSCBNo;
            pstSCB->stPreviewCall.ulCalleeLegNo = pstCalleeCB->ulCBNo;

            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCallee), pstCallingCB->stCall.stNumInfo.szOriginalCalling);
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCalling), pstCallingCB->stCall.stNumInfo.szOriginalCallee);

            /* @TODO ͨ�������趨ѡ�����к��룬���õ� pstCalleeCB->stCall.stNumInfo.szRealCalling */
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szRealCalling), pstCallingCB->stCall.stNumInfo.szOriginalCalling);

            if (sc_make_call2pstn(pstSCB, pstCalleeCB) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Make call to pstn fail.");
                goto fail_proc;
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTING;
            break;

        case SC_PREVIEW_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, pstSCB->stPreviewCall.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                goto fail_proc;
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTED;
            break;

        case SC_PREVIEW_CALL_CONNECTED:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_PROCESS:
            /* ������ǩ֮�ڵ�һ������ */
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

unauth_proc:
    return DOS_FAIL;

fail_proc:
    return DOS_FAIL;
}

U32 sc_preview_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_MSG_EVT_RINGING_ST   *pstRinging;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstRinging = (SC_MSG_EVT_RINGING_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Proccessing preview call setup event event.");

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            ulRet = DOS_FAIL;
            goto unauth_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
            /* Ǩ�Ƶ�alerting״̬ */
            if (pstRinging->ulWithMedia)
            {
                sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, DOS_TRUE);
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_ALERTING;

            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_ALERTING:
            break;

        case SC_PREVIEW_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
            /* Ǩ�Ƶ�alerting״̬ */
            /* �����ý����Ҫbridge���У�����������Ż����� */
            if (pstRinging->ulWithMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, pstSCB->stPreviewCall.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto fail_proc;
                }
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_ALERTING2;
            break;

        case SC_PREVIEW_CALL_ALERTING2:
            /* �������ý��״̬Ǩ�Ƶ���ý�壬��Ҫ�����зŻ��� */
            /* �������ý��״̬Ǩ�Ƶ���ý�壬�ŽӺ��� */
            break;

        case SC_PREVIEW_CALL_CONNECTED:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
unauth_proc:
    return DOS_FAIL;

fail_proc:
    return DOS_FAIL;

}

U32 sc_preview_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    return DOS_SUCC;
}

U32 sc_preview_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB     *pstCallingCB = NULL;
    SC_LEG_CB     *pstCalleeCB  = NULL;
    SC_MSG_EVT_HUNGUP_ST  *pstHungup = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Proccessing preview call hungup event.");

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
        case SC_PREVIEW_CALL_ALERTING:
        case SC_PREVIEW_CALL_ACTIVE:
            /* ���ʱ��Ҷ�ֻ������ϯ��LEG������Դ���� */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");

            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            if (pstCallingCB)
            {
                sc_lcb_free(pstCallingCB);
            }

            sc_scb_free(pstSCB);
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
            /* ���ʱ��Ҷϣ���������ϯҲ���ܿͻ�������ǿͻ���Ҫע��LEG��״̬ */
            break;

        case SC_PREVIEW_CALL_CONNECTED:
            /* ���ʱ��Ҷϣ����������ͷŵĽ��࣬������ͺ� */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");


            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (pstSCB->stPreviewCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                }

                sc_lcb_free(pstCalleeCB);
                pstCallingCB = NULL;
            }
            else
            {
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                }

                sc_lcb_free(pstCallingCB);
                pstCalleeCB = NULL;
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_PROCESS;
            break;

        case SC_PREVIEW_CALL_PROCESS:
            /* ��ϯ�������ˣ��Ҷ� */
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

}

U32 sc_preview_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* �������ֶ��β��� */
    return DOS_SUCC;
}

U32 sc_preview_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* ����¼������ */
    return DOS_SUCC;
}

U32 sc_preview_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* ������������ */
    return DOS_SUCC;
}

U32 sc_voice_verify_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auth rsp event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            ulRet = sc_make_call2pstn(pstSCB, pstLCB);
            if (ulRet != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Make call for voice verify fail.");
                goto proc_finishe;
            }

            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_EXEC;
            break;

        case SC_VOICE_VERIFY_EXEC:
            break;

        case SC_VOICE_VERIFY_PROC:
            break;

        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:

    if (ulRet != DOS_SUCC)
    {
        if (pstLCB)
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    sc_trace_scb(pstSCB, "Processed auth rsp event for voice verify. Ret: %s", ulRet != DOS_SUCC ? "FAIL" : "succ");


    if (ulRet != DOS_SUCC)
    {
        if (pstSCB)
        {
            sc_scb_free(pstSCB);
            pstSCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_voice_verify_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call setup event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_VOICE_VERIFY_PROC:
            break;

        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }


proc_finishe:
    sc_trace_scb(pstSCB, "Processed call setup event for voice verify. Ret: %s", (ulRet == DOS_SUCC) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call ringing event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Processed call ringing event for voice verify.");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;
    U32        ulLoop  = DOS_FAIL;
    SC_MSG_CMD_PLAYBACK_ST  stPlaybackRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call answer event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_ACTIVE;

            stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
            stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stPlaybackRsp.stMsgTag.usInterErr = 0;
            stPlaybackRsp.ulMode = 0;
            stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
            stPlaybackRsp.ulLegNo = pstLCB->ulCBNo;
            stPlaybackRsp.ulLoopCnt = SC_NUM_VERIFY_TIME;
            stPlaybackRsp.ulInterval = 0;
            stPlaybackRsp.ulSilence  = 0;
            stPlaybackRsp.enType = SC_CND_PLAYBACK_SYSTEM;
            stPlaybackRsp.ulTotalAudioCnt = 0;
            stPlaybackRsp.blNeedDTMF = DOS_FALSE;

            stPlaybackRsp.aulAudioList[0] = pstSCB->stVoiceVerify.ulTipsHitNo1;
            stPlaybackRsp.ulTotalAudioCnt++;
            for (ulLoop=0; ulLoop<SC_MAX_AUDIO_NUM; ulLoop++)
            {
                if ('\0' == pstSCB->stVoiceVerify.szVerifyCode[ulLoop])
                {
                    break;
                }

                switch (pstSCB->stVoiceVerify.szVerifyCode[ulLoop])
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        stPlaybackRsp.aulAudioList[stPlaybackRsp.ulTotalAudioCnt] = SC_SND_0 + (pstSCB->stVoiceVerify.szVerifyCode[ulLoop] - '0');
                        stPlaybackRsp.ulTotalAudioCnt++;
                        break;
                    default:
                        DOS_ASSERT(0);
                        break;
                }
            }

            if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            ulRet = DOS_SUCC;

            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Processed call answer event for voice verify.");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call release event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        return DOS_FAIL;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
        case SC_VOICE_VERIFY_AUTH:
        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
        case SC_VOICE_VERIFY_RELEASE:
            /* ���ͻ��� */
            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }

            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }
            break;
    }

    sc_trace_scb(pstSCB, "Processed call release event for voice verify.");

    return DOS_SUCC;
}

U32 sc_voice_verify_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call release event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        return DOS_FAIL;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
        case SC_VOICE_VERIFY_AUTH:
        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            break;
        case SC_VOICE_VERIFY_RELEASE:
            /* ���ͻ��� */
            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }

            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }
            break;
    }

    sc_trace_scb(pstSCB, "Processed call release event for voice verify.");

    return DOS_SUCC;
}

U32 sc_interception_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception auth event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_AUTH:
            switch (pstLCB->stCall.ucPeerType)
            {
                case SC_LEG_PEER_OUTBOUND:
                    ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_TT:
                    ulRet = sc_make_call2eix(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_INTERNAL:
                    ulRet = sc_make_call2sip(pstSCB, pstLCB);
                    break;

                default:
                    sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                    goto proc_finishe;
                    break;
            }

            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_EXEC;
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
        case SC_INTERCEPTION_ACTIVE:
        case SC_INTERCEPTION_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_interception_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception call setup event event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
        case SC_INTERCEPTION_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            /* Ǩ��״̬��proc */
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;

}

U32 sc_interception_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception ringing event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            break;

        case SC_INTERCEPTION_AUTH:
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_ALERTING:
            break;

        case SC_INTERCEPTION_ACTIVE:
            break;

        case SC_INTERCEPTION_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_interception_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    SC_LEG_CB  *pstAgentLCB = NULL;
    U32        ulRet        = DOS_FAIL;
    SC_MSG_CMD_MUX_ST stInterceptRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception answer event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    pstAgentLCB = sc_lcb_get(pstSCB->stInterception.ulAgentLegNo);
    if (DOS_ADDR_INVALID(pstAgentLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            break;

        case SC_INTERCEPTION_AUTH:
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_ACTIVE;

            stInterceptRsp.stMsgTag.ulMsgType = SC_CMD_MUX;
            stInterceptRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.stMsgTag.usInterErr = 0;

            stInterceptRsp.ulMode = SC_MUX_CMD_INTERCEPT;
            stInterceptRsp.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.ulLegNo = pstLCB->ulCBNo;
            stInterceptRsp.ulAgentLegNo = pstAgentLCB->ulCBNo;

            if (sc_send_cmd_mux(&stInterceptRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            ulRet = DOS_SUCC;

            break;

        case SC_INTERCEPTION_ACTIVE:
            break;

        case SC_INTERCEPTION_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_interception_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    U32        ulRet        = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception release event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        return DOS_FAIL;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            break;

        case SC_INTERCEPTION_AUTH:
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            break;

        case SC_INTERCEPTION_ACTIVE:
        case SC_INTERCEPTION_RELEASE:
            /* ���ͻ��� */
            if (ulRet != DOS_SUCC)
            {
                if (DOS_ADDR_VALID(pstLCB))
                {
                    sc_lcb_free(pstLCB);
                    pstLCB = NULL;
                }
            }
            pstSCB->stInterception.stSCBTag.bValid = DOS_FALSE;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;

}

U32 sc_whisper_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception auth event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_AUTH:
            switch (pstLCB->stCall.ucPeerType)
            {
                case SC_LEG_PEER_OUTBOUND:
                    ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_TT:
                    ulRet = sc_make_call2eix(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_INTERNAL:
                    ulRet = sc_make_call2sip(pstSCB, pstLCB);
                    break;

                default:
                    sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                    goto proc_finishe;
                    break;
            }

            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_EXEC;
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
        case SC_WHISPER_ACTIVE:
        case SC_WHISPER_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception call setup event event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
        case SC_WHISPER_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            /* Ǩ��״̬��proc */
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;

}

U32 sc_whisper_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception ringing event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_ALERTING:
            break;

        case SC_WHISPER_ACTIVE:
            break;

        case SC_WHISPER_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    SC_LEG_CB  *pstAgentLCB = NULL;
    U32        ulRet        = DOS_FAIL;
    SC_MSG_CMD_MUX_ST stInterceptRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception answer event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    pstAgentLCB = sc_lcb_get(pstSCB->stWhispered.ulAgentLegNo);
    if (DOS_ADDR_INVALID(pstAgentLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_ACTIVE;

            stInterceptRsp.stMsgTag.ulMsgType = SC_CMD_MUX;
            stInterceptRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.stMsgTag.usInterErr = 0;

            stInterceptRsp.ulMode = SC_MUX_CMD_WHISPER;
            stInterceptRsp.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.ulLegNo = pstLCB->ulCBNo;
            stInterceptRsp.ulAgentLegNo = pstAgentLCB->ulCBNo;

            if (sc_send_cmd_mux(&stInterceptRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            ulRet = DOS_SUCC;

            break;

        case SC_WHISPER_ACTIVE:
            break;

        case SC_WHISPER_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    U32        ulRet        = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception release event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        return DOS_FAIL;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            break;

        case SC_WHISPER_ACTIVE:
        case SC_WHISPER_RELEASE:
            /* ���ͻ��� */
            if (ulRet != DOS_SUCC)
            {
                if (DOS_ADDR_VALID(pstLCB))
                {
                    sc_lcb_free(pstLCB);
                    pstLCB = NULL;
                }
            }
            pstSCB->stWhispered.stSCBTag.bValid = DOS_FALSE;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;

}

U32 sc_auto_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    SC_LEG_CB                  *pstCalleeLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call auth event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */

        return DOS_SUCC;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_AUTH:
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_EXEC;
            pstLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstLegCB))
            {
                sc_scb_free(pstSCB);

                DOS_ASSERT(0);

                return DOS_FAIL;
            }
            ulRet = sc_make_call2pstn(pstSCB, pstLegCB);
            break;
        case SC_AUTO_CALL_AUTH2:
            /* ������ϯʱ�����е���֤��������ϯ */
            pstCalleeLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeLegCB))
            {
                /* TODO */
                return DOS_FAIL;
            }
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_EXEC2;
            ulRet = sc_make_call2pstn(pstSCB, pstCalleeLegCB);
            break;

        default:
            break;
    }

    return ulRet;
}

U32 sc_auto_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call stup event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PORC:
        case SC_AUTO_CALL_ALERTING:
            /* Ǩ��״̬��proc */
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PORC;
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_EXEC2:
            /* ��ϯ��leg���� */
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PORC2;
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_auto_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call ringing event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PORC:
        case SC_AUTO_CALL_ALERTING:
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_AFTER_KEY:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_PORC2:
            /* ��ϯ���� */
            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo, pstSCB->stAutoCall.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                ulRet = DOS_FAIL;
                goto proc_finishe;
            }

            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_CONNECTED;
            break;
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call ringing event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}


U32 sc_auto_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32          ulRet       = DOS_FAIL;
    U32          ulTaskMode  = U32_BUTT;
    SC_LEG_CB    *pstLCB     = NULL;
    SC_MSG_CMD_PLAYBACK_ST  stPlaybackRsp;
    U32          ulErrCode   = CC_ERR_NO_REASON;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call answer event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PORC:
        case SC_AUTO_CALL_ALERTING:
            /* �������� */
            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                DOS_ASSERT(0);

                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                ulRet = DOS_FAIL;
                goto proc_finishe;
            }

            switch (ulTaskMode)
            {
                /* ��Ҫ�����ģ�ͳһ�ȷ������ڷ����������봦���������� */
                case SC_TASK_MODE_KEY4AGENT:
                case SC_TASK_MODE_KEY4AGENT1:
                case SC_TASK_MODE_AUDIO_ONLY:
                case SC_TASK_MODE_AGENT_AFTER_AUDIO:
                    stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
                    stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.stMsgTag.usInterErr = 0;
                    stPlaybackRsp.ulMode = 0;
                    stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.ulLegNo = pstLCB->ulCBNo;
                    stPlaybackRsp.ulLoopCnt = sc_task_get_playcnt(pstSCB->stAutoCall.ulTcbID);
                    stPlaybackRsp.ulInterval = 0;
                    stPlaybackRsp.ulSilence  = 0;
                    stPlaybackRsp.enType = SC_CND_PLAYBACK_FILE;
                    stPlaybackRsp.blNeedDTMF = DOS_TRUE;
                    stPlaybackRsp.ulTotalAudioCnt++;
                    if (sc_task_get_audio_file(pstSCB->stAutoCall.ulTcbID) == NULL)
                    {
                        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                        ulRet = DOS_FAIL;
                        goto proc_finishe;
                    }

                    dos_strncpy(stPlaybackRsp.szAudioFile, sc_task_get_audio_file(pstSCB->stAutoCall.ulTcbID), SC_MAX_AUDIO_FILENAME_LEN-1);
                    stPlaybackRsp.szAudioFile[SC_MAX_AUDIO_FILENAME_LEN - 1] = '\0';

                    if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
                    {
                        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                        goto proc_finishe;
                    }

                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_ACTIVE;
                    ulRet = DOS_SUCC;
                    break;

                /* ֱ�ӽ�ͨ��ϯ */
                case SC_TASK_MODE_DIRECT4AGETN:
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    /* ����������ж���ҵ����ƿ� */
                    pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                    pstSCB->ulCurrentSrv++;
                    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                    pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                    pstSCB->stIncomingQueue.ulLegNo = pstSCB->stAutoCall.ulCallingLegNo;
                    pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                    if (sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szRealCallee) != DOS_SUCC)
                    {
                        /* �������ʧ�� */
                        DOS_ASSERT(0);
                        ulRet = DOS_FAIL;
                    }
                    else
                    {
                        /* ������ʾ�ͻ��ȴ� */
                        pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                        sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CONNECTING, 1, 0, 0);
                        ulRet = DOS_SUCC;
                    }

                    break;
                default:
                    DOS_ASSERT(0);
                    ulRet = DOS_FAIL;
                    ulErrCode = CC_ERR_SC_CONFIG_ERR;
                    goto proc_finishe;
            }

            break;
        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
            /* TODO */
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO ʧ�ܵĴ��� */
    }

    return ulRet;
}

U32 sc_auto_call_palayback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* �ж�һ��Ⱥ�������ģʽ������Ǻ��к�ת��ϯ����ת��ϯ������ͨ������ */
    SC_LEG_CB              *pstLCB          = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;
    U32                    ulTaskMode       = U32_BUTT;
    U32                    ulErrCode        = CC_ERR_NO_REASON;
    U32                    ulRet            = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the auto call playback stop msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    if (pstLCB->stPlayback.usStatus == SC_SU_PLAYBACK_INIT)
    {
        /* ������ silence_stream ��ʱ��stop�¼�������Ҫ������ ����������⣬������playstop�¼�û���ϴ� */
        ulRet = DOS_SUCC;

        goto proc_finishe;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ACTIVE:
            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                DOS_ASSERT(0);

                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                ulRet = DOS_FAIL;
                goto proc_finishe;
            }

            switch (ulTaskMode)
            {
                /* ��Ҫ�����ģ�ͳһ�ȷ������ڷ����������봦���������� */
                case SC_TASK_MODE_AGENT_AFTER_AUDIO:
                    /* ת��ϯ */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    /* ����������ж���ҵ����ƿ� */
                    pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                    pstSCB->ulCurrentSrv++;
                    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                    pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                    pstSCB->stIncomingQueue.ulLegNo = pstSCB->stAutoCall.ulCallingLegNo;
                    pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                    if (sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szRealCallee) != DOS_SUCC)
                    {
                        /* �������ʧ�� */
                        DOS_ASSERT(0);
                        ulRet = DOS_FAIL;
                    }
                    else
                    {
                        /* ������ʾ�ͻ��ȴ� */
                        pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                        sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CONNECTING, 1, 0, 0);
                        ulRet = DOS_SUCC;
                    }

                    break;
                case SC_TASK_MODE_AUDIO_ONLY:
                    /* �ҶϿͻ� */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
                    if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                    {

                    }

                    break;
                default:
                    break;
            }
    }

proc_finishe:

    sc_trace_scb(pstSCB, "Proccessed auto call playk stop event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_auto_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;
    U32                   ulRet         = DOS_FAIL;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call queue event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_AFTER_KEY:
            if (SC_LEAVE_CALL_QUE_TIMEOUT == pstMsg->usInterErr)
            {
                /* ������г�ʱ */
            }
            else if (SC_LEAVE_CALL_QUE_SUCC == pstMsg->usInterErr)
            {
                if (DOS_ADDR_INVALID(pstEvtCall->pstAgentNode))
                {
                    /* ���� */
                }
                else
                {
                    /* ������ϯ */
                    sc_agent_auto_callback(pstSCB, pstEvtCall->pstAgentNode);
                }
            }
        default:
            break;

     }

    sc_trace_scb(pstSCB, "Proccessed auto call answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO ʧ�ܵĴ��� */
    }

    return ulRet;

}

U32 sc_auto_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;
    U32                   ulTaskMode    = U32_BUTT;
    S32                   lKey          = 0;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call dtmf event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    lKey = pstDTMF->cDTMFVal - '0';

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ACTIVE:
            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                /* TODO */
                DOS_ASSERT(0);
                //ulErrCode = CC_ERR_SC_CONFIG_ERR;
                return DOS_FAIL;
            }

            switch (ulTaskMode)
            {
                case SC_TASK_MODE_KEY4AGENT1:
                    if (lKey != 0)
                    {
                        break;
                    }
                case SC_TASK_MODE_KEY4AGENT:
                    /* ת��ϯ */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szCallee);
                    break;

                default:
                    break;
            }
            break;
         default:
            break;
    }


    return DOS_SUCC;
}

U32 sc_auto_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB     *pstCallingCB = NULL;
    SC_LEG_CB     *pstCalleeCB  = NULL;
    SC_MSG_EVT_HUNGUP_ST  *pstHungup = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Proccessing auto call hungup event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PORC:
        case SC_AUTO_CALL_ALERTING:
        case SC_AUTO_CALL_ACTIVE:
            /* ���ʱ��Ҷ�ֻ���ǿͻ���LEG������Դ���� */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");

            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (pstCallingCB)
            {
                sc_lcb_free(pstCallingCB);
            }

            sc_scb_free(pstSCB);
            break;

        case SC_AUTO_CALL_AFTER_KEY:
        case SC_AUTO_CALL_AUTH2:
        case SC_AUTO_CALL_EXEC2:
        case SC_AUTO_CALL_PORC2:
        case SC_AUTO_CALL_ALERTING2:
            /* ���ʱ��Ҷϣ���������ϯҲ���ܿͻ�������ǿͻ���Ҫע��LEG��״̬ */
            break;
        case SC_AUTO_CALL_CONNECTED:
            /* ���ʱ��Ҷϣ����������ͷŵĽ��࣬������ͺã��ж���ϯ��״̬ */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");
            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (pstSCB->stAutoCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                }

                sc_lcb_free(pstCalleeCB);
                pstCallingCB = NULL;
            }
            else
            {
                if (pstCalleeCB->ulIndSCBNo != U32_BUTT)
                {
                    /* TODO ��ǩ���ж�һ���Ƿ���Ҫ��ǿͻ���Ҫע��scb���ͷ� */
                }
                else
                {
                    if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                    {
                        DOS_ASSERT(0);
                    }
                }
                sc_lcb_free(pstCallingCB);
                pstCalleeCB = NULL;
            }

            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PROCESS;
            break;

        case SC_AUTO_CALL_PROCESS:
            /* ��ϯ�������ˣ��Ҷ� */
            pstCalleeCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        case SC_AUTO_CALL_RELEASE:
            pstCalleeCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;

            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed auto call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}


U32 sc_sigin_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing the sigin auth msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    if (!pstSCB->stSigin.stSCBTag.bValid)
    {
        /* û������ ��ǩҵ�� */
        sc_scb_free(pstSCB);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Scb(%u) not have sigin server.", pstSCB->ulSCBNo);
        return DOS_FAIL;
    }

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_scb_free(pstSCB);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */

        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_AUTH:
            /* ������� */
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;
            ulRet = sc_make_call2pstn(pstSCB, pstLegCB);
            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_sigin_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB   *pstLegCB = NULL;
    SC_MSG_EVT_CALL_ST  *pstCallSetup;
    U32         ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_CALL_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the sigin setup msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_EXEC:
            /* ������� */
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_PORC;
            ulRet = DOS_SUCC;
            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_sigin_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_ST *pstEvent = NULL;
    SC_LEG_CB             *pstLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "process the sigin alerting msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulLegNo = pstSCB->stSigin.ulLegNo;

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_IDEL:
        case SC_SIGIN_AUTH:
        case SC_SIGIN_PORC:
        case SC_SIGIN_EXEC:
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_ALERTING;
            break;

        case SC_SIGIN_ALERTING:
        case SC_SIGIN_ACTIVE:
        case SC_SIGIN_RELEASE:
            break;
    }

    return DOS_SUCC;
}

U32 sc_sigin_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ANSWER_ST  *pstEvtAnswer   = NULL;
    SC_LEG_CB             *pstLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the sigin answer msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ALERTING:
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_ACTIVE;

            if (DOS_ADDR_VALID(pstSCB->stSigin.pstAgentNode))
            {
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_TRUE;
            }
            /* �ų�ǩ�� */
            sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

            break;
        default:
            break;
    }

    return DOS_SUCC;
}


U32 sc_sigin_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLegCB        = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the sigin playback stop msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ACTIVE:
            if (DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode))
            {
                break;
            }

            if (pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected)
            {
                if (pstLegCB->stPlayback.usStatus == SC_SU_PLAYBACK_INIT)
                {
                    /* �ų�ǩ�� */
                    sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                }
            }
            else
            {

            }

            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_sigin_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF = NULL;
    SC_LEG_CB             *pstLCB =  NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ��ǩ�绰û��ҵ��ʱ������Ч */
    if (pstLCB->ulSCBNo != U32_BUTT)
    {
        return DOS_SUCC;
    }

    if (DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode)
        || DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode->pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!pstSCB->stAccessCode.stSCBTag.bValid)
    {
        /* ���� ������ ҵ�� */
        if (pstDTMF->cDTMFVal != '*')
        {
            /* ��ǩʱ��������ĵ�һ���ַ������� '*'  */
            return DOS_SUCC;
        }

        pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stAccessCode.szDialCache[0] = '\0';
        pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
        pstSCB->stAccessCode.ulAgentID = pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulAgentID;
        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;
    }

    return DOS_SUCC;
}

U32 sc_sigin_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLegCB        = NULL;
    U32                     ulRet           = DOS_FAIL;
    SC_MSG_CMD_CALL_ST stCallMsg;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the sigin release msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode))
    {
        sc_scb_free(pstSCB);
        sc_lcb_free(pstLegCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ACTIVE:
            if (pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected)
            {
                /* ��Ҫ���º�����ϯ�����г�ǩ */
                pstLegCB->stPlayback.usStatus = SC_SU_PLAYBACK_INIT;
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulLegNo = U32_BUTT;

                if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                {
                    /* ��Ҫ��֤ */
                    pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_AUTH;
                    ulRet = sc_send_usr_auth2bs(pstSCB, pstLegCB);
                    if (ulRet != DOS_SUCC)
                    {
                        sc_scb_free(pstSCB);
                        sc_lcb_free(pstLegCB);

                        return DOS_FAIL;
                    }
                }
                else
                {
                    /* ������� */
                    /* ����һ�º��� */
                    dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
                    dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

                    dos_snprintf(pstLegCB->stCall.stNumInfo.szCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
                    dos_snprintf(pstLegCB->stCall.stNumInfo.szCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

                    pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;

                    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
                    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stCallMsg.stMsgTag.usInterErr = 0;
                    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
                    stCallMsg.ulLCBNo = pstLegCB->ulCBNo;

                    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
                    {
                        sc_scb_free(pstSCB);
                        sc_lcb_free(pstLegCB);

                        return DOS_FAIL;
                    }
                }
            }
            else
            {
                /* �ͷ� */
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulLegNo = U32_BUTT;
                sc_scb_free(pstSCB);
                sc_lcb_free(pstLegCB);
            }
            break;
        default:
            /* �ͷ� */
            pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;
            pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
            sc_scb_free(pstSCB);
            sc_lcb_free(pstLegCB);
            break;
    }

    return DOS_SUCC;
}

U32 sc_incoming_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing leave call queue event. status : %u", pstSCB->stIncomingQueue.stSCBTag.usStatus);

    switch (pstSCB->stIncomingQueue.stSCBTag.usStatus)
    {
        case SC_INQUEUE_ACTIVE:
            {
                pstSCB->stIncomingQueue.stSCBTag.bWaitingExit = DOS_TRUE;

            }
            break;
        default:
            break;
    }


    return DOS_SUCC;
}

U32 sc_mark_custom_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       = NULL;
    U32                   ulRet         = DOS_SUCC;
    U32                   ulKey         = 0;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing mark custom dtmf event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_PROC:
            /* �ͻ���ǣ���һ���ַ�����Ϊ '*' */
            if (pstLCB->stCall.stNumInfo.szDial[0] != '*')
            {
                pstLCB->stCall.stNumInfo.szDial[0] = '\0';
                break;
            }

            dos_strcpy(pstSCB->stMarkCustom.szDialCache, pstLCB->stCall.stNumInfo.szDial);

            if (pstSCB->stMarkCustom.szDialCache[0] == '\0'
                && pstDTMF->cDTMFVal != '*')
            {
                /* ��һ���ַ����� * ���� #�����ñ��� */
                break;
            }

            dos_strcpy(pstSCB->stMarkCustom.szDialCache, pstLCB->stCall.stNumInfo.szDial);

            /* ���Ϊ * ���� #���ж��Ƿ����  [*|#]D[*|#] */
            if ((pstDTMF->cDTMFVal == '*' || pstDTMF->cDTMFVal == '#')
                    && dos_strlen(pstSCB->stMarkCustom.szDialCache) > 1)
            {
                if (pstSCB->stMarkCustom.szDialCache[0] == '*'
                    && pstSCB->stMarkCustom.szDialCache[3]  == '\0'
                    && 1 == dos_sscanf(pstSCB->stMarkCustom.szDialCache+1, "%u", &ulKey)
                    && ulKey <= 9)
                {
                    /* �ͻ���� */
                    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall)
                         && DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo))
                    {
                        sc_agent_marker_update_req(pstSCB->ulCustomerID, pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ulAgentID, ulKey, pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->szLastCustomerNum);
                    }

                    /* ֹͣ��ʱ�� */
                    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.stTmrHandle))
                    {
                        dos_tmr_stop(&pstSCB->stMarkCustom.stTmrHandle);
                        pstSCB->stMarkCustom.stTmrHandle = NULL;
                    }

                    /* ֹͣ���� */
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstLCB->ulCBNo);

                    /* �ж���ϯ�Ƿ��ǳ�ǩ�����������Ҷϵ绰 */
                    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall)
                         && DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo))
                    {
                        if (pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
                        {
                            sc_agent_set_idle(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo, OPERATING_TYPE_PHONE);
                        }
                    }

                    if (pstLCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* ��ǩ���������� */
                        pstLCB->ulSCBNo = U32_BUTT;
                        sc_req_play_sound(pstLCB->ulIndSCBNo, pstLCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                        /* �ͷŵ� SCB */
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                    }
                    else
                    {
                        pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                        sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                        pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_ACTIVE;
                    }
                }
                else
                {
                    /* ��ʽ������ջ��� */
                    pstSCB->stMarkCustom.szDialCache[0] = '\0';
                    pstLCB->stCall.stNumInfo.szDial[0] = '\0';
                }
            }

            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_mark_custom_playback_start(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    sc_trace_scb(pstSCB, "Processing mark custom play start event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_IDEL:
            pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_PROC;
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_mark_custom_playback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLCB          = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing mark custom play end event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_PROC:
            pstLCB = sc_lcb_get(pstRlayback->ulLegNo);
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_req_play_sound(pstSCB->ulSCBNo, (pstLCB)->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
            }
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_mark_custom_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB            *pstMarkLeg    = NULL;

    sc_trace_scb(pstSCB, "Processing mark custom realse event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_IDEL:
            break;
        case SC_MAKR_CUSTOM_PROC:
        case SC_MAKR_CUSTOM_ACTIVE:
            /* ��Ҫ���ɿͻ���ǵĻ�����leg������ҵ�����ͷ� */
            pstMarkLeg = sc_lcb_get(pstSCB->stMarkCustom.ulLegNo);
            if (DOS_ADDR_VALID(pstMarkLeg))
            {
                /* ���к����ΪΪ �ͻ���� */
                if (pstSCB->stMarkCustom.szDialCache[0] == '*'
                    && pstSCB->stMarkCustom.szDialCache[1] >= '0'
                    && pstSCB->stMarkCustom.szDialCache[1] <= '9'
                    && (pstSCB->stMarkCustom.szDialCache[2] == '*' || pstSCB->stMarkCustom.szDialCache[2] == '#')
                    && pstSCB->stMarkCustom.szDialCache[3] == '\0')
                {
                    dos_strcpy(pstMarkLeg->stCall.stNumInfo.szOriginalCallee, pstSCB->stMarkCustom.szDialCache);
                }
                else
                {
                    dos_strcpy(pstMarkLeg->stCall.stNumInfo.szOriginalCallee, "*#");
                }

                sc_send_billing_stop2bs(pstSCB, pstMarkLeg, NULL);
            }

            if (DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall)
                         && DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo))
            {
                if (pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
                {
                    sc_agent_set_idle(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo, OPERATING_TYPE_PHONE);
                }
                pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
            }

            pstSCB->stMarkCustom.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_play_balance(U64 LBalance, U32 aulAudioList[], U32 *pulCount)
{
    U64   i             = 0;
    S32   j             = 0;
    S32   lInteger      = 0;
    S64   LRemainder    = 0;
    BOOL  bIsGetMSB     = DOS_FALSE;
    BOOL  bIsZero       = DOS_FALSE;
    U32   ulIndex       = 0;

    /* �������ƴ�ӳ����� */
    aulAudioList[ulIndex++] = SC_SND_YOUR_BALANCE;

    if (LBalance < 0)
    {
        LBalance = 0 - LBalance;
        aulAudioList[ulIndex++] = SC_SND_FU;
    }

    LRemainder = LBalance;
    bIsGetMSB = DOS_FALSE;

    for (i=100000000000,j=0; j<10; i/=10,j++)
    {
        lInteger = LRemainder / i;
        LRemainder = LRemainder % i;

        if ((0 == lInteger || lInteger > 9) && DOS_FALSE == bIsGetMSB)
        {
            /* TODO ����9ʱ��˵����������һ���� */
            continue;
        }

        bIsGetMSB = DOS_TRUE;

        if (0 == lInteger)
        {
            bIsZero = DOS_TRUE;
        }
        else
        {
            if (bIsZero == DOS_TRUE)
            {
                bIsZero = DOS_FALSE;
                aulAudioList[ulIndex++] = SC_SND_0;
                aulAudioList[ulIndex++] = SC_SND_0 + lInteger;
            }
            else
            {
                aulAudioList[ulIndex++] = SC_SND_0 + lInteger;
            }
        }

        switch (j)
        {
            case 0:
            case 4:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_QIAN;
                }
                break;
            case 1:
            case 5:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_BAI;
                }
                break;
            case 2:
            case 6:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_SHI;
                }
                break;
            case 3:
                aulAudioList[ulIndex++] = SC_SND_WAN;
                bIsZero = DOS_FALSE;
                break;
            case 7:
                aulAudioList[ulIndex++] = SC_SND_YUAN;
                bIsZero = DOS_FALSE;
                break;
            case 8:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_JIAO;
                }
                bIsZero = DOS_FALSE;
                break;
            case 9:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_FEN;
                }
                break;
            default:
                break;
        }
    }

    if (DOS_FALSE == bIsGetMSB)
    {
        /* �����0 */
        aulAudioList[ulIndex++] = SC_SND_0;
        aulAudioList[ulIndex++] = SC_SND_YUAN;
    }

    *pulCount = ulIndex;

    return DOS_SUCC;
}

U32 sc_access_code_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    U32                         ulRet = DOS_FAIL;
    SC_MSG_CMD_PLAYBACK_ST      stPlaybackRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing access code event. status : %u", pstSCB->stAccessCode.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */

        return DOS_SUCC;
    }

    pstLegCB = sc_lcb_get(pstSCB->stAccessCode.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        /* ������ */
    }

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_ACTIVE:
            pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_RELEASE;

            switch (pstSCB->stAccessCode.ulSrvType)
            {
                case SC_ACCESS_BALANCE_ENQUIRY:
                    /* ������� */
                    stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
                    stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.stMsgTag.usInterErr = 0;
                    stPlaybackRsp.ulMode = 0;
                    stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.ulLegNo = pstLegCB->ulCBNo;
                    stPlaybackRsp.ulLoopCnt = 1;
                    stPlaybackRsp.ulInterval = 0;
                    stPlaybackRsp.ulSilence  = 0;
                    stPlaybackRsp.enType = SC_CND_PLAYBACK_SYSTEM;
                    stPlaybackRsp.blNeedDTMF = DOS_TRUE;
                    sc_play_balance(pstAuthRsp->LBalance, stPlaybackRsp.aulAudioList, &stPlaybackRsp.ulTotalAudioCnt);

                    if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
                    {
                        /* ������ */
                    }
                    break;
                default:
                    break;
            }

            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_access_code_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF = NULL;
    SC_LEG_CB             *pstLCB  =  NULL;
    U32                   ulRet    = DOS_SUCC;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_OVERLAP:
            /* ��һ���ַ�����Ϊ '*' ���� '#' */
            if (pstLCB->stCall.stNumInfo.szDial[0] != '*' && pstLCB->stCall.stNumInfo.szDial[0] != '#')
            {
                pstLCB->stCall.stNumInfo.szDial[0] = '\0';
                break;
            }

            dos_strcpy(pstSCB->stAccessCode.szDialCache, pstLCB->stCall.stNumInfo.szDial);

            if (dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_HANGUP_CUSTOMER1].szCodeFormat) == 0
                 || dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_HANGUP_CUSTOMER2].szCodeFormat) == 0)
            {
                /* ## / ** */
                pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_ACTIVE;
                astSCAccessList[SC_ACCESS_HANGUP_CUSTOMER1].fn_init(pstSCB, pstLCB);
                /* ��ջ��� */
                pstSCB->stAccessCode.szDialCache[0] = '\0';
            }
            else if (pstDTMF->cDTMFVal == '#' && dos_strlen(pstSCB->stAccessCode.szDialCache) > 1)
            {
                /* # Ϊ���������յ��󣬾�Ӧ��ȥ����, �ر�ģ������һ���ַ�Ϊ#,����Ҫȥ���� */
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Secondary dialing. caller : %s, DialNum : %s", pstLCB->stCall.stNumInfo.szRealCalling, pstSCB->stAccessCode.szDialCache);
                /* ���������һ�� # */
                pstSCB->stAccessCode.szDialCache[dos_strlen(pstSCB->stAccessCode.szDialCache) - 1] = '\0';
                ulRet = sc_call_access_code(pstSCB, pstLCB, pstSCB->stAccessCode.szDialCache, DOS_TRUE);
                /* ��ջ��� */
                pstSCB->stAccessCode.szDialCache[0] = '\0';
                pstLCB->stCall.stNumInfo.szDial[0] = '\0';
            }
            else if (pstDTMF->cDTMFVal == '*' && dos_strlen(pstSCB->stAccessCode.szDialCache) > 1)
            {
                /* �жϽ��յ� * �ţ��Ƿ���Ҫ���� */
                if (dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_MARK_CUSTOMER].szCodeFormat)
                    && dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_BLIND_TRANSFER].szCodeFormat)
                    && dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_ATTENDED_TRANSFER].szCodeFormat))
                {
                    /* ���� */
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Secondary dialing. caller : %s, DialNum : %s", pstLCB->stCall.stNumInfo.szRealCalling, pstSCB->stAccessCode.szDialCache);
                    /* ���������һ�� * */
                    pstSCB->stAccessCode.szDialCache[dos_strlen(pstSCB->stAccessCode.szDialCache) - 1] = '\0';
                    ulRet = sc_call_access_code(pstSCB, pstLCB, pstSCB->stAccessCode.szDialCache, DOS_TRUE);
                    /* ��ջ��� */
                    pstSCB->stAccessCode.szDialCache[0] = '\0';
                    pstLCB->stCall.stNumInfo.szDial[0] = '\0';
                }
            }
            break;
        default:
            break;
     }

    return ulRet;
}

U32 sc_access_code_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLegCB        = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the access code playback stop msg. status: %u", pstSCB->stAccessCode.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stAccessCode.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_ACTIVE:
            switch (pstSCB->stAccessCode.ulSrvType)
            {
                case SC_ACCESS_BALANCE_ENQUIRY:
                case SC_ACCESS_AGENT_ONLINE:
                case SC_ACCESS_AGENT_OFFLINE:
                    if (pstLegCB->stPlayback.usStatus == SC_SU_PLAYBACK_INIT)
                    {
                        /* �Ҷϵ绰 */
                        pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_RELEASE;
                        sc_req_hungup(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    }
                    break;
                case SC_ACCESS_AGENT_EN_QUEUE:
                case SC_ACCESS_AGENT_DN_QUEUE:
                    break;
                default:
                    break;
            }

            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_access_code_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST *pstEvtCall = NULL;

    pstEvtCall = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing access code realse event. status : %u", pstSCB->stIncomingQueue.stSCBTag.usStatus);

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_ACTIVE:
        case SC_ACCESS_CODE_RELEASE:
            pstSCB->stAccessCode.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_transfer_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp      = NULL;
    SC_LEG_CB                  *pstLegCB        = NULL;
    SC_LEG_CB                  *pstPublishLeg   = NULL;
    SC_AGENT_NODE_ST           *pstAgentNode    = NULL;
    U32                         ulRet           = DOS_FAIL;
    SC_MSG_CMD_CALL_ST          stCallMsg;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer auth event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */

        return DOS_SUCC;
    }

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_AUTH:
            /* ���е����� */
            pstPublishLeg = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
            if (DOS_ADDR_INVALID(pstPublishLeg))
            {
                /* TODO ������ */
                return DOS_FAIL;
            }

            /* �޸���ϯ��״̬����æ */
            pstAgentNode =  sc_agent_get_by_id(pstSCB->stTransfer.ulPublishLegNo);
            if (DOS_ADDR_VALID(pstAgentNode) && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
            {
                sc_agent_set_busy(pstAgentNode->pstAgentInfo, OPERATING_TYPE_PHONE);
                pstAgentNode->pstAgentInfo->ulLegNo = pstPublishLeg->ulCBNo;
            }

            /* �ж���ϯ�Ƿ�ǩ */
            if (pstPublishLeg->ulIndSCBNo != U32_BUTT)
            {
                if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
                {
                    /* ��ǩ��ֱ�ӹҶϷ��𷽣�bridge�ڶ����͵����� */
                    sc_req_hungup(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                }
                else
                {
                    /* Э��ת��hold ���ķ��������𷽷Ż����� */
                    sc_req_hold(pstSCB->ulSCBNo, pstSCB->stTransfer.ulSubLegNo, SC_HOLD_FLAG_HOLD);
                    sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stTransfer.ulNotifyLegNo, DOS_TRUE);
                }

                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulPublishLegNo, pstSCB->stTransfer.ulSubLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    return DOS_FAIL;
                }
                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_FINISHED;

                break;
            }

            if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
            {
                /* holdס���ķ����ҶϷ��� */
                sc_req_hold(pstSCB->ulSCBNo, pstSCB->stTransfer.ulSubLegNo, SC_HOLD_FLAG_HOLD);
                sc_req_hungup(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            }
            else
            {
                sc_req_hold(pstSCB->ulSCBNo, pstSCB->stTransfer.ulSubLegNo, SC_HOLD_FLAG_HOLD);
                sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stTransfer.ulNotifyLegNo, DOS_TRUE);
            }

            /* ���е�����������һ�º��� */
            dos_snprintf(pstPublishLeg->stCall.stNumInfo.szRealCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szRealCallee), pstPublishLeg->stCall.stNumInfo.szOriginalCallee);
            dos_snprintf(pstPublishLeg->stCall.stNumInfo.szRealCalling, sizeof(pstPublishLeg->stCall.stNumInfo.szRealCalling), pstPublishLeg->stCall.stNumInfo.szOriginalCalling);

            dos_snprintf(pstPublishLeg->stCall.stNumInfo.szCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szCallee), pstPublishLeg->stCall.stNumInfo.szOriginalCallee);
            dos_snprintf(pstPublishLeg->stCall.stNumInfo.szCalling, sizeof(pstPublishLeg->stCall.stNumInfo.szCalling), pstPublishLeg->stCall.stNumInfo.szOriginalCalling);

            pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_EXEC;

            stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
            stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stCallMsg.stMsgTag.usInterErr = 0;
            stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
            stCallMsg.ulLCBNo = pstPublishLeg->ulCBNo;

            if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
            {
                /* TODO ������ */
                return DOS_FAIL;
            }
            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32        ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer setup event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_EXEC:
            pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_PROC;
            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32        ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer ringing event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_PROC:
            pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_ALERTING;
            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32         ulRet         = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer answer event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_ALERTING:
            if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
            {
                /* äת���� A �� C ��ͨ */
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulPublishLegNo, pstSCB->stTransfer.ulSubLegNo) != DOS_SUCC)
                {
                    /* ������ */
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    ulRet = DOS_FAIL;
                }
                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_FINISHED;
            }
            else
            {
                /* Э��ת����B��C��ͨ */
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulNotifyLegNo, pstSCB->stTransfer.ulPublishLegNo) != DOS_SUCC)
                {
                    /* ������ */
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    ulRet = DOS_FAIL;
                }
                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_TRANSFER;
            }

            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST    *pstEvtCall           = NULL;
    SC_LEG_CB               *psthungLegCB         = NULL;
    SC_LEG_CB               *pstOtherLegCB        = NULL;
    SC_AGENT_NODE_ST        *pstNotifyAgentNode   = NULL;
    SC_AGENT_NODE_ST        *pstHungAgentNode     = NULL;
    SC_AGENT_NODE_ST        *pstOtherAgentNode    = NULL;
    U32                     ulRet                 = DOS_SUCC;

    pstEvtCall = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer realse event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    psthungLegCB = sc_lcb_get(pstEvtCall->ulLegNo);
    if (DOS_ADDR_INVALID(psthungLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_AUTH:
        case SC_TRANSFER_EXEC:
        case SC_TRANSFER_ACTIVE:
            if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulNotifyLegNo)
            {
                /* ���� �Ҷϣ�����ת�ӵĻ���;���ɻ�����Ӧ��ɾ��ת��ҵ���B��Ӧ��ҵ��������� ulNotifyAgentID������Ҫ�޸���ϯ��״̬ */
                /* �޸� B ��Ӧ����ϯ��״̬ */
                pstNotifyAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulNotifyAgentID);
                if (DOS_ADDR_VALID(pstNotifyAgentNode) && DOS_ADDR_VALID(pstNotifyAgentNode->pstAgentInfo))
                {
                    sc_agent_set_idle(pstNotifyAgentNode->pstAgentInfo, OPERATING_TYPE_PHONE);
                }

                pstSCB->stTransfer.ulNotifyLegNo = U32_BUTT;
                pstSCB->stTransfer.ulNotifyAgentID = 0;
            }
            else if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulPublishLegNo)
            {

            }
            else
            {

            }

            break;
        case SC_TRANSFER_TRANSFER:
            /* Э��ת */
            if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulNotifyLegNo)
            {
                /* ��ͨ A �� C������ B �Ļ��� */
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulPublishLegNo, pstSCB->stTransfer.ulSubLegNo) != DOS_SUCC)
                {
                    /* ������ */
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    ulRet = DOS_FAIL;
                    break;
                }

                /* �޸� B ��Ӧ����ϯ��״̬ */
                pstNotifyAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulNotifyAgentID);
                if (DOS_ADDR_VALID(pstNotifyAgentNode) && DOS_ADDR_VALID(pstNotifyAgentNode->pstAgentInfo))
                {
                    sc_agent_set_idle(pstNotifyAgentNode->pstAgentInfo, OPERATING_TYPE_PHONE);
                }

                pstSCB->stTransfer.ulNotifyLegNo = U32_BUTT;
                pstSCB->stTransfer.ulNotifyAgentID = 0;

                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_FINISHED;
            }
            else if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulPublishLegNo)
            {
                /* TODO �������Ҷ� */
            }
            else
            {
                /* TODO ��һ���Ҷ� */
            }

            break;
        case SC_TRANSFER_FINISHED:
            /* �����Ҷϣ��ж���ϯ�Ƿ�ǩ���ж��Ƿ���Ҫ���пͻ���ǵ� */
            if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulSubLegNo)
            {
                pstHungAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulSubAgentID);
                pstOtherAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
                pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
            }
            else
            {
                pstHungAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
                pstOtherAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulSubAgentID);
                pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulSubLegNo);
            }

            if (DOS_ADDR_VALID(pstHungAgentNode) && DOS_ADDR_VALID(pstHungAgentNode->pstAgentInfo))
            {
                sc_agent_set_idle(pstHungAgentNode->pstAgentInfo, OPERATING_TYPE_PHONE);
            }

            if (DOS_ADDR_VALID(pstOtherAgentNode) && DOS_ADDR_VALID(pstOtherAgentNode->pstAgentInfo))
            {
                sc_agent_set_idle(pstOtherAgentNode->pstAgentInfo, OPERATING_TYPE_PHONE);
            }

            if (pstOtherLegCB->ulIndSCBNo != U32_BUTT)
            {
                /* ��ǩ���������� */
                pstOtherLegCB->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLegCB->ulIndSCBNo, pstOtherLegCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* �ͷŵ� SCB */
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else
            {
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;
            }

            pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_RELEASE;
            break;
        case SC_TRANSFER_RELEASE:
            if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulSubLegNo)
            {
                pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
            }
            else
            {
                pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulSubLegNo);
            }

            sc_lcb_free(psthungLegCB);
            psthungLegCB = NULL;
            if (DOS_ADDR_VALID(pstOtherLegCB))
            {
                sc_lcb_free(pstOtherLegCB);
                pstOtherLegCB = NULL;
            }
            sc_scb_free(pstSCB);

            break;
        default:
            break;
    }

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

