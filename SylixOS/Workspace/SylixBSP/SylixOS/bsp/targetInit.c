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
** ��   ��   ��: targetInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 12 �� 09 ��
**
** ��        ��: S3C2440 �ؼ�Ӳ����ʼ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define __MDIV_INDEX        0
#define __PDIV_INDEX        1
#define __SDIV_INDEX        2
/*********************************************************************************************************
  PLL ���ñ� (2: �����ⲿ���� 7: �������÷�ʽ 3: ÿ�����÷�ʽ 3 ������)
*********************************************************************************************************/
static const unsigned int  __GuiPllCfgTbl[2][7][3] = {

    {                                                                   /*  12.0000 MHz ������Ϣ        */
        {56,  2, 2},                                                    /*  48.00  MHz for USB          */
        {56,  2, 1},                                                    /*  96.00  MHz for USB          */
        {173, 2, 2},                                                    /*  271.50 MHz for CPU          */
        {68,  1, 1},                                                    /*  304.00 MHz for CPU          */
        {127, 2, 1},                                                    /*  405.00 MHz for CPU          */
        {125, 1, 1},                                                    /*  532.00 MHz for CPU          */
        {127, 2, 1},                                                    /*  405.00 MHz for CPU          */
    },

    {                                                                   /*  16.9344 MHz ������Ϣ        */
        {60,  4, 2},                                                    /*  47.98  MHz for USB          */
        {60,  4, 1},                                                    /*  95.96  MHz for USB          */
        {118, 2, 2},                                                    /*  266.72 MHz for CPU          */
        {97,  1, 2},                                                    /*  296.35 MHz for CPU          */
        {110, 3, 1},                                                    /*  399.65 MHz for CPU          */
        {86,  1, 1},                                                    /*  530.61 MHz for CPU          */
        {118, 1, 1},                                                    /*  533.43 MHz for CPU          */
    },
};
/*********************************************************************************************************
  PLL Ƶ�ʱ�
*********************************************************************************************************/
const unsigned int    __GuiPllFreqTbl[2][7] = {

    {                                                                   /*  12.0000 MHz ����Ƶ��        */
         48000000,
         96000000,
        271500000,
        304000000,
        405000000,
        532000000,
        405000000,
    },

    {                                                                   /*  16.9344 MHz ����Ƶ��        */
         47980000,
         95960000,
        266720000,
        296350000,
        399650000,
        530610000,
        533430000,
    },
};
/*********************************************************************************************************
  SDRAM ���ñ�
*********************************************************************************************************/
static const unsigned int  __GuiSdram100Cfg[]  = {                      /*  ���ⲿ��������Ϊ100M        */
    0x12111110, 0x00000700, 0x00000700, 0x00000700,
    0x00000700, 0x00000700, 0x00002D50, 0x00018005,
    0x00018005, 0x009804EC, 0x000000B2, 0x00000030,
    0x00000030,
};
static const unsigned int  __GuiSdram133Cfg[]  = {                      /*  ���ⲿ��������Ϊ133M        */
    0x12111110, 0x00000700, 0x00000700, 0x00000700,
    0x00000700, 0x00000700, 0x00002D50, 0x00018005,
    0x00018005, 0x009803E4, 0x000000B2, 0x00000020,
    0x00000020,
};
/*********************************************************************************************************
** ��������: sdramInit
** ��������: ��ʼ�� SDRAM
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
void sdramInit (void)
{
    rBWSCON     = 0x12111110;
    rBANKCON0   = 0x00000700;
    rBANKCON1   = 0x00000700;
    rBANKCON2   = 0x00000700;
    rBANKCON3   = 0x00000700;
    rBANKCON4   = 0x00000700;
    rBANKCON5   = 0x00002D50;
    rBANKCON6   = 0x00018005;
    rBANKCON7   = 0x00018005;
    rREFRESH    = 0x009804EC;
    rBANKSIZE   = 0x000000B2;
    rMRSRB6     = 0x00000030;
    rMRSRB7     = 0x00000030;
}
/*********************************************************************************************************
** ��������: pllInit
** ��������: ��ʼ�� PLL
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void  pllInit (void)
{
    volatile int    i;
    unsigned int    uiMDiv, uiPDiv, uiSDiv;

    uiMDiv = __GuiPllCfgTbl[INPUT_FREQ][UPLL_FREQ][__MDIV_INDEX];
    uiPDiv = __GuiPllCfgTbl[INPUT_FREQ][UPLL_FREQ][__PDIV_INDEX];
    uiSDiv = __GuiPllCfgTbl[INPUT_FREQ][UPLL_FREQ][__SDIV_INDEX];

    /*
     *  UPLL SETTING
     */
    rUPLLCON  = (unsigned int)((56 << 12) + (2 << 4) + (1));            /*  ���� 96 MHz                 */
    rLOCKTIME = 0x1FFFFFFF;                                             /*  PLL LOCK Time               */
    for (i = 0; i < 500; i++) {
    }
                                                                        /*  ���� 48 MHz                 */
    rUPLLCON = (unsigned int)((uiMDiv << 12) + (uiPDiv << 4) + (uiSDiv));

    {
        volatile int    x;
        for (x = 0; x < 50; x++);
    }

    uiMDiv = __GuiPllCfgTbl[INPUT_FREQ][MPLL_FREQ][__MDIV_INDEX];
    uiPDiv = __GuiPllCfgTbl[INPUT_FREQ][MPLL_FREQ][__PDIV_INDEX];
    uiSDiv = __GuiPllCfgTbl[INPUT_FREQ][MPLL_FREQ][__SDIV_INDEX];

    /*
     *  MPLL SETTING
     */
    rMPLLCON = (unsigned int)((uiMDiv << 12) + (uiPDiv << 4) + (uiSDiv));

    rLOCKTIME = 0x1FFFFFFF;                                             /*  PLL LOCK Time               */
}
/*********************************************************************************************************
** ��������: intInit
** ��������: �жϳ�ʼ��
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void  intInit (void)
{
    INTER_ALL_IRQ();                                                    /*  �����ж϶�Ϊ IRQ ģʽ       */
    INTER_ALL_MSK();                                                    /*  ���������ж�                */

    INTER_CLR_SUBSRCPND(BIT_SUB_ALLMSK);                                /*  ����������ж�              */
    INTER_CLR_SRCPND(BIT_ALLMSK);                                       /*  �������Ӳ���ж�            */
    INTER_CLR_INTPND(BIT_ALLMSK);
}
/*********************************************************************************************************
** ��������: clockInit
** ��������: ʱ�ӳ�ʼ��
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void  clockInit (void)
{
    unsigned int    uiHDiv, uiPDiv;

    switch (AHB_DIV) {

    case __AHBCLK_DIV2:                                                 /*  FCLK / HCLK = 2             */
        uiHDiv = 1;
        break;

    case __AHBCLK_DIV3:                                                 /*  FCLK / HCLK = 3             */
        uiHDiv = 3;
        BIT_CLR(rCAMDIVN, 8);
        break;

    case __AHBCLK_DIV4:                                                 /*  FCLK / HCLK = 4             */
        uiHDiv = 2;
        BIT_CLR(rCAMDIVN, 9);
        break;

    case __AHBCLK_DIV6:                                                 /*  FCLK / HCLK = 6             */
        uiHDiv = 3;
        BIT_SET(rCAMDIVN, 8);
        break;

    case __AHBCLK_DIV8:                                                 /*  FCLK / HCLK = 8             */
        uiHDiv = 2;
        BIT_SET(rCAMDIVN, 9);
        break;

    default:                                                            /*  FCLK / HCLK = 4             */
        uiHDiv = 2;
        BIT_CLR(rCAMDIVN, 9);
    }

    switch (APB_DIV) {

    case __APBCLK_DIV1:                                                 /*  HCLK / PCLK = 1             */
        uiPDiv = 0;
        break;

    case __APBCLK_DIV2:                                                 /*  HCLK / PCLK = 2             */
        uiPDiv = 1;
        break;

    default:
        uiPDiv = 1;
        break;
    }

    rCLKDIVN = (uiHDiv << 1) + (uiPDiv);                                /*  ���÷�Ƶ�Ĵ���              */

    if (UPLL_FREQ == __UPLL_FREQ_96_MHZ) {
        BIT_SET(rCLKDIVN, 3);
    }

    armAsyncBusMode();                                                  /*  �첽���߹���ģʽ            */
}
/*********************************************************************************************************
** ��������: targetInit
** ��������: ϵͳ��ʼ��
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
void  targetInit (void)
{
    intInit();
    clockInit();
    pllInit();
}
/*********************************************************************************************************
  semihosting
*********************************************************************************************************/
#define import(__use_no_semihosting_swi)

void _ttywrch (int ch)
{
int uartSendByte(int   iCom, unsigned char  ucData);
void uartSendString(int   iCom, char  *pcData);

    uartSendString(0, "__use_no_semihosting_swi _ttywrch()!\n");
    uartSendByte(0, (unsigned char)ch);
    while (1);
}

void _sys_exit (int return_code) 
{
void uartSendString(int   iCom, char  *pcData);
    
    uartSendString(0, "__use_no_semihosting_swi _sys_exit()!\n");
    while (1);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
