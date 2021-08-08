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
VOID __fstesterUtilSequentialWrite(UINT uiPerWriteSize, UINT uiOccupyFactor,  PCHAR pMountPoint){
    UINT    i, j;
    UINT    uiWriteSize, uiWriteTimes;
    PCHAR   pWriteBuffer;
    INT     iFdTemp;
    PCHAR   pcTempPath;
    struct  statfs stat;

    statfs(pMountPoint, &stat);
    uiWriteSize = stat.f_bsize * uiOccupyFactor / HUNDRED_PERCENT;      /* д��fs�� @uiOccupyFactor% */
    uiWriteTimes = uiWriteSize / uiPerWriteSize;

    asprintf(&pcTempPath, "%s/seq-write-test", pMountPoint);
    iFdTemp = open(pcTempPath, O_CREAT | O_TRUNC | O_RDWR);

    pWriteBuffer  = (PCHAR)lib_malloc(uiPerWriteSize);

    for (i = 0; i < uiWriteTimes; i++)                  
    {
        for (j = 0; j < uiPerWriteSize; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        write(iFdTemp, pWriteBuffer, uiPerWriteSize);
    }
    
    lib_free(pWriteBuffer);
    lib_free(pcTempPath);
    close(iFdTemp);
    remove(iFdTemp);
    nor_reset(NOR_FLASH_BASE);                          /* ����Norflash״̬������GCӰ�� */
    return;
}


/*********************************************************************************************************
** ��������: __fstesterRandomRead
** ��������: �����ʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterRandomRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){
    UINT    i, uiReadIter;
    UINT    uiRandomReadOffset;
    UINT    uiRandomReadSize;
    PCHAR   pReadBuffer;
    UINT    uiInnerLoopTimes = uiTestRange / 4;
    pReadBuffer = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiLoopTimes; i++)
    {
        for (uiReadIter = 0; uiReadIter < uiInnerLoopTimes; uiReadIter++)
        {
            lib_memset(pReadBuffer, 0, MAX_IO_SZ);
            uiRandomReadOffset  = lib_random() % uiTestRange;                 /* [0 ~  size] */
            uiRandomReadSize    = RANDOM_RANGE(MIN_IO_SZ, MAX_IO_SZ);         /* [MIN_IO_SZ ~ MAX_IO_SZ]����� */ 
            lseek(iFdTest, uiRandomReadOffset, SEEK_SET);
            read(iFdTest, pReadBuffer, uiRandomReadSize);
        }
    }
    lib_free(pReadBuffer);
    return ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __fstesterSequentialRead
** ��������: ˳���ʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterSequentialRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){
    (VOID)  uiLoopTimes;
    UINT    i, uiReadIter;
    UINT    uiReadTimes = uiTestRange / MAX_IO_SZ;       /* ÿ�ζ� MAX_IO_SZ����ȫ������ */
    PCHAR   pReadBuffer;

    pReadBuffer = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiReadTimes; i++)
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
**            uiLoopTimes   ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterRandomWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){     
    UINT    i, j;
    UINT    uiRandomWriteOffset;
    UINT    uiRandomWriteSize;
    PCHAR   pWriteBuffer;

    pWriteBuffer         = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiLoopTimes; i++)
    {
        uiRandomWriteOffset  = lib_random() % uiTestRange;              /* [0 ~  size] */
        uiRandomWriteSize    = RANDOM_RANGE(MIN_IO_SZ, MAX_IO_SZ);     
        for (j = 0; j < uiRandomWriteSize; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        if(i == 6)
            printf("loops: %d\n", i);
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
**            uiLoopTimes   ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterSequentialWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){ 
    (VOID)  iFdTest, uiTestRange, uiLoopTimes;
    __fstesterUtilSequentialWrite(MAX_IO_SZ, 100, pMountPoint);
    return ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __fstesterSmallWrite
** ��������: ���дʵ��
** �䡡��  : iFdTest        �������ļ�������
**            uiTestRange   �ļ���С
**            uiLoopTimes   ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterSmallWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){ 
    (VOID)  iFdTest, uiTestRange, uiLoopTimes;
    __fstesterUtilSequentialWrite(1, 30, pMountPoint);
    return ERROR_NONE;
}
