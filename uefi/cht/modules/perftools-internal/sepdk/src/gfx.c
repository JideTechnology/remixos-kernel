/*COPYRIGHT**
    Copyright (C) 2009-2014 Intel Corporation.

    This file is part of SEP Development Kit

    SEP Development Kit is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    SEP Development Kit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SEP Development Kit; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
**COPYRIGHT*/

#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "lwpmudrv_defines.h"
#include "lwpmudrv_types.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_gfx.h"
#include "rise_errors.h"
#include "lwpmudrv.h"
#include "gfx.h"

static char *gfx_virtual_addr    = NULL;
static U32   gfx_code            = GFX_CTRL_DISABLE;
static U32   gfx_counter[GFX_NUM_COUNTERS] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static U32   gfx_overflow[GFX_NUM_COUNTERS] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

/*!
 * @fn     OS_STATUS GFX_Read
 *
 * @brief  Reads the counters into the buffer provided for the purpose
 *
 * @param  buffer  - buffer to read the counts into
 *
 * @return STATUS_SUCCESS if read succeeded, otherwise error
 *
 * @note 
 */
extern OS_STATUS
GFX_Read (
    S8 *buffer
)
{
    U64  *samp  = (U64 *)buffer;
    U32   i;
    U32   val;
    char *reg_addr;

    // GFX counting was not specified
    if (gfx_virtual_addr == NULL || gfx_code == GFX_CTRL_DISABLE) {
        return OS_INVALID;
    }

    // check for sampling buffer
    if (! samp) {
        return OS_INVALID;
    }

    // set the GFX register address
    reg_addr = gfx_virtual_addr + GFX_PERF_REG;

    // for all counters - save the information to the sampling stream
    for (i = 0; i < GFX_NUM_COUNTERS; i++) {
        // read the ith GFX event count
        reg_addr += 4;
        val = *(U32 *)(reg_addr);
#if defined(GFX_COMPUTE_DELTAS)
        // if the current count is bigger than the previous one, then the counter overflowed
        // so make sure the delta gets adjusted to account for it
        if (val < gfx_counter[i]) {
            samp[i] = val + (GFX_CTR_OVF_VAL - gfx_counter[i]);
        }
        else {
            samp[i] = val - gfx_counter[i];
        }
#else   // just keep track of raw count for this counter
        // if the current count is bigger than the previous one, then the counter overflowed
        if (val < gfx_counter[i]) {
            gfx_overflow[i]++;
        }
        samp[i] = val + gfx_overflow[i]*GFX_CTR_OVF_VAL;
#endif
        // save the current count
        gfx_counter[i] = val;
    }

    return OS_SUCCESS;
}

/*!
 * @fn     OS_STATUS GFX_Set_Event_Code
 *
 * @brief  Programs the Graphics PMU with the right event code
 *
 * @param  arg - buffer containing graphics event code
 *
 * @return STATUS_SUCCESS if success, otherwise error
 *
 * @note 
 */
extern OS_STATUS
GFX_Set_Event_Code (
    IOCTL_ARGS arg
)
{
    U32        i;
    char      *reg_addr;
    U32        reg_value;

    // extract the graphics event code from usermode
    if (get_user(gfx_code, (int*)arg->w_buf)) {
        SEP_PRINT_ERROR("GFX_Set_Event_Code: unable to obtain gfx_code from usermode!\n");
        return OS_FAULT;
    }
    SEP_PRINT_DEBUG("GFX_Set_Event_Code: got gfx_code=0x%x\n", gfx_code);

    // memory map the address to GFX counters, if not already done
    if (gfx_virtual_addr == NULL) {
        gfx_virtual_addr = (char *)(UIOP)ioremap_nocache(GFX_BASE_ADDRESS + GFX_BASE_NEW_OFFSET, PAGE_SIZE);
    }

    // initialize the GFX counts
    for (i =  0; i < GFX_NUM_COUNTERS; i++) {
        gfx_counter[i] = 0;
        gfx_overflow[i] = 0;  // only used if storing raw counts (i.e., GFX_COMPUTE_DELTAS is undefined)
    }

    // get current GFX event code
    if (gfx_virtual_addr == NULL) {
        SEP_PRINT_ERROR("GFX_Start: invalid gfx_virtual_addr=0x%p\n", gfx_virtual_addr);
        return OS_INVALID;
    }

    reg_addr = gfx_virtual_addr + GFX_PERF_REG;
    reg_value = *(U32 *)(reg_addr);
    SEP_PRINT_DEBUG("GFX_Set_Event_Code: read reg_value=0x%x from reg_addr=0x%p\n", reg_value, reg_addr);

    /* Update the GFX counter group */
    // write the GFX counter group with reset = 1 for all counters
    reg_value = (gfx_code | GFX_REG_CTR_CTRL);
    *(U32 *)(reg_addr) = reg_value;
    SEP_PRINT_DEBUG("GFX_Set_Event_Code: wrote reg_value=0x%x to reg_addr=0x%p\n", reg_value, reg_addr);

    return OS_SUCCESS;
}

/*!
 * @fn     OS_STATUS GFX_Start
 *
 * @brief  Starts the count of the Graphics PMU
 *
 * @param  NONE
 *
 * @return OS_SUCCESS if success, otherwise error
 *
 * @note 
 */
extern OS_STATUS
GFX_Start (
    void
)
{
    U32   reg_value;
    char *reg_addr;

    // GFX counting was not specified
    if (gfx_virtual_addr == NULL || gfx_code == GFX_CTRL_DISABLE) {
        SEP_PRINT_ERROR("GFX_Start: invalid gfx_virtual_addr=0x%p or gfx_code=0x%x\n", gfx_virtual_addr, gfx_code);
        return OS_INVALID;
    }

    // turn on GFX counters as per event code
    reg_addr = gfx_virtual_addr + GFX_PERF_REG;
    *(U32 *)(reg_addr) = gfx_code;

    // verify event code was written properly
    reg_value = *(U32 *)reg_addr;
    if (reg_value != gfx_code) {
        SEP_PRINT_ERROR("GFX_Start: got register value 0x%x, expected 0x%x\n", reg_value, gfx_code);
        return OS_INVALID;
    }

    return OS_SUCCESS;
}

/*!
 * @fn     OS_STATUS GFX_Stop
 *
 * @brief  Stops the count of the Graphics PMU
 *
 * @param  NONE
 *
 * @return OS_SUCCESS if success, otherwise error
 *
 * @note 
 */
extern OS_STATUS
GFX_Stop (
    void
)
{
    char *reg_addr;

    // GFX counting was not specified
    if (gfx_virtual_addr == NULL || gfx_code == GFX_CTRL_DISABLE) {
        SEP_PRINT("GFX_Stop: invalid gfx_virtual_addr=0x%p or gfx_code=0x%x\n", gfx_virtual_addr, gfx_code);
        return OS_INVALID;
    }

    // turn off GFX counters
    reg_addr = gfx_virtual_addr + GFX_PERF_REG;
    *(U32 *)(reg_addr) = GFX_CTRL_DISABLE;

    // unmap the memory mapped virtual address
    iounmap(gfx_virtual_addr);

    // reset the GFX global variables
    gfx_code           = GFX_CTRL_DISABLE;
    gfx_virtual_addr   = NULL;

    return OS_SUCCESS;
}
