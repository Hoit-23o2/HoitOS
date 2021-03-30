#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
//#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"
//#include "../SylixOS/kernel/inline/inlErrorHandle.h"
//#include "../SylixOS/system/logLib/logLib.h"


/*********************************************************************************************************
  ����ͷ�ļ�
*********************************************************************************************************/
#include "lFsLib.h"


/*********************************************************************************************************
  LFSȫ�ֱ���
*********************************************************************************************************/
/* ÿ��block��һ��summary secotr����һ��Ϊinode�ţ��ڶ���Ϊ��Чλ */
static UINT                             BLOCK_SUMMARY[NOR_FLASH_SECTPBLK*NOR_FLASH_NBLK][2]; 

/* ��sectorΪ��λ�Ļ����� */  
static CHAR                              SEC_BUF[NOR_FLASH_SECTORSZ];

/* Block Buffer */
static CHAR                             BLK_BUF[NOR_FLASH_BLKSZ];

/* 
    CHECKPOINT_REGION�����ڴ���ά����umountʱ��д��
    ��һ�����ֱ�ʾimap����sector���ڶ�����ʾblock��ǰ����sector
 */
static CHECKPOINT_REGION                CPR;

/* IMAPҲ���ڴ���ά�� */
static UINT                             IMAP[MAX_FILE+1];

/* ��ǰBlock����Χ(2~31)��0��1��ռ�� */
static UINT                             BLOCK_NO = 2;   

/* �ڵ�ǰBLOCK_NO�У���һ������sector����Χ(0~15) */
static UINT                             AVAILABLE_SECTOR = 0;

/* �鿴�Ŀ�BLOCK�Ƿ������ */
static UINT8                            CLEAN_BLOCK[NOR_FLASH_NBLK];

/*********************************************************************************************************
  LFS��ʼ��
*********************************************************************************************************/
/*
    ��ʼ��ÿ��block���Լ�summary sector
*/
VOID initBlocks(){
    UINT i;
    UINT neg1 = -1;
    UINT summary_sec_offset;
    lib_bzero(SEC_BUF,sizeof(SEC_BUF));
    /* CPR_FN_1_BLK �� CPR_FN_2_BLK ��������summary sector���洦�� */
    for (i = 3; i < NOR_FLASH_NBLK; i++)
    {
        /* ������NOR_FLASH_BUF��ֱ��д��flash */
        write_nor(GET_NOR_OFFSET_FROM_BLKN(i),
                    (PCHAR)SEC_BUF,
                    NOR_FLASH_SECTORSZ);
    }
    /* summary sector �� ����¼summary sector����һ���ֳ�ʼ�� */
    summary_sec_offset = GET_OFFSET_FROM_BLK_AND_SECN(2,0);
    SEC_BUF[summary_sec_offset] = SUMMARY_SUMMARY_TYPE;
    SEC_BUF[summary_sec_offset+1] = neg1;
    write_nor(GET_NOR_OFFSET_FROM_BLKN(2),
                (PCHAR)SEC_BUF,
                NOR_FLASH_SECTORSZ);   
}

/*
    ��ʼ��CPR_FNMap���򣬼�CheckpointRegion_FileNameMap��������������block���档
    CPR.imap_sec����ǰimap����sector��ռ4B
    CPR.summary_sec����ǰsummary sector����λ�ã�ռ4B
    CPR.free_sec����ǰblock����sector��ռ32B
        block1              block2
    +---------------+   +---------------+
    |     CPR       |   | File511 name  |
    +---------------+   +---------------+
    |  File0 name   |   |      ...      |
    +---------------+   +---------------+
    |     ...       |   |      ...      |
    +---------------+   +---------------+
    |  File510 name |   | File1022 name |
    +---------------+   +---------------+
*/
VOID initCPR_FNMap()
{
    UINT i;
    CHAR buf[NOR_FLASH_BLKSZ];
    lib_bzero(buf,NOR_FLASH_BLKSZ);
    UINT neg1 = -1;
    UINT temp_sec = 2*NOR_FLASH_SECTPBLK;
    PCHAR temp_p = (PCHAR)&temp_sec;
    /* CPR.imap_sec��ʼ��Ϊ-1 */
    /* CPR.summary_sec��ʼ��Ϊ 2*NOR_FLASH_SECTPBLK��������block�ĵ�һ��secotr */
    /* CPR.free_sec �Լ�FileNameȫ��ʼ��Ϊ0 */ 
    for ( i = 0; i < 4; i++)
        buf[i] = neg1;

    for ( i = 0; i < 4; i++)
        buf[i+4] = *(temp_p+i);

    /* д��flash */
    write_nor(GET_NOR_OFFSET_FROM_BLKN(CPR_FN_1_BLK),
                (PCHAR)buf,
                NOR_FLASH_BLKSZ);

    lib_bzero(buf,NOR_FLASH_BLKSZ);
    write_nor(GET_NOR_OFFSET_FROM_BLKN(CPR_FN_2_BLK),
                (PCHAR)buf,
                NOR_FLASH_BLKSZ);

}

/*
    ��ʼ��CleanBlock
*/
VOID initCleanBlock()
{
    lib_bzero(CLEAN_BLOCK,NOR_FLASH_NBLK);
}

/*
    TODO:��ʼ��LFS�ĸ��ֻ�����
*/
INT initLFS(PLFS_VOLUME plfs)
{
    return ERROR_NONE;
}

