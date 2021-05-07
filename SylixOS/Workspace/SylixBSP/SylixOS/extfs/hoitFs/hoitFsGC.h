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
** ��   ��   ��: hoitFsGC.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 04 �� 25 ��
**
** ��        ��: ��������ʵ��
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_HOITFS_HOITGC_H_
#define SYLIXOS_EXTFS_HOITFS_HOITGC_H_

#include "hoitType.h"

#define MAX_MSG_COUNTER     2
#define MAX_MSG_BYTE_SIZE   40

#define MSG_BG_GC_START         "msg_gc_background_start"
#define MSG_BG_GC_END           "msg_gc_background_end"

/*********************************************************************************************************
  GC�����ṹ��
*********************************************************************************************************/
typedef struct hoitGCAttr
{
    PHOIT_VOLUME pfs; 
    UINT uiThreshold;
} HOIT_GC_ATTR;

typedef HOIT_GC_ATTR * PHOIT_GC_ATTR;

typedef enum hoitGCLevel
{
    GC_FOREGROUND,
    GC_BACKGROUND
} ENUM_HOIT_GC_LEVEL;

//TODO:ע�� ��ɾ��RawInfo��ʱ��һ��Ҫ�ǵõ��������ڵ�GC_Sector����������
//!�ò����Ѿ����޸�Ϊ�ϲ㲻��ɾ��RawInfo�������������Ա�ǣ�ɾ�������²����
/*********************************************************************************************************
  GC��غ���
*********************************************************************************************************/
VOID    hoitGCBackgroundThread(PHOIT_VOLUME pfs);
VOID    hoitGCForgroudForce(PHOIT_VOLUME pfs);
VOID    hoitGCThread(PHOIT_GC_ATTR pGCAttr);

/*********************************************************************************************************
  GC��ʼ�����캯��
*********************************************************************************************************/
static inline VOID hoitStartGCThread(PHOIT_VOLUME pfs, UINT uiThreshold){
    LW_CLASS_THREADATTR     gcThreadAttr;
    PHOIT_GC_ATTR           pGCAttr;

    pGCAttr                 = (PHOIT_GC_ATTR)lib_malloc(sizeof(HOIT_GC_ATTR));
    pGCAttr->pfs            = pfs;
    pGCAttr->uiThreshold    = uiThreshold;
    
    API_ThreadAttrBuild(&gcThreadAttr,
                        4 * LW_CFG_KB_SIZE, 
                        LW_PRIO_NORMAL,
                        LW_OPTION_THREAD_STK_CHK, 
                        (VOID *)pGCAttr);

    API_ThreadCreate("t_hoit_gc_thread",
                     (PTHREAD_START_ROUTINE)hoitGCThread,
                     &gcThreadAttr,
                     LW_NULL);
}

#endif /* SYLIXOS_EXTFS_HOITFS_HOITGC_H_ */
