/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: spifFs.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ�ӿڲ�
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
//TODO  �ļ���ṹ����ļ��ڵ�ṹ�������������
//!     ����������ṹ����и�����ע���spifFs.c�����޸�
//!     Ĭ�Ͽ���ͨ���ļ��ڵ��ȡ�ļ�����Ϣ������ֻ�����__SPIFFS_VOL_LOCK(pspiffs)���ļ�ϵͳ����
typedef struct {
    LW_DEV_HDR          SPIFFS_devhdrHdr;                                /*  ramfs �ļ�ϵͳ�豸ͷ        */
    LW_OBJECT_HANDLE    SPIFFS_hVolLock;                                 /*  �������                    */
    LW_LIST_LINE_HEADER SPIFFS_plineFdNodeHeader;                        /*  fd_node ����                */
    
    BOOL                SPIFFS_bForceDelete;                             /*  �Ƿ�����ǿ��ж�ؾ�          */
    BOOL                SPIFFS_bValid;
    
    uid_t               SPIFFS_uid;                                      /*  �û� id                     */
    gid_t               SPIFFS_gid;                                      /*  ��   id                     */
    mode_t              SPIFFS_mode;                                     /*  �ļ� mode                   */
    time_t              SPIFFS_time;                                     /*  ����ʱ��                    */
    ULONG               SPIFFS_ulCurBlk;                                 /*  ��ǰ�����ڴ��С            */
    ULONG               SPIFFS_ulMaxBlk;                                 /*  ����ڴ�������              */
} SPIF_VOLUME;
typedef SPIF_VOLUME     *PSPIF_VOLUME;

typedef struct ramfs_node {
    PSPIF_VOLUME         SPIFN_pramfs;                                   /*  �ļ�ϵͳ                    */
    
    BOOL                SPIFN_bChanged;                                  /*  �ļ������Ƿ����            */
    mode_t              SPIFN_mode;                                      /*  �ļ� mode                   */
    time_t              SPIFN_timeCreate;                                /*  ����ʱ��                    */
    time_t              SPIFN_timeAccess;                                /*  ������ʱ��                */
    time_t              SPIFN_timeChange;                                /*  ����޸�ʱ��                */
    
    size_t              SPIFN_stSize;                                    /*  ��ǰ�ļ���С (���ܴ��ڻ���) */
    size_t              SPIFN_stVSize;                                   /*  lseek ���������С          */
    
    uid_t               SPIFN_uid;                                       /*  �û� id                     */
    gid_t               SPIFN_gid;                                       /*  ��   id                     */
    PCHAR               SPIFN_pcName;                                    /*  �ļ�����                    */
    PCHAR               SPIFN_pcLink;                                    /*  ����Ŀ��                    */
    
    PLW_LIST_LINE       SPIFN_plineBStart;                               /*  �ļ�ͷ                      */
    PLW_LIST_LINE       SPIFN_plineBEnd;                                 /*  �ļ�β                      */
    ULONG               SPIFN_ulCnt;                                     /*  �ļ����ݿ�����              */
} SPIFN_NODE;
typedef SPIFN_NODE       *PSPIFN_NODE;

#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_ */