/*
    ��ȡCPR���ڴ�
*/
INT readInCheckpointRegion(PLFS_VOLUME plfs)
{
    UINT                i;
    read_content_t      content;
    PCHECKPOINT_REGION  CPR;
    PCHAR               temp;
    CPR = plfs->LFS_cpr;

    content = read_nor(GET_NOR_OFFSET_FROM_BLKN(CPR_FN_1_BLK),READ_SECTOR);
    if (content.is_success)
    {
        /* ��ȡCPR.imap_sec */
        temp = (PCHAR)CPR;
        // for ( i = 0; i < 3; i++)
        // {
        //     *(temp+i)=content.content[i];
        // }

        // /* ��ȡCPR.summary_sector_no */

        // for ( i = 0; i < 3; i++)
        // {
        //     *(temp+i)=content.content[i+4];
        // }

        // /* ��ȡCPR.free_sec */
        // temp = (CHAR*)CPR.free_sec;
        // for ( i = 0; i < 32; i++)
        // {
        //     *(temp+i) = content.content[i+8];       /* ����CPRǰ������� */
        // }
        for ( i = 0; i < sizeof(CHECKPOINT_REGION); i++)
        {
            *(temp+i) = content.content[i];
        }
        

        return ERROR_NONE;
    }
    else
    {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "nor flash read failed .\r\n");
        _ErrorHandle(EFAULT); 
        return PX_ERROR;
    }
}

/*
    ����һ������Block���ѽ��������ȫ�ֱ���BLOCK_NO��AVAILABLE_SECTOR��
*/
INT findNextAvailableSector()
{
    if(CPR.imap_sec!=(UINT)-1)
    {
        BLOCK_NO = GET_BLKN_OF_SECN(CPR.imap_sec);
        AVAILABLE_SECTOR = GET_SEC_IDX_IN_BLK(CPR.imap_sec);
        return ERROR_NONE;
    }
    else
    {
        return PX_ERROR;
    }
}

/* 
    ��ȡ��ǰsummary sector��BLOCK_SUMMARY��
 */
INT readSummary()
{
    read_content_t content;
    content = read_nor(GET_NOR_OFFSET_FROM_SECN(CPR.summary_sec),READ_SECTOR);
    if(content.is_success)
    {
        lib_memcpy(BLOCK_SUMMARY,content.content,NOR_FLASH_SECTORSZ);
        return ERROR_NONE;
    }
    else
        return PX_ERROR;
}

/*
    ��ȡ��ǰsector��SEC_BUF��
    @sector_no ȫ��sector��
*/
INT readInSector(PLFS_VOLUME plfs, UINT sector_no)
{
    read_content_t content;
    UINT    i = GET_NOR_OFFSET_FROM_SECN(sector_no);
    if(sector_no<0||sector_no>NOR_FLASH_NSECTOR)
        return PX_ERROR;

    content = read_nor(GET_NOR_OFFSET_FROM_SECN(sector_no),READ_SECTOR);
    if(content.is_success)
    {
        lib_memcpy((PCHAR)plfs->LFS_secBuf,
                        content.content,
                        NOR_FLASH_SECTORSZ);
        return ERROR_NONE;
    }
    else
        return PX_ERROR; 
}

/*
    ����ǰblock��BLK_BUF��
    @blk_no block��
    modified
*/
INT readInBlock(PLFS_VOLUME plfs, UINT blk_no)
{
    UINT i;
    read_content_t content;
    /* һ��sectorһ��sector�ض�ȡ */
    for ( i = 0; i < NOR_FLASH_SECTPBLK; i++)
    {
        content = read_nor(GET_OFFSET_FROM_BLK_AND_SECN(blk_no,i),READ_SECTOR);
        if(content.is_success)
        {
            lib_memcpy((PCHAR)&plfs->LFS_blkBuf[i*NOR_FLASH_SECTORSZ],
                            content.content,
                            NOR_FLASH_SECTORSZ);
        }
        else
            return PX_ERROR;        
    }
    return ERROR_NONE;
}

/*
    ��imap sector��IMAP��
*/
UINT readInImapSector(UINT address, UINT fragment_no)
{
    read_content_t content;
    content = read_nor(GET_NOR_OFFSET_FROM_SECN(CPR.imap_sec),READ_SECTOR);
    if(content.is_success)
    {
        lib_memcpy(IMAP,content.content,NOR_FLASH_SECTORSZ);
        return ERROR_NONE;
    }
    else
        return PX_ERROR;    
}

/* 
    ��дCheckoutPoint Region 
*/
INT updateCheckpointRegion(PLFS_VOLUME plfs)
{
    UINT i;
    /* ��ΪCPRֻռblock��һ���֣������ȰѸ�sector������ */
    read_content_t content;
    content = read_nor(GET_NOR_OFFSET_FROM_BLKN(CPR_FN_1_BLK),READ_SECTOR);
    if(content.is_success)
    {
        lib_memcpy(SEC_BUF,content.content,NOR_FLASH_SECTORSZ);
        return ERROR_NONE;
    }
    else
        return PX_ERROR;
    
    /* CPRд��buf */
    PCHAR temp = (PCHAR)plfs->LFS_cpr;
    for ( i = 0; i < sizeof(CHECKPOINT_REGION); i++)
    {
        plfs->LFS_secBuf[i] = *(temp+i);
    }

    /* д��flash */   
    write_nor(GET_NOR_OFFSET_FROM_BLKN(CPR_FN_1_BLK),
                (PCHAR)plfs->LFS_secBuf,
                NOR_FLASH_SECTORSZ);
}

