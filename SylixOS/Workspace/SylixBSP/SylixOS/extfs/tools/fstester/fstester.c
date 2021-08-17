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
#include "fstester.h"
#include "driver/mtd/nor/nor.h"
#include "lorem.h"

#define NAMESPACE fstester

USE_LIST_TEMPLATE(NAMESPACE, FSTESTER_FUNC_NODE);
List(FSTESTER_FUNC_NODE) _G_FuncNodeList;

#define GET_ARG(ppcArgV, i)     *(ppcArgV + i);  
#define IS_STR_SAME(str1, str2) (lib_strcmp(str1, str2) == 0)
#define MOUNT_FILE_DIRTY_RATE       0.3
#define CALC_TIME_DIFF(timeStart, timeEnd) (1000 * ((LONG)timeEnd.tv_sec - (LONG)timeStart.tv_sec) +      \
                                           ((timeEnd.tv_usec - timeStart.tv_usec) / 1000.0))              \

/*********************************************************************************************************
** ��������: __fstester_write_test_file
** ��������: дһ���̶���С���ļ�
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT __fstester_write_test_file(INT iFdTest, ULONG testFileSize) {
    PCHAR       pWriteBuf = (PCHAR)__SHEAP_ALLOC(testFileSize);
    ULONG       dataSize;
    UINT        writeCount;

    INT         i;
    PCHAR       pTemp = pWriteBuf;
    if (pWriteBuf == LW_NULL) {
        printf("%s lower memory, test fail!\n", __func__);
        return PX_ERROR;
    }
    dataSize    = lib_strlen(_G_pLoremFull);    /* �ļ���С */
    writeCount  = testFileSize / dataSize ;
    for (i = 0 ; i < writeCount ; i++){
        lib_memcpy(pTemp, _G_pLoremFull, dataSize);
        pTemp += dataSize;
    }
    lib_memcpy(pTemp, _G_pLoremFull, (testFileSize % dataSize));
    if (write(iFdTest, pWriteBuf, testFileSize) != testFileSize)
        return PX_ERROR;
    lseek(iFdTest, 0, SEEK_SET);                                        /* ��ͷ��ʼ */
    lib_free(pWriteBuf);
    return  ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __fstester_prepare_test
