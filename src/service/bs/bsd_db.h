/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���
 *
 *  ����ʱ��:
 *  ��    ��:
 *  ��    ��:
 *  �޸���ʷ:
 */

#ifndef __BSD_DB_H__
#define __BSD_DB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/* �����ı����Ͷ��� */
enum BS_TABLE_TYPE_E
{
    BS_TBL_TYPE_CUSTOMER            = 0,        /* �ͻ��� */
    BS_TBL_TYPE_AGENT               = 1,        /* ��ϯ�� */
    BS_TBL_TYPE_BILLING_PACKAGE     = 2,        /* �ʷѱ� */
    BS_TBL_TYPE_SETTLE              = 3,        /* ����� */

    BS_TBL_TYPE__BUTT               = 255
};


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif



