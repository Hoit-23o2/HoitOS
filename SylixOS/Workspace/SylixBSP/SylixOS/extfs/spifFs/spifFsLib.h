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

/*********************************************************************************************************
 * SPIFFS �¼���ض���
*********************************************************************************************************/
// updating an object index contents
#define SPIFFS_EV_IX_UPD                (0)
// creating a new object index
#define SPIFFS_EV_IX_NEW                (1)
// deleting an object index
#define SPIFFS_EV_IX_DEL                (2)
// moving an object index without updating contents
#define SPIFFS_EV_IX_MOV                (3)
// updating an object index header data only, not the table itself
#define SPIFFS_EV_IX_UPD_HDR            (4)


typedef INT32 (*spiffsVisitorFunc)(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                   const PVOID pUserConst, PVOID pUserVar);

/*********************************************************************************************************
 * SPIFFS �⺯������
*********************************************************************************************************/

INT32 spiffsPhysCpy(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, UINT32 uiDst,
                    UINT32 uiSrc, UINT32 uiLen);
/*********************************************************************************************************
 * SPIFFS Lookup Vistor���
*********************************************************************************************************/
INT32 spiffsObjLookUpFindEntryVisitor(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                                      UINT8 flags, SPIFFS_OBJ_ID objId, spiffsVisitorFunc vistor,
                                      const PVOID pUserConst, PVOID pUserVar, SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry);
INT32 spiffsObjLookUpScan(PSPIFFS_VOLUME pfs);
INT32 spiffsObjLookUpFindFreeObjId(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID *pObjId, const PUCHAR pucConflictingName);
INT32 spiffsObjLookUpFindId(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                            SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry);
INT32 spiffsObjLookUpFindFreeEntry(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                                   SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry);
INT32 spiffsObjLookUpFindIdAndSpan(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_SPAN_IX spanIX,
                                   SPIFFS_PAGE_IX pageIXExclusion, SPIFFS_PAGE_IX *pPageIX);
INT32 spiffsObjectFindObjectIndexHeaderByName(PSPIFFS_VOLUME pfs, UCHAR ucName[SPIFFS_OBJ_NAME_LEN], SPIFFS_PAGE_IX *pPageIX);

/*********************************************************************************************************
 * SPIFFS ҳ�湤��
*********************************************************************************************************/
INT32 spiffsPageIndexCheck(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd, SPIFFS_PAGE_IX pageIX, SPIFFS_SPAN_IX spanIX);
INT32 spiffsPageDataCheck(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd,  SPIFFS_PAGE_IX pageIX, SPIFFS_SPAN_IX spanIX);
INT32 spiffsPageDelete(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX);
INT32 spiffsPageAllocateData(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, PSPIFFS_PAGE_HEADER pPageHeader,
                             PUCHAR pData, UINT32 uiLen, UINT32 uiPageOffs, BOOL bIsFinalize,
                             SPIFFS_PAGE_IX *pageIX);
/*********************************************************************************************************
 * SPIFFS Object ���
*********************************************************************************************************/
INT32 spiffsObjectUpdateIndexHdr(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd, SPIFFS_OBJ_ID objId, SPIFFS_PAGE_IX pageIXObjIXHdr,
                                 PUCHAR pucNewObjIXHdrData , const UCHAR ucName[], UINT32 uiSize, SPIFFS_PAGE_IX *pageIXNew);
INT32 spiffsObjectOpenByPage(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX, PSPIFFS_FD pFd, SPIFFS_FLAGS flags, SPIFFS_MODE mode);

INT32 spiffsObjectCreate(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId,
                         const UCHAR cPath[], SPIFFS_OBJ_TYPE type, SPIFFS_PAGE_IX* pObjIndexHdrPageIX);
INT32 spiffsObjectTruncate(PSPIFFS_FD pFd, UINT32 uiNewSize, BOOL bIsRemoveFull); 
//DONE��--spiffsObjectAppend(���)--��--spiffsObjectModify(���)--��--spiffsObjectRead(���)--�� 
//DONE: --spiffsFileWrite(���)--�� --spiffsFileRead(���)--
INT32 spiffsObjectAppend(PSPIFFS_FD pFd, UINT32 uiOffset, PUCHAR pContent, UINT32 uiLen);
INT32 spiffsObjectModify(PSPIFFS_FD pFd, UINT32 uiOffset, PUCHAR pContent, UINT32 uiLen);
INT32 spiffsObjectRead(PSPIFFS_FD pFd, UINT32 uiOffset, UINT32 uiLen, PUCHAR puDst);
/*********************************************************************************************************
 * SPIFFS ��������
*********************************************************************************************************/
INT32 spiffsStatPageIX(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX, SPIFFS_FILE fileHandler, PSPIFFS_STAT pStat);
/*********************************************************************************************************
 * SPIFFS ��д���
*********************************************************************************************************/
INT32 spiffsFileWrite(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, PVOID pContent, UINT32 uiOffset, INT32 iLen);
INT32 spiffsFileRead(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, PVOID pContent, INT32 iLen);
#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_ */