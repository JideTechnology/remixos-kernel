/*COPYRIGHT**
 * -------------------------------------------------------------------------
 *               INTEL CORPORATION PROPRIETARY INFORMATION
 *  This software is supplied under the terms of the accompanying license
 *  agreement or nondisclosure agreement with Intel Corporation and may not
 *  be copied or disclosed except in accordance with the terms of that
 *  agreement.
 *        Copyright (C) 2012-2014 Intel Corporation. All Rights Reserved.
 * -------------------------------------------------------------------------
**COPYRIGHT*/


#include "lwpmudrv_defines.h"
#include <linux/version.h>
#include <linux/fs.h>

#include "lwpmudrv_types.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"

#include "lwpmudrv.h"
#include "utility.h"
#include "control.h"
#include "output.h"
#include "valleyview_sochap.h"
#include "inc/ecb_iterators.h"
#include "inc/pci.h"

#if defined(PCI_HELPERS_API)
#include <asm/intel_mid_pcihelpers.h>
#endif

extern U64           *read_counter_info;
static U64           *uncore_current_data = NULL;
static U64           *uncore_to_read_data = NULL;

extern VOID SOCPERF_Read_Data(PVOID data_buffer);



/*!
 * @fn         static VOID valleyview_VISA_Initialize(PVOID)
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
valleyview_VISA_Initialize (
    VOID  *param
)
{
    // Allocate memory for reading GMCH counter values + the group id
    if (!uncore_current_data) {
        uncore_current_data = CONTROL_Allocate_Memory((VLV_CHAP_MAX_COUNTERS+1)*sizeof(U64));
        if (!uncore_current_data) {
            return;
        }
    }
    if (!uncore_to_read_data) {
        uncore_to_read_data = CONTROL_Allocate_Memory((VLV_CHAP_MAX_COUNTERS+1)*sizeof(U64));
        if (!uncore_to_read_data) {
            return;
        }
    }

    return;
}


/*!
 * @fn         static VOID valleyview_VISA_Enable_PMU(PVOID)
 *
 * @brief      Start counting
 *
 * @param      param - device index
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
valleyview_VISA_Enable_PMU (
    PVOID  param
)
{
    U32 this_cpu  = CONTROL_THIS_CPU();
    CPU_STATE pcpu  = &pcb[this_cpu];

    if (!CPU_STATE_system_master(pcpu)) {
        return;
    }

    SEP_PRINT_DEBUG("Starting the counters...\n");
    if (uncore_current_data) {
        memset(uncore_current_data, 0, (VLV_CHAP_MAX_COUNTERS+1)*sizeof(U64));
    }
    if (uncore_to_read_data) {
        memset(uncore_to_read_data, 0, (VLV_CHAP_MAX_COUNTERS+1)*sizeof(U64));
    }

    return;
}


/*!
 * @fn         static VOID valleyview_VISA_Disable_PMU(PVOID)
 *
 * @brief      Unmap the virtual address when sampling/driver stops
 *
 * @param      param - device index
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
valleyview_VISA_Disable_PMU (
    PVOID  param
)
{
    U32                   this_cpu  = CONTROL_THIS_CPU();
    CPU_STATE             pcpu      = &pcb[this_cpu];

    if (!CPU_STATE_system_master(pcpu)) {
        return;
    }
    SEP_PRINT_DEBUG("Stopping the counters...\n");
    if (GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_PREPARE_STOP) {
        uncore_current_data = CONTROL_Free_Memory(uncore_current_data);
        uncore_to_read_data = CONTROL_Free_Memory(uncore_to_read_data);
    }

    return;
}



/*!
 * @fn         static VOID valleyview_VISA_Clean_Up(PVOID)
 *
 * @brief      Reset any registers or addresses
 *
 * @param      param
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
valleyview_VISA_Clean_Up (
    VOID   *param
)
{
    return;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn valleyview_VISA_Read_Counts(param, id)
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
valleyview_VISA_Read_Counts (
    PVOID  param,
    U32    id
)
{
    U64            *data            = (U64*) param;
    U32             cur_grp         = 0;
    ECB             pecb            = NULL;
    U32             data_index      = 0;
    U32             data_reg        = 0;
    U32             pecb_index      = 0;
    U32             group_index     = 0;

    if (GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_UNINITIALIZED ||
        GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_IDLE          ||
        GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_RESERVED      ||
        GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_PREPARE_STOP  ||
        GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_STOPPED) {
        return;
    }

    if (param == NULL) {
        return;
    }
    if (uncore_to_read_data == NULL) {
        return;
    }

    cur_grp = (U32)uncore_to_read_data[data_index];
    if (cur_grp == 0) {
        pecb_index = 0;
        group_index = cur_grp + 1;
    }
    else {
        pecb_index  = cur_grp - 1;
        group_index = cur_grp;
    }
    pecb    = LWPMU_DEVICE_PMU_register_data(&devices[id])[pecb_index];
    data    = (U64*)((S8*)data + ECB_group_offset(pecb));
    *data   = group_index;
    data_index++;

    FOR_EACH_PCI_REG_RAW_GROUP(pecb, i, id, pecb_index) {
        if (ECB_entries_reg_type(pecb,i) == CCCR) {
            data_reg           = i + ECB_cccr_pop(pecb);
            if (ECB_entries_reg_type(pecb,data_reg) == DATA) {
                data  = (U64 *)((S8*)param + ECB_entries_counter_event_offset(pecb,data_reg));
                *data = uncore_to_read_data[data_index++];
            }
        }

    } END_FOR_EACH_PCI_REG_RAW_GROUP;

    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn valleyview_VISA_Read_PMU_Data(param)
 *
 * @param    param    The device index
 *
 * @return   None     No return needed
 *
 * @brief    Read the Uncore count data and store into the buffer param;
 *
 */
