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
** ��   ��   ��: hoitCache.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 04 �� 02 ��
**
** ��        ��: �����
*********************************************************************************************************/
#include "hoitFsCache.h"
#include "driver/mtd/nor/nor.h"
BOOL hoitEnableCache(UINT8 uiCacheBlockSize, UINT8 uiCacheBlockNums){
    
}

BOOL hoitReadFromCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize){
    read_nor(uiOfs, pContent, uiSize);
}

BOOL hoitWriteToCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize){
    
}
