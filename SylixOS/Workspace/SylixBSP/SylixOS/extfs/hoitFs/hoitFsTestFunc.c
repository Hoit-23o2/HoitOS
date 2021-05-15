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
** 文   件   名: hoitFsMid.h
**
** 创   建   人: 张楠
**
** 文件创建日期: 2021 年 05 月 15 日
**
** 描        述: 测试函数，用于测试hoitfs文件系统
*********************************************************************************************************/

#include "./hoitFsTestFunc.h"

/*********************************************************************************************************
 * File Tree Test
*********************************************************************************************************/

UINT max_depth      = 6;
UINT max_dirNo      = 5;
UINT max_fileNo     = 5;
UINT max_data_write = 16;
UINT ERROR_FLAG     = 0;
UINT depth  = 1;

void createDir(PUCHAR pFileName, UINT dirNo) {
    mode_t  mode        = DEFAULT_DIR_PERM;
    PUCHAR  pEnd        = pFileName;
    UCHAR   dirname[9]  = "Newdir0/\0";
    INT     iFd;
    /* 赋予新的名字 */
    dirname[6]  = '0'+dirNo;
    dirNo ++;
    while (*pEnd != 0)
    {
        pEnd ++;
    }

    lib_memcpy(pEnd, dirname, sizeof(dirname));

    mode &= ~S_IFMT;
    
    iFd = open(pFileName, O_RDWR | O_CREAT | O_EXCL, S_IFDIR | mode);   /*  排他性创建 */ 
    if (iFd < 0) {
        printf("error!\n");
        ERROR_FLAG = 1;
        return ;
    }
    close(iFd);
    /* 进入文件夹，开始下一轮递归测试 */
    if (depth <= max_depth) {
        depth ++;
        FileTreeTestStart(pFileName);
        if (ERROR_FLAG == 1) 
            return;
        depth --;
    }
    /* 清空目录名 */
    lib_memset(pEnd, 0, sizeof(dirname));
}

void createFile(PUCHAR pFileName, UINT fileNo) {
    mode_t  mode            = DEFAULT_FILE_PERM;
    PUCHAR  pEnd            = pFileName;
    UCHAR   filename[9]     = "Newfile0\0";
    UCHAR   filewrite[16]   = "Hello hoitfs   \0";
    UCHAR   fileread[16];

    UINT    writetimes      = max_data_write*sizeof(UCHAR)/sizeof(filewrite);
    INT     writebytes      = 0;
    INT     iFd;
    INT     i;
    lib_memset(fileread, 0, sizeof(fileread));
    /* 赋予新的名字 */
    filename[7]  = '0'+fileNo;
    fileNo ++;
    while (*pEnd != 0)
    {
        pEnd ++;
    }
    lib_memcpy(pEnd, filename, sizeof(filename));

    iFd = open(pFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);   /*  排他性创建 */ 
    /* 文件统一写入"Hello hoitfs" */
    printf("\t%s starts writing.\n", pFileName);
    for(i=0; i<writetimes ; i++) {        
        writebytes = write(iFd, filewrite, sizeof(filewrite));
    }
    
    // read(iFd, fileread, sizeof(fileread));

    if (iFd < 0) {
        printf("error!\n");
        ERROR_FLAG = 1;
        return ;
    }
    close(iFd);
    lib_memset(pEnd, 0, sizeof(filename));
//    lib_memcpy(pEnd, zero, sizeof(zero));
}
/*
    创建文件和目录
*/
void FileTreeTestStart(PUCHAR pFileName) {
    UINT    file_num    = max_fileNo;
    UINT    dir_num     = max_dirNo;
    UINT    i;
    /* 创建随机数 */
    // srand((UINT)&file_num);
    // file_num = rand() % max_fileNo + 1;
    // srand((UINT)&dir_num);
    // dir_num = rand() % max_dirNo + 1;

    /* 创建文件 */
    for (i=0 ; i<file_num ; i++) {
        createFile(pFileName, i);
        if (ERROR_FLAG == 1) {
            return;
        }
    }

    /* 创建目录 */
    for (i=0 ; i<dir_num ; i++) {
        createDir(pFileName, i);
        if (ERROR_FLAG == 1) {
            return;
        }        
    }
}

void checkDir(PUCHAR pFileName, UINT dirNo) {
    mode_t  mode        = DEFAULT_DIR_PERM;
    PUCHAR  pEnd        = pFileName;
    UCHAR   dirname[9]  = "Newdir0/\0";
    INT     iFd;
    /* 赋予新的名字 */
    dirname[6]  = '0'+dirNo;
    dirNo ++;
    while (*pEnd != 0)
    {
        pEnd ++;
    }

    lib_memcpy(pEnd, dirname, sizeof(dirname));

    mode &= ~S_IFMT;
    
    iFd = open(pFileName, O_RDWR, S_IFDIR | mode);   /*  排他性创建 */ 
    if (iFd < 0) {
        printf("\"%s\" does not exist!\n",pFileName);
        ERROR_FLAG = 1;
        return ;
    }
    close(iFd);
    /* 进入文件夹，开始下一轮递归测试 */
    if (depth <= max_depth) {
        depth ++;
        FileTreeTestCheck(pFileName);
        if (ERROR_FLAG == 1) 
            return;
        depth --;
    }
    /* 清空目录名 */
    lib_memset(pEnd, 0, sizeof(dirname));
}

