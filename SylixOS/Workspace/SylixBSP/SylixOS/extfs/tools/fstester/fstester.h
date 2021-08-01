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
** ��   ��   ��: fstester.h
**
** ��   ��   ��: Pan Yanqi
**
** �ļ���������: 2021 �� 07 �� 27 ��
**
** ��        ��: ���ɲ��Խű��벶׽���
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL

#ifndef SYLIXOS_EXTFS_TOOLS_FSTESTER_FSTESTER_H_
#define SYLIXOS_EXTFS_TOOLS_FSTESTER_FSTESTER_H_
#include "SylixOS.h"

typedef enum fs_type {
    FS_TYPE_SPIFFS,
    FS_TYPE_HOITFS,
                                                    /* ��ʱ����ʵ�� */
    FS_TYPE_YAFFS,                                                  
    FS_TYPE_FATFS
} FS_TYPE;

typedef enum test_type{
    TEST_TYPE_RDM_WR,
    TEST_TYPE_RDM_RD,           
    
    TEST_TYPE_SEQ_WR,
    TEST_TYPE_SEQ_RD,           

    TEST_TYPE_CLEAN_MNT,
    TEST_TYPE_DIRTY_MNT,

    TEST_TYPE_SMALL_WR
} TEST_TYPE;

static inline const PCHAR translateFSType(FS_TYPE fsType){
    switch (fsType)
    {
    case FS_TYPE_SPIFFS:
        return "spiffs";
    case FS_TYPE_HOITFS:
        return "hoitfs";
    default:
        return "unsupported";
    }
}

static inline const PCHAR getFSMountPoint(FS_TYPE fsType){
    switch (fsType)
    {
    case FS_TYPE_SPIFFS:
        return "/mnt/spiffs";
    case FS_TYPE_HOITFS:
        return "/mnt/hoitfs";
    default:
        return "unsupported";
    }
}  
static inline const PCHAR getFSTestOutputDir(FS_TYPE fsType, TEST_TYPE testType){
    switch (fsType)
    {
    case FS_TYPE_SPIFFS:
        return "/root/spiffs-test";
    case FS_TYPE_HOITFS:
        return "/root/hoitfs-test";
    default:
        return "unsupported";
    }
}
static inline PCHAR getFSTestOutputPath(FS_TYPE fsType, TEST_TYPE testType){
    PCHAR pOutputPath;
    PCHAR pOutputDir;
    PCHAR pOutputFileName;
    PCHAR pOutputBasedir = "/root";
    switch (fsType)
    {
    case FS_TYPE_SPIFFS:
        pOutputDir = "spiffs-test";
        break;
    case FS_TYPE_HOITFS:
        pOutputDir = "hoitfs-test";
        break;
    default:
        pOutputDir = "unsupported";
        break;
    }

    switch (testType)
    {
    case TEST_TYPE_RDM_WR:
        pOutputFileName = "out-random-write-test";
        break;
    case TEST_TYPE_RDM_RD:
        pOutputFileName = "out-random-read-test";
        break;
    case TEST_TYPE_SEQ_WR:
        pOutputFileName = "out-sequence-write-test";
        break;
    case TEST_TYPE_SEQ_RD:
        pOutputFileName = "out-sequence-read-test";
        break;
    case TEST_TYPE_CLEAN_MNT:
        pOutputFileName = "out-clean-mount-test";
        break;
    case TEST_TYPE_DIRTY_MNT:
        pOutputFileName = "out-dirty-mount-test";
        break;
    case TEST_TYPE_SMALL_WR:
        pOutputFileName = "out-small-write-test";
        break;
    default:
        break;
    }
    asprintf(&pOutputPath, "%s/%s/%s", pOutputBasedir, pOutputDir, pOutputFileName);
    return pOutputPath;
}

VOID register_fstester_cmd();

#endif /* SYLIXOS_EXTFS_TOOLS_FSTESTER_FSTESTER_H_ */