** ��������: ������ǰ��׼��������  +  ��һ����ʼ�ļ�����СΪ�ļ�ϵͳ��testFileSizeRate
** �䡡��  : 
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstester_prepare_test(PCHAR pTestPath, double testFileSizeRate, 
                            PCHAR pMountPoint, PCHAR pFSType, BOOL bIsReset){
    struct statfs   pstatfs;
    LONG            testFileSize;
    INT             iFdTest = -1;

    API_Mount("1", pMountPoint, pFSType);
    /* pTempPath = /mnt/fstype(hoitfs)/write-for-test */
    // sleep(1);

    //!ZN д������ļ���������
    if(bIsReset){
        statfs(pMountPoint, &pstatfs);   /* ��ȡ�ļ�ϵͳ�ռ� */
        testFileSize = (LONG)(testFileSizeRate * (double)(pstatfs.f_blocks * pstatfs.f_bsize));
        if(access(pTestPath, F_OK) == ERROR_NONE){
            remove(pTestPath);
        }
        iFdTest = open(pTestPath, O_CREAT | O_TRUNC | O_RDWR, DEFAULT_FILE_PERM);
        if(iFdTest < 0){
            printf("[%s] can't create output file [%s]", __func__, pTestPath);
            return PX_ERROR;
        }
        if(__fstester_write_test_file(iFdTest, testFileSize) != ERROR_NONE){
            return PX_ERROR;
        }
        return iFdTest;
    }
    else {
        return ERROR_NONE;
    }
}
/*********************************************************************************************************
** ��������: __fstester_terminate_test
** ��������: �ر�iFdTest��Ȼ��ɾ��iFdTest�����umount�ļ�ϵͳ������norflash
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : 
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstester_terminate_test(INT iFdTest, PCHAR pTestPath, PCHAR pMountPoint) {
    close(iFdTest);
    remove(pTestPath);
    API_Unmount(pMountPoint);
    nor_reset(NOR_FLASH_BASE);
    return ERROR_NONE;
}
/*********************************************************************************************************
** ��������: fstester_generic_test
** ��������: nor flash�ļ�ϵͳͨ�ò���
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID fstester_generic_test(FS_TYPE fsType, TEST_TYPE testType, UINT uiLoopTimes, double testFileSizeRate, 
                           FSTESTER_FUNCTIONALITY functionality, PVOID pUserValue){         
    
    PCHAR           pMountPoint;
    PCHAR           pFSType;
    PCHAR           pOutputDir;
    PCHAR           pOutputPath;
    INT             i, iIOBytes;
    INT             iFdOut, iFdTest;
	struct timeval  timeStart;
    struct timeval  timeEnd;
    struct timeval  timeDiff;
    double          dTimeDiff, dResult;
    PCHAR           pOutContent;
    INT             iByteWriteOnce  = 0;
    struct stat     stat;

    PCHAR           pTestPath;

    pMountPoint     = getFSMountPoint(fsType);
    pOutputDir      = getFSTestOutputDir(fsType, testType);
    pOutputPath     = getFSTestOutputPath(fsType, testType);
    pFSType         = translateFSType(fsType);
    /* �趨�ض����� */
    lib_srand(FSTESTER_SEED);
    if(access(pOutputDir, F_OK) != ERROR_NONE){         /* û�ҵ�OutĿ¼ */
        mkdir(pOutputDir, 0);                           /* ��һ��Ŀ¼ */
    }
    if(access(pOutputPath, F_OK) == ERROR_NONE){        /* �ҵ���Out�ļ� */
        remove(pOutputPath);                            /* ɾ�����ļ� */
    }
    iFdOut          = open(pOutputPath, O_CREAT | O_TRUNC | O_RDWR, DEFAULT_FILE_PERM);
    if(iFdOut < 0){
        printf("[%s] can't create output file [%s]\n", __func__, pOutputPath);
        return;
    }

    if(testType == TEST_TYPE_GC){           /* ���GC�Ĳ��� */
        if(testFileSizeRate < 0.1){
            testFileSizeRate = 0.1;
        }
        else if(testFileSizeRate > 0.9){
            testFileSizeRate = 0.9;
        }
    }

    asprintf(&pTestPath, "%s/write-for-test", pMountPoint);
    iFdTest = __fstester_prepare_test(pTestPath, testFileSizeRate, pMountPoint, pFSType, TRUE);
    if(iFdTest != PX_ERROR){
        fstat(iFdTest, &stat);
    }

    if(testType == TEST_TYPE_MOUNT){        
        //! ZN 
        // ���д��һ�����������ݣ������ɺ궨��MOUNT_FILE_DIRTY_RATEȷ��
        close(iFdTest);
    }

    else if(testType == TEST_TYPE_GC){
        close(iFdTest);
        remove(pTestPath);
        lib_free(pTestPath);          

        asprintf(&pTestPath, "%s/write-for-gc", pMountPoint);
        iFdTest    = open(pTestPath, O_CREAT | O_TRUNC | O_RDWR, DEFAULT_FILE_PERM);
        if(iFdTest < 0){
            printf("[%s]: can not create gc file %s\n",__func__, pTestPath);
            return;
        } 
    }

    for (i = 0; i < uiLoopTimes; i++)
    {
        printf("====== TEST %d ======\n", i);
        if(i == 2){
            printf("debug\n");
        }
        if(testType == TEST_TYPE_MOUNT) {                                                   /* ����ʱ�� */
            API_Unmount(pMountPoint);
            lib_gettimeofday(&timeStart, LW_NULL);
            functionality(-1, 0, uiLoopTimes, pMountPoint, pUserValue);
            lib_gettimeofday(&timeEnd, LW_NULL);

            dTimeDiff       = CALC_TIME_DIFF(timeStart, timeEnd);                                                       /* mount ��sleep ��1s */
            if(dTimeDiff < 0){
                dTimeDiff = -dTimeDiff;
            }
            if(dTimeDiff == 0){
                dTimeDiff = 1;                                                              /* ����̫�ͣ�����1ms */                                        
            }
            dResult      = dTimeDiff;
        }
        else if(testType == TEST_TYPE_MERGEABLE_TREE){                                      /* �����ڴ� */
            dResult      = functionality(iFdTest, stat.st_size, uiLoopTimes, 
                                         pMountPoint, pUserValue);
        }
        else if(testType != TEST_TYPE_MOUNT){                                               /* �������� */
            lib_gettimeofday(&timeStart, LW_NULL);
            {
                iIOBytes = functionality(iFdTest, stat.st_size, uiLoopTimes, pMountPoint, pUserValue);
                if(iIOBytes < 0){
                    printf("%10s: [TEST %d Fail]\n", translateTestType(testType), i);
                    break;
                }
            }
            lib_gettimeofday(&timeEnd, LW_NULL);
            dTimeDiff       = CALC_TIME_DIFF(timeStart, timeEnd);
            if(dTimeDiff < 0){
                dTimeDiff = -dTimeDiff;
            }
            if(dTimeDiff == 0){
                dTimeDiff = 1;                                                              /* ����̫�ͣ�����1ms */                                        
            }
            dResult     = iIOBytes / dTimeDiff;                                             /* KB / s */
        }
        iByteWriteOnce  = asprintf(&pOutContent, "%.2f\n", dResult);                        /* ����2λС�� */
        write(iFdOut, pOutContent, iByteWriteOnce);
        lib_free(pOutContent);
    }

    if(testType == TEST_TYPE_MOUNT){        /* mount��󣬳�ʼ���ļ�û�д򿪣�����close */
        remove(pTestPath);
        API_Unmount(pMountPoint);
        nor_reset(NOR_FLASH_BASE);
    }
    else {
        __fstester_terminate_test(iFdTest, pTestPath, pMountPoint);
    }
    lib_free(pTestPath);

    close(iFdOut);
    lib_free(pOutputPath);
    return;
}
#ifdef CONFIG_FSTESTER_SCRIPT
/*********************************************************************************************************
** ��������: fstester_generate_script
** ��������: spiffs close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID fstester_generate_script(FS_TYPE fsType, UINT uiCountDowns){
    PCHAR       pContent;
    INT         i;
    INT         iByteWriteOnce  = 0;
    INT         iByteWriteTotal = 0;
    INT         iFd;
    CPCHAR      cpTips = "# This script is generated by hoit group\n# --------------------------------------\n";
    INT         iTipsLen = lib_strlen(cpTips);
    PCHAR       pcOutShfilePath     = "/root/read_test.sh";
    PCHAR       pcOutTestPathTemp   = "/root/out_temp";
    PCHAR       pcOutTestPath       = "/root/out";

    if(access(pcOutTestPath, F_OK) == ERROR_NONE){
        remove(pcOutTestPath);
    }

    iFd = open(pcOutShfilePath, O_CREAT | O_RDWR | O_TRUNC);
    if(iFd < 0){
        printf("[%s] can not create file %s\n", __func__, pcOutShfilePath);
        return;
    }
    /* д��ű�ͷ�� */
    write(iFd, cpTips, iTipsLen);
    iByteWriteTotal += iTipsLen;
    /* д����ʱ�ű� */
    for (i = 0; i < uiCountDowns; i++)
    {
        iByteWriteOnce = asprintf(&pContent, "\
echo '====== TEST %d ======'\n\
mount -t hoitfs 1 /mnt/hoitfs\n\
sleep 1\n\
hoit -t ftt 4 3 2 >>%s\n\
sleep 1\n\
umount /mnt/hoitfs\n\
sleep 1\n\
fls  -reset\n\
        ", i, pcOutTestPathTemp); 
        iByteWriteTotal += iByteWriteOnce;
        lseek(iFd, iByteWriteTotal, SEEK_SET);
        write(iFd, pContent, iByteWriteOnce);
        lib_free(pContent);
    }
    close(iFd);
    
    API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
    printf("Done. now run 'shfile ./read_test.sh'\n");
    API_TShellColorEnd(STD_OUT);
}
/*********************************************************************************************************
** ��������: fstester_parse_out
** ��������: spiffs close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID fstester_parse_out(){
    INT         i;
    INT         iByteWriteOnce  = 0;
    INT         iByteWriteTotal = 0;
    INT         iFd, iFd2;
    PCHAR       pcOutTestPathTemp   = "/root/out_temp";
    PCHAR       pcOutTestPath       = "/root/out";

    PCHAR       pOutContentTemp;
    PCHAR       pOutContent;
    PCHAR       pStart;
    PCHAR       pEnd;
    struct stat stat;

    iFd = open(pcOutTestPathTemp, O_RDONLY);
    iFd2 = open(pcOutTestPath, O_CREAT | O_RDWR | O_TRUNC);

    if(iFd < 0 || iFd2 < 0){
        printf("[%s] %s or %s is not exists\n", __func__, pcOutTestPathTemp, pcOutTestPath);
    }
    fstat(iFd, &stat);
    pOutContentTemp = API_VmmMmap(NULL, stat.st_size, LW_VMM_SHARED_CHANGE, LW_VMM_FLAG_READ, iFd, 0);
    iByteWriteTotal = 0;
    iByteWriteOnce  = 0;
    for (i = 0; i < stat.st_size - 3; i++)
    {
        if(*pOutContentTemp == 'c' 
        && *(pOutContentTemp + 1) == 'm'
        && *(pOutContentTemp + 2) == 'd'){              /* ʱ����Ϣ */
            pStart = pOutContentTemp;
            while (*pStart < '0' || *pStart > '9')      /* ֻ�ҵ����� */
            {
                pStart++;
            }
            
            pEnd = pStart;
            while (*pEnd >= '0' && *pEnd <= '9')
            {
                pEnd++;
            }
            iByteWriteOnce = pEnd - pStart + 1;
            pOutContent = (PCHAR)lib_malloc(iByteWriteOnce);  
            lib_memcpy(pOutContent, pStart, iByteWriteOnce - 1);
            *(pOutContent + iByteWriteOnce - 1) = '\n';
            iByteWriteTotal += iByteWriteOnce;
            lseek(iFd2, iByteWriteTotal, SEEK_SET);
            write(iFd2, pOutContent, iByteWriteOnce);
            lib_free(pOutContent);
        }   
        pOutContentTemp++;
    }
    API_VmmMunmap(pOutContentTemp, stat.st_size);

    close(iFd);
    close(iFd2);
}
#endif