/* 
    Ѱ����һ�����е�Segment����CPR�в���
    ����segment��block�е�λ�û�-1
    modified
*/
UINT findNextCleanBlock(PLFS_VOLUME plfs)
{
    UINT i;
    for (i = 2; i < NOR_FLASH_NBLK; i++)
    {
        if ((plfs->LFS_cpr)->free_sec[i]==BLK_CLEAN)
        {
            //updateCheckpointRegion(plfs);
            return i;
        } 
    }
    //_DebugHandle(__ERRORMESSAGE_LEVEL, "No available memory remaining.\r\n");
    return (PX_ERROR);
}

/* 
    ����ǰSEC_BUF����д��LFS_curBlockNo��sector�У�������LFS_availableSector
    �����block���һ��sector����Ҫ����һ����
    ����д���sector��ȫ�����
*/
UINT writeOutSector(PLFS_VOLUME plfs)
{
    UINT next_block_no;
    UINT sector_to_write;
    write_nor(GET_OFFSET_FROM_BLK_AND_SECN(plfs->LFS_curBlockNo,
                                            plfs->LFS_availableSector),
                (PCHAR)plfs->LFS_secBuf,
                NOR_FLASH_SECTORSZ);
    sector_to_write = plfs->LFS_curBlockNo*NOR_FLASH_SECTPBLK+plfs->LFS_availableSector;
    /* �ɹ�д��sector��Ҫ��ʱ����CPR�͵�ǰ����sector�� */
    plfs->LFS_availableSector++;
    (plfs->LFS_cpr)->free_sec[plfs->LFS_curBlockNo] = plfs->LFS_availableSector;

    if(plfs->LFS_availableSector==NOR_FLASH_SECTPBLK) {
        next_block_no = findNextCleanBlock(plfs);
        if (next_block_no == PX_ERROR)
        {
            printk("lfs: all blocks have been dirty.\r\n");
            plfs->LFS_availableSector = 0; 
            return PX_ERROR;

        } 

        plfs->LFS_curBlockNo = next_block_no;
        plfs->LFS_availableSector = (plfs->LFS_cpr)->free_sec[next_block_no];   
        
    }
    return sector_to_write;
}


/* 
    ����ǰBLK_BUF����д��BLOCK_NOָ����block��
    modified
*/
VOID writeOutBlock(PLFS_VOLUME plfs, UINT block_no)
{
    UINT next_block_no;
    write_nor(GET_NOR_OFFSET_FROM_BLKN(block_no),
                (PCHAR)plfs->LFS_blkBuf,
                NOR_FLASH_BLKSZ);
    
    if(block_no == CPR_FN_1_BLK || block_no == CPR_FN_2_BLK) {
        /* ���ֻ���޸�FileNameMap�������飬�Ͳ���Ҫ������Ϊ�࣬Ҳ����Ҫ����һ���ɾ��� */
        return;
    }

    plfs->LFS_cleanBlock[block_no] = BLK_DIRTY;
    (plfs->LFS_cpr)->free_sec[block_no] = 0;        /* д���block����free_sector����Ϊ0 */

    next_block_no = findNextCleanBlock(plfs);
    if (next_block_no == PX_ERROR)
    {
        printk("lfs: all blocks have been dirty.\r\n");
        plfs->LFS_availableSector = 0; 
        return ;

    } 
    
    /* 
        д��blockһ���Ǹ�block���ˣ�����Ҫ����һ��block��
        ���Ǿ�Ҫ��AVAILABLE_SECTOR����Ϊ0��
     */
    plfs->LFS_curBlockNo = next_block_no;
    plfs->LFS_availableSector = (plfs->LFS_cpr)->free_sec[next_block_no];   
}

/*
    Ѱ����һ��Inode Number��ͨ��FileMap������
    modified
*/
UINT nextInodeNumber(PLFS_VOLUME plfs)
{
    UINT i;
    /* �Ȳ���CPR_FN_1_BLK */
    readInBlock(plfs, CPR_FN_1_BLK);
    for (i = 1; i < FN_ENTRY_PBLK; i++)
    {
        if(!(plfs->LFS_blkBuf[i*FN_ENTRY_SZ]))
            return (i-1);
    }
    /* �ٲ���CPR_FN_2_BLK */
    readInBlock(plfs, CPR_FN_2_BLK);
    for (i = 0; i < FN_ENTRY_PBLK; i++)
    {
        if((!plfs->LFS_blkBuf[i*FN_ENTRY_SZ]))
            return (i+511);
    }
    return UINT_MAX;
}

