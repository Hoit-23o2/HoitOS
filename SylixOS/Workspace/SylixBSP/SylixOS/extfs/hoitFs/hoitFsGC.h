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

#define GC_DEBUG

//TODO:ע�� ��ɾ��RawInfo��ʱ��һ��Ҫ�ǵõ��������ڵ�GC_Sector����������

VOID    hoitFsGCBackgroundThread(PHOIT_VOLUME pfs);
VOID    hoitFsGCForgroudForce(PHOIT_VOLUME pfs);
VOID    hoitFsGCThread(PHOIT_VOLUME pfs);

#endif /* SYLIXOS_EXTFS_HOITFS_HOITGC_H_ */
