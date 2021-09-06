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
** ��   ��   ��: functionality.c
**
** ��   ��   ��: Pan Yanqi
**
** �ļ���������: 2021 �� 08 �� 02 ��
**
** ��        ��: ����ʵ�ֺ���
*********************************************************************************************************/
#include "fstester.h"
#include "../driver/mtd/nor/nor.h"
#define IO_SZ  256

#define RANDOM_RANGE(a, b)                                  (lib_rand() % (b - a) + a)
#define RANDOM_ALPHABET()                                   (CHAR)(lib_rand() % 26 + 'a')
#define HUNDRED_PERCENT                                     (100)
#define IOS_PER_LOOPS(uiTestRange, uiLoopTimes, uiIOSize)   ((uiTestRange / uiLoopTimes) / uiIOSize)

extern INT __fstester_prepare_test(PCHAR pTestPath, double testFileSizeRate, 
                                   PCHAR pMountPoint, PCHAR pFSType, BOOL bIsNeedInitialize);
/*********************************************************************************************************
** ��������: __fstesterUtilSequentialWrite
** ��������: ˳��д���ߣ�����С��������д��������д��
** �䡡��  : uiPerWriteSize  ÿ��д���ݴ�С
**            dOccupyFactor  ռ���ռ�ٷֱȣ�[0 ~ 1]
**            pMountPoint      ���ص㣺 /mnt/hoitfs
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterUtilSequentialWrite(INT iFdTest, UINT uiPerWriteSize, UINT uiLoopTimes, double dOccupyFactor, UINT uiAccurayWriteTotalSize, PCHAR pMountPoint){
    UINT    i, j;
    UINT    uiWriteSize;
    UINT    uiInnerLoopTimes;
    PCHAR   pWriteBuffer;
    struct  statfs stat;

    statfs(pMountPoint, &stat);
    if(dOccupyFactor != -1){
        uiWriteSize     = (stat.f_bsize * stat.f_blocks * dOccupyFactor);      /* д��fs�� @uiOccupyFactor% */
    }
    else if(uiAccurayWriteTotalSize != -1){
        uiWriteSize = uiAccurayWriteTotalSize;
    }

    uiInnerLoopTimes  = IOS_PER_LOOPS(uiWriteSize, uiLoopTimes, uiPerWriteSize);

    pWriteBuffer      = (PCHAR)lib_malloc(uiPerWriteSize);
    
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        for (j = 0; j < uiPerWriteSize; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        write(iFdTest, pWriteBuffer, uiPerWriteSize);
    }
    
    
    lib_free(pWriteBuffer);
    return uiInnerLoopTimes * uiPerWriteSize;
}


