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
** ��   ��   ��: samsungdma.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 19 ��
**
** ��        ��: DMA ��������
**
** BUG:
2009.09.16  DMA auto reload ��������.
2009.12.12  ����ϵͳ��������ص� DMA �����ӿ�, ��������� DMA �ӿ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
#include "SylixOS.h"
#include "samsungdma.h"
/*********************************************************************************************************
  2440 DMA ͨ����������
*********************************************************************************************************/
#define __2440_DMA_CHAN_NUM     4
/*********************************************************************************************************
  2440 DMA ͨ���ṹ
*********************************************************************************************************/
typedef struct {

    volatile unsigned int       uiSrcAddr;                              /*  Դ�˵�ַ                    */
    volatile unsigned int       uiSrcCtl;                               /*  Դ�˵�ַ����                */
    
    volatile unsigned int       uiDstAddr;                              /*  Ŀ�Ķ˵�ַ                  */
    volatile unsigned int       uiDstCtl;                               /*  Ŀ�Ķ˵�ַ����              */
    
    volatile unsigned int       uiDMACtl;                               /*  DMA ͨ������                */
    volatile unsigned int       uiDMAStat;                              /*  DMA ͨ��״̬                */
    
    volatile unsigned int       uiCurScr;                               /*  DMA ��ǰԴ�˵�ַ            */
    volatile unsigned int       uiCurDst;                               /*  DMA ��ǰĿ�Ķ˵�ַ          */
    
    volatile unsigned int       uiMaskTigger;                           /*  �������ƼĴ���              */
    
} __DMA_PHY_CHANNEL;
/*********************************************************************************************************
  2410 DMA ��ַ����
*********************************************************************************************************/
#define __2440_DMA_PHY_ADDR    {(__DMA_PHY_CHANNEL *)0x4b000000,        \
                                (__DMA_PHY_CHANNEL *)0x4b000040,        \
                                (__DMA_PHY_CHANNEL *)0x4b000080,        \
                                (__DMA_PHY_CHANNEL *)0x4b0000c0}
/*********************************************************************************************************
  �ĸ�ͨ���� DMA ������
*********************************************************************************************************/
__DMA_PHY_CHANNEL              *__GpdmaphychanTbl[__2440_DMA_CHAN_NUM] = __2440_DMA_PHY_ADDR;
/*********************************************************************************************************
  2440 DMA �ؼ��ԵĲ������� (DMASKTRIG)
*********************************************************************************************************/
#define __DMA_PHY_STOP          (1 << 2)                                /*  DMASKTRIG [2]               */
#define __DMA_PHY_ON            (1 << 1)                                /*  DMASKTRIG [1]               */
#define __DMA_SW_TRIGGER        (1 << 0)                                /*  DMASKTRIG [0]               */
/*********************************************************************************************************
  2440 DMA �ؼ��ԵĲ������� (DMASKTRIG)
*********************************************************************************************************/
#define __DMA_PHY_STAT          (0x3 << 20)                             /*  DSTAT [21:20]               */
/*********************************************************************************************************
** Function name:           __dmaHwIsr
** Descriptions:            DMA �жϴ�������
** input parameters:        iChannel    ͨ����
** output parameters:       NONE
** Returned value:          �жϷ��񷵻�ֵ
** Created by:              Hanhui
** Created Date:            2007/10/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static irqreturn_t  __dmaHwIsr (int  iChannel)
{
    switch (iChannel) {                                                 /*  ���ָ���жϱ�־            */
    
    case LW_DMA_CHANNEL0:
        INTER_CLR_PNDING(BIT_DMA0);
        break;
    
    case LW_DMA_CHANNEL1:
        INTER_CLR_PNDING(BIT_DMA1);
        break;
        
    case LW_DMA_CHANNEL2:
        INTER_CLR_PNDING(BIT_DMA2);
        break;
        
    case LW_DMA_CHANNEL3:
        INTER_CLR_PNDING(BIT_DMA3);
        break;
        
    default:
        return  (LW_IRQ_HANDLED);
    }
    
    API_DmaContext(iChannel);                                           /*  ���� DMA ������           */
    
    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** Function name:           __dmaHwReset
