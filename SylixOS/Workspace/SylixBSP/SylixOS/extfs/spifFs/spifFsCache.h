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
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ�����
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSCACHE_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSCACHE_H_



INT32 spiffsCacheFflush(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler);
PSPIFFS_CACHE_PAGE spiffsCachePageGetByFd(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd);
VOID spiffsCacheFdRelease(PSPIFFS_VOLUME pfs, PSPIFFS_CACHE_PAGE pCachePage);
#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFSCACHE_H_ */
