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
#define MAX_IO_SZ  128
#define MIN_IO_SZ  56

#define RANDOM_RANGE(a, b) (lib_rand() % (b - a) + a)
#define RANDOM_ALPHABET()  (CHAR)(lib_rand() % 26 + 'a')
#define HUNDRED_PERCENT    (100)
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
VOID __fstesterUtilSequentialWrite(INT iFdTest, UINT uiPerWriteSize, UINT uiLoopTimes, UINT uiOccupyFactor, UINT uiAccurayWriteTotalSize, PCHAR pMountPoint){
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

    uiInnerLoopTimes  = ((uiWriteSize / uiLoopTimes) / uiPerWriteSize);

    pWriteBuffer    = (PCHAR)lib_malloc(uiPerWriteSize);
    
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        for (j = 0; j < uiPerWriteSize; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        write(iFdTest, pWriteBuffer, uiPerWriteSize);
    }
    
    
    lib_free(pWriteBuffer);
    return;
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
    UINT    uiRandomReadSize;
    PCHAR   pReadBuffer;
    UINT    uiInnerLoopTimes = 1000;
    pReadBuffer = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        lib_memset(pReadBuffer, 0, MAX_IO_SZ);
        uiRandomReadOffset  = lib_random() % uiTestRange;                 /* [0 ~  size] */
        uiRandomReadSize    = RANDOM_RANGE(MIN_IO_SZ, MAX_IO_SZ);         /* [MIN_IO_SZ ~ MAX_IO_SZ]����� */ 
        lseek(iFdTest, uiRandomReadOffset, SEEK_SET);
        read(iFdTest, pReadBuffer, uiRandomReadSize);
    }
    lib_free(pReadBuffer);
    return ERROR_NONE;
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
    UINT    uiInnerLoopTimes = uiTestRange / MAX_IO_SZ;       /* ÿ�ζ� MAX_IO_SZ����ȫ������ */
    
    pReadBuffer = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    { 
        lib_memset(pReadBuffer, 0, MAX_IO_SZ);
        read(iFdTest, pReadBuffer, MAX_IO_SZ);
    }
    lib_free(pReadBuffer);
    return ERROR_NONE;
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
    UINT    uiRandomWriteSize;
    PCHAR   pWriteBuffer;
    UINT    uiInnerLoopTimes = 10;

    pWriteBuffer  = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        uiRandomWriteOffset  = lib_random() % uiTestRange;              /* [0 ~  size] */
        uiRandomWriteSize    = RANDOM_RANGE(MIN_IO_SZ, MAX_IO_SZ);     
        for (j = 0; j < uiRandomWriteSize; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        lseek(iFdTest, uiRandomWriteOffset, SEEK_SET);
        write(iFdTest, pWriteBuffer, uiRandomWriteSize);
    }
    lib_free(pWriteBuffer);
    return ERROR_NONE;
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
    __fstesterUtilSequentialWrite(iFdTest, MAX_IO_SZ, uiLoopTimes, uiOccupyFactor, -1, pMountPoint);
    return ERROR_NONE;
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
    __fstesterUtilSequentialWrite(iFdTest, 1, uiLoopTimes, -1, 1000, pMountPoint);
    return ERROR_NONE;
}
