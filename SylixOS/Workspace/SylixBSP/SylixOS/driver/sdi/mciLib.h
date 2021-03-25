/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mciLib.h
**
** ��   ��   ��: Lu.ZhenPing (¬��ƽ)
**
** �ļ���������: 2014 �� 10 �� 15 ��
**
** ��        ��: SD��ý�忨�ӿڿ�
*********************************************************************************************************/

#ifndef __MCILIB_H
#define __MCILIB_H

/*********************************************************************************************************
  cplusplus
*********************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  ����ѡ��
*********************************************************************************************************/
#undef DEBUG_SDI

/*********************************************************************************************************
   ���Ĵ���λ����
*********************************************************************************************************/
#define SDI_CON_SDRESET                 (1 << 8)
#define SDI_CON_MMCCLOCK                (1 << 5)
#define SDI_CON_BYTEORDER               (1 << 4)
#define SDI_CON_SDI_OIRQ                (1 << 3)
#define SDI_CON_RWAITEN                 (1 << 2)
#define SDI_CON_FIFORESET               (1 << 1)
#define SDI_CON_CLOCKTYPE               (1 << 0)

#define SDI_CCON_ABORT                  (1 << 12)
#define SDI_CCON_WITHDATA               (1 << 11)
#define SDI_CCON_LONGRSP                (1 << 10)
#define SDI_CCON_WAITRSP                (1 << 9 )
#define SDI_CCON_CMDSTART               (1 << 8 )
#define SDI_CCON_SENDERHOST             (1 << 6 )
#define SDI_CCON_INDEX                  (0x3F   )

#define SDI_CSTA_CRCFAIL                (1 << 12)
#define SDI_CSTA_CMDSENT                (1 << 11)
#define SDI_CSTA_CMDTIMEOUT             (1 << 10)
#define SDI_CSTA_RSPFIN                 (1 << 9 )
#define SDI_CSTA_XFERING                (1 << 8 )
#define SDI_CSTA_INDEX                  (0xFF   )

#define SDI_DCON_DS_BYTE                (0 << 22)
#define SDI_DCON_DS_HALFWORD            (1 << 22)
#define SDI_DCON_DS_WORD                (2 << 22)
#define SDI_DCON_IRQPERIOD              (1 << 21)
#define SDI_DCON_TXAFTERRESP            (1 << 20)
#define SDI_DCON_RXAFTERCMD             (1 << 19)
#define SDI_DCON_BUSYAFTERCMD           (1 << 18)
#define SDI_DCON_BLOCKMODE              (1 << 17)
#define SDI_DCON_WIDEBUS                (1 << 16)
#define SDI_DCON_DMAEN                  (1 << 15)
#define SDI_DCON_STOP                   (1 << 14)
#define SDI_DCON_DATSTART               (1 << 14)
#define SDI_DCON_DATMODE                (3 << 12)
#define SDI_DCON_BLKNUM                 (0x7FF  )

#define SDI_DCON_XFER_READY             (0 << 12)
#define SDI_DCON_XFER_CHKSTART          (1 << 12)
#define SDI_DCON_XFER_RXSTART           (2 << 12)
#define SDI_DCON_XFER_TXSTART           (3 << 12)

#define SDI_DCON_BLKNUM_MASK            (0xFFF)
#define SDI_DCNT_BLKNUM_SHIFT           (12   )

#define SDI_DSTA_NOBYS                  (1 << 11)
#define SDI_DSTA_RDYWAITREQ             (1 << 10)
#define SDI_DSTA_SDI_OIRQDETECT         (1 << 9 )
#define SDI_DSTA_FIFOFAIL               (1 << 8 )                       /*  reserved on 2440            */
#define SDI_DSTA_CRCFAIL                (1 << 7 )
#define SDI_DSTA_RXCRCFAIL              (1 << 6 )
#define SDI_DSTA_DATATIMEOUT            (1 << 5 )
#define SDI_DSTA_XFERFINISH             (1 << 4 )
#define SDI_DSTA_BUSYFINISH             (1 << 3 )
#define SDI_DSTA_SBITERR                (1 << 2 )                       /*  reserved on 2410a/2440      */
#define SDI_DSTA_TXDATAON               (1 << 1 )
#define SDI_DSTA_RXDATAON               (1 << 0 )

