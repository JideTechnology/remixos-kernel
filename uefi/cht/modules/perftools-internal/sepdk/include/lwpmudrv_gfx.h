/*COPYRIGHT**
 * -------------------------------------------------------------------------
 *               INTEL CORPORATION PROPRIETARY INFORMATION
 *  This software is supplied under the terms of the accompanying license
 *  agreement or nondisclosure agreement with Intel Corporation and may not
 *  be copied or disclosed except in accordance with the terms of that
 *  agreement.
 *        Copyright (C) 2011-2014 Intel Corporation.  All Rights Reserved.
 * -------------------------------------------------------------------------
**COPYRIGHT*/

#ifndef _LWPMUDRV_GFX_H_
#define _LWPMUDRV_GFX_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define GFX_BASE_ADDRESS       0xFF200000
#define GFX_BASE_NEW_OFFSET    0x00080000
#define GFX_PERF_REG           0x040         // location of GFX counter relative to base
#define GFX_NUM_COUNTERS       9             // max number of GFX counters per counter group
#define GFX_CTR_OVF_VAL        0xFFFFFFFF    // overflow value for GFX counters

#define GFX_REG_CTR_CTRL       0x01FF
#define GFX_CTRL_DISABLE       0x1E00

//#define GFX_COMPUTE_DELTAS     1             // use event count deltas instead of raw counts

#if defined(__cplusplus)
}
#endif

#endif

