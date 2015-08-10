/*
    Copyright (C) 2011-2014 Intel Corporation.  All Rights Reserved.

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
*/


#include "lwpmudrv_defines.h"
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/fs.h>

#include "lwpmudrv_types.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"

#include "lwpmudrv.h"
#include "utility.h"
#include "control.h"
#include "output.h"
#include "haswellunc_sa.h"
#include "ecb_iterators.h"
#include "inc/pci.h"

extern U64           *read_counter_info;

extern VOID SOCPERF_Read_Data(PVOID data_buffer);



/*!
 * @fn         static VOID hswunc_sa_Initialize(PVOID)
 *
 * @brief      Initialize any registers or addresses
 *
 * @param      param
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
hswunc_sa_Initialize (
    VOID  *param
)
{
    return;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn hswunc_sa_Read_Counts(param, id)
 *
 * @param    param    The read thread node to process
 * @param    id       The id refers to the device index
 *
 * @return   None     No return needed
 *
 * @brief    Read the Uncore count data and store into the buffer param;
 *
 */
static VOID
hswunc_sa_Read_Counts (
    PVOID  param,
    U32    id
)
{
    U64  *data         = (U64*) param;
    U32   cur_grp      = LWPMU_DEVICE_cur_group(&devices[id]);
    ECB   pecb         = LWPMU_DEVICE_PMU_register_data(&devices[id])[cur_grp];

    // group id
    data    = (U64*)((S8*)data + ECB_group_offset(pecb));
    SOCPERF_Read_Data((void*)data);

    return;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn hswunc_sa_Read_PMU_Data(param)
 *
 * @param    param    the device index
 *
 * @return   None     No return needed
 *
 * @brief    Read the Uncore count data and store into the buffer param;
 *
 */
static VOID
hswunc_sa_Read_PMU_Data (
    PVOID  param
)
{
    S32                   j;
    U64                  *buffer       = read_counter_info;
    U32                   dev_idx      = *((U32*)param);
    U32                   start_index;
    DRV_CONFIG            pcfg_unc;
    U32                   data_val     = 0;
    U32                   this_cpu     = CONTROL_THIS_CPU();
    CPU_STATE             pcpu         = &pcb[this_cpu];
    U32                   cur_grp      = LWPMU_DEVICE_cur_group(&devices[(dev_idx)]);
    U32                   event_index  = 0;
    U64                   counter_buffer[HSWUNC_SA_MAX_COUNTERS+1];

    if (!CPU_STATE_system_master(pcpu)) {
        return;
    }
    pcfg_unc    = (DRV_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    start_index = DRV_CONFIG_emon_unc_offset(pcfg_unc, cur_grp);

    SOCPERF_Read_Data((void*)counter_buffer);

    FOR_EACH_PCI_DATA_REG_RAW(pecb, i, dev_idx) {
        j        = start_index + ECB_entries_group_index(pecb,i) + ECB_entries_emon_event_id_index_local(pecb,i);
        data_val  = counter_buffer[event_index+1];
        buffer[j] = data_val;
        event_index++;
    } END_FOR_EACH_PCI_DATA_REG_RAW;

    return;
}


/*
 * Initialize the dispatch table
 */
DISPATCH_NODE  hswunc_sa_dispatch =
{
    hswunc_sa_Initialize,        // initialize
    NULL,                        // destroy
    NULL ,                       // write
    NULL,                        // freeze
    NULL,                        // restart
    hswunc_sa_Read_PMU_Data,     // read
    NULL,                        // check for overflow
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    hswunc_sa_Read_Counts,       //read_counts
    NULL,
    NULL,
    NULL,
    NULL,
    NULL                         // scan for uncore
};
