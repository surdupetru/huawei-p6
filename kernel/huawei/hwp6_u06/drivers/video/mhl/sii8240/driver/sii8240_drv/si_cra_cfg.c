/**********************************************************************************/
/*  Copyright (c) 2011, Silicon Image, Inc.  All rights reserved.                 */
/*  No part of this work may be reproduced, modified, distributed, transmitted,   */
/*  transcribed, or translated into any language or computer format, in any form  */
/*  or by any means without written permission of: Silicon Image, Inc.,           */
/*  1140 East Arques Avenue, Sunnyvale, California 94085                          */
/**********************************************************************************/

//***************************************************************************
//!file     si_cra_cfg.c
//!brief    Chip Register Access aka CRA configuration data.
//
//***************************************************************************/


#include "si_common.h"
#include "si_cra.h"
#include "si_cra_cfg.h"


//------------------------------------------------------------------------------
// si_cra_cfg.h
//------------------------------------------------------------------------------

// Index to this array is the virtual page number in the MSB of the REG_xxx address values
// Indexed with siiRegPageIndex_t value shifted right 8 bits
// DEV_PAGE values must correspond to the order specified in the siiRegPageIndex_t array
#define CI2CA HIGH
pageConfig_t    g_addrDescriptor[SII_CRA_MAX_DEVICE_INSTANCES][SII_CRA_DEVICE_PAGE_COUNT] =
{
#if (CI2CA == LOW)
        {
                { DEV_I2C_0,  DEV_PAGE_TPI_0           },  // TPI 0
                { DEV_I2C_0,  DEV_PAGE_TX_L0_0      },  // TX 0 Legacy 0
                { DEV_I2C_0,  DEV_PAGE_TX_L1_0      },  // TX 0 Legacy 1
                { DEV_I2C_0,  DEV_PAGE_TX_2_0       },  // TX 0 Page 2 (not legacy)
                { DEV_I2C_0,  DEV_PAGE_TX_3_0       },  // TX 0 Page 3 (not legacy)

                { DEV_I2C_0,  DEV_PAGE_CBUS_0       },    // CBUS 0

                { DEV_I2C_0,  DEV_PAGE_DDC_EDID     },  // TX EDID DDC
                { DEV_I2C_0,  DEV_PAGE_DDC_SEGM     }   // TX EDID DDC  Segment address
        }
#else
        {
                { DEV_I2C_0,  DEV_PAGE_TPI_1              },             // TPI 0
                { DEV_I2C_0,  DEV_PAGE_TX_L0_1          },             // TX 0 Legacy 0
                { DEV_I2C_0,  DEV_PAGE_TX_L1_1          },             // TX 0 Legacy 1
                { DEV_I2C_0,  DEV_PAGE_TX_2_1            },             // TX 0 Page 2 (not legacy)
                { DEV_I2C_0,  DEV_PAGE_TX_3_1            },             // TX 0 Page 3 (not legacy)

                { DEV_I2C_0,  DEV_PAGE_CBUS_1           },               // CBUS 0

                { DEV_I2C_0,  DEV_PAGE_DDC_EDID },  // TX EDID DDC
                { DEV_I2C_0,  DEV_PAGE_DDC_SEGM }                // TX EDID DDC  Segment address
        }
#endif
};


#if 0
pageConfig_t    g_addrDescriptor[SII_CRA_MAX_DEVICE_INSTANCES][SII_CRA_DEVICE_PAGE_COUNT] =
{
        {
                { DEV_I2C_0,  DEV_PAGE_TPI_0    },  // TPI 0
                { DEV_I2C_0,  DEV_PAGE_TX_L0_0  },  // TX 0 Legacy 0
                { DEV_I2C_0,  DEV_PAGE_TX_L1_0  },  // TX 0 Legacy 1
                { DEV_I2C_0,  DEV_PAGE_TX_2_0   },  // TX 0 Page 2 (not legacy)
                { DEV_I2C_0,  DEV_PAGE_TX_3_0   },  // TX 0 Page 3 (not legacy)

                { DEV_I2C_0,  DEV_PAGE_CBUS     },    // CBUS 0

                { DEV_I2C_0,  DEV_PAGE_DDC_EDID },  // TX EDID DDC
                { DEV_I2C_0,  DEV_PAGE_DDC_SEGM }   // TX EDID DDC  Segment address
        }
};
#endif

// Register addresses for re-assigning page base addresses
// These registers specify the I2C address that the SI device will
// respond to for the specific control register page
SiiReg_t g_siiRegPageBaseRegs [SII_CRA_DEVICE_PAGE_COUNT] =
{
        TX_PAGE_L0 | 0xFF,     // Device Base - Cannot be reassigned
        TX_PAGE_L0 | 0xFF,     // Device Base - Cannot be reassigned
        TX_PAGE_L0 | 0xFC,     // TX Legacy Page 1
        TX_PAGE_L0 | 0xFD,     // TX Page 2
        TX_PAGE_L0 | 0xFE,     // TX Page 3
        TX_PAGE_L0 | 0xFF,     // CBUS - Cannot be reassigned
        TX_PAGE_L0 | 0xFF,     // TX DDC EDID  - Cannot be reassigned
        TX_PAGE_L0 | 0xFF,     // TX DDC EDID Segment address  - Cannot be reassigned
};

// TODO:OEM - Add entries to reassign register page base addresses if needed
SiiReg_t g_siiRegPageBaseReassign [] =
{
//        PP_PAGE_3 | 0xFC,       // Example of changing default page 3 device ID (0xFA) to 0xFC
//                                // Note that the DEV_PAGE_IPV enum value must also be changed

//#if (FPGA_BUILD == 1)
//        PP_PAGE_2 | 0xAA,       // Change default 0x66 to 0xAA
//        PP_PAGE_A | 0xBB,       // Change default 0x64 to 0xBB
//#endif

        0xFFFF      // End of reassignment list
};