/* 
    ��LFS����µ�inode�����inode���ڣ������imap����Ӧ��λ�á�
    modified
*/
INT writeInode(PLFS_VOLUME plfs, const PLFS_INODE plfsn, UINT inode_no)
{
    UINT inode_in_sector;                                           /* inode��sector�е�λ�� */
    UINT global_inode_sector_no;                                    /* inode��sector��ȫ��λ�� */
    UINT global_inode_sector_no_last = plfs->LFS_imap[inode_no];   /* ���inode֮ǰ���ڣ����ʾ��һ�δ��ڵ�sector�� */
    //inode д��sector
    inode_in_sector = inode_no%INODE_PSEC;

    /* ���inode���ڣ�summary����Чλ��Ϊ-1����ʾ��Ч����summary��imap��ͬ�� */
    if(global_inode_sector_no_last!=(UINT)-1) {
        plfs->LFS_blockSummary[global_inode_sector_no_last][0] = (UINT)-1;
    }


    lib_memcpy(&(plfs->LFS_secBuf[inode_in_sector*sizeof(LFS_INODE)]),
                (PCHAR)plfsn,
                sizeof(LFS_INODE));
    global_inode_sector_no = writeOutSector(plfs);

    if (global_inode_sector_no == (UINT)-1)
    {
        return (PX_ERROR);
    }
    

    /* ���µ�ǰ block ��Ӧ�� summary */
    plfs->LFS_blockSummary[global_inode_sector_no][0] = inode_no;
    plfs->LFS_blockSummary[global_inode_sector_no][1] = SUMMARY_INODE_TYPE;

    /* ����imap */
    if(updateImap(plfs,inode_no,global_inode_sector_no)<0) {
        return (PX_ERROR);
    }

    /* �޸�FileNameMap */
    if(updateFilemap(plfs,inode_no,plfsn->LFSN_pcname)<0) {
        return (PX_ERROR);
    }
    
    /* ����summary sector */
    if(updateSummary(plfs)<0) {
        return (PX_ERROR);
    }

    /* ����CPR */
    if(updateCheckpointRegion(plfs)<0) {
        return (PX_ERROR);
    }
    return (ERROR_NONE);
}

/* ����Filemap
  @inode_number��inode��
  @lfs_filename�������ļ�����ΪNULLʱ����ɾ�����ļ���

  ��Ϊÿ��Filemap�ͱ�����Valid Byte��Name Bytes
    modified
*/
INT updateFilemap(PLFS_VOLUME plfs, UINT inode_number, PCHAR lfs_filename)
{
    UINT    i;
    PCHAR   temp;
    BOOL    bRemove = (lfs_filename)?LW_FALSE:LW_TRUE;
    /* ����inode_number������ȡ��һ��block */
    if(inode_number<511&&inode_number>=0)
    {
        readInBlock(plfs, CPR_FN_1_BLK);
        temp = &(plfs->LFS_blkBuf[(inode_number+1)*FN_ENTRY_SZ]);
        i = CPR_FN_1_BLK;
    }
    else if(inode_number>=511&&inode_number<MAX_FILE)
    {
        readInBlock(plfs, CPR_FN_2_BLK);
        temp = &(plfs->LFS_blkBuf[(inode_number-511)*FN_ENTRY_SZ]);
        i = CPR_FN_2_BLK;
    }
    else
    {
        return PX_ERROR;
    }
    if(bRemove == LW_FALSE) {
        *temp = 1;
        temp++;
        lib_strcpy(temp, lfs_filename);        
    } else {
        *temp = 0;
    }

    writeOutBlock(plfs,i);
    return ERROR_NONE;
}

/*
    ���µ�imapд���µ�sector�����޸��ڴ���block summary
    ����CPR.imap_sec
    @inode_number��     inode���
    @sector_position��  imap�����µ�sectorȫ�����
    ������Ϊһ��sector�б�����16��inode�����޸�һ�������п���Ҫ�޸�����15����inodeλ��
    modified
*/
INT updateImap(PLFS_VOLUME plfs, UINT inode_number, UINT sector_position)
{
    UINT    old_imap_sector         = (plfs->LFS_cpr)->imap_sec;
    UINT    head_inode_in_sector    = (inode_number/INODE_PSEC)*INODE_PSEC;
    UINT    i;
    if (sector_position<2*NOR_FLASH_SECTPBLK||sector_position>=NOR_FLASH_NSECTOR)
        return PX_ERROR;
    for ( i = head_inode_in_sector; i < INODE_PSEC; i++)
    {   /* ���ͬ��һ��sector�е�����inode���ڣ���ҲҪ�޸����ǵ�imap�� */
        if(plfs->LFS_imap[i]!=(UINT)-1) {
            plfs->LFS_imap[i] = sector_position;
        }
    }
    /* ע�⣬��������inode��imap������֮ǰ�治���ڣ����Ǳ�Ȼ�޸ĵ� */
    plfs->LFS_imap[inode_number] = sector_position;

    /* ����CPR */
    (plfs->LFS_cpr)->imap_sec = plfs->LFS_availableSector + plfs->LFS_curBlockNo*NOR_FLASH_SECTPBLK;
    
    /* ����block summary */
    if(old_imap_sector != (UINT)-1) {
        plfs->LFS_blockSummary[old_imap_sector][0] = 0;
        //plfs->LFS_blockSummary[old_imap_sector][1] = 0;
    }

    plfs->LFS_blockSummary[(plfs->LFS_cpr)->imap_sec][0] = (UINT) -1;
    plfs->LFS_blockSummary[(plfs->LFS_cpr)->imap_sec][1] = SUMMARY_IMAP_TYPE;

    /* IMAP��д��SECOTR_BUF��AVAILABLE_SECTOR�� */
    lib_memcpy(plfs->LFS_secBuf,
                (PCHAR)plfs->LFS_imap,
                NOR_FLASH_SECTORSZ);

    /* ��д imap sector */
    writeOutSector(plfs);

    return ERROR_NONE;
}

