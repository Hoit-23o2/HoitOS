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
#include "../driver/mtd/nor/nor.h"
#define MAX_IO_SZ  128
#define MIN_IO_SZ  56

#define RANDOM_RANGE(a, b) (lib_rand() % (b - a) + a)
#define RANDOM_ALPHABET()  (CHAR)(lib_rand() % 26 + 'a')
#define HUNDRED_PERCENT    (100)
/*********************************************************************************************************
** 函数名称: __fstesterUtilSequentialWrite
** 功能描述: 顺序写工具，用于小数据连续写入与连续写入
** 输　入  : uiPerWriteSize  每次写数据大小
**            uiOccupyFactor  占满空间百分比，[0 ~ 100]
**            pMountPoint      挂载点： /mnt/hoitfs
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __fstesterUtilSequentialWrite(UINT uiPerWriteSize, UINT uiOccupyFactor,  PCHAR pMountPoint){
    UINT    i, j;
    UINT    uiWriteSize, uiWriteTimes;
    PCHAR   pWriteBuffer;
    INT     iFdTemp;
    PCHAR   pcTempPath;
    struct  statfs stat;

    statfs(pMountPoint, &stat);
    uiWriteSize = stat.f_bsize * uiOccupyFactor / HUNDRED_PERCENT;      /* 写满fs的 @uiOccupyFactor% */
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
    nor_reset(NOR_FLASH_BASE);                          /* 重置Norflash状态，避免GC影响 */
    return;
}


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
            uiRandomReadSize    = RANDOM_RANGE(MIN_IO_SZ, MAX_IO_SZ);         /* [MIN_IO_SZ ~ MAX_IO_SZ]随机数 */ 
            lseek(iFdTest, uiRandomReadOffset, SEEK_SET);
            read(iFdTest, pReadBuffer, uiRandomReadSize);
        }
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
INT __fstesterSequentialRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){
    (VOID)  uiLoopTimes;
    UINT    i, uiReadIter;
    UINT    uiReadTimes = uiTestRange / MAX_IO_SZ;       /* 每次读 MAX_IO_SZ，读全部数据 */
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
** 函数名称: __fstesterRandomWrite
** 功能描述: 随机写实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
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
** 函数名称: __fstesterSequentialWrite
** 功能描述: 随机写实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterSequentialWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){ 
    (VOID)  iFdTest, uiTestRange, uiLoopTimes;
    __fstesterUtilSequentialWrite(MAX_IO_SZ, 100, pMountPoint);
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
INT __fstesterSmallWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint){ 
    (VOID)  iFdTest, uiTestRange, uiLoopTimes;
    __fstesterUtilSequentialWrite(1, 30, pMountPoint);
    return ERROR_NONE;
}
