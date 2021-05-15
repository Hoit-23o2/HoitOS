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
** ��   ��   ��: hoitFsTest.h
**
** ��   ��   ��: ���
**
** �ļ���������: 2021 �� 05 �� 15 ��
**
** ��        ��: ���Ժ��������ڲ���hoitfs�ļ�ϵͳ
*********************************************************************************************************/

#include "./hoitFsTest.h"

/*********************************************************************************************************
 * File Tree Test �ļ�����������
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
    /* �����µ����� */
    dirname[6]  = '0'+dirNo;
    dirNo ++;
    while (*pEnd != 0)
    {
        pEnd ++;
    }

    lib_memcpy(pEnd, dirname, sizeof(dirname));

    mode &= ~S_IFMT;
    
    iFd = open(pFileName, O_RDWR | O_CREAT | O_EXCL, S_IFDIR | mode);   /*  �����Դ��� */ 
    if (iFd < 0) {
        printf("error!\n");
        ERROR_FLAG = 1;
        return ;
    }
    close(iFd);
    /* �����ļ��У���ʼ��һ�ֵݹ���� */
    if (depth <= max_depth) {
        depth ++;
        FileTreeTestStart(pFileName);
        if (ERROR_FLAG == 1) 
            return;
        depth --;
    }
    /* ���Ŀ¼�� */
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
    /* �����µ����� */
    filename[7]  = '0'+fileNo;
    fileNo ++;
    while (*pEnd != 0)
    {
        pEnd ++;
    }
    lib_memcpy(pEnd, filename, sizeof(filename));

    iFd = open(pFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);   /*  �����Դ��� */ 
    if (iFd < 0) {
        printf("%s create error!\n",pFileName);
        ERROR_FLAG = 1;
        return ;
    }
    /* �ļ�ͳһд��"Hello hoitfs" */
    printf("\t%s starts writing.\n", pFileName);
    for(i=0; i<writetimes ; i++) {        
        writebytes = write(iFd, filewrite, sizeof(filewrite));
    }
    close(iFd);

    // iFd = open(pFileName, O_RDONLY | O_CREAT | O_TRUNC, mode);
    // if (iFd < 0) {
    //     printf("error!\n");
    //     ERROR_FLAG = 1;
    //     return ;
    // }
    // read(iFd, fileread, sizeof(fileread));
    // close(iFd);

    lib_memset(pEnd, 0, sizeof(filename));
}
/*
    �����ļ���Ŀ¼
*/
void FileTreeTestStart(PUCHAR pFileName) {
    UINT    file_num    = max_fileNo;
    UINT    dir_num     = max_dirNo;
    UINT    i;
    /* ��������� */
    // srand((UINT)&file_num);
    // file_num = rand() % max_fileNo + 1;
    // srand((UINT)&dir_num);
    // dir_num = rand() % max_dirNo + 1;

    /* �����ļ� */
    for (i=0 ; i<file_num ; i++) {
        createFile(pFileName, i);
        if (ERROR_FLAG == 1) {
            return;
        }
    }

    /* ����Ŀ¼ */
    for (i=0 ; i<dir_num ; i++) {
        createDir(pFileName, i);
        if (ERROR_FLAG == 1) {
            return;
        }        
    }
}
/******************************  check part  ******************************/
void checkDir(PUCHAR pFileName, UINT dirNo) {
    mode_t  mode        = DEFAULT_DIR_PERM;
    PUCHAR  pEnd        = pFileName;
    UCHAR   dirname[9]  = "Newdir0/\0";
    INT     iFd;
    /* �����µ����� */
    dirname[6]  = '0'+dirNo;
    dirNo ++;
    while (*pEnd != 0)
    {
        pEnd ++;
    }

    lib_memcpy(pEnd, dirname, sizeof(dirname));

    mode &= ~S_IFMT;
    
    iFd = open(pFileName, O_RDWR, S_IFDIR | mode);   /*  �����Դ��� */ 
    if (iFd < 0) {
        printf("\"%s\" does not exist!\n",pFileName);
        ERROR_FLAG = 1;
        return ;
    }
    close(iFd);
    /* �����ļ��У���ʼ��һ�ֵݹ���� */
    if (depth <= max_depth) {
        depth ++;
        FileTreeTestCheck(pFileName);
        if (ERROR_FLAG == 1) 
            return;
        depth --;
    }
    /* ���Ŀ¼�� */
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
    /* �����µ����� */
    filename[7]  = '0'+fileNo;
    fileNo ++;
    while (*pEnd != 0)
    {
        pEnd ++;
    }
    lib_memcpy(pEnd, filename, sizeof(filename));
    
    printf("\tcheck %s content\n", pFileName);
    iFd = open(pFileName, O_RDONLY, mode);   /*  �����Դ��� */ 
    if (iFd < 0) {
        printf("\"%s\" does not exist!\n",pFileName);
        ERROR_FLAG = 1;
        return ;
    }    
    /* �鿴�ļ����� "Hello hoitfs" */

    for(i=0; i < writetimes ; i++) {        
        writebytes = read(iFd, fileread, sizeof(filewrite));
        if (writebytes == 0) {
            printf("\"%s\" is empty!\n");
            ERROR_FLAG = 1;
            return ;            
        }
        if (lib_strcmp(fileread, filewrite) != 0) {
            printf("\"%s\" read content is not correct!\n");
        }
        else {
            API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
            printf("\tcheck %s ok\n", pFileName);
            API_TShellColorEnd(STD_OUT);
        }
    }

    close(iFd);
    lib_memset(pEnd, 0, sizeof(filename));
//    lib_memcpy(pEnd, zero, sizeof(zero));
}
/*
    ��鴴�����ļ��Ƿ���ڣ������Ƿ�һ��
    ���Ŀ¼�Ƿ����
*/
void FileTreeTestCheck(PUCHAR pFileName) {
    UINT    i;
    /* ����ļ� */
    for (i=0 ; i<max_fileNo ; i++) {
        checkFile(pFileName, i);
        if (ERROR_FLAG == 1) {
            return;
        }
    }
 
    /* ���Ŀ¼ */
    for (i=0 ; i<max_dirNo ; i++) {
        checkDir(pFileName, i);
        if (ERROR_FLAG == 1) {
            return;
        }        
    }    
}