INT fstester_register_functionality(PCHAR ppcOpts[], INT iOptCnt, PCHAR pUsage, TEST_TYPE testType, 
                                    FSTESTER_FUNCTIONALITY functionality){
    PFSTESTER_FUNC_NODE          pFuncNode;
    pFuncNode = newFstesterCmdNode(ppcOpts, iOptCnt, pUsage, testType, functionality);
    _G_FuncNodeList->append(_G_FuncNodeList, pFuncNode);
    return ERROR_NONE;
}

INT fstester_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    Iterator(FSTESTER_FUNC_NODE) iter;
    INT                          iArgPos = 1;
    PCHAR                        pArg;
    FS_TYPE                      fsTarget           = FS_TYPE_SPIFFS;
    FSTESTER_FUNCTIONALITY       functionality      = __fstesterRandomRead;
    TEST_TYPE                    testType           = TEST_TYPE_RDM_RD;
    PFSTESTER_FUNC_NODE          pFuncNode;
    INT                          i;
    UINT                         uiTestCount        = 10;
    PCHAR                        pUserValue         = LW_NULL;
    double                       dTestFileSizeRate  = 0.5;

    InitIterator(iter, NAMESPACE, FSTESTER_FUNC_NODE);
    while (iArgPos <= iArgC)
    {
        pArg = GET_ARG(ppcArgV, iArgPos++);
        if(IS_STR_SAME(pArg, "-t")){           /* �����ļ�ϵͳ���� */
            pArg = GET_ARG(ppcArgV, iArgPos++);
            fsTarget = getFSTypeByStr(pArg);
        }
        else if(IS_STR_SAME(pArg, "-h")){
            printf("================================================\n");
            printf("=     FSTESTER implemented By HoitFS Group     =\n");
            printf("================================================\n");
            printf("[Basic Usage]:        fstester -t [FSType] -l [LoopTimes] [TestType] -s [initial file size]\n");
            printf("[Supported FSType]:   spiffs hoitfs\n");
            printf("[Supported TestType]: \n");
            for (iter->begin(iter, _G_FuncNodeList);iter->isValid(iter);iter->next(iter))
            {
                pFuncNode = iter->get(iter);
                printf("%s:\n\t%s\n\n", pFuncNode->ppcOpts[0], pFuncNode->pUsage);
            }
            return;
        }
        else if(IS_STR_SAME(pArg, "-l")){      /* ���ò��Դ��� */
            pArg = GET_ARG(ppcArgV, iArgPos++);
            uiTestCount = lib_atoi(pArg);
        }
        else if(IS_STR_SAME(pArg, "-args")){
            pArg = GET_ARG(ppcArgV, iArgPos++);
            asprintf(&pUserValue, "%s", pArg);
        }
        else if(IS_STR_SAME(pArg, "-s")){       /* ���ò����ļ���С */
            pArg = GET_ARG(ppcArgV, iArgPos++);
            dTestFileSizeRate = lib_atof(pArg);
            if (dTestFileSizeRate <= 0) {
                dTestFileSizeRate = 0;
            }
            if (dTestFileSizeRate >= 1.0) {
                dTestFileSizeRate = 1.0;
            }
        }        
        else {                      /* �������� */
            for (iter->begin(iter, _G_FuncNodeList);iter->isValid(iter);iter->next(iter))
            {
                pFuncNode = iter->get(iter);
                for (i = 0; i < pFuncNode->iOptCnt; i++)
                {
                    if(IS_STR_SAME(pFuncNode->ppcOpts[i], pArg)){
                        functionality = pFuncNode->functionality;
                        testType      = pFuncNode->testType;
                        break;
                    }
                }
            }
        }
    }
    printf("Test With Options Below:\n");
    printf("\tfilesystem_type=%s\n", translateFSType(fsTarget));
    printf("\ttest_count=%d\n", uiTestCount);
    printf("\ttest_file_factor=%f\n", dTestFileSizeRate);
    printf("\ttest_type=%s\n", translateTestType(testType));
    printf("Press Any Key to Continue\n");
    getchar();
    
    fstester_generic_test(fsTarget, testType, uiTestCount, dTestFileSizeRate, functionality, pUserValue);
    FreeIterator(iter);
}

