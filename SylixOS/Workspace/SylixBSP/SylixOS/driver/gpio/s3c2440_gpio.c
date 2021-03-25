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
** ��   ��   ��: s3c2440_gpio.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2014 �� 05 �� 17 ��
**
** ��        ��: S3C2440 GPIO ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"                                                    /*  ����ϵͳ                    */
#include "s3c2440_gpio.h"
/*********************************************************************************************************
** ��������: s3c2440GpioGetDirection
** ��������: ���ָ�� GPIO ����
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: ���� 1:���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3c2440GpioGetDirection (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT32  uiTemp;

    if (uiOffset == 0) {
        uiTemp = (rGPBCON >> 2) & 0x3;
        if (uiTemp == 0) {
            return  (0);
        } else if (uiTemp == 1) {
            return  (1);
        } else {
            return  (-1);
        }
    } else if (uiOffset == 1) {
        return  (0);
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** ��������: s3c2440GpioDirectionInput
** ��������: ����ָ�� GPIO Ϊ����ģʽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3c2440GpioDirectionInput (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT32  uiTemp;

    if (uiOffset == 0) {
        uiTemp = rGPBCON;
        uiTemp &= ~(3 << 2);
        rGPBCON = uiTemp;

        return  (0);
    } else if (uiOffset == 1) {
        return  (0);
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** ��������: s3c2440GpioGet
** ��������: ���ָ�� GPIO ��ƽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: �͵�ƽ 1:�ߵ�ƽ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3c2440GpioGet (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT32  uiValue;
    UINT32  uiTemp;

    if (uiOffset == 0) {
        uiTemp = rGPBDAT;
        uiValue = (uiTemp & (1 << 1));
        if (uiValue) {
            return  (1);
        } else {
            return  (0);
        }
    } else if (uiOffset == 1) {
        return  (0);
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** ��������: s3c2440GpioDirectionOutput
** ��������: ����ָ�� GPIO Ϊ���ģʽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iValue      �����ƽ
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3c2440GpioDirectionOutput (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, INT  iValue)
{
    UINT32  uiTemp;

    if (uiOffset == 0) {
        uiTemp = rGPBCON;
        uiTemp = (uiTemp & ~(3U << 2)) | (1U << 2);
        rGPBCON = uiTemp;

        uiTemp = rGPBDAT;
        if (iValue) {
            uiTemp |= (1 << 1);
        } else {
            uiTemp &= ~(1 << 1);
        }
        rGPBDAT = uiTemp;

        return  (0);
    } else if (uiOffset == 1) {
        return  (0);
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** ��������: s3c2440GpioSet
** ��������: ����ָ�� GPIO ��ƽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iValue      �����ƽ
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3c2440GpioSet (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, INT  iValue)
{
    UINT32  uiTemp;

    if (uiOffset != 0) {
        return;
    }

    uiTemp = rGPBDAT;
    if (iValue) {
        uiTemp |= (1 << 1);
    } else {
        uiTemp &= ~(1 << 1);
    }
    rGPBDAT = uiTemp;
}
/*********************************************************************************************************
** ��������: s3c2440GpioSetupIrq
** ��������: ����ָ�� GPIO Ϊ�ⲿ�ж�����ܽ�
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           bIsLevel    �Ƿ�Ϊ��ƽ����
**           uiType      ���Ϊ��ƽ����, 1 ��ʾ�ߵ�ƽ����, 0 ��ʾ�͵�ƽ����
**                       ���Ϊ���ش���, 1 ��ʾ�����ش���, 0 ��ʾ�½��ش���, 2 ˫���ش���
** ��  ��  : IRQ ������ -1:����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  s3c2440GpioSetupIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, BOOL  bIsLevel, UINT  uiType)
{
    UINT32  uiValue;

    if (uiOffset != 1) {
        return  (-1);
    }

    rGPFCON   = (rGPFCON & (~(0x03 << 14))) | (0x02 << 14);             /*  ���� GPF7 Ϊ EINT7          */

    rGPFUP    = rGPFUP | (1 << 7);                                      /*  �ر� GPF7 ����������        */

    if (bIsLevel) {
        if (uiType) {
            uiValue = 0x01;
        } else {
            uiValue = 0x00;
        }
    } else {
        if (uiType == 0) {
            uiValue = 0x02;
        } else if (uiType == 1) {
            uiValue = 0x04;
        } else {
            uiValue = 0x06;
        }
    }

    rEXTINT0  = (rEXTINT0 & (~(0x07 << 28))) | (uiValue << 28);         /*  ���� EINT7 �жϴ�����ʽ     */

    rEINTMASK = rEINTMASK & (~(1 << 7));                                /*  ʹ�� EINT7                  */

    return  (VIC_CHANNEL_EINT4_7);
}
/*********************************************************************************************************
** ��������: s3c2440GpioClearIrq
** ��������: ���ָ�� GPIO �жϱ�־
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3c2440GpioClearIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT32  uiTemp;

    if (uiOffset != 1) {
        return;
    }

    uiTemp = rEINTPEND;
    rEINTPEND = uiTemp;

    INTER_CLR_PNDING(BIT_EINT4_7);
}
/*********************************************************************************************************
** ��������: s3c2440GpioSvrIrq
** ��������: �ж� GPIO �жϱ�־
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  s3c2440GpioSvrIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT32  uiTemp;

    if (uiOffset != 1) {
        return  (LW_IRQ_NONE);
    }

    uiTemp = rEINTPEND;

    if (uiTemp & (1 << 7)) {
        return  (LW_IRQ_HANDLED);
    } else {
        return  (LW_IRQ_NONE);
    }
}
/*********************************************************************************************************
  GPIO ��������
*********************************************************************************************************/
static LW_GPIO_CHIP  _G_s3c2440GpioChip = {
        .GC_pcLabel              = "S3C2440 GPIO",
        .GC_ulVerMagic           = LW_GPIO_VER_MAGIC,

        .GC_pfuncRequest         = LW_NULL,
        .GC_pfuncFree            = LW_NULL,

        .GC_pfuncGetDirection    = s3c2440GpioGetDirection,
        .GC_pfuncDirectionInput  = s3c2440GpioDirectionInput,
        .GC_pfuncGet             = s3c2440GpioGet,
        .GC_pfuncDirectionOutput = s3c2440GpioDirectionOutput,
        .GC_pfuncSetDebounce     = LW_NULL,
        .GC_pfuncSetPull         = LW_NULL,
        .GC_pfuncSet             = s3c2440GpioSet,
        .GC_pfuncSetupIrq        = s3c2440GpioSetupIrq,
        .GC_pfuncClearIrq        = s3c2440GpioClearIrq,
        .GC_pfuncSvrIrq          = s3c2440GpioSvrIrq,

        .GC_uiBase               = S3C2440_GPIO_BASE,
        .GC_uiNGpios             = 2,
};
/*********************************************************************************************************
** ��������: s3c2440GpioDrv
** ��������: ���� S3C2440 GPIO ����
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  s3c2440GpioDrv (VOID)
{
    gpioChipAdd(&_G_s3c2440GpioChip);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
