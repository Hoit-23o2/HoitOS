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
** ��   ��   ��: hoitCache.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 04 �� 02 ��
**
** ��        ��: �����
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_
#define SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_

#include "hoitType.h"
#include "SylixOS.h"
/*********************************************************************************************************
 * �ṹ��
*********************************************************************************************************/

typedef struct HOIT_CACHE_HDR
{
    UINT8               HOITCACHE_blockSize;    /* ����cache��С */
    UINT8               HOITCACHE_blockNums;    /* cache������� */
    LW_OBJECT_HANDLE    HOITCACHE_hVolLock;     /* cache������ */
}HOIT_CACHE_HDR;
typedef HOIT_CACHE_HDR * PHOIT_CACHE_HDR;

typedef struct HOIT_CACHE_BLK
{
    BOOL        HOITBLK_bValid;
    PCHAR       HOITBLK_buf;
}HOIT_CACHE_BLK;
typedef HOIT_CACHE_BLK * PHOIT_CACHE_BLK;

/*********************************************************************************************************
 * ����
*********************************************************************************************************/
BOOL     hoitEnableCache(UINT8 uiCacheBlockSize, UINT8 uiCacheBlockNums);
BOOL     hoitReadFromCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL     hoitWriteToCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL     hoitFlushCache();

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_ */
