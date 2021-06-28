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
** ��   ��   ��: spifFsCache.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 09��
**
** ��        ��: Spiffs�ļ�ϵͳ�����
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSCACHE_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSCACHE_H_
#include "spifFsType.h"

/*********************************************************************************************************
 * SPIFFS Cache Flag
*********************************************************************************************************/
#define SPIFFS_CACHE_FLAG_DIRTY       (1<<0)
#define SPIFFS_CACHE_FLAG_WRTHRU      (1<<1)
#define SPIFFS_CACHE_FLAG_OBJLU       (1<<2)
#define SPIFFS_CACHE_FLAG_OBJIX       (1<<3)
#define SPIFFS_CACHE_FLAG_DATA        (1<<4)
#define SPIFFS_CACHE_FLAG_TYPE_WR     (1<<7)

/*********************************************************************************************************
 * SPIFFS Cache��غ궨��
*********************************************************************************************************/
/**
 * @brief ��ȡCachePage��С = CachePageͷ��Ϣ��С + �߼�ҳ���С
 */
#define SPIFFS_CACHE_PAGE_SIZE(pfs)                     (sizeof(SPIFFS_CACHE_PAGE) + SPIFFS_CFG_LOGIC_PAGE_SZ(pfs))
#define SPIFFS_GET_CACHE_PAGE_SIZE(pfs)                 (sizeof(SPIFFS_CACHE_PAGE) + SPIFFS_CFG_LOGIC_PAGE_SZ(pfs))

#define SPIFFS_GET_CACHE_HDR(pfs)                       (PSPIFFS_CACHE)pfs->pCache
#define SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, ix)      ((PSPIFFS_CACHE_PAGE)(&((pCache)->Cpages[(ix) * SPIFFS_GET_CACHE_PAGE_SIZE(pfs)])))
#define SPIFFS_GET_CACHE_PAGE_CONTENT(pfs, pCache, ix)  ((PUCHAR)(&((pCache)->Cpages[(ix) * SPIFFS_CACHE_PAGE_SIZE(pfs)]) \
                                                        + sizeof(SPIFFS_CACHE_PAGE)))
/*********************************************************************************************************
 * SPIFFS Cache��غ���
*********************************************************************************************************/
INT32 spiffsCacheInit(PSPIFFS_VOLUME pfs);
INT32 spiffsCacheRead(PSPIFFS_VOLUME pfs, UINT8 uiOps, SPIFFS_FILE fileHandler, 
                      UINT32 uiAddr, UINT32 uiLen, PUCHAR pDst);
INT32 spiffsCacheWrite(PSPIFFS_VOLUME pfs, UINT8 uiOps, SPIFFS_FILE fileHandler, 
                       UINT32 uiAddr, UINT32 uiLen, PUCHAR pSrc);
INT32 spiffsEraseBlk(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIX);



INT32 spiffsCacheFflush(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler);
PSPIFFS_CACHE_PAGE spiffsCachePageGetByFd(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd);
VOID spiffsCacheFdRelease(PSPIFFS_VOLUME pfs, PSPIFFS_CACHE_PAGE pCachePage);
#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFSCACHE_H_ */