#define SDI_FSTA_FIFORESET              (1 << 16)
#define SDI_FSTA_FIFOFAIL               (3 << 14)                       /*  3 is correct (2 bits)       */
#define SDI_FSTA_TFDET                  (1 << 13)
#define SDI_FSTA_RFDET                  (1 << 12)
#define SDI_FSTA_TFHALF                 (1 << 11)
#define SDI_FSTA_TFEMPTY                (1 << 10)
#define SDI_FSTA_RFLAST                 (1 << 9 )
#define SDI_FSTA_RFFULL                 (1 << 8 )
#define SDI_FSTA_RFHALF                 (1 << 7 )
#define SDI_FSTA_COUNTMASK              (0x7F   )

#define SDI_IMSK_RESPONSECRC            (1 << 17)
#define SDI_IMSK_CMDSENT                (1 << 16)
#define SDI_IMSK_CMDTIMEOUT             (1 << 15)
#define SDI_IMSK_RESPONSEND             (1 << 14)
#define SDI_IMSK_READWAIT               (1 << 13)
#define SDI_IMSK_SDI_OIRQ               (1 << 12)
#define SDI_IMSK_FIFOFAIL               (1 << 11)
#define SDI_IMSK_CRCSTATUS              (1 << 10)
#define SDI_IMSK_DATACRC                (1 << 9 )
#define SDI_IMSK_DATATIMEOUT            (1 << 8 )
#define SDI_IMSK_DATAFINISH             (1 << 7 )
#define SDI_IMSK_BUSYFINISH             (1 << 6 )
#define SDI_IMSK_SBITERR                (1 << 5 )                       /* reserved 2440/2410a          */
#define SDI_IMSK_TXFIFOHALF             (1 << 4 )
#define SDI_IMSK_TXFIFOEMPTY            (1 << 3 )
#define SDI_IMSK_RXFIFOLAST             (1 << 2 )
#define SDI_IMSK_RXFIFOFULL             (1 << 1 )
#define SDI_IMSK_RXFIFOHALF             (1 << 0 )

#define SDI_MSK                         (0xFFFFFFFF)

#define GPE8    (8)

/*********************************************************************************************************
  �����궨��
*********************************************************************************************************/
#define SDI_MAX_TX_FIFO                 64
#define SDI_MAX_RX_FIFO                 64
#define SDI_SET_TIMEOUT                 0x7FFFFFF

#define SD_KHZ                          1000u
#define SD_MHZ                          (1000 * SD_KHZ)
#define SD_SLOW_CLK_FREQ                (400 * SD_KHZ)
#define SD_HIGH_CLK_FREQ                (12 * SD_MHZ)

#define SDI_GPIO_PULLEN                 (1 << 5) |   \
                                        (1 << 6) |   \
                                        (1 << 7) |   \
                                        (1 << 8) |   \
                                        (1 << 9) |   \
                                        (1 << 10)

#define SDI_GPIO_FUNC                   (2 << 10) |  \
                                        (2 << 12) |  \
                                        (2 << 14) |  \
                                        (2 << 16) |  \
                                        (2 << 18) |  \
                                        (2 << 20)

#define SDI_DATSTAT_RESET               (SDI_DSTA_RDYWAITREQ      |       \
                                         SDI_DSTA_SDI_OIRQDETECT  |       \
                                         SDI_DSTA_FIFOFAIL        |       \
                                         SDI_DSTA_CRCFAIL         |       \
                                         SDI_DSTA_RXCRCFAIL       |       \
                                         SDI_DSTA_DATATIMEOUT     |       \
                                         SDI_DSTA_XFERFINISH      |       \
                                         SDI_DSTA_BUSYFINISH      |       \
                                         SDI_DSTA_SBITERR         |       \
                                         SDI_DSTA_TXDATAON        |       \
                                         SDI_DSTA_RXDATAON        |       \
                                         SDI_DSTA_NOBYS)

