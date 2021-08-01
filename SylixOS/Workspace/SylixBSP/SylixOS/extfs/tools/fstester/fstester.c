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

VOID fstester_generic_test(FS_TYPE fsType, TEST_TYPE testType, UINT uiTestCount){
    PCHAR           pMountPoint;
    PCHAR           pFSType;
    PCHAR           pOutputDir;
    PCHAR           pOutputPath;
    UINT            i;
    INT             iFdOut, iFdTemp;
    ULONG           ulUsecPerTick;
	ULONG           ulMsecStart;
    ULONG           ulMsecEnd;
    PCHAR           pOutContent;
    INT             iByteWriteOnce  = 0;
    INT             iByteWriteTotal = 0;
    struct stat     stat;

    PCHAR           pReadBuffer;
    UINT            uiRandomReadOffset;
    UINT            uiRandomReadSize;
    UINT            uiReadOffset = 0;
    UINT            uiReadSize   = 5;
    
    PCHAR           pTempPath;
    pMountPoint     = getFSMountPoint(fsType);
    pOutputDir      = getFSTestOutputDir(fsType, testType);
    pOutputPath     = getFSTestOutputPath(fsType, testType);
    pFSType         = translateFSType(fsType);
	ulUsecPerTick   = 1000000 / API_TimeGetFrequency();
    if(access(pOutputDir, F_OK) != ERROR_NONE){
        mkdir(pOutputDir, 0);
    }
    iFdOut          = open(pOutputPath, O_CREAT | O_TRUNC | O_RDWR);
    if(iFdOut < 0){
        printf("[%s] can't create output file [%s]\n", __func__, pOutputPath);
        return;
    }
    switch (testType)
    {
    case TEST_TYPE_RDM_RD: {
        API_Mount("1", pMountPoint, pFSType);
        /* pTempPath = /mnt/fstype(hoitfs)/write-for-test */
        asprintf(&pTempPath, "%s/write-for-test", pMountPoint);
        sleep(1);
        iFdTemp = open(pTempPath, O_CREAT | O_TRUNC | O_RDWR);
        write(iFdTemp, _G_pLorem, lib_strlen(_G_pLorem));
        fstat(iFdTemp, &stat);
        if(iFdTemp < 0){
            printf("[%s] can't create output file [%s]", __func__, pTempPath);
            return;
        }
        for (i = 0; i < uiTestCount; i++)
        {
            printf("====== TEST %d ======\n", i);
            ulMsecStart         = API_TimeGet() * ulUsecPerTick / 1000;
            {
                uiRandomReadOffset  = lib_random() % stat.st_size;  /* [0 ~  size] */
                uiRandomReadSize    = lib_rand() % 80 + 20;         /* [20 ~ 100]����� */ 
                pReadBuffer         = (PCHAR)lib_malloc(uiRandomReadSize);
                lseek(iFdTemp, uiRandomReadOffset, SEEK_SET);
                read(iFdTemp, pReadBuffer, uiRandomReadSize);
                lib_free(pReadBuffer);
            }
            ulMsecEnd           = API_TimeGet()  * ulUsecPerTick / 1000;
            iByteWriteOnce      = asprintf(&pOutContent, "%d\n", ulMsecEnd - ulMsecStart);
            lseek(iFdOut, iByteWriteTotal, SEEK_SET);
            write(iFdOut, pOutContent, iByteWriteOnce);
            lib_free(pOutContent);
            iByteWriteTotal +=iByteWriteOnce;
        }
        close(iFdTemp);
        remove(pTempPath);
        lib_free(pTempPath);
        API_Unmount(pMountPoint);
        nor_reset(NOR_FLASH_BASE);
        break;
    }
    case TEST_TYPE_SEQ_RD: {
        API_Mount("1", pMountPoint, pFSType);
        asprintf(&pTempPath, "%s/write-for-test", pMountPoint);
        sleep(1);
        iFdTemp = open(pTempPath, O_CREAT | O_TRUNC | O_RDWR);
        write(iFdTemp, _G_pLorem, lib_strlen(_G_pLorem));
        fstat(iFdTemp, &stat);
        if(iFdTemp < 0){
            printf("[%s] can't create output file [%s]", __func__, pTempPath);
            return;
        }
        for (i = 0; i < uiTestCount; i++)
        {
            printf("====== TEST %d ======\n", i);
            ulMsecStart         = API_TimeGet() * ulUsecPerTick / 1000;
            {
                pReadBuffer         = (PCHAR)lib_malloc(uiReadSize);
                lseek(iFdTemp, uiReadOffset, SEEK_SET);
                read(iFdTemp, pReadBuffer, uiRandomReadSize);
                uiReadOffset += uiRandomReadSize;
                lib_free(pReadBuffer);
            }
            ulMsecEnd           = API_TimeGet()  * ulUsecPerTick / 1000;
            iByteWriteOnce      = asprintf(&pOutContent, "%d\n", ulMsecEnd - ulMsecStart);
            lseek(iFdOut, iByteWriteTotal, SEEK_SET);
            write(iFdOut, pOutContent, iByteWriteOnce);
            lib_free(pOutContent);
            iByteWriteTotal += iByteWriteOnce;
        }
        close(iFdTemp);
        remove(pTempPath);
        lib_free(pTempPath);
        API_Unmount(pMountPoint);
        nor_reset(NOR_FLASH_BASE);
        break;
    }
    default:
        break;
    }
    close(iFdOut);
    lib_free(pOutputPath);
    return;
}
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

INT fstester_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    FS_TYPE fsTarget = FS_TYPE_SPIFFS;
    fstester_generic_test(FS_TYPE_HOITFS, TEST_TYPE_RDM_RD, 5);
    // if(iArgC == 1){
    //     fstester_generate_script(fsTarget, 5);
    // }
    // if(iArgC > 1){
    //     fstester_parse_out();
    // }
}

VOID register_fstester_cmd(){
    API_TShellKeywordAdd("fstester", fstester_cmd_wrapper);
}
