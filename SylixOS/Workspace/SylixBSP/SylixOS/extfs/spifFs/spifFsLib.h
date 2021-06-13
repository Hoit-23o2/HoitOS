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
** ��   ��   ��: spifFsLib.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ����
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_
#include "spifFsType.h"
/*********************************************************************************************************
 * SPIFFS Vistor��ض���
*********************************************************************************************************/
// check id, only visit matching objec ids
#define SPIFFS_VIS_CHECK_ID     (1<<0)
// report argument object id to visitor - else object lookup id is reported
#define SPIFFS_VIS_CHECK_PH     (1<<1)
// stop searching at end of all look up pages
#define SPIFFS_VIS_NO_WRAP      (1<<2)

// visitor result, continue searching
#define SPIFFS_VIS_COUNTINUE            (SPIFFS_ERR_INTERNAL - 20)
// visitor result, continue searching after reloading lu buffer
#define SPIFFS_VIS_COUNTINUE_RELOAD     (SPIFFS_ERR_INTERNAL - 21)
// visitor result, stop searching
#define SPIFFS_VIS_END                  (SPIFFS_ERR_INTERNAL - 22)


typedef INT32 (*spiffsVisitorFunc)(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                   const PVOID pUserConst, PVOID pUserVar);

/*********************************************************************************************************
 * SPIFFS �⺯������
*********************************************************************************************************/
INT32 spiffsObjLookUpScan(PSPIFFS_VOLUME pfs);
INT32 spiffsObjLookUpFindEntryVisitor(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                                      UINT8 flags, SPIFFS_OBJ_ID objId, spiffsVisitorFunc vistor,
                                      const PVOID pUserConst, PVOID pUserVar, SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry);
INT32 spiffsObjLookUpFindFreeObjId(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID *pObjId, const PUCHAR pucConflictingName);

#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_ */