/*********************************************************************************************************
  ��ϵ�ṹ��صĿ��Ʋ��� DMAT_iTransMode
*********************************************************************************************************/
#define DMA_TRANSMODE_SINGLE    0x00000000                              /*  ���δ���                    */
#define DMA_TRANSMODE_WHOLE     0x00000001                              /*  ��ȫ����                    */

#define DMA_TRANSMODE_CLKAPB    0x00000000                              /*  APB ʱ��Դ                  */
#define DMA_TRANSMODE_CLKAHB    0x00000002                              /*  AHB ʱ��Դ                  */

#define DMA_TRANSMODE_NORMAL    0x00000000                              /*  ������ʽ����                */
#define DMA_TRANSMODE_BURST     0x00000004                              /*  ͻ��ģʽ����                */

#define DMA_TRANSMODE_ONESHOT   0x00000000                              /*  ������ɺ�, ����������      */
#define DMA_TRANSMODE_RELOAD    0x00000008                              /*  ������ɺ�, �Զ�����        */

#define DMA_TRANSMODE_BYTE      0x00000000                              /*  �ֽڴ���                    */
#define DMA_TRANSMODE_DBYTE     0x00000010                              /*  ���ִ���                    */
#define DMA_TRANSMODE_4BYTE     0x00000020                              /*  �ִ���                      */

/*********************************************************************************************************
  ��ϵ�ṹ��صĿ��Ʋ��� DMAT_ulOption
*********************************************************************************************************/
#define DMA_OPTION_SRCBUS_AHB   0x00000000                              /*  Դ�� AHB ����ͨ��           */
#define DMA_OPTION_SRCBUS_APB   0x00000001                              /*  Դ�� APB ����ͨ��           */

#define DMA_OPTION_DSTBUS_AHB   0x00000000                              /*  Ŀ�� AHB ����ͨ��           */
#define DMA_OPTION_DSTBUS_APB   0x00000002                              /*  Ŀ�� APB ����ͨ��           */

/*********************************************************************************************************
  SDIO Э����غ�
*********************************************************************************************************/
#define SDIO_WR_FLAG            (0x80000000)
#define SDIO_RD_FLAG            (0x00000000)
#define SDIO_OP_CODE_FLAG       (0x04000000)
#define SDIO_REG_ADDR_MSK       BIT_LOFFSET(0x0000007f, 9)
#define SDIO_BLK_MODE_FLAG      (0x08000000)
#define SDIO_WR_BACK_FLAG       (0x08000000)
#define BIT_LOFFSET(x, y)       (x << y)
#define BIT_ROFFSET(x, y)       (x >> y)

/*********************************************************************************************************
  ʹ�� DMA ��
*********************************************************************************************************/
//#define SDIO_DMA_EN

/*********************************************************************************************************
  MCI ״̬����
*********************************************************************************************************/
enum mciWaiter {
    COMPLETION_NONE,
    COMPLETION_FINALIZE,
    COMPLETION_CMDSENT,
    COMPLETION_RSPFIN,
    COMPLETION_XFERFINISH,
    COMPLETION_XFERFINISH_RSPFIN,
};

/*********************************************************************************************************
  �ṹ�嶨��
*********************************************************************************************************/
typedef struct __sdio_data SDIO_DAT, *PSDIO_DAT;

typedef struct __sdio_protocol {
    INT              SDIOP_iBlkMode;
    INT              SDIOP_iRdWr;
    UINT8            SDIOP_iFuncNum;
    INT              SDIOP_iIncOps;
    INT              SDIOP_iRegAddr;
    UINT             SDIOP_uiOcr;
    UINT8            SDIOP_iMemPre;
    UINT16           SDIOP_usRca;
} SDIO_PROTOCOL;

typedef struct {
    VOID  (*MCIDRV_request) (PSDIO_DAT  psdio);
} MCI_DRV;

struct __sdio_data {
    SDIO_PROTOCOL       SDIO_sdiopHeader;

    INT                 SDIO_ucDmaEn;
    UINT8              *SDIO_puiBuf;
    UINT32              SDIO_uiBytes;                                   /*  FIFO ��д���ݵ�ʣ���ֽ���   */
    UINT8               SDIO_uiData;
    INT                 SDIO_iBusWidth;