/*
    ���µ� block summary д���µ� sector
    ����CPR.summary_sec
    modified
*/
INT updateSummary(PLFS_VOLUME plfs)
{

    UINT old_summary_sector = (plfs->LFS_cpr)->summary_sec;
    /* ����CPR */
    (plfs->LFS_cpr)->summary_sec = plfs->LFS_availableSector + plfs->LFS_curBlockNo*NOR_FLASH_SECTPBLK;

    /* ����block summary */
    if(old_summary_sector != (UINT) -1) {
        plfs->LFS_blockSummary[old_summary_sector][0] = 0;
        //plfs->LFS_blockSummary[old_summary_sector][1] = 0;
    }

    plfs->LFS_blockSummary[(plfs->LFS_cpr)->summary_sec][0] = (UINT) -1;
    plfs->LFS_blockSummary[(plfs->LFS_cpr)->summary_sec][1] = SUMMARY_SUMMARY_TYPE;

    lib_memcpy(plfs->LFS_secBuf,
                (PCHAR)plfs->LFS_blockSummary,
                NOR_FLASH_SECTORSZ);   

    /* ��д imap sector */
    writeOutSector(plfs); 
    return ERROR_NONE;    
}


/*
    ��ȡinode������inodeָ�롣���ʧ�ܣ�����Null��
    @inode_number��inode���
    modified
*/
PLFS_INODE getInodeFromInumber(PLFS_VOLUME plfs,
                                UINT inode_number)
{
    UINT sector_position = plfs->LFS_imap[inode_number];
    /* sector���blockλ�� */
    UINT local_sector_pos = sector_position%NOR_FLASH_SECTPBLK;
    /* ��ȡinode�ṹ����sector�����ƫ�ƣ����ֽ�Ϊ��λ */
    UINT local_inode_pos = (inode_number%INODE_PSEC)*sizeof(LFS_INODE);
    PLFS_INODE meta;
    read_content_t content;
    PCHAR temp;

    if(inode_number > MAX_FILE || sector_position == (UINT)-1){
        return LW_NULL;
    }

    meta = (PLFS_INODE)__SHEAP_ALLOC(sizeof(LFS_INODE));
    if (meta==LW_NULL)
    {
        _ErrorHandle(ENOMEM);
        return  LW_NULL;
    }

    /* ����Ƿ����ڴ��У���blkBuf��ı䣬�������ˣ� */
    //if(BLOCK_NO==block_location)
    if(LW_FALSE)
    {
        lib_memcpy(meta,
                    ((PCHAR)&plfs->LFS_blkBuf[local_sector_pos*NOR_FLASH_SECTORSZ])+local_inode_pos,
                    sizeof(LFS_INODE));
    }
    else
    {
        /* ��������ڴ棬��ֱ�Ӵ�flash�ж�ȡ */
        content = read_nor(GET_NOR_OFFSET_FROM_SECN(sector_position),READ_SECTOR);
        if (content.is_success)
        {
            temp = content.content;
            lib_memcpy(meta,
                        (PCHAR)(temp+local_inode_pos),
                        sizeof(LFS_INODE));
        }
        else
            return LW_NULL;
    }
    return meta;
}

/*
    �����ļ�����ȡinode���
    modified
*/
UINT getInodeNumberOfFile(PLFS_VOLUME plfs, PCHAR pcname)
{
    UINT i;
    PCHAR temp;
    /* �ȶ�ȡCPR_FN_1_BLK */
    readInBlock(plfs ,CPR_FN_1_BLK);
    for (i = 0; i < 511; i++)
    {
        temp = &(plfs->LFS_blkBuf[(i+1)*FN_ENTRY_SZ]);
        if (*temp)/* �����Чλ */
        {
            temp++;
            if (lib_strcmp(temp,pcname)==0)
                return i;    
        }
    }
    /* �ٶ�ȡCPR_FN_2_BLK */
    readInBlock(plfs ,CPR_FN_2_BLK);
    for (i = 0; i < 512; i++)
    {
        temp = &(plfs->LFS_blkBuf[(i+511)*FN_ENTRY_SZ]);
        if (*temp)/* �����Чλ */
        {
            temp++;
            if (lib_strcmp(temp,pcname)==0)
                return (i+511);    
        }
    }  
    return UINT_MAX;  
}

/*
    ����inode��Ÿ����ļ���
    modified
*/

INT getFileNameOfInodeNumber(PLFS_VOLUME plfs, PCHAR pcname,UINT inode_number)
{
    PCHAR temp;
    if (pcname == LW_NULL)
    {
        _ErrorHandle(EFAULT);
        return (PX_ERROR);
    }

    if (inode_number>MAX_FILE)
    {
        return (PX_ERROR);
    }
    
    if(inode_number<511) {
        if(readInBlock(plfs ,CPR_FN_1_BLK)<0){
            return (PX_ERROR);
        }
        temp = &(plfs->LFS_blkBuf[(inode_number+1)*FN_ENTRY_SZ]);
    } else if (inode_number <MAX_FILE) {
        if(readInBlock(plfs ,CPR_FN_2_BLK)<0){
            return (PX_ERROR);
        }
        temp = &(plfs->LFS_blkBuf[(inode_number-511)*FN_ENTRY_SZ]);        
    }

    if(*temp==0)
        return (PX_ERROR);
    
    temp++;
    lib_strcpy(pcname,(CPCHAR)temp);
    return (ERROR_NONE);
}