** Descriptions:            ��ʼ�� DMA ��ֹͣ
** input parameters:        iChannel:   DMA ͨ����
**                          pdmafuncs   û��ʹ��
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/10/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void  __dmaHwReset (UINT   uiChannel, PLW_DMA_FUNCS  pdmafuncs)
{
    __DMA_PHY_CHANNEL   *pdmaphychanCtl = __GpdmaphychanTbl[uiChannel];
    
    if (uiChannel >= __2440_DMA_CHAN_NUM) {
        return;                                                         /*  ͨ������                    */
    }
    
    pdmaphychanCtl->uiMaskTigger = (__DMA_PHY_STOP | __DMA_PHY_ON);     /*  ֹͣ DMA                    */
    
    /*
     *  ��װ�жϴ�������
     */
    switch (uiChannel) {
    
    case LW_DMA_CHANNEL0:
        INTER_CLR_PNDING(BIT_DMA0);                                     /*  ��������жϱ�־            */
        API_InterVectorConnect(VIC_CHANNEL_DMA0, 
                               (PINT_SVR_ROUTINE)__dmaHwIsr,
                               (PVOID)uiChannel,
                               "dma0_isr");                             /*  ��װ�жϴ�������            */
        INTER_CLR_MSK((1u << VIC_CHANNEL_DMA0));
        break;
        
    case LW_DMA_CHANNEL1:
        INTER_CLR_PNDING(BIT_DMA1);                                     /*  ��������жϱ�־            */
        API_InterVectorConnect(VIC_CHANNEL_DMA1, 
                               (PINT_SVR_ROUTINE)__dmaHwIsr,
                               (PVOID)uiChannel,
                               "dma1_isr");                             /*  ��װ�жϴ�������            */
        INTER_CLR_MSK((1u << VIC_CHANNEL_DMA1));
        break;
        
    case LW_DMA_CHANNEL2:
        INTER_CLR_PNDING(BIT_DMA2);                                     /*  ��������жϱ�־            */
        API_InterVectorConnect(VIC_CHANNEL_DMA2, 
                               (PINT_SVR_ROUTINE)__dmaHwIsr,
                               (PVOID)uiChannel,
                               "dma2_isr");                             /*  ��װ�жϴ�������            */
        INTER_CLR_MSK((1u << VIC_CHANNEL_DMA2));
        break;
    
    case LW_DMA_CHANNEL3:
        INTER_CLR_PNDING(BIT_DMA3);                                     /*  ��������жϱ�־            */
        API_InterVectorConnect(VIC_CHANNEL_DMA3, 
                               (PINT_SVR_ROUTINE)__dmaHwIsr,
                               (PVOID)uiChannel,
                               "dma3_isr");                             /*  ��װ�жϴ�������            */
        INTER_CLR_MSK((1u << VIC_CHANNEL_DMA3));
        break;
        
    }
}
/*********************************************************************************************************
** Function name:           __dmaHwGetStatus
** Descriptions:            ��� DMA ��ǰ״̬
** input parameters:        iChannel:               DMA ͨ����
**                          pdmafuncs               û��ʹ��
** output parameters:       NONE
** Returned value:          0:����  1:æ  -1:����
** Created by:              Hanhui
** Created Date:            2007/10/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int  __dmaHwGetStatus (UINT   uiChannel, PLW_DMA_FUNCS  pdmafuncs)
{
    __DMA_PHY_CHANNEL   *pdmaphychanCtl = __GpdmaphychanTbl[uiChannel];
    
    if (uiChannel >= __2440_DMA_CHAN_NUM) {
        return  (PX_ERROR);                                             /*  ͨ������                    */
    }
    
    if (pdmaphychanCtl->uiDMAStat & __DMA_PHY_STAT) {                   /*  ���״̬                    */
        return  (LW_DMA_STATUS_BUSY);
    } else {
        return  (LW_DMA_STATUS_IDLE);
    }
}
/*********************************************************************************************************
** Function name:           __dmaHwTransact
** Descriptions:            ��ʼ��һ�� DMA ����
** input parameters:        iChannel:                 DMA ͨ����
**                          pdmafuncs                 û��ʹ��
**                          pdmatMsg:                 DMA װ�ز���
** output parameters:       NONE
** Returned value:          -1:����   0:����
** Created by:              Hanhui
** Created Date:            2007/10/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int  __dmaHwTransact (UINT   uiChannel, PLW_DMA_FUNCS  pdmafuncs, PLW_DMA_TRANSACTION  pdmatMsg)
{
    __DMA_PHY_CHANNEL   *pdmaphychanCtl = __GpdmaphychanTbl[uiChannel];
    
    if (uiChannel >= __2440_DMA_CHAN_NUM) {
        return  (-1);                                                   /*  ͨ������                    */
    }
    
    if (!pdmatMsg) {
        return  (-1);                                                   /*  ��Ϣָ�����                */
    }
    
    /*
     *  �����ַ��Ϣ
     */
    pdmaphychanCtl->uiSrcAddr = (unsigned int)pdmatMsg->DMAT_pucSrcAddress;
    pdmaphychanCtl->uiDstAddr = (unsigned int)pdmatMsg->DMAT_pucDestAddress;
    
    /*
     *  ����Դ�˵�ַ������Ϣ
     */
    switch (pdmatMsg->DMAT_iSrcAddrCtl) {
    
    case LW_DMA_ADDR_INC:
        if (pdmatMsg->DMAT_ulOption & DMA_OPTION_SRCBUS_APB) {          /*  Դ��Ϊ APB ����             */
            pdmaphychanCtl->uiSrcCtl = (1 << 1);
        } else {                                                        /*  Դ��Ϊ AHB ����             */
            pdmaphychanCtl->uiSrcCtl = 0;
        }
        break;
        
    case LW_DMA_ADDR_FIX:
        if (pdmatMsg->DMAT_ulOption & DMA_OPTION_SRCBUS_APB) {          /*  Դ��Ϊ APB ����             */
            pdmaphychanCtl->uiSrcCtl = ((1 << 1) | 1);
        } else {                                                        /*  Դ��Ϊ AHB ����             */
            pdmaphychanCtl->uiSrcCtl = 1;
        }
        break;
        
    default:
        return  (PX_ERROR);                                             /*  ��֧��                      */
    }
    
    /*
     *  ����Ŀ�Ķ˵�ַ������Ϣ
     */
    switch (pdmatMsg->DMAT_iDestAddrCtl) {
    
    case LW_DMA_ADDR_INC:
        if (pdmatMsg->DMAT_ulOption & DMA_OPTION_DSTBUS_APB) {          /*  Ŀ�Ķ�Ϊ APB ����           */
            pdmaphychanCtl->uiDstCtl = (1 << 1);
        } else {                                                        /*  Ŀ�Ķ�Ϊ AHB ����           */
            pdmaphychanCtl->uiDstCtl = 0;
        }
        break;
        
    case LW_DMA_ADDR_FIX:
        if (pdmatMsg->DMAT_ulOption & DMA_OPTION_DSTBUS_APB) {          /*  Ŀ�Ķ�Ϊ APB ����           */
            pdmaphychanCtl->uiDstCtl = ((1 << 1) | 1);
        } else {                                                        /*  Ŀ�Ķ�Ϊ AHB ����           */
            pdmaphychanCtl->uiDstCtl = 1;
        }
        break;
        
    default:
        return  (PX_ERROR);                                             /*  ��֧��                      */
    }
    
    /*
     *  ���� DMA ַ������Ϣ
     */
    {
        int     iHandshake     = 0;                                     /*  ������ģʽ��������          */
        int     iSyncClk       = 0;                                     /*  �����ͬ��ʱ��              */
        int     iInterEn       = 1;                                     /*  �����ж�                    */
        int     iTransferMode  = 0;                                     /*  �䷢���ִ���              */
        int     iServiceMode   = 0;                                     /*  ��ȫ or ���δ���ģʽ        */
        int     iReqScr        = 0;                                     /*  ����Դ                      */
        int     iSwOrHwReg     = 0;                                     /*  ����������ʽ                */
        int     iAutoReloadDis = 1;                                     /*  �Ƿ�����Զ�����            */
        int     iDataSizeOnce  = 0;                                     /*  һ�δ�������ݿ��          */
        int     iLength;                                                /*  ����ĳ���                  */
        
        if (pdmatMsg->DMAT_bHwHandshakeEn) {
            iHandshake = 1;                                             /*  ʹ��Ӳ������                */
        }
        
        if (pdmatMsg->DMAT_iTransMode & DMA_TRANSMODE_WHOLE) {
            iServiceMode = 1;                                           /*  ��ȫ����ģʽ                */
        }
        
        if (pdmatMsg->DMAT_iTransMode & DMA_TRANSMODE_CLKAHB) {
            iSyncClk = 1;                                               /*  AHB ʱ��Դ                  */
        }
        
        if (pdmatMsg->DMAT_iTransMode & DMA_TRANSMODE_BURST) {
            iTransferMode = 1;                                          /*  �䷢ģʽ                    */
        }
        
        iReqScr = pdmatMsg->DMAT_iHwReqNum;                             /*  ����Դ���                  */
        
        if (pdmatMsg->DMAT_bHwReqEn) {
            iSwOrHwReg = 1;                                             /*  Ӳ����������                */
        }
        
        if (pdmatMsg->DMAT_iTransMode & DMA_TRANSMODE_DBYTE) {
            iDataSizeOnce = 1;                                          /*  ���ִ���                    */
        } else if (pdmatMsg->DMAT_iTransMode & DMA_TRANSMODE_4BYTE) {
            iDataSizeOnce = 2;                                          /*  �ִ���                      */
        }
        
        switch (iDataSizeOnce) {                                        /*  ȷ�����䳤��                */
        
        case 0:
            iLength = (INT)pdmatMsg->DMAT_stDataBytes;
            break;
            
        case 1:
            iLength = (INT)pdmatMsg->DMAT_stDataBytes / 2;
            break;
            
        case 2:
            iLength = (INT)pdmatMsg->DMAT_stDataBytes / 4;
            break;
        }
        
        pdmaphychanCtl->uiDMACtl = ((unsigned)iHandshake << 31)
                                 | (iSyncClk       << 30)
                                 | (iInterEn       << 29)
                                 | (iTransferMode  << 28)
                                 | (iServiceMode   << 27)
                                 | (iReqScr        << 24)
                                 | (iSwOrHwReg     << 23)
                                 | (iAutoReloadDis << 22)
                                 | (iDataSizeOnce  << 20)
                                 | (iLength);                           /*  ���ÿ��ƼĴ���              */
                                 
        /*
         *  ���� DMA 
         */
        if (iSwOrHwReg == 0) {                                          /*  ѡ�����������ʽ            */
            pdmaphychanCtl->uiMaskTigger = 
                            (__DMA_PHY_ON | __DMA_SW_TRIGGER);          /*  �����������                */
        } else {
            pdmaphychanCtl->uiMaskTigger = __DMA_PHY_ON;                /*  ����ͨ�����ȴ�Ӳ������    */
        }
    }

    return  (0);
}
/*********************************************************************************************************
** Function name:           dmaGetFuncs
** Descriptions:            ��ȡ 2440 DMA ����
** input parameters:        iChannel        DMA ͨ����
** output parameters:       pulMaxBytes     ������ֽ���
** Returned value:          ָ��ͨ�� DMA ����������������
** Created by:              Hanhui
** Created Date:            2007/10/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
PLW_DMA_FUNCS  dmaGetFuncs (int   iChannel, ULONG   *pulMaxBytes)
{
    static LW_DMA_FUNCS     pdmafuncsS3c2440a;
    
    if (pdmafuncsS3c2440a.DMAF_pfuncReset == LW_NULL) { 
        pdmafuncsS3c2440a.DMAF_pfuncReset  = __dmaHwReset;
        pdmafuncsS3c2440a.DMAF_pfuncTrans  = __dmaHwTransact;
        pdmafuncsS3c2440a.DMAF_pfuncStatus = __dmaHwGetStatus;
    }
    
    if (pulMaxBytes) {
        *pulMaxBytes = (1 * LW_CFG_MB_SIZE);
    }
    
    return  (&pdmafuncsS3c2440a);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
