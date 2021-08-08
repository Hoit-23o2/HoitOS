/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: fstester.h
**
** 创   建   人: Pan Yanqi
**
** 文件创建日期: 2021 年 07 月 27 日
**
** 描        述: 生成测试脚本与捕捉输出
*********************************************************************************************************/
#include "fstester.h"
#include "driver/mtd/nor/nor.h"
#include "lorem.h"

#define NAMESPACE fstester

USE_LIST_TEMPLATE(NAMESPACE, FSTESTER_FUNC_NODE);
List(FSTESTER_FUNC_NODE) _G_FuncNodeList;

#define GET_ARG(ppcArgV, i)     *(ppcArgV + i);  
#define IS_STR_SAME(str1, str2) (lib_strcmp(str1, str2) == 0)
/*********************************************************************************************************
** 函数名称: __fstester_write_test_file
** 功能描述: 写一个固定大小的文件
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
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
    dataSize    = lib_strlen(_G_pLoremFull);    /* 文件大小 */
    writeCount  = testFileSize / dataSize ;
    for (i = 0 ; i < writeCount ; i++){
        lib_memcpy(pTemp, _G_pLoremFull, dataSize);
        pTemp += dataSize;
    }
    lib_memcpy(pTemp, _G_pLoremFull, (testFileSize % dataSize));
    if (write(iFdTest, pWriteBuf, testFileSize) != testFileSize)
        return PX_ERROR;
    lseek(iFdTest, 0, SEEK_SET);                                        /* 从头开始 */
    lib_free(pWriteBuf);
    return  ERROR_NONE;
}
/*********************************************************************************************************
** 函数名称: fstester_generic_test
** 功能描述: nor flash文件系统通用测试
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID fstester_generic_test(FS_TYPE fsType, TEST_TYPE testType, UINT uiLoopTimes, double testFileSizeRate, 
                           FSTESTER_FUNCTIONALITY functionality, PVOID pUserValue){         
    
    PCHAR           pMountPoint;
    PCHAR           pFSType;
    PCHAR           pOutputDir;
    PCHAR           pOutputPath;
    INT             i, iRes;
    INT             iFdOut, iFdTest;
	struct timeval  timeStart;
    struct timeval  timeEnd;
    struct timeval  timeDiff;
    double          dTimeDiff;
    PCHAR           pOutContent;
    INT             iByteWriteOnce  = 0;
    struct stat     stat;
    struct statfs   pstatfs;
    LONG            testFileSize;
    
    PCHAR           pTestPath;

    pMountPoint     = getFSMountPoint(fsType);
    pOutputDir      = getFSTestOutputDir(fsType, testType);
    pOutputPath     = getFSTestOutputPath(fsType, testType);
    pFSType         = translateFSType(fsType);
    /* 设定特定种子 */
    lib_srand(FSTESTER_SEED);

    if(access(pOutputDir, F_OK) != ERROR_NONE){         /* 没找到Out目录 */
        mkdir(pOutputDir, 0);                           /* 建一个目录 */
    }
    if(access(pOutputPath, F_OK) == ERROR_NONE){        /* 找到了Out文件 */
        remove(pOutputPath);                            /* 删除该文件 */
    }
    iFdOut          = open(pOutputPath, O_CREAT | O_TRUNC | O_RDWR);
    if(iFdOut < 0){
        printf("[%s] can't create output file [%s]\n", __func__, pOutputPath);
        return;
    }

    if(testType != TEST_TYPE_MOUNT){
        API_Mount("1", pMountPoint, pFSType);
        statfs(pMountPoint, &pstatfs);   /* 获取文件系统空间 */
        testFileSize = (LONG)(testFileSizeRate * (double)(pstatfs.f_blocks * pstatfs.f_bsize));

        /* pTempPath = /mnt/fstype(hoitfs)/write-for-test */
        asprintf(&pTestPath, "%s/write-for-test", pMountPoint);
        sleep(1);
        if(access(pTestPath, F_OK) == ERROR_NONE){
            remove(pTestPath);
        }
        
        //!ZN 写更大的文件，参数化
        iFdTest = open(pTestPath, O_CREAT | O_TRUNC | O_RDWR);
        if(iFdTest < 0){
            printf("[%s] can't create output file [%s]", __func__, pTestPath);
            return;
        }
        if(__fstester_write_test_file(iFdTest, testFileSize) != ERROR_NONE){
            return ;
        }
        fstat(iFdTest, &stat);
    }

    for (i = 0; i < uiLoopTimes; i++)
    {
        printf("====== TEST %d ======\n", i);
        if(i == 94){
            printf("debug\n");
        }
        lib_gettimeofday(&timeStart, LW_NULL);
        {
            iRes = functionality(iFdTest, stat.st_size, uiLoopTimes, pMountPoint, pUserValue);
            if(iRes != ERROR_NONE){
                printf("[TEST %d Fail]\n",i);
                break;
            }
        }
        lib_gettimeofday(&timeEnd, LW_NULL);
        dTimeDiff       = 1000 * ((LONG)timeEnd.tv_sec - (LONG)timeStart.tv_sec) +      /* 计算ms时间差 */
                          ((timeEnd.tv_usec - timeStart.tv_usec) / 1000.0);
        if(dTimeDiff < 0){
            dTimeDiff = -dTimeDiff;
        }
        iByteWriteOnce  = asprintf(&pOutContent, "%.2f\n", dTimeDiff);      /* 保留2位小数 */
        write(iFdOut, pOutContent, iByteWriteOnce);
        lib_free(pOutContent);
    }

    if(testType != TEST_TYPE_MOUNT){
        close(iFdTest);
        remove(pTestPath);
        lib_free(pTestPath);
        API_Unmount(pMountPoint);
        nor_reset(NOR_FLASH_BASE);
    }
    close(iFdOut);
    lib_free(pOutputPath);
    return;
}
#ifdef CONFIG_FSTESTER_SCRIPT
/*********************************************************************************************************
** 函数名称: fstester_generate_script
** 功能描述: spiffs close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
    /* 写入脚本头部 */
    write(iFd, cpTips, iTipsLen);
    iByteWriteTotal += iTipsLen;
    /* 写入临时脚本 */
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
** 函数名称: fstester_parse_out
** 功能描述: spiffs close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
        && *(pOutContentTemp + 2) == 'd'){              /* 时间信息 */
            pStart = pOutContentTemp;
            while (*pStart < '0' || *pStart > '9')      /* 只找到数字 */
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
        if(IS_STR_SAME(pArg, "-t")){           /* 设置文件系统类型 */
            pArg = GET_ARG(ppcArgV, iArgPos++);
            fsTarget = getFSTypeByStr(pArg);
        }
        else if(IS_STR_SAME(pArg, "-h")){
            printf("================================================\n");
            printf("=     FSTESTER implemented By HoitFS Group     =\n");
            printf("================================================\n");
            printf("[Basic Usage]:        fstester -t [FSType] -l [LoopTimes] [TestType]\n");
            printf("[Supported FSType]:   spiffs hoitfs\n");
            printf("[Supported TestType]: \n");
            for (iter->begin(iter, _G_FuncNodeList);iter->isValid(iter);iter->next(iter))
            {
                pFuncNode = iter->get(iter);
                printf("%s:\n\t%s\n\n", pFuncNode->ppcOpts[0], pFuncNode->pUsage);
            }
            return;
        }
        else if(IS_STR_SAME(pArg, "-l")){      /* 设置测试次数 */
            pArg = GET_ARG(ppcArgV, iArgPos++);
            uiTestCount = lib_atoi(pArg);
        }
        else if(IS_STR_SAME(pArg, "-args")){
            pArg = GET_ARG(ppcArgV, iArgPos++);
            asprintf(&pUserValue, "%s", pArg);
        }
        else if(IS_STR_SAME(pArg, "-s")){       /* 设置测试文件大小 */
            pArg = GET_ARG(ppcArgV, iArgPos++);
            dTestFileSizeRate = lib_atof(pArg);
            if (dTestFileSizeRate <= 0) {
                dTestFileSizeRate = 0;
            }
            if (dTestFileSizeRate >= 1.0) {
                dTestFileSizeRate = 1.0;
            }
        }        
        else {                      /* 测试类型 */
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

    PCHAR cOpt5[2] = {"-smlwr"};
    fstester_register_functionality(cOpt5,  1, "Small Write Test", TEST_TYPE_SMALL_WR, __fstesterSmallWrite);

    API_TShellKeywordAdd("fstester", fstester_cmd_wrapper);
}