VOID register_fstester_cmd(){
    
    InitList(_G_FuncNodeList, NAMESPACE, FSTESTER_FUNC_NODE);
    
    PCHAR cOpt1[2] = {"-rndrd", "-rrd"};
    fstester_register_functionality(cOpt1,  2, "Random Read Test", TEST_TYPE_RDM_RD, __fstesterRandomRead);

    PCHAR cOpt2[2] = {"-seqrd", "-srd"};
    fstester_register_functionality(cOpt2,  2, "Sequence Read Test", TEST_TYPE_SEQ_RD, __fstesterSequentialRead);

    PCHAR cOpt3[2] = {"-rndwr", "-rwr"};
    fstester_register_functionality(cOpt3,  2, "Random Write Test", TEST_TYPE_RDM_WR, __fstesterRandomWrite);
    
    PCHAR cOpt4[2] = {"-seqwr", "-swr"};
    fstester_register_functionality(cOpt4,  2, "Sequence Write Test", TEST_TYPE_SEQ_WR, __fstesterSequentialWrite);

    PCHAR cOpt5[1] = {"-smlwr"};
    fstester_register_functionality(cOpt5,  1, "Small Write Test", TEST_TYPE_SMALL_WR, __fstesterSmallWrite);

    PCHAR cOpt6[2] = {"-mnt", "-mount"};
    fstester_register_functionality(cOpt6,  2, "Mount Test", TEST_TYPE_MOUNT, __fstesterMount);

    PCHAR cOpt7[1] = {"-gc"};
    fstester_register_functionality(cOpt7,  1, "Garbage Collection Test", TEST_TYPE_GC, __fstesterGC);
    
    PCHAR cOpt8[2] = {"-mtree", "-mergeabletree"};
    fstester_register_functionality(cOpt8,  2, "Mergable Tree Test", TEST_TYPE_MERGEABLE_TREE, __fstesterMergeableTree);

    API_TShellKeywordAdd("fstester", fstester_cmd_wrapper);
}