static VOID
valleyview_VISA_Read_PMU_Data (
    PVOID  param
)
{
    S32                   j;
    U64                  *buffer       = read_counter_info;
    U32                   dev_idx      = *((U32*)param);
    U32                   start_index;
    DRV_CONFIG            pcfg_unc;
    U32                   data_reg     = 0;
    U32                   this_cpu     = CONTROL_THIS_CPU();
    CPU_STATE             pcpu         = &pcb[this_cpu];
    U32                   event_index  = 0;
    U32                   cur_grp      = LWPMU_DEVICE_cur_group(&devices[(dev_idx)]);
    U64                   counter_buffer[VLV_CHAP_MAX_COUNTERS+1];

    if (!CPU_STATE_socket_master(pcpu)) {
        return;
    }

    pcfg_unc    = (DRV_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    start_index = DRV_CONFIG_emon_unc_offset(pcfg_unc, cur_grp);

    SOCPERF_Read_Data((void*)counter_buffer);

    FOR_EACH_PCI_REG_RAW(pecb, i, dev_idx) {
        if (ECB_entries_reg_type(pecb,i) == CCCR) {
            data_reg           = i + ECB_cccr_pop(pecb);
            if (ECB_entries_reg_type(pecb,data_reg) == DATA) {
                j = start_index + ECB_entries_group_index(pecb, data_reg) + ECB_entries_emon_event_id_index_local(pecb, data_reg);
                buffer[j] = counter_buffer[event_index+1];
                event_index++;
            }
        }

    } END_FOR_EACH_PCI_REG_RAW;

}


/* ------------------------------------------------------------------------- */
/*!
 * @fn valleyview_Trigger_Read()
 *
 * @param    None
 *
 * @return   None     No return needed
 *
 * @brief    Read the SoCHAP counters when timer is triggered
 *
 */
static VOID
valleyview_Trigger_Read (
    VOID
)
{
    U64             *temp;

    if (GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_UNINITIALIZED ||
        GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_IDLE          ||
        GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_RESERVED      ||
        GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_PREPARE_STOP  ||
        GLOBAL_STATE_current_phase(driver_state) == DRV_STATE_STOPPED) {
        return;
    }

    if (uncore_current_data == NULL) {
        return;
    }

    SOCPERF_Read_Data((void*)uncore_current_data);

    temp                = uncore_to_read_data;
    uncore_to_read_data = uncore_current_data;
    uncore_current_data = temp;

    return;
}


/*
 * Initialize the dispatch table
 */
DISPATCH_NODE  valleyview_visa_dispatch =
{
    valleyview_VISA_Initialize,        // initialize
    NULL,                              // destroy
    NULL,                              // write
    valleyview_VISA_Disable_PMU,       // freeze
    valleyview_VISA_Enable_PMU,        // restart
    valleyview_VISA_Read_PMU_Data,     // read
    NULL,                              // check for overflow
    NULL,
    NULL,
    valleyview_VISA_Clean_Up,
    NULL,
    NULL,
    NULL,
    valleyview_VISA_Read_Counts,       // read counts
    NULL,
    NULL,                              // read_ro
    NULL,
    valleyview_Trigger_Read,
    NULL                               // scan for uncore
};
