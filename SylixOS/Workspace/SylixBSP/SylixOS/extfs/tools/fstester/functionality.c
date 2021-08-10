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
/*********************************************************************************************************
** ��������: __fstesterUtilSequentialWrite
** ��������: ˳��д���ߣ�����С��������д��������д��
** �䡡��  : uiPerWriteSize  ÿ��д���ݴ�С
**            uiOccupyFactor  ռ���ռ�ٷֱȣ�[0 ~ 100]
**            pMountPoint      ���ص㣺 /mnt/hoitfs
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterUtilSequentialWrite(INT iFdTest, UINT uiPerWriteSize, UINT uiLoopTimes, UINT uiOccupyFactor, UINT uiAccurayWriteTotalSize, PCHAR pMountPoint){
    UINT    i, j;
    UINT    uiWriteSize;
    UINT    uiInnerLoopTimes;
    PCHAR   pWriteBuffer;
    struct  statfs stat;

    statfs(pMountPoint, &stat);
    if(uiOccupyFactor != -1){
        uiWriteSize     = (stat.f_bsize * stat.f_blocks * uiOccupyFactor) / HUNDRED_PERCENT;      /* д��fs�� @uiOccupyFactor% */
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
    INT uiOccupyFactor = 60;
    if(pUserValue != LW_NULL)
        uiOccupyFactor = lib_atoi((PCHAR)pUserValue);
    return __fstesterUtilSequentialWrite(iFdTest, IO_SZ, uiLoopTimes, 
                                         uiOccupyFactor, -1, pMountPoint);
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
