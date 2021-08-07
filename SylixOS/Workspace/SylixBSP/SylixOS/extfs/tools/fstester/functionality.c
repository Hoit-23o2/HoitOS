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

#define MAX_IO_SZ  128
#define MIN_IO_SZ  56

#define RANDOM_RANGE(a, b) (lib_rand() % (b - a) + a)
#define RANDOM_ALPHABET()  (CHAR)(lib_rand() % 26 + 'a')
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
    UINT    i;
    UINT    uiRandomReadOffset;
    UINT    uiRandomReadSize;
    PCHAR   pReadBuffer;

    pReadBuffer = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiLoopTimes; i++)
    {
        lib_memset(pReadBuffer, 0, MAX_IO_SZ);
        uiRandomReadOffset  = lib_random() % uiTestRange;       /* [0 ~  size] */
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
**            uiLoopTimes   ѭ����������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __fstesterSequentialRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){
    UINT    i;
    UINT    uiReadSize = (uiTestRange / uiLoopTimes) > MAX_IO_SZ ? 
                          MAX_IO_SZ : (uiTestRange / uiLoopTimes);
    PCHAR   pReadBuffer;

    pReadBuffer = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiLoopTimes; i++)
    { 
        lib_memset(pReadBuffer, 0, MAX_IO_SZ);
        read(iFdTest, pReadBuffer, uiReadSize);
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
    (VOID)  iFdTest;
    UINT    i, j;
    UINT    uiRandomWriteSize;
    PCHAR   pWriteBuffer;
    INT     iFd;
    
    pWriteBuffer  = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiLoopTimes; i++)
    {
        uiRandomWriteSize  = RANDOM_RANGE(MIN_IO_SZ, MAX_IO_SZ);     
        for (j = 0; j < uiRandomWriteSize; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        write(iFdTest, pWriteBuffer, uiRandomWriteSize);
    }
    lib_free(pWriteBuffer);
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
    UINT    i, j;
    UINT    uiWriteOffset;

    for (i = 0; i < uiLoopTimes; i++)
    {
        if (i == 2) {
            printf("debug\n");
        }
        write(iFdTest, RANDOM_ALPHABET(), sizeof(CHAR));
    }
    return ERROR_NONE;
}