/* 
    ���Sector
    @sector_no����Ҫ�����sectorȫ�ֵ�λ��
*/
VOID printSector(UINT sector_no)
{
    UINT block_location = GET_BLKN_OF_SECN(sector_no);
    UINT local_sector_pos = sector_no%NOR_FLASH_SECTPBLK;
    PCHAR temp;
    read_content_t content;
    UINT i;
    /* ������ڴ棬��ֱ�Ӷ�ȡ */
    if (block_location==BLOCK_NO)
    {
        temp = &BLK_BUF[local_sector_pos*NOR_FLASH_SECTORSZ];
    }
    /* ������write_nor */
    else
    {
        read_nor(sector_no*NOR_FLASH_SECTORSZ,READ_SECTOR);
        if (content.is_success)
        {
            temp = content.content;
        }
        else
        {
            //_DebugHandle(__ERRORMESSAGE_LEVEL, "nor flash read fail.\r\n");
            return;
        }
    }
    for (i = 0; i < NOR_FLASH_SECTORSZ; i++)
    {
        printk("%c",*temp);
        temp++;
    }
    printk("\r\n");
}

/*
    lfs ��һ���ļ�
*/
PLFS_INODE __lfs_open(  PLFS_VOLUME     plfs,
                        PCHAR           pcName,
                        BOOL            *pbRoot)
{

    UINT        inode_no;
    PLFS_INODE  pinode;
    CHAR                pcTempName[MAX_FN_SZ];

    if (*pcName == PX_ROOT) {                                           /*  ���Ը�����                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    } else {
        lib_strlcpy(pcTempName, pcName, PATH_MAX);
    }

    if (pcTempName[0] == PX_EOS) {
        if (pbRoot) {
            *pbRoot = LW_TRUE;                                          /*  pcName Ϊ��                 */
        }
        return  (LW_NULL);
    
    } else {
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName ��Ϊ��               */
        }
    }

    inode_no = getInodeNumberOfFile(plfs, pcTempName);
    if (inode_no == UINT_MAX)
    {
        return LW_NULL;
    }

    pinode = getInodeFromInumber(plfs, inode_no);
    return pinode;
}
/*
    lfs ����һ���ļ�
*/
PLFS_INODE __lfs_maken( PLFS_VOLUME     plfs,
                        PCHAR           pcname,
                        mode_t          mode)
{
    PLFS_INODE  plfsn = (PLFS_INODE)__SHEAP_ALLOC(sizeof(LFS_INODE));
    CPCHAR      pcFileName;
    UINT        inode_no;
    INT         i;
    if(plfs->LFS_availableFile>0) {
        plfs->LFS_availableFile --;
    } else {
        printk("no more file can be created.\r\n");
        return LW_NULL;
    }

    inode_no = nextInodeNumber(plfs);
    if (inode_no == UINT_MAX)
    {
        return LW_NULL;
    }
    

    if(plfsn == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);        
    }
    lib_bzero(plfsn,sizeof(LFS_INODE));


    pcFileName = lib_rindex(pcname, PX_DIVIDER);            /* ���￼�ǵ��༶Ŀ¼,��pcFileNameָ��"\"��һ���ַ�*/
    if (pcFileName) {                                       /* ��Ȼ��ϵͳ�ݲ�֧�ֶ༶Ŀ¼�������Ǳ��� */
        pcFileName++;
    } else {
        pcFileName = pcname;
    }    

    if ((mode & S_IFMT) == 0) {
        mode |= S_IFREG;
    }

    plfsn->LFSN_pcname = (PCHAR)__SHEAP_ALLOC(MAX_FN_SZ);
    if(plfsn->LFSN_pcname == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);        
    }
    lib_strcpy(plfsn->LFSN_pcname,pcFileName);

    plfsn->LFSN_inodeNo     = inode_no;
    plfsn->LFSN_stSize      = 0;
    plfsn->LFSN_stVSize     = 0;
    plfsn->LFSN_mode        = mode;
    plfsn->LFSN_timeCreate  = lib_time(LW_NULL);
    plfsn->LFSN_timeAccess  = plfsn->LFSN_timeCreate;
    plfsn->LFSN_timeChange  = plfsn->LFSN_timeCreate;
    plfsn->LFSN_uid         = getuid();
    plfsn->LFSN_gid         = getgid();

    

    for(i = 0; i<MAX_DATA_SECTOR;i++)
        plfsn->LFSN_sectorLocation[i] = (UINT) -1;
    
    return  (plfsn);
}

/*
    lfs ��ȡ�ļ�״̬
*/
VOID __lfs_stat(PLFS_INODE plfsn, PLFS_VOLUME plfs, struct stat  *pstat)
{
    INT     i;
    if(plfsn) {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&plfs->LFS_devhdrHdr);
        pstat->st_ino     = (ino_t)plfs;
        pstat->st_mode    = plfsn->LFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = plfsn->LFSN_uid;
        pstat->st_gid     = plfsn->LFSN_gid;
        pstat->st_rdev    = 1;
        pstat->st_size    = (off_t)plfsn->LFSN_stSize;
        pstat->st_atime   = plfsn->LFSN_timeAccess;
        pstat->st_mtime   = plfsn->LFSN_timeChange;
        pstat->st_ctime   = plfsn->LFSN_timeCreate;
        pstat->st_blksize = NOR_FLASH_SECTORSZ;

        for ( i = 0; i < MAX_DATA_SECTOR; i++)
        {
            if(plfsn->LFSN_sectorLocation[i]!=(UINT)-1)
                pstat->st_blocks++;
        }

    } else {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&plfs->LFS_devhdrHdr);
        pstat->st_ino     = (ino_t)0;
        pstat->st_mode    = plfs->LFS_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = plfs->LFS_uid;
        pstat->st_gid     = plfs->LFS_gid;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_atime   = plfs->LFS_time;
        pstat->st_mtime   = plfs->LFS_time;
        pstat->st_ctime   = plfs->LFS_time;
        pstat->st_blksize = NOR_FLASH_SECTORSZ;
        pstat->st_blocks  = 0;        
    }

    pstat->st_resv1 = LW_NULL;
    pstat->st_resv2 = LW_NULL;
    pstat->st_resv3 = LW_NULL;
}

