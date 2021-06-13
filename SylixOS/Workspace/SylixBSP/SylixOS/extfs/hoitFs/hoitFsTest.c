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
extern PHOIT_VOLUME _G_Volumn;

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
/*
** ��������:    hoitTestFileTree
** ��������:    �ļ�������
** �䡡��  :
** �䡡��  :    ���Գɹ�����ERROR_NONE��ʧ�ܷ���PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*/
INT hoitTestFileTree (INT  iArgC, PCHAR  ppcArgV[])
{
    UCHAR       filename[512];
    UINT        temp;
    lib_memset(filename, 0 ,sizeof(filename));
    lib_strcpy(filename, "/mnt/hoitfs/\0");

    if (iArgC != 2 && iArgC != 4) {
        fprintf(STD_ERR, "arguments error!\n");
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
            fprintf(STD_ERR, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
    } else if (iArgC == 4) {
        temp = lib_atoi(ppcArgV[1]);
        if (temp < max_depth)
            max_depth = temp;

        temp = lib_atoi(ppcArgV[2]);
        if (temp < max_fileNo)
            max_fileNo = temp;

        temp = lib_atoi(ppcArgV[3]);
        if (temp < max_dirNo)
            max_dirNo = temp;

    } else {
            fprintf(STD_ERR, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
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
    UCHAR   writeData[1024+256];
    UCHAR   readData[1024+256];
    INT     iFd;
    UCHAR   data    = '1';
    INT     i;

    lib_memset(writeData, 0, sizeof(writeData));

    printf("===========  File Overwrite Test!       ===========\n");
    iFd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_PERM);   /*  �����Դ��� */
    lib_memset(readData, 0, sizeof(readData));
    /* ��ʼд��64��'1' */
    for (i=0 ; i<64 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
        writeData[i] = data;
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n",readData);
    if (lib_strcmp(writeData,readData) == 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        printf("\tcheck No.%c ok\n", data);
        API_TShellColorEnd(STD_OUT);
    } else {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("\tcheck No.%c failed\n", data);
        API_TShellColorEnd(STD_OUT);
        goto __fot_end;
    }

    /* �ر��ļ��ٴ򿪣�׷��д36��'2' */
    data ++;
    close(iFd);
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    for (i=64 ; i<96 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
        writeData[i] = data;
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n",readData);
    if (lib_strcmp(writeData,readData) == 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        printf("\tcheck No.%c ok\n", data);
        API_TShellColorEnd(STD_OUT);
    } else {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("\tcheck No.%c failed\n", data);
        API_TShellColorEnd(STD_OUT);
        goto __fot_end;
    }

    /* �����޸�ǰ32����Ϊ"3" */
    data ++;
    close(iFd);
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    for(i=0 ; i<32 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
        writeData[i] = data;
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n",readData);
    if (lib_strcmp(writeData,readData) == 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        printf("\tcheck No.%c ok\n", data);
        API_TShellColorEnd(STD_OUT);
    } else {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("\tcheck No.%c failed\n", data);
        API_TShellColorEnd(STD_OUT);
        goto __fot_end;
    }

    /* ��64λ��ʼд64��'4' */
    data ++;
    close(iFd);
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    for(i=64 ; i<128 ; i++) {
        lseek(iFd, (i)*sizeof(CHAR), SEEK_SET);
        write(iFd, &data, sizeof(CHAR));
        writeData[i] = data;
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n",readData);
    if (lib_strcmp(writeData,readData) == 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        printf("\tcheck No.%c ok\n", data);
        API_TShellColorEnd(STD_OUT);
    } else {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("\tcheck No.%c failed\n", data);
        API_TShellColorEnd(STD_OUT);
        goto __fot_end;
    }
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
    for (i=0 ; i<16; i++) {
        writeData[i] = 'x';
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n", readData);
    if (lib_strcmp(writeData,readData) == 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        printf("\tcheck No.%c ok\n", 'x');
        API_TShellColorEnd(STD_OUT);
    } else {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("\tcheck No.%c failed\n", 'x');
        API_TShellColorEnd(STD_OUT);
        goto __fot_end;
    }

    data ++;
    close(iFd);
    /* ��ǰ8��x����y */
    iFd = open(filename, O_WRONLY, DEFAULT_FILE_PERM);
    lseek(iFd, 0, SEEK_SET);
    write(iFd, "yyyyyyyy", 8);
    for (i=0 ; i<8; i++) {
        writeData[i] = 'y';
    }
    printf("\nNo.%c result:\n", data);
    close(iFd);
    iFd = open(filename, O_RDONLY, DEFAULT_FILE_PERM);
    read(iFd, readData, sizeof(readData));
    printf("%s\n", readData);
    close(iFd);
    if (lib_strcmp(writeData,readData) == 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        printf("\tcheck No.%c ok\n", 'y');
        API_TShellColorEnd(STD_OUT);
    } else {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("\tcheck No.%c failed\n", 'y');
        API_TShellColorEnd(STD_OUT);
        goto __fot_end;
    }

__fot_end:
    printf("\n");
    printf("===========  File Overwrite Test End!    ===========\n");
    return  ERROR_NONE;
}

/*********************************************************************************************************
 * ��Ӳ���Ӳ���
*********************************************************************************************************/
/*
    ��ȡiFd���ݣ���write_data�ȶ�
*/
INT checkData(INT iFd, UCHAR write_data[128]){
    UCHAR read_data[128];
    /* �ȶ�ȡ�ļ��鿴�����Ƿ�һ�� */
    lib_bzero(read_data, sizeof(read_data));
    read(iFd, read_data, sizeof(read_data));
    if (lib_memcmp(read_data, write_data, sizeof(write_data))!= 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("data is not consist\n");
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }
    return ERROR_NONE;
}

void checkOK(char const* const _Format) {
    API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
    printf("%s check ok\n",_Format);
    API_TShellColorEnd(STD_OUT);
}

/*
    ��Ӳ���Ӳ����������£�
    step 1: ����Դ�ļ�
        ��AĿ¼�£�����Դ�ļ���д��ԭʼ����
    step 2: ������Ӳ����
        ��BĿ¼�£�����Դ�ļ���������
        ��BĿ¼�¡�����Դ�ļ���Ӳ����
    step 3: ��������д
        ͨ�������Ӷ�ȡԴ�ļ����ݣ������ȷ�ԣ���д�������ݣ�ͨ��Դ�ļ���ȡ������
        ͨ��Ӳ���Ӷ�ȡԴ�ļ����ݣ������ȷ�ԣ���д�������ݣ�ͨ��Դ�ļ���ȡ������
    step 4: ��鴫������
        �������Ӵ��������ӣ�ͨ���μ������Ӷ�ȡ���ݡ�
    step 5:���ɾ�����Ӷ�Դ�ļ���Ӱ��
        ɾ�������ӣ��ٴ�Դ�ļ���ȡ����
        ɾ��Ӳ���ӣ��ٴ�Դ�ļ���ȡ����
    step 6:��鲻ͬ�ļ�ϵͳ֮���Ƿ���Խ����������ļ���
        ��ramfs�´���ָ��hoitfs��Դ�ļ��������ӣ���ȡ�ļ����ݡ�
        ����ramfs����ramfs�´������ļ�����hoitfs����ָ����������ӣ���ȡ�ļ����ݡ�
*/
INT hoitTestLink (INT  iArgC, PCHAR  ppcArgV[]) {
    UCHAR   file_name[64]           = "/mnt/hoitfs/A/Test_LinkSourceFile\0";
    UCHAR   file_temp[64]           = "A/Test_LinkSourceFile\0";
    UCHAR   outer_file_name[64]     = "/mnt/ramfs/Test_OuterFile\0";    /* hoitfs������ָ��ramfs��Ŀ���ļ� */
    UCHAR   file_dir[64]            = "/mnt/hoitfs/A\0";

    UCHAR   hard_link[64]           = "B/Test_HardLinkFile\0";
    UCHAR   soft_link[64]           = "/mnt/hoitfs/B/Test_SoftLinkFile\0";
    UCHAR   second_soft_link[64]    = "/mnt/hoitfs/B/Test_SoftLinkFile2\0";
    UCHAR   inner_link[64]          = "/mnt/hoitfs/B/Test_InnerLinkFile\0"; /* ��hoitfsָ��ramfs�������� */
    UCHAR   outer_link[64]          = "/mnt/ramfs/Test_OuterLinkFile\0";    /* ��ramfsָ��hoitfs�������� */

    UCHAR   link_dir[64]            = "/mnt/hoitfs/B\0";

    mode_t  dir_mode                = DEFAULT_DIR_PERM & (~S_IFMT);
    mode_t  file_mode               = DEFAULT_FILE_PERM;

    UCHAR   write_data[128];
    UCHAR   read_data[128];
    UCHAR   data = 'x';
    INT     iFd;
    INT     i;

    /* ����Ŀ¼ */
    iFd = open(file_dir, O_RDWR | O_CREAT | O_EXCL, S_IFDIR | dir_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("create \" %s \" error!\n", file_dir);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }
    close(iFd);
    iFd = open(link_dir, O_RDWR | O_CREAT | O_EXCL, S_IFDIR | dir_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("create \" %s \" error!\n", link_dir);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }
    close(iFd);

    /******************************* step1 *******************************/
    iFd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("create \" %s \" error!\n", file_name);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    /* д128�� "x" */
    for (i=0 ; i<128 ; i++) {
        write(iFd, &data, sizeof(data));
        write_data[i] = data;
    }
    close(iFd);

    /******************************* step2 *******************************/
    /* ������ */
    if (symlink(file_name, soft_link) != ERROR_NONE) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("soft link creating failed!\n");
        API_TShellColorEnd(STD_OUT);
    }

    /* Ӳ���� */
    if (_G_Volumn == LW_NULL) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("can't get the hoitfs header!\n");
        API_TShellColorEnd(STD_OUT);
    }
    if (__hoitFsHardlink(_G_Volumn, hard_link, file_temp) != ERROR_NONE) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("hard link creating failed!\n");
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    /******************************* step3 *******************************/
    /******************************* ������ *******************************/
    printf("check soft link read...\n");
    iFd = open(soft_link, O_RDWR, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", soft_link);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }
    /* �ȶ�ȡ�ļ��鿴�����Ƿ�һ�� */
    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }
    checkOK("soft link read");

    /* �ļ�ǰ�벿'x'��Ϊ'y' */
    printf("check soft link write...\n");
    lseek(iFd, 0, SEEK_SET);
    data = 'y';
    for (i=0; i< 64 ; i++) {
        write(iFd, &data, sizeof(UCHAR));
        write_data[i] = data;
    }
    close(iFd);

    /* ͨ��Դ�ļ���� */
    iFd = open(file_name, O_RDONLY, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", file_name);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }
    close(iFd);
    checkOK("soft link write");

    /******************************* Ӳ���� *******************************/
    printf("check hard link read...\n");
    iFd = open(hard_link, O_RDWR, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", hard_link);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    /* �ȶ�ȡ�ļ��鿴�����Ƿ�һ�� */
    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }
    checkOK("hard link read");

    /* �ļ���벿'x'��Ϊ'z' */
    printf("check hard link write...\n");
    lseek(iFd, 0, SEEK_SET);
    data = 'z';
    for (i=0; i< 64 ; i++) {
        write(iFd, &data, sizeof(UCHAR));
        write_data[i] = data;
    }
    close(iFd);

    /* ͨ��Դ�ļ���� */
    iFd = open(file_name, O_RDONLY, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", file_name);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }
    close(iFd);
    checkOK("hard link write");

    /******************************* step4 *******************************/
    /***************************** ���������� ****************************/
    printf("check passing check link read...\n");
    if (symlink(soft_link, second_soft_link) != ERROR_NONE) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("second soft link creating failed!\n");
        API_TShellColorEnd(STD_OUT);
    }
    iFd = open(second_soft_link, O_RDWR, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", second_soft_link);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }
    /* �ȶ�ȡ�ļ��鿴�����Ƿ�һ�� */
    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }

    checkOK("passing link read");

    /* �ļ�����ȫ�ĳ�'0' */
    printf("check passing check link write...\n");
    lseek(iFd, 0, SEEK_SET);
    data = '0';
    for (i=0; i< 128 ; i++) {
        write(iFd, &data, sizeof(UCHAR));
        write_data[i] = data;
    }
    close(iFd);

    /* ͨ��Դ�ļ���� */
    iFd = open(file_name, O_RDONLY, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", file_name);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }
    close(iFd);
    checkOK("passing link write");

    /******************************* step5 *******************************/
    /*********************** ɾ�������Ӻʹ��������� ***********************/
    printf("delete soft link...\n");
    if(unlink(second_soft_link)==PX_ERROR) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("delete soft link failed!\n");
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }
    if(unlink(soft_link)==PX_ERROR) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("delete soft link failed!\n");
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    /* ͨ��Դ�ļ���� */
    printf("check soft link deleting influence...\n");
    iFd = open(file_name, O_RDONLY, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", file_name);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }
    close(iFd);
    checkOK("");
    /***************************** ɾ��Ӳ���� ****************************/
    printf("delete hard link...\n");
    if (unlink(hard_link)==PX_ERROR) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("delete hard link failed!\n");
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    /* ͨ��Դ�ļ���� */
    printf("check hard link deleting influence...\n");
    iFd = open(file_name, O_RDONLY, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", file_name);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }
    close(iFd);
    checkOK("");
    /******************************* step6 *******************************/
    /* ����ramfs */
    printf("mount ramfs...\n");
    if (API_MountEx("10000","/mnt/ramfs\0","ramfs\0",LW_NULL)!=ERROR_NONE) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("mount ramfs failed!\n");
        API_TShellColorEnd(STD_OUT);
        return (0);
    }
    API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
    printf("mount ramfs OK\n");
    API_TShellColorEnd(STD_OUT);

    /* ��ramfs��ָ��hoitfs�������� */
    if (symlink(file_name, outer_link) != ERROR_NONE) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("outer link creating failed!\n");
        API_TShellColorEnd(STD_OUT);
    }

    printf("check outer link...\n");
    iFd = open(outer_link, O_RDWR, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", outer_link);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

    if (checkData(iFd, write_data)!=ERROR_NONE) {
        close(iFd);
        return PX_ERROR;
    }
    close(iFd);
    checkOK("outer link");

    /* ����hoitfsָ��ramfs�������� */
    /* �����ļ� */
    iFd = open(outer_file_name, O_WRONLY | O_CREAT | O_TRUNC, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("create \" %s \" error!\n", outer_file_name);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }
    for (i=0 ; i<128 ; i++) {
        write(iFd, &write_data[i], sizeof(data));
    }
    close(iFd);

    if (symlink(outer_file_name, inner_link) != ERROR_NONE) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("inner link creating failed!\n");
        API_TShellColorEnd(STD_OUT);
    }
    printf("check inner link...\n");
    iFd = open(inner_link, O_RDONLY, file_mode);
    if (iFd < 0) {
        API_TShellColorStart2(LW_TSHELL_COLOR_RED, STD_OUT);
        printf("open \" %s \" error!\n", inner_link);
        API_TShellColorEnd(STD_OUT);
        return PX_ERROR;
    }

   if (checkData(iFd, write_data)!=ERROR_NONE) {
       close(iFd);
       return PX_ERROR;
   }
    close(iFd);
    checkOK("");
    /* ɾ���ļ������� */
    unlink(outer_link);
    unlink(outer_file_name);
    unlink(inner_link);
    unlink(file_name);
}


/*********************************************************************************************************
 * GC����
*********************************************************************************************************/
#define FILE_MODE                       (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define BIG_FILE                        "RealBigFiles"
#define FILE_SIZE                       60 * 1024
#define PER_SIZE                        1024

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
    for (i = 0; i < FILE_SIZE / PER_SIZE; i++)
    {
        pcWriteBuffer = (PCHAR)lib_malloc(PER_SIZE);
        printf("start cycle %d \n", i);
        for (j = 0; j < PER_SIZE; j++)
        {
            *(pcWriteBuffer + j) = 'a';
        }
        write(iFd, pcWriteBuffer, PER_SIZE);
        uiSizeWritten += (PER_SIZE / 1024);
        printf("write cycle %d ok, %dKB has written, now sector is %d\n" , i, uiSizeWritten,
                pfs->HOITFS_now_sector->HOITS_bno);
        lib_free(pcWriteBuffer);
    }

    API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
    printf("Write BigFile OK \n");
    API_TShellColorEnd(STD_OUT);

    close(iFd);
}
