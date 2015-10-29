/******************************************************************************
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2012 - 2015 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110,
 * USA
 *
 * The full GNU General Public License is included in this distribution
 * in the file called COPYING.
 *
 * Contact Information:
 *  Intel Linux Wireless <ilw@linux.intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 * BSD LICENSE
 *
 * Copyright(c) 2012 - 2015 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

#ifndef __FM_TEST_IUI_H
#define __FM_TEST_IUI_H

#include <linux/errno.h>

#ifdef IUI_FM /* Platform has FM - use platform h file */
# include_next <linux/fm/iui_fm.h>
#else /* Platform doesn't have FM - use test h file */
# include "iui_fm_test.h"
#endif /* IUI_FM */

#ifdef CPTCFG_IWLWIFI_FRQ_MGR_TEST
int32_t
iwl_mvm_fm_test_register_callback(const enum iui_fm_macro_id macro_id,
				  const iui_fm_mitigation_cb mitigation_cb);
int32_t
iwl_mvm_fm_test_notify_frequency(const enum iui_fm_macro_id macro_id,
				 const struct iui_fm_freq_notification *
				 const notification);
#endif /* CPTCFG_IWLWIFI_FRQ_MGR_TEST */

/*
 * If platform has FM then we have 2 modes:
 *	Regular mode: Call the FM register function
 *	Debug mode: Don't call the FM register function & use test mode
 *	implementation.
 * If platform does not have FM then we use test mode implementation.
 */
static inline
int32_t iwl_mvm_fm_register_callback(bool dbg_mode,
				     const enum iui_fm_macro_id macro_id,
				     const iui_fm_mitigation_cb mitigation_cb)
{
#ifdef CPTCFG_IWLWIFI_FRQ_MGR_TEST
	/* Platform does not have a FM or test mode was requested */
	if (dbg_mode)
		return iwl_mvm_fm_test_register_callback(macro_id,
							 mitigation_cb);
#endif /* CPTCFG_IWLWIFI_FRQ_MGR_TEST */

#ifdef IUI_FM /* Platform has a FM */
	/* Calling the FM  notify function - test mode was not requested */
	return iui_fm_register_mitigation_callback(macro_id, mitigation_cb);
#else
	return -EPERM; /* fm_debug_mode was not set */
#endif /* IUI_FM */
}

/*
 * If platform has FM then we have 2 modes:
 *	Regular mode: Call the FM notify function
 *	Debug mode: Don't call the FM notify function & use test mode
 *	implementation.
 * If platfom does not have FM then we use test mode implementation.
 */
static inline
int32_t iwl_mvm_fm_notify_frequency(bool dbg_mode,
				    const enum iui_fm_macro_id macro_id,
				    const struct iui_fm_freq_notification *
				    const notification)
{
#ifdef CPTCFG_IWLWIFI_FRQ_MGR_TEST
	/* Platform does not have a FM or test mode was requested */
	if (dbg_mode)
		return iwl_mvm_fm_test_notify_frequency(macro_id,
							notification);
#endif /* CPTCFG_IWLWIFI_FRQ_MGR_TEST */

#ifdef IUI_FM /* Platform has a FM */
	/* Calling the FM  notify function - test mode was not requested */
	return iui_fm_notify_frequency(macro_id, notification);
#else
	return -EPERM; /* fm_debug_mode was not set */
#endif /* IUI_FM */
}

#endif /* __FM_IUI_H */