/*
    LFS ɾ��һ���ļ�
*/
VOID __lfs_close(PLFS_INODE plfsn, INT iFlag) 
{
    plfsn->LFSN_timeAccess = lib_time(LW_NULL);

    // O_ACCMODE ָ����ֻ����ֻд����д����һ��
    if (plfsn->LFSN_bChange && (iFlag & O_ACCMODE) != O_RDONLY) {
        plfsn->LFSN_timeChange = plfsn->LFSN_timeAccess;
    }
}

/*
    LFS ɾ��һ���ļ�
*/
INT __lfs_unlink(PLFS_VOLUME plfs, PLFS_INODE plfsn)
{
    INT     i;
    UINT    data_sector_no;
    UINT    inode_no;

    if(plfsn == NULL)
        return (PX_ERROR);

    inode_no = plfsn->LFSN_inodeNo;
    
    /* ֻ�и�Ŀ¼ΪĿ¼���ͣ�������Ϊ�ļ�����*/
    if (S_ISDIR(plfsn->LFSN_mode) 
        && plfs->LFS_availableFile != MAX_FILE)
    {
        _ErrorHandle(ENOTEMPTY);
        return  (PX_ERROR);        
    }

    /* ɾ���ļ����� */
    /* ����inode */

    for(i = 0 ; i<MAX_DATA_SECTOR; i++)
    {
        if (plfsn->LFSN_sectorLocation[i] != -1)
        {
            data_sector_no = plfsn->LFSN_sectorLocation[i];
            /* �޸�inode�����б� */
            plfsn->LFSN_sectorLocation[i] = -1;
            /* �������sector�޸�summary */
            plfs->LFS_blockSummary[data_sector_no][0] = 0;
            plfs->LFS_blockSummary[data_sector_no][1] = 0;
        }
    } 
    /* �޸�imap */
    updateImap(plfs, inode_no, (UINT)-1);

    /* �޸�fileNameMap */
    updateFilemap(plfs, inode_no, LW_NULL);

    /* ����������� */
    plfs->LFS_availableFile ++;
    plfs->LFS_inodeMap[inode_no] = 0;
    
    /* �ͷſռ� */
    __SHEAP_FREE(plfsn->LFSN_pcname);
    __SHEAP_FREE(plfsn);

    return (ERROR_NONE);
}

/*
    LFS ж��
    ʵ��ʱ�ݹ�ɾ���ļ�ϵͳ������Ŀ¼�µ������ļ�
    ����LFS��֧��Ŀ¼��������ɾ�������ļ�����
*/
VOID __lfs_unmount(PLFS_VOLUME plfs)
{
    UINT         i;
    PLFS_INODE  plfsn;
    for (i = 0; i<MAX_FILE ; i++) {
        plfsn = getInodeFromInumber(plfs,i);
        __lfs_unlink(plfs, plfsn);
    }
}

/*
    LFS д���ļ�����
     �䡡��  :  pramn            �ļ��ڵ�
                pvBuffer         ������
                stNBytes         ��Ҫд��Ĵ�С
                stOft            ƫ����
    д����̣�
        data_sector -> inode -> imap ->summary -> CPR
*/
ssize_t  __lfs_write (PLFS_VOLUME plfs, PLFS_INODE  plfsn, CPVOID  pvBuffer, size_t  stNBytes, size_t  stOft)
{
    UINT8          *pucDest = (UINT8 *)pvBuffer;
    size_t          stEnd   = stOft + stNBytes;
    size_t          stWrite = 0;
    size_t          stStart;
    UINT            NSector;                    /* д����ܹ���sector */
    UINT            curSectorIndex;             /* ��ǰҪд���ļ��ĵڼ���Sector*/
    UINT            curSector;                  /* ��ǰ��д���ļ���Sectorȫ����� */

    NSector = (ULONG)(stEnd / NOR_FLASH_SECTORSZ);
    
    if (stEnd % NOR_FLASH_SECTORSZ) {
        NSector++;
    }

    /* 
        ��ɴ˴�д����Ҫ���ܿ������ڵ����ļ�������
        �򳬹��������ļ�ϵͳ���ܿ�������ᱨ��
     */
    if ((NSector > MAX_DATA_SECTOR)||
        (plfs->LFS_ulCurSector+NSector)>plfs->LFS_ulMaxSector)
    {
        return (0);        
    }
    
    stStart = stOft % NOR_FLASH_SECTORSZ;
    curSectorIndex = stOft / NOR_FLASH_SECTORSZ;

    while (stNBytes)
    {
        size_t  stBufSize = (NOR_FLASH_SECTORSZ - stStart);
        /* �ȼ���Ƿ�Ḳ��ԭ������ */
        if(curSectorIndex<plfsn->LFSN_Cnt) {
            if(readInSector(plfs,plfsn->LFSN_sectorLocation[curSectorIndex])<0) {
                goto __lfs_write_end;
            }
        }

        if (stBufSize >= stNBytes) { /* д���һ�� */   
            lib_memcpy(&plfs->LFS_secBuf[stStart], pucDest, stNBytes);
            /* ��дsector������summary */
            curSector = writeOutSector(plfs);
            if(curSector<0) {
                goto __lfs_write_end;
            }
            plfsn->LFSN_sectorLocation[curSectorIndex] = curSector;
            if(stStart == 0)
                plfsn->LFSN_Cnt++;

            plfs->LFS_blockSummary[curSector][0] =  (UINT) -1;
            plfs->LFS_blockSummary[curSector][1] = plfsn->LFSN_inodeNo;

            stWrite += stNBytes;
            break;
        } else {/* дһ���� */
            lib_memcpy(&plfs->LFS_secBuf[stStart], pucDest, stBufSize);
            /* ��дsector������summary */
            curSector = writeOutSector(plfs);
            if(curSector<0) {
                goto __lfs_write_end;
            }
            plfsn->LFSN_sectorLocation[curSectorIndex] = curSector;
            plfsn->LFSN_Cnt++;

            plfs->LFS_blockSummary[curSector][0] =  (UINT) -1;
            plfs->LFS_blockSummary[curSector][1] = plfsn->LFSN_inodeNo; 

            pucDest     += stBufSize;
            stWrite     += stBufSize;
            stNBytes    -= stBufSize;
            stStart      = 0;
            curSectorIndex++;
        }
    }
    
    if (plfsn->LFSN_stSize < (stOft + stWrite)) {
        plfsn->LFSN_stSize = (stOft + stWrite);
        if(plfsn->LFSN_stVSize < plfsn->LFSN_stSize) {
            plfsn->LFSN_stVSize = plfsn->LFSN_stSize;
        }
    }

__lfs_write_end:
    writeInode(plfs, plfsn, plfsn->LFSN_inodeNo);
    return  ((ssize_t)stWrite);
}

