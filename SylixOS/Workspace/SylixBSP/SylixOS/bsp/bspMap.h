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
** ��   ��   ��: bspMap.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 23 ��
**
** ��        ��: C ������ڲ���. �����ҳ�ռ���ȫ��ӳ���ϵ����.
*********************************************************************************************************/

#ifndef __BSPMAP_H
#define __BSPMAP_H

/*********************************************************************************************************
   �ڴ�����ϵͼ

    +-----------------------+--------------------------------+
    |       ͨ���ڴ���      |          VMM ������            |
    |         CACHE         |                                |
    +-----------------------+--------------------------------+

*********************************************************************************************************/
/*********************************************************************************************************
  physical memory
*********************************************************************************************************/
#ifdef  __BSPINIT_MAIN_FILE

LW_MMU_PHYSICAL_DESC    _G_physicalDesc[] = {
    {                                                                   /*  �ж�������                  */
        BSP_CFG_RAM_BASE,
        0,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_VECTOR
    },

    {                                                                   /*  �ں˴����                  */
        BSP_CFG_RAM_BASE,
        BSP_CFG_RAM_BASE,
        BSP_CFG_TEXT_SIZE,
        LW_PHYSICAL_MEM_TEXT
    },

    {                                                                   /*  �ں����ݶ�                  */
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE,
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE,
        BSP_CFG_DATA_SIZE,
        LW_PHYSICAL_MEM_DATA
    },

    {                                                                   /*  DMA ������                  */
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE + BSP_CFG_DATA_SIZE,
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE + BSP_CFG_DATA_SIZE,
        BSP_CFG_DMA_SIZE,
        LW_PHYSICAL_MEM_DMA
    },

    {                                                                   /*  APP ͨ���ڴ�                */
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE + BSP_CFG_DATA_SIZE + BSP_CFG_DMA_SIZE,
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE + BSP_CFG_DATA_SIZE + BSP_CFG_DMA_SIZE,
        BSP_CFG_APP_SIZE,
        LW_PHYSICAL_MEM_APP
    },

    /*
     *  external io & memory interface
     */
    {                                                                   /*  BANK4 - CAN CONTROLER       */
        0x20000000,
        0x20000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR                                         /*  ״̬���� NCNB               */
    },
    {                                                                   /*  BANK5 - AX88796             */
        0x28000000,
        0x28000000,
        (1 * 1024 * 1024),
        LW_PHYSICAL_MEM_BOOTSFR                                         /*  ״̬���� NCNB               */
    },
    
    /*
     *  internal sfr area
     */
    {                                                                   /*  memory controller           */
        0x48000000,
        0x48000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  USB HOST controller         */
        0x49000000,
        0x49000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  INTERRUPT controller        */
        0x4a000000,
        0x4a000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  DMA controller              */
        0x4b000000,
        0x4b000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  CLOCK & POWER controller    */
        0x4c000000,
        0x4c000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  LCD controller              */
        0x4d000000,
        0x4d000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  NAND FLASH controller       */
        0x4E000000,
        0x4E000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  CAMERA controller           */
        0x4F000000,
        0x4F000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  UART0 controller            */
        0x50000000,
        0x50000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  UART1 controller            */
        0x50004000,
        0x50004000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  UART2 controller            */
        0x50008000,
        0x50008000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  PWM TIMER controller        */
        0x51000000,
        0x51000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  USB DEV controller          */
        0x52000000,
        0x52000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  WATCH DOG TIMER controller  */
        0x53000000,
        0x53000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  IIC controller              */
        0x54000000,
        0x54000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  IIS controller              */
        0x55000000,
        0x55000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  I/O PORT  controller        */
        0x56000000,
        0x56000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  RTC  controller             */
        0x57000000,
        0x57000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  A/DC  controller            */
        0x58000000,
        0x58000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
    
    {                                                                   /*  SPI  controller             */
        0x59000000,
        0x59000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },

    {                                                                   /*  SD Interface  controller    */
        0x5a000000,
        0x5a000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },

    {                                                                   /*  ����                        */
        0,
        0,
        0,
        0
    }
};
/*********************************************************************************************************
  virtual memory
*********************************************************************************************************/
LW_MMU_VIRTUAL_DESC    _G_virtualDesc[] = {
    {                                                                   /*  Ӧ�ó�������ռ�            */
        0x60000000,
        ((size_t)2 * LW_CFG_GB_SIZE),
        LW_VIRTUAL_MEM_APP
    },

    {
        0xe0000000,                                                     /*  ioremap �ռ�                */
        (256 * LW_CFG_MB_SIZE),
        LW_VIRTUAL_MEM_DEV
    },

    {                                                                   /*  ����                        */
        0,
        0,
        0
    }
};

#endif                                                                  /*  __BSPINIT_MAIN_FILE         */
#endif                                                                  /*  __BSPMAP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
