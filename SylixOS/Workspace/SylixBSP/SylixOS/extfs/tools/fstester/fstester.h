/*********************************************************************************************************
**
**                                    �й�������Դ��֯
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
#include "config.h"
#include "../list/list_interface.h"

#define MAX_FUNC_NAME_LEN  128

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


typedef INT (*fstester_functionality)(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes);
typedef fstester_functionality FSTESTER_FUNCTIONALITY;

typedef struct fstester_func_node
{
    PCHAR                  *ppcOpts;
    INT                    iOptCnt;
    PCHAR                  pUsage;
    TEST_TYPE              testType;
    FSTESTER_FUNCTIONALITY functionality;
} FSTESTER_FUNC_NODE;
typedef FSTESTER_FUNC_NODE * PFSTESTER_FUNC_NODE;

static inline PFSTESTER_FUNC_NODE newFstesterCmdNode(PCHAR ppcOpts[], INT iOptCnt, PCHAR pUsage, TEST_TYPE testType,  
                                                     FSTESTER_FUNCTIONALITY functionality){
    PFSTESTER_FUNC_NODE pFuncNode = (PFSTESTER_FUNC_NODE)lib_malloc(sizeof(FSTESTER_FUNC_NODE));
    PCHAR               *ppcOptsCpys = (PCHAR *)lib_malloc(sizeof(PCHAR));
    INT                 i;
    PCHAR               pOpt;
    PCHAR               pOptCpy;
    for (i = 0; i < iOptCnt; i++)
    {
        pOpt = ppcOpts[i];
        pOptCpy = (PCHAR)lib_malloc(lib_strlen(pOpt));
        lib_strcpy(pOptCpy, pOpt);
        ppcOptsCpys[i] = pOptCpy;
    }
    pFuncNode->ppcOpts       = ppcOptsCpys;
    pFuncNode->iOptCnt       = iOptCnt;
    pFuncNode->pUsage        = pUsage;
    pFuncNode->testType      = testType;
    pFuncNode->functionality = functionality;
    return pFuncNode;
}

DECLARE_LIST_TEMPLATE(FSTESTER_FUNC_NODE);
/*********************************************************************************************************
  ���Ժ���API
*********************************************************************************************************/
INT __fstesterRandomRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes);
INT __fstesterSequentialRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes);
INT __fstesterRandomWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes);
INT __fstesterSequentialWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes);
INT __fstesterSmallWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes);
/*********************************************************************************************************
  ���ߺ���
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: translateFSType
** ��������: ����FSType
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
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
static inline const PCHAR translateTestType(TEST_TYPE testType){
    switch (testType)
    {
    case TEST_TYPE_RDM_WR:
        return "random-write-test";
    case TEST_TYPE_RDM_RD:
        return "random-read-test";
    case TEST_TYPE_SEQ_WR:
        return "sequence-write-test";
    case TEST_TYPE_SEQ_RD:
        return "sequence-read-test";
    case TEST_TYPE_CLEAN_MNT:
        return "clean-mount-test";
    case TEST_TYPE_DIRTY_MNT:
        return "dirty-mount-test";
    case TEST_TYPE_SMALL_WR:
        return "small-write-test";
    default:
        return "unsupported";
    }
}
/*********************************************************************************************************
** ��������: translateFSType
** ��������: ����FSType
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static inline FS_TYPE getFSTypeByStr(PCHAR fsTypeStr){
    if(lib_strcmp(fsTypeStr, "spiffs") == 0){
        return FS_TYPE_SPIFFS;
    }
    else if(lib_strcmp(fsTypeStr, "hoitfs") == 0){
        return FS_TYPE_HOITFS;
    }
    else {                                                  /* �����Ȳ���ѡ�� */
        return FS_TYPE_HOITFS;
    }
}
/*********************************************************************************************************
** ��������: getFSMountPoint
** ��������: ���ɹ��ص�
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
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
/*********************************************************************************************************
** ��������: getFSTestOutputDir
** ��������: ���ɲ�������ļ���·��
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
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
/*********************************************************************************************************
** ��������: getFSTestOutputPath
** ��������: ���ɲ�������ļ�·��
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
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
/*********************************************************************************************************
** ��������: uiPower
** ��������: ������
** �䡡��  : index      ָ��
**           base       ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static inline UINT uiPower(UINT index, UINT base) {
    UINT result = 1;
    UINT i;
    for(i = 0; i < index; i++) {
        result *= base;
    }
    return result;
}

#define FSTESTER_SEED   2302
VOID register_fstester_cmd();

#endif /* SYLIXOS_EXTFS_TOOLS_FSTESTER_FSTESTER_H_ */