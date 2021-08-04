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
** 文   件   名: functionality.c
**
** 创   建   人: Pan Yanqi
**
** 文件创建日期: 2021 年 08 月 02 日
**
** 描        述: 功能实现函数
*********************************************************************************************************/
#include "fstester.h"

#define MAX_IO_SZ  4096
#define MIN_IO_SZ  1024

#define RANDOM_RANGE(a, b) (lib_rand() % (b - a) + a)
#define RANDOM_ALPHABET()  (CHAR)(lib_rand() % 26 + 'a')
/*********************************************************************************************************
** 函数名称: __fstesterRandomRead
** 功能描述: 随机读实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterRandomRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes){
    UINT    i;
    UINT    uiRandomReadOffset;
    UINT    uiRandomReadSize;
    PCHAR   pReadBuffer;

    pReadBuffer = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiLoopTimes; i++)
    {
        lib_memset(pReadBuffer, 0, MAX_IO_SZ);
        uiRandomReadOffset  = lib_random() % uiTestRange;       /* [0 ~  size] */
        uiRandomReadSize    = RANDOM_RANGE(MIN_IO_SZ, MAX_IO_SZ);         /* [MIN_IO_SZ ~ MAX_IO_SZ]随机数 */ 
        lseek(iFdTest, uiRandomReadOffset, SEEK_SET);
        read(iFdTest, pReadBuffer, uiRandomReadSize);
    }
    lib_free(pReadBuffer);
    return ERROR_NONE;
}
/*********************************************************************************************************
** 函数名称: __fstesterSequentialRead
** 功能描述: 顺序读实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterSequentialRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes){
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
** 函数名称: __fstesterRandomWrite
** 功能描述: 随机写实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterRandomWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes){     
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
        //printf("%d : %d\n", uiRandomWriteOffset, uiRandomWriteSize);
        write(iFdTest, pWriteBuffer, uiRandomWriteSize);
    }
    lib_free(pWriteBuffer);
    return ERROR_NONE;

}
/*********************************************************************************************************
** 函数名称: __fstesterSequentialWrite
** 功能描述: 随机写实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterSequentialWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes){ 
    UINT    i, j;
    UINT    uiWriteSize;
    PCHAR   pWriteBuffer;

    pWriteBuffer         = (PCHAR)lib_malloc(MAX_IO_SZ);
    for (i = 0; i < uiLoopTimes; i++)
    {
        uiWriteSize    = RANDOM_RANGE(MIN_IO_SZ, MAX_IO_SZ);     
        for (j = 0; j < uiWriteSize; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        write(iFdTest, pWriteBuffer, uiWriteSize);
    }
    lib_free(pWriteBuffer);
    return ERROR_NONE;
}
/*********************************************************************************************************
** 函数名称: __fstesterSmallWrite
** 功能描述: 随机写实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterSmallWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes){ 
    UINT    i, j;
    UINT    uiWriteOffset;

    for (i = 0; i < uiLoopTimes; i++)
    {
        write(iFdTest, RANDOM_ALPHABET(), sizeof(CHAR));
    }
    return ERROR_NONE;
}