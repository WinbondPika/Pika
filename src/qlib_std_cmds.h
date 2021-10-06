/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_std_cmds.h
* @brief      This file contains Winbond flash devices (W25Qxx) definitions
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_STD_CMDS_H__
#define __QLIB_STD_CMDS_H__

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#define STD_FLASH_MANUFACTURER__WINBOND_SERIAL_FLASH 0xEF
#define STD_FLASH_DEVICE_ID__W25Q256JW               0x18
#define STD_FLASH_DEVICE_ID__W25Q128JW               0x17
#define STD_FLASH_DEVICE_ID__W25Q32JW                0x15
#define STD_FLASH_DEVICE_ID__W25Q16JW                0x14
#define STD_FLASH_DEVICE_ID__W77Q                    STD_FLASH_DEVICE_ID__W25Q32JW

/*---------------------------------------------------------------------------------------------------------*/
/* Flash characteristics                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define FLASH_BLOCK_SIZE  _64KB_
#define FLASH_SECTOR_SIZE _4KB_  // minimum erase size
#define FLASH_PAGE_SIZE   _256B_ // maximum write size

#define SPI_FLASH_MAKE_CMD(cmdBuf, cmd) \
    memset(cmdBuf, 0, sizeof(cmdBuf));  \
    cmdBuf[0] = cmd;

#define SPI_FLASH_MAKE_CMD_ADDR(cmdBuf, cmd, addr) \
    SPI_FLASH_MAKE_CMD(cmdBuf, cmd)                \
    cmdBuf[1] = LSB2(addr);                        \
    cmdBuf[2] = LSB1(addr);                        \
    cmdBuf[3] = LSB0(addr);

#define SPI_FLASH_MAKE_CMD_ADDR_DATA(cmdBuf, cmd, addr, outData) \
    SPI_FLASH_MAKE_CMD_ADDR(cmdBuf, cmd, addr)                   \
    memcpy(&cmdBuf[4], outData, sizeof(outData));

/*---------------------------------------------------------------------------------------------------------*/
/* Standard SPI Instructions                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__NONE 0x00

#define SPI_FLASH_CMD__DEVICE_ID                       0xAB
#define SPI_FLASH_CMD__MANUFACTURER_AND_DEVICE_ID      0x90
#define SPI_FLASH_CMD__MANUFACTURER_AND_DEVICE_ID_DUAL 0x92
#define SPI_FLASH_CMD__MANUFACTURER_AND_DEVICE_ID_QUAD 0x94
#define SPI_FLASH_CMD__READ_JEDEC                      0x9F
#define SPI_FLASH_CMD__READ_UNIQUE_ID                  0x4B
#define SPI_FLASH_CMD__READ_SFDP_TABLE                 0x5A
#define SPI_FLASH_CMD__SET_BURST_WITH_WRAP             0x77

/*---------------------------------------------------------------------------------------------------------*/
/* Status Registers                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__READ_STATUS_REGISTER_1 0x05
#define SPI_FLASH_CMD__READ_STATUS_REGISTER_2 0x35
#define SPI_FLASH_CMD__READ_STATUS_REGISTER_3 0x15

#define SPI_FLASH_CMD__REGISTER_WRITE_ENABLE   0x50
#define SPI_FLASH_CMD__WRITE_STATUS_REGISTER_1 0x01
#define SPI_FLASH_CMD__WRITE_STATUS_REGISTER_2 0x31
#define SPI_FLASH_CMD__WRITE_STATUS_REGISTER_3 0x11

/*---------------------------------------------------------------------------------------------------------*/
/* QPI mode                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__ENTER_QPI           0x38
#define SPI_FLASH_CMD__EXIT_QPI            0xFF
#define SPI_FLASH_CMD__SET_READ_PARAMETERS 0xC0

/*---------------------------------------------------------------------------------------------------------*/
/* Power Instructions                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__POWER_DOWN         0xB9
#define SPI_FLASH_CMD__RELEASE_POWER_DOWN 0xAB

/*---------------------------------------------------------------------------------------------------------*/
/* Reset Instructions                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__RESET_ENABLE 0x66
#define SPI_FLASH_CMD__RESET_DEVICE 0x99

/*---------------------------------------------------------------------------------------------------------*/
/* Erase / Program Suspend / Resume Instructions                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__SUSPEND 0x75
#define SPI_FLASH_CMD__RESUME  0x7A

/*---------------------------------------------------------------------------------------------------------*/
/* Die select Instruction                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__DIE_SELECT 0xC2

/************************************************************************************************************
 * Read Instructions
************************************************************************************************************/
// *** The Quad Enable (QE) bit in Status Register-2 must be set to 1 before the device will accept the Fast Read Quad Output Instruction
#define SPI_FLASH_CMD__READ_DATA__1_1_1      0x03 // single  - 0 Dummy clocks
#define SPI_FLASH_CMD__READ_FAST__1_1_1      0x0B // single  - 8 Dummy clocks
#define SPI_FLASH_CMD__READ_FAST__1_1_2      0x3B // dual    - 8 Dummy clocks
#define SPI_FLASH_CMD__READ_FAST__1_2_2      0xBB // dual    - 4 Dummy clocks (Continuous Read Mode allowed)
#define SPI_FLASH_CMD__READ_FAST__1_1_4      0x6B // quad    - 8 Dummy clocks
#define SPI_FLASH_CMD__READ_FAST__1_4_4      0xEB // quad    - 6 Dummy clocks (Continuous Read Mode allowed)
#define SPI_FLASH_CMD__READ_FAST__4_4_4      0x0B // QPI     - 2 Dummy clocks (Can be configured to 2/4/6/8)
#define SPI_FLASH_CMD__READ_FAST_WRAP__4_4_4 0x0C // QPI     - 2 Dummy clocks (Can be configured to 2/4/6/8)
#define SPI_FLASH_CMD__READ_FAST_DTR__1_1_1  0x0D // DTR     - 6/8 Dummy clocks
#define SPI_FLASH_CMD__READ_FAST_DTR__1_2_2  0xBD // DTR     - 6/8 Dummy clocks (Continuous Read Mode allowed)
#define SPI_FLASH_CMD__READ_FAST_DTR__1_4_4  0xED // DTR     - 8 Dummy clocks (Continuous Read Mode allowed)
#define SPI_FLASH_CMD__READ_FAST_DTR__4_4_4  0x0D // DTR+QPI - 8 Dummy clocks

/*---------------------------------------------------------------------------------------------------------*/
/* Write Instructions                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__WRITE_ENABLE  0x06
#define SPI_FLASH_CMD__WRITE_DISABLE 0x04

#define SPI_FLASH_CMD__PAGE_PROGRAM       0x02
#define SPI_FLASH_CMD__PAGE_PROGRAM_1_1_4 0x32

/*---------------------------------------------------------------------------------------------------------*/
/* Erase Instructions                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__ERASE_SECTOR          0x20
#define SPI_FLASH_CMD__ERASE_BLOCK_32        0x52
#define SPI_FLASH_CMD__ERASE_BLOCK_64        0xD8
#define SPI_FLASH_CMD__ERASE_CHIP            0xC7
#define SPI_FLASH_CMD__ERASE_CHIP_DEPRECATED 0x60

/*---------------------------------------------------------------------------------------------------------*/
/* 3 x 256B Security registers Instructions                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__SEC_REG_READ    0x48
#define SPI_FLASH_CMD__SEC_REG_PROGRAM 0x42
#define SPI_FLASH_CMD__SEC_REG_ERASE   0x44

/*---------------------------------------------------------------------------------------------------------*/
/* Block / Sector locking Instructions                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__INDIVIDUAL_BLOCK_SECTOR_LOCK   0x36
#define SPI_FLASH_CMD__INDIVIDUAL_BLOCK_SECTOR_UNLOCK 0x39
#define SPI_FLASH_CMD__READ_BLOCK_SECTOR_LOCK_STATE   0x3D
#define SPI_FLASH_CMD__GLOBAL_BLOCK_SECTOR_LOCK       0x7E
#define SPI_FLASH_CMD__GLOBAL_BLOCK_SECTOR_UNLOCK     0x98

/*---------------------------------------------------------------------------------------------------------*/
/* Address mode Instructions                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_CMD__4_BYTE_ADDRESS_MODE_ENTER 0xB7
#define SPI_FLASH_CMD__4_BYTE_ADDRESS_MODE_EXIT  0xE9

/*---------------------------------------------------------------------------------------------------------*/
/* Instructions size                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_UNIQUE_ID_SIZE  8
#define SPI_FLASH_SFDP_TABLE_SIZE 256

/*---------------------------------------------------------------------------------------------------------*/
/* Status registers fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH__STATUS_1_FIELD__BUSY 0, 1 // ERASE/WRITE IN PROGRESS
#define SPI_FLASH__STATUS_1_FIELD__WEL  1, 1 // WRITE ENABLE LATCH
#define SPI_FLASH__STATUS_1_FIELD__BP   2, 3 // BLOCK PROTECT BITS
#define SPI_FLASH__STATUS_1_FIELD__TB   5, 1 // TOP/BOTTOM PROTECT
#define SPI_FLASH__STATUS_1_FIELD__SEC  6, 1 // SECTOR PROTECT
#define SPI_FLASH__STATUS_1_FIELD__SRP  7, 1 // STATUS REGISTER PROTECT

#define SPI_FLASH__STATUS_2_FIELD__SRL       0, 1 // Status Register Lock
#define SPI_FLASH__STATUS_2_FIELD__QE        1, 1 // Quad Enable
#define SPI_FLASH__STATUS_2_FIELD__RESERVED1 2, 1 // Reserved
#define SPI_FLASH__STATUS_2_FIELD__LB        3, 3 // Security Register Lock Bits
#define SPI_FLASH__STATUS_2_FIELD__CMP       6, 1 // Complement Protect
#define SPI_FLASH__STATUS_2_FIELD__SUS       7, 1 // Suspend Status (read only)

#define SPI_FLASH__STATUS_3_FIELD__A24 0, 1 // Extended Address Bit 24
#define SPI_FLASH__STATUS_3_FIELD__WPS      2, 1 // Write Protect Selection
#define SPI_FLASH__STATUS_3_FIELD__DRV      5, 2 // Output Driver Strength
#define SPI_FLASH__STATUS_3_FIELD__HOLD_RST 7, 1 // Set hold/reset pin function

/************************************************************************************************************
 * Status registers type
************************************************************************************************************/
typedef struct
{
    // Status register 1
    union
    {
        U8 asUint;
        struct
        {
            U8 busy : 1;
            U8 writeEnable : 1;
            U8 BP : 3;
            U8 TB : 1;
            U8 SEC : 1;
            U8 SRP : 1;
        } asStruct;
    } SR1;

    // Status register 2
    union
    {
        U8 asUint;
        struct
        {
            U8 SRL : 1;        ///< Status Register Lock
            U8 quadEnable : 1; ///< Quad Enable
            U8 R : 1;          ///< Reserved
            U8 LB : 3;         ///< Security Register Lock Bits
            U8 CMP : 1;        ///< Complement Protect
            U8 suspend : 1;    ///< Suspend Status (read only)
        } asStruct;
    } SR2;

    union
    {
        U8 asUint;

        struct
        {
            U8 A24 : 1;        ///< Extended Address Bit 24
            U8 RESERVED1 : 1;  ///< Reserved
            U8 WPS : 1;        ///< Write Protect Selection
            U8 RESERVED2 : 1;  ///< Reserved
            U8 RESERVED3 : 1;  ///< Reserved
            U8 DRV0 : 2;       ///< Output Driver Strength
            U8 HOLD_RESET : 1; ///< Hold/Reset pin configuration
        } asStruct;

    } SR3;

} STD_FLASH_STATUS_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Dummy cycles                                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_DUMMY_CYCLES__DEVICE_ID__1_1_1                  (3 * 8)
#define SPI_FLASH_DUMMY_CYCLES__DEVICE_ID__4_4_4                  (3 * 2)
#define SPI_FLASH_DUMMY_CYCLES__MANUFACTURER_AND_DEVICE_ID__1_1_1 (0)
#define SPI_FLASH_DUMMY_CYCLES__MANUFACTURER_AND_DEVICE_ID__1_2_2 (1 * 4) // 1 for M7-0
#define SPI_FLASH_DUMMY_CYCLES__MANUFACTURER_AND_DEVICE_ID__1_4_4 (3 * 2) // 1 for M7-0 and 2 for Dummy Bytes
#define SPI_FLASH_DUMMY_CYCLES__MANUFACTURER_AND_DEVICE_ID__4_4_4 (0)
#define SPI_FLASH_DUMMY_CYCLES__UNIQUE_ID                         (4 * 8)
#define SPI_FLASH_DUMMY_CYCLES__UNIQUE_ID_ADDRESS_MODE_4B         (5 * 8)
#define SPI_FLASH_DUMMY_CYCLES__SFDP_TABLE                        (1 * 8)
#define SPI_FLASH_DUMMY_CYCLES__SECURITY_REGS                     (1 * 8)
#define SPI_FLASH_DUMMY_CYCLES__SET_BURST_WITH_WRAP               (3 * 2)
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_1_1                  (1 * 8) // deprecated, use "1_1_X" instead
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_1_X                  (8)
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_2_2                  (4)
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_4_4                  (6)
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ__4_4_4                  (2)
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ_DTR__1_1_1              (6)
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ_DTR__1_2_2              (6)
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ_DTR__1_4_4              (8)
#define SPI_FLASH_DUMMY_CYCLES__FAST_READ_DTR__4_4_4              (8)

/*---------------------------------------------------------------------------------------------------------*/
/* Security registers parameters                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define SPI_FLASH_SECURITY_REGISTER_1_ADDR 0x1000
#define SPI_FLASH_SECURITY_REGISTER_2_ADDR 0x2000
#define SPI_FLASH_SECURITY_REGISTER_3_ADDR 0x3000

/*---------------------------------------------------------------------------------------------------------*/
/* Time outs                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define STD_FLASH__TIMEOUT_SCALE_ms_G2 67 // At sys_clk=100M: 1 cycle = 15us => 66.6 = 1ms
#define STD_FLASH__TIMEOUT_INIT        100
#define STD_FLASH__TIMEOUT             2010                          // 2ms      (30ms max) (Write Status Register Time)
#define STD_FLASH__TIMEOUT_WRITE       335                           // 0.8ms    (5ms max)
#define STD_FLASH__TIMEOUT_ERASE_4k    26800                         // 45ms     (400ms max)
#define STD_FLASH__TIMEOUT_ERASE_32K   107200                        // 120ms    (1,600ms max)
#define STD_FLASH__TIMEOUT_ERASE_64K   134000                        // 200ms    (2,000ms max)
#define STD_FLASH__TIMEOUT_ERASE_CHIP  3350000                       // 10,000ms (50,000ms max)
#define STD_FLASH__TIMEOUT_ERASE       STD_FLASH__TIMEOUT_ERASE_CHIP // TODO - temporary
#define STD_FLASH__TIMEOUT_MAX         0xFFFFFFFF

/*---------------------------------------------------------------------------------------------------------*/
/* Other properties                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define STD_FLASH__MAX_READ_DATA_CMD_CLK (50 * _1MHz_)
#define STD_FLASH__MAX_SPI_CLK           (133 * _1MHz_)


#endif // __QLIB_STD_CMDS_H__