    LW_OBJECT_HANDLE    SDIO_hTaskId;                                   /*  ���ݴ�������                */
    LW_OBJECT_HANDLE    SDIO_hTaskSync;                                 /*  ���ݴ���ͬ��                */
    LW_OBJECT_HANDLE    SDIO_hTaskSdio;                                 /*  sdio �жϴ�������           */
    LW_OBJECT_HANDLE    SDIO_hSdioSync;                                 /*  sdio ͬ��                   */
    LW_OBJECT_HANDLE    SDIO_hComplete;
    LW_OBJECT_HANDLE    SDIO_hDmaSync;

    INT                 SDIO_iIoAct;
    enum mciWaiter      SDIO_eCompleteWhat;
    INT                 SDIO_iDmaComplete;
    LW_SD_COMMAND      *SDIO_psdcmd;
    LW_SD_DATA         *SDIO_psddata;
    BOOL                SDIO_bData;
    UINT32              SDIO_uiCmdError;
    UINT32              SDIO_uiDataError;

    ULONG               SDIO_ulVector;

    /*
     *  �жϲ���״̬
     */
    BOOL                SDIO_bIrqDisabled;
    BOOL                SDIO_bIrqEnabled;
    BOOL                SDIO_bIrqState;
    BOOL                SDIO_bSdioIrqEn;

    LW_SPINLOCK_DEFINE (SDIO_slCompLock);
    VOID               (*SDIO_userFunc)(ULONG  ulArg);
    ULONG               SDIO_ulFuncArg;
    MCI_DRV             SDIO_request;
};

/*********************************************************************************************************
** ��������: sdioFree
** ��������: �ͷ� SDIO
** ��    ��: psdio   SDIO �ṹ
** ��    ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
VOID  sdioFree(PSDIO_DAT  psdio);
/*********************************************************************************************************
** ��������: sdioAlloc
** ��������: ���� SDIO �ռ�
** ��    ��: psdio   SDIO �ṹ
** ��    ��: NONE
** ��    ��: SDIO �ṹ
*********************************************************************************************************/
PSDIO_DAT  sdioAlloc(VOID);
/*********************************************************************************************************
** ��������: sdioInit
** ��������: sdio ��ʼ��
** ��    ��: psdio   SDIO �ṹ
** ��    ��: NONE
** ��    ��: �����
*********************************************************************************************************/
INT  sdioInit(PSDIO_DAT  psdio);
/*********************************************************************************************************
** ��������: sdioReset
** ��������: card ��λ
** �䡡  ��: psdio  SDIO �ṹ
** �䡡  ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
VOID  sdioReset(PSDIO_DAT  psdio);
/*********************************************************************************************************
** ��������: sdioStartClock
** ��������: SDIO ʱ�ӿ�
** �䡡  ��: NONE
** �䡡  ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
VOID  sdioStartClock(VOID);
/*********************************************************************************************************
** ��������: sdioStopClock
** ��������: SDIO ʱ�ӹ�
** �䡡  ��: NONE
** �䡡  ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
VOID  sdioStopClock(VOID);
/*********************************************************************************************************
** ��������: sdioSetClock
** ��������: SDIO ʱ�ӹ�
** �䡡  ��: NONE
** �䡡  ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
VOID  sdioSetClock(UINT32 uiRate);
/*********************************************************************************************************
** ��������: sdioSetBusWidth
** ��������: SDIO ʱ�ӹ�
** �䡡  ��: NONE
** �䡡  ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
INT   sdioSetBusWidth(INT iBusWidth);
/*********************************************************************************************************
** ��������: sdioIntEnable
** ��������: SDIO ʹ���ж�
** �䡡  ��: iVector  �ж�������
**           iEn      ʹ�ܱ�־
** �䡡  ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
VOID  sdioIntEnable(PSDIO_DAT  psdio, INT  iEn);
/*********************************************************************************************************
** ��������: sdioDeInit
** ��������: sdio ������Դ
** ��    ��: psdio   SDIO �ṹ
** ��    ��: NONE
** ��    ��: ERROR CODE
*********************************************************************************************************/
INT  sdioDeInit(PSDIO_DAT  psdio);
/*********************************************************************************************************
  cplusplus
*********************************************************************************************************/
#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */
#endif                                                                  /*  __MCILIB_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