/*********************************************************************************************************
** ��������: __lfs_read
** ��������: ramfs ��ȡ�ļ�����
** �䡡��  : pramn            �ļ��ڵ�
**           pvBuffer         ������
**           stSize           ��������С
**           stOft            ƫ����
** �䡡��  : ��ȡ���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/

ssize_t __lfs_read(PLFS_VOLUME plfs, PLFS_INODE plfsn, PVOID pvBuffer, size_t stSize, size_t stOft)
{
    UINT8          *pucDest = (UINT8 *)pvBuffer;
    size_t          stDataLeft;
    size_t          stNBytes;
    size_t          stRead  = 0;
    size_t          stStart;
    UINT            curSectorIndex;

    /*
        stOft���ļ���ǰָ�룬����ļ�ָ�볬�����ļ���С���򲻻�����κζ�����
    */
    if( plfsn->LFSN_stVSize <= stOft) {
        return (0);
    }

    stDataLeft = plfsn->LFSN_stVSize - stOft; 
    stNBytes   = __MIN(stDataLeft, stSize);
    
    stStart = stOft % NOR_FLASH_SECTORSZ;
    curSectorIndex = stOft / NOR_FLASH_SECTORSZ;
    do {
        size_t stBufSize = (NOR_FLASH_SECTORSZ - stStart);
        if(stBufSize >= stNBytes) {
            if(readInSector(plfs,plfsn->LFSN_sectorLocation[curSectorIndex])<0) {
                return ((ssize_t)stRead);
            }
            lib_memcpy(pucDest, &plfs->LFS_secBuf[stStart], stNBytes);
            stRead += stNBytes;
            break;
        } else {
            if(readInSector(plfs,plfsn->LFSN_sectorLocation[curSectorIndex])<0) {
                return ((ssize_t)stRead);
            }
            lib_memcpy(pucDest, &plfs->LFS_secBuf[stStart], stBufSize);
            pucDest     += stBufSize; 
            stRead      += stBufSize;
            stNBytes    -= stBufSize; 
            stStart      = 0;  
            curSectorIndex++;    
        }
    } while (stNBytes);

    return  ((ssize_t)stRead);
}

/*
**  LFS �ƶ�����������һ���ļ�
**  ����LFSû����Ŀ¼ϵͳ�������������ֻ������������
**  ע�⣬�޷��������ļ�ϵͳ�ĸ�Ŀ¼
** �䡡��  : plfsn            �ļ��ڵ�
**           pcNewName        �µ�����
** �䡡��  : ERROR    
*/
INT __lfs_move (PLFS_VOLUME plfs, PLFS_INODE plfsn, PCHAR pcNewName)
{
    PLFS_INODE      plfsnTemp;
    BOOL            bRoot;
    PCHAR           pcFileName;

    plfsnTemp = __lfs_open(plfs, pcNewName, &bRoot);
    /* ֻҪ�ļ�Ϊ���ڵ㣬ֱ�ӱ��� */
    if (!plfsnTemp) {
        if(bRoot) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    /* �ļ�ԭ��������һ����ֱ�ӷ��� */
    if (plfsn == plfsnTemp) {                                           /*  ��ͬ                        */
        return  (ERROR_NONE);
    }    

    /* �Ӷ༶Ŀ¼·���л�ȡ�ļ�������LFS��֧��Ŀ¼�ļ����ⲿ��Ҳ���Ժ��� */
    pcFileName = lib_rindex(pcNewName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    } else {
        pcFileName = pcNewName;
    }

    return updateFilemap(plfs, plfsn->LFSN_inodeNo, pcFileName);
}
