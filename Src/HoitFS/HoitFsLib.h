/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: HoitFsLib.h
**
** ��   ��   ��: Hoit Group
**
** �ļ���������: 2021 �� 03 �� 20 ��
**
** ��        ��: Hoit�ļ�ϵͳ�ڲ�����.
*********************************************************************************************************/

#ifndef __HOITFSLIB_H
#define __HOITFSLIB_H

#include "SylixOS.h"                                                    /*  ����ϵͳ                    */

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 //&& LW_CFG_RAMFS_EN > 0

/*********************************************************************************************************
  HoitFs�궨��
*********************************************************************************************************/
#define HOITFS_MAGIC_NUM 0x05201314
#define HOITFS_FIELD_TYPE 0xE0000000    //ǰ3λ��ΪTYPE��

#define HOITFS_TYPE_INODE 0x20000000    //raw_inode���� 001
#define HOITFS_TYPE_DIRENT 0x40000000   //raw_dirent����  010

#define __HOITFS_SB_LOCK(pfs)        API_SemaphoreMPend(pfs->HOITFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __HOITFS_SB_UNLOCK(pfs)      API_SemaphoreMPost(pfs->HOITFS_hVolLock)

/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  HoitFs super block����
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          HOITFS_devhdrHdr;                                /*  HoitFs �ļ�ϵͳ�豸ͷ        */
    LW_OBJECT_HANDLE    HOITFS_hVolLock;                                 /*  �������                    */
    LW_LIST_LINE_HEADER HOITFS_plineFdNodeHeader;                        /*  fd_node ����                */
    LW_LIST_LINE_HEADER HOITFS_plineSon;                                 /*  ��������                    */

    BOOL                HOITFS_bForceDelete;                             /*  �Ƿ�����ǿ��ж�ؾ�          */
    BOOL                HOITFS_bValid;

    uid_t               HOITFS_uid;                                      /*  �û� id                     */
    gid_t               HOITFS_gid;                                      /*  ��   id                     */
    mode_t              HOITFS_mode;                                     /*  �ļ� mode                   */
    time_t              HOITFS_time;                                     /*  ����ʱ��                    */
    ULONG               HOITFS_ulCurBlk;                                 /*  ��ǰ�����ڴ��С            */
    ULONG               HOITFS_ulMaxBlk;                                 /*  ����ڴ�������              */
} HOIT_SB;
typedef HOIT_SB* PHOIT_SB;

/*********************************************************************************************************
  HoitFs ����ʵ��Ĺ���Header����
*********************************************************************************************************/
typedef struct {
    UINT32 MAGIC_NUM;
    UINT32 FLAG;
} HOIT_RAW_HEADER;


/*********************************************************************************************************
  HoitFs raw inode����
*********************************************************************************************************/
typedef struct {
    UINT32 MAGIC_NUM;
    UINT32 FLAG;
} HOIT_RAW_INODE;
typedef HOIT_RAW_INODE* PHOIT_RAW_INODE;

/*********************************************************************************************************
  HoitFs raw dirent����
*********************************************************************************************************/
typedef struct {
    UINT32 MAGIC_NUM;
    UINT32 FLAG;
} HOIT_RAW_DIRENT;
typedef HOIT_RAW_DIRENT* PHOIT_RAW_DIRENT;

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
#endif                                                                  /*  __HOITFSLIB_H                */