INT hoitTestFileTree (INT  iArgC, PCHAR  ppcArgV[])
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
INT hoitTestFileOverWrite (INT  iArgC, PCHAR  ppcArgV[]) {
    UCHAR   filename[30]     = "/mnt/hoitfs/OverWriteTest\0";
    UCHAR   readData[1024+256];
    INT     iFd;
    UCHAR   data    = '1';
    INT     i;
    printf("===========  File Overwrite Test!       ===========\n");
    iFd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_PERM);   /*  �����Դ��� */ 
    lib_memset(readData, 0, sizeof(readData));
    /* ��ʼд��512��'1' */
    for (i=0 ; i<64 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n",readData);

    /* �ر��ļ��ٴ򿪣�׷��д256��'2' */
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

    /* �����޸�ǰ256����Ϊ"3" */
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

    /* ��512λ��ʼд512��'4' */
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

    /* ��1024λ��ʼ�м��128�ֽڣ�Ȼ��д128��'5' */
//    data ++;
//    close(iFd);
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

    data ++;
    close(iFd);
    /* �ڿ�ͷд��16��x */
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    lseek(iFd, 0, SEEK_SET);
    write(iFd, "xxxxxxxxxxxxxxxx", 16);
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n", readData);

    data ++;
    close(iFd);
    /* ��ǰ8��x����y */
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    lseek(iFd, 0, SEEK_SET);
    write(iFd, "yyyyyyyy", 8);
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n", readData);
    close(iFd);

    printf("\n");
    printf("===========  File Overwrite Test End!    ===========\n");
    return  ERROR_NONE;
}

/*********************************************************************************************************
 * GC����
*********************************************************************************************************/
#define FILE_MODE                       (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define BIG_FILE                        "RealBigFiles"
#define FILE_SIZE                       64 * 1024
INT hoitTestGC(PHOIT_VOLUME pfs){
    INT   iFd;
    UINT  i, j;
    UINT  uiSizeWritten;
    PCHAR pcWriteBuffer;
    
    iFd = open(BIG_FILE, O_RDWR | O_CREAT | O_TRUNC, FILE_MODE);
    if(iFd < 0){
        printf("[Create " BIG_FILE "Fail]");
        return;
    }

    uiSizeWritten = 0;
    /* д�� 64 * 1024B */
    for (i = 0; i < 64; i++)
    {
        pcWriteBuffer = (PCHAR)lib_malloc(26 * 1024);
        printf("start cycle %d \n", i);
        for (j = 0; j < 26 * 1024; j++)
        {
            *(pcWriteBuffer + j) = 'a';
        }
        write(iFd, pcWriteBuffer, 26 * 1024);
        uiSizeWritten += 26;
        printf("write cycle %d ok, %dKB has written, now sector is %d\n" , i, uiSizeWritten, 
                pfs->HOITFS_now_sector->HOITS_bno);
        lib_free(pcWriteBuffer);
    }
    
    API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
    printf("Write BigFile OK \n");
    API_TShellColorEnd(STD_OUT);

    close(iFd);
}