/*********************************************************************************************************
** ��������: __fstesterRandomRead
** ��������: �����ʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ���ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterRandomRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){
    UINT    i;
    UINT    uiRandomReadOffset;
    PCHAR   pReadBuffer;
    UINT    uiInnerLoopTimes = 10 * IOS_PER_LOOPS(uiTestRange, uiLoopTimes, IO_SZ);
    
    pReadBuffer = (PCHAR)lib_malloc(IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        lib_memset(pReadBuffer, 0, IO_SZ);
        uiRandomReadOffset  = lib_random() % uiTestRange;                 /* [0 ~  size] */
        lseek(iFdTest, uiRandomReadOffset, SEEK_SET);
        read(iFdTest, pReadBuffer, IO_SZ);
    }
    lib_free(pReadBuffer);
    return uiInnerLoopTimes * IO_SZ;
}
/*********************************************************************************************************
** ��������: __fstesterSequentialRead
** ��������: ˳���ʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ���ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterSequentialRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){
    (VOID) uiTestRange, uiLoopTimes, pUserValue;

    UINT    i, uiReadIter;
    PCHAR   pReadBuffer;
    UINT    uiInnerLoopTimes = IOS_PER_LOOPS(uiTestRange, uiLoopTimes, IO_SZ);       /* ÿ�ζ� IO_SZ����ȫ������ */
    
    pReadBuffer = (PCHAR)lib_malloc(IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    { 
        lib_memset(pReadBuffer, 0, IO_SZ);
        read(iFdTest, pReadBuffer, IO_SZ);
    }
    lib_free(pReadBuffer);
    return uiInnerLoopTimes * IO_SZ;
}
/*********************************************************************************************************
** ��������: __fstesterRandomWrite
** ��������: ���дʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterRandomWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){     
    (VOID)  pUserValue,  uiLoopTimes;
    UINT    i, j;
    UINT    uiRandomWriteOffset;
    PCHAR   pWriteBuffer;
    UINT    uiInnerLoopTimes = 10;

    pWriteBuffer  = (PCHAR)lib_malloc(IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        uiRandomWriteOffset  = lib_random() % uiTestRange;              /* [0 ~  size] */     
        for (j = 0; j < IO_SZ; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        lseek(iFdTest, uiRandomWriteOffset, SEEK_SET);
        write(iFdTest, pWriteBuffer, IO_SZ);
    }
    lib_free(pWriteBuffer);
    return uiInnerLoopTimes * IO_SZ;
}
/*********************************************************************************************************
** ��������: __fstesterSequentialWrite
** ��������: ���дʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ���ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterSequentialWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){ 
    (VOID)  iFdTest, uiTestRange;
    double dOccupyFactor = 0.6;
    if(pUserValue != LW_NULL)
        dOccupyFactor = lib_atof((PCHAR)pUserValue);
    return __fstesterUtilSequentialWrite(iFdTest, IO_SZ, uiLoopTimes, 
                                         dOccupyFactor, -1, pMountPoint);
}
/*********************************************************************************************************
** ��������: __fstesterSmallWrite
** ��������: ���дʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ���ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterSmallWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){ 
    (VOID)  iFdTest, uiTestRange;
    INT uiAccurayWriteTotalSize = 1000;
    if(pUserValue != LW_NULL)
        uiAccurayWriteTotalSize = lib_atoi((PCHAR)pUserValue);
    return __fstesterUtilSequentialWrite(iFdTest, 1, uiLoopTimes, 
                                         -1, uiAccurayWriteTotalSize, pMountPoint);
}
/*********************************************************************************************************
** ��������: __fstesterSmallWrite
** ��������: ���дʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ���ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterMount(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){
    (VOID) iFdTest, uiTestRange;
    API_Unmount(pMountPoint);
    PCHAR pFSType = lib_strcmp(pMountPoint, "/mnt/spiffs") == 0 ? "spiffs" : "hoitfs"; 
    __fstester_prepare_test(LW_NULL, 0, pMountPoint, pFSType, LW_FALSE);
    return ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __fstesterGC
** ��������: GC��������
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ���ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterGC(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){
    (VOID)  uiTestRange;
    double  dOccupyFactor = 0.5;
    INT     iIOBytes;

    if(pUserValue != LW_NULL)
        dOccupyFactor = lib_atof((PCHAR)pUserValue);
    if(dOccupyFactor > 0.5){
        dOccupyFactor = 0.5;
        return PX_ERROR;
    }
    return __fstesterUtilSequentialWrite(iFdTest, IO_SZ, uiLoopTimes, 
                                         dOccupyFactor, -1, pMountPoint);
}
/*********************************************************************************************************
** ��������: __fstesterMergeableTree
** ��������: Mergeable Tree�ڴ����
** �䡡��  : 
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterMergeableTree(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){
    (VOID)  uiTestRange;
    INT uiAccurayWriteTotalSize = 1000;
    struct stat stat;
    if(pUserValue != LW_NULL)
        uiAccurayWriteTotalSize = lib_atoi((PCHAR)pUserValue);
    __fstesterUtilSequentialWrite(iFdTest, 1, uiLoopTimes, 
                                  -1, uiAccurayWriteTotalSize, pMountPoint);
    fstat(iFdTest, &stat);
    return *(INT *)stat.st_resv1;
}
/*********************************************************************************************************
** ��������: __fstesterPowerFailure
** ��������: Power Failure�������
** �䡡��  : 
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterPowerFailure(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue) {
    
}
    