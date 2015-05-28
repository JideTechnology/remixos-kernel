/*
 * message.c: PD Protocol framework for creating PD messages
 *
 * Copyright (C) 2015 Intel Corporation
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. Seee the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Kannappan, R <r.kannappan@intel.com>
 * Author: Albin, B <albin.bala.krishnan@intel.com>
 */

#include <linux/mutex.h>
#include <linux/types.h>
#include "protocol.h"
#include "message.h"

int pd_ctrl_msg(struct pd_prot *pd, u8 msg_type, u8 msg_id)
{
	struct pd_packet *buf = &pd->tx_buf;
	struct pd_pkt_header *header = &buf->header;

	header->msg_type = msg_type & PD_MSG_HEAD_MSG_TYPE;
	header->data_role = pd->new_data_role & PD_MSG_HEADER_ROLE_BITS_MASK;
	header->rev_id = PD_REV_ID_2 & PD_MSG_HEADER_REVID_BITS_MASK;
	if (pd->new_pwr_role == PD_POWER_ROLE_PROVIDER)
		header->pwr_role = 1;
	else
		header->pwr_role = 0;
	header->msg_id = msg_id & PD_MSG_HEADER_MSGID_BITS_MASK;
	header->num_data_obj = 0;
	return 0;
}
EXPORT_SYMBOL_GPL(pd_ctrl_msg);

int pd_data_msg(struct pd_prot *pd, int len, u8 msg_type)
{
	struct pd_packet *buf = &pd->tx_buf;
	struct pd_pkt_header *header = &buf->header;

	header->msg_type = msg_type & PD_MSG_HEAD_MSG_TYPE;
	header->data_role = pd->new_data_role & PD_MSG_HEADER_ROLE_BITS_MASK;
	header->rev_id = PD_REV_ID_2 & PD_MSG_HEADER_REVID_BITS_MASK;
	if (pd->new_pwr_role == PD_POWER_ROLE_PROVIDER)
		header->pwr_role = 1;
	else
		header->pwr_role = 0;
	header->msg_id = pd->tx_msg_id & PD_MSG_HEADER_MSGID_BITS_MASK;
	header->num_data_obj = len & PD_MSG_HEADER_N_DOBJ_BITS_MASK;

	return 0;
}
EXPORT_SYMBOL_GPL(pd_data_msg);