void checkFile(PUCHAR pFileName, UINT fileNo) {
    mode_t  mode            = DEFAULT_FILE_PERM;
    PUCHAR  pEnd            = pFileName;
    UCHAR   filename[9]     = "Newfile0\0";
    UCHAR   filewrite[16]   = "Hello hoitfs   \0";
    UCHAR   fileread[16];

    UINT    writetimes      = max_data_write*sizeof(UCHAR)/sizeof(filewrite);
    INT     writebytes      = 0;
    INT     iFd;
    INT     i;

    
    lib_memset(fileread, 0, sizeof(fileread));
    /* 赋予新的名字 */
    filename[7]  = '0'+fileNo;
    fileNo ++;
    while (*pEnd != 0)
    {
        pEnd ++;
    }
    lib_memcpy(pEnd, filename, sizeof(filename));
    printf("\tcheck %s content\n", pFileName);
    iFd = open(pFileName, O_RDONLY, mode);   /*  排他性创建 */ 
    if (iFd < 0) {
        printf("\"%s\" does not exist!\n",pFileName);
        ERROR_FLAG = 1;
        return ;
    }    
    /* 查看文件内容 "Hello hoitfs" */

    for(i=0; i<writetimes ; i++) {        
        writebytes = read(iFd, fileread, sizeof(filewrite));
        if (writebytes == 0) {
            printf("read failed!\n");
            ERROR_FLAG = 1;
            return ;            
        }
        if (lib_strcmp(fileread, filewrite) != 0) {
            printf("read content is not !\n");
        }
    }

    close(iFd);
    lib_memset(pEnd, 0, sizeof(filename));
//    lib_memcpy(pEnd, zero, sizeof(zero));
}
/*
    检查创建的文件是否存在，内容是否一致
    检查目录是否存在
*/
void FileTreeTestCheck(PUCHAR pFileName) {
    UINT    i;
    /* 检查文件 */
    for (i=0 ; i<max_fileNo ; i++) {
        checkFile(pFileName, i);
        if (ERROR_FLAG == 1) {
            return;
        }
    }
 
    /* 检查目录 */
    for (i=0 ; i<max_dirNo ; i++) {
        checkDir(pFileName, i);
        if (ERROR_FLAG == 1) {
            return;
        }        
    }    
}

INT FileTreeTest (INT  iArgC, PCHAR  ppcArgV[])
{
    UCHAR       filename[512];
    UINT        temp;
    lib_memset(filename, 0 ,sizeof(filename));
    lib_strcpy(filename, "/mnt/hoitfs/\0");

    if (iArgC != 2 && iArgC != 5) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (iArgC == 2) {
        if (lib_strcmp("-h",ppcArgV[1]) == 0) {
            printf("file tree test argc:\n");
            printf("first argc  :   max file tree depth\n");
            printf("second argc :   max file number in a directory\n");
            printf("thrid argc  :   max dir number in a directory\n");
            return ERROR_NONE;
        } else {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
    } else if (iArgC == 5) {
        if (lib_strcmp("-t",ppcArgV[1]) == 0) {
            temp = lib_atoi(ppcArgV[2]);
            if (temp < max_depth)
                max_depth = temp;

            temp = lib_atoi(ppcArgV[3]);
            if (temp < max_fileNo)
                max_fileNo = temp;

            temp = lib_atoi(ppcArgV[4]);
            if (temp < max_dirNo)
                max_dirNo = temp;
        } else {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }        
    }

    printf("===========  File Tree Test!         ===========\n");
    printf("===========  Create File And Dir!    ===========\n");
    FileTreeTestStart(filename);
    printf("===========  Check File And Dir!     ===========\n");
    FileTreeTestCheck(filename);
    printf("===========  File Tree Test End!     ===========\n");
    return  ERROR_NONE;
}

/*********************************************************************************************************
 * File Over Write Test
*********************************************************************************************************/
INT FileOverWriteTest (INT  iArgC, PCHAR  ppcArgV[]) {
    UCHAR   filename[30]     = "/mnt/hoitfs/OverWriteTest\0";
    UCHAR   readData[1024+256];
    INT     iFd;
    UCHAR   data    = '1';
    INT     i;
    printf("===========  File Overwrite Test!       ===========\n");
    iFd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_PERM);   /*  排他性创建 */ 
    lib_memset(readData, 0, sizeof(readData));
    /* 初始写入512个'1' */
    for (i=0 ; i<64 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n",readData);

    /* 关闭文件再打开，追加写256个'2' */
    data ++;
    close(iFd);
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    for (i=64 ; i<96 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n",readData);

    /* 覆盖修改前256个字为"3" */
    data ++;
    close(iFd);
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    for(i=0 ; i<32 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));    
    printf("%s\n",readData);

    /* 从512位开始写512个'4' */
    data ++;
    close(iFd);
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    for(i=64 ; i<128 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n",readData);

    /* 从1024位开始中间空128字节，然后写128个'5' */
//    data ++;
    close(iFd);
//    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
//    for(i=128 ; i<160 ; i++) {
//        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
//        write(iFd, &data, sizeof(CHAR));
//    }
//    printf("\nNo.%c result:\n", data);
//    close(iFd);
//    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
//    read(iFd, readData, sizeof(readData));
//    for ( i = 0; i < sizeof(readData)/sizeof(UCHAR); i++)
//    {
//        if(readData[i] != 0)
//            printf("%c",readData[i]);
//        else
//            printf(" ");
//    }
    printf("\n");
    printf("===========  File Overwrite Test End!    ===========\n");
    return  ERROR_NONE;
}
