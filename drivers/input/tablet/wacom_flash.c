#include "wacom.h"

#define TBD 0xffff
#define ERRPRINT(str_1, str_2)({printk("%s: %s \n", str_1, str_2);})
#define CMD_RES 0x00
#define W9008_STARTADDR   0x2000
#define W9008_ENDADDR     0x17fff
#define W9008_BLOCKSIZE   128
#define W9008_PROGSIZE    128
#define W9008_OFFSET      2

#define RTYPE_FEATURE     0x03

#define REPORT_ID_1       0x01
#define REPORT_ID_2       0x02
#define RID_ENTER         REPORT_ID_2
#define RID_EXIT          REPORT_ID_1
#define RID_ERASE         REPORT_ID_1
#define RID_WRITE         REPORT_ID_1
#define RID_VERIFY        REPORT_ID_1

#define DATA_SIZE         136
#define EXIT_SIZE         DATA_SIZE
#define ERASE_SIZE        DATA_SIZE
#define WRITE_SIZE        (135 + 1)
#define VERIFY_SIZE       WRITE_SIZE
#define RESULT_SIZE       4

enum ERROR {
	ERR_ENTER_BOOT = 56,
	ERR_EXIT_BOOT,
	ERR_ERASE,
	ERR_WRITE,
	ERR_VERIFY,
};

bool wacom_set_feature(struct usb_interface *intf, u8 report_id, unsigned int buf_size, u8 *data)
{
	int ret = -1;
	bool bRet = false;

	ret = usb_control_msg(interface_to_usbdev(intf),
			      usb_sndctrlpipe(interface_to_usbdev(intf), 0),
			      USB_REQ_SET_REPORT, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			      (RTYPE_FEATURE << 8) | report_id, intf->altsetting[0].desc.bInterfaceNumber,
			      data, buf_size, 1000);
	if (ret < 0) {
		ERRPRINT(__func__, "Set feature error");
		goto out;
	}

	bRet = true;
 out:
	return bRet;
}

bool wacom_get_feature(struct usb_interface *intf, u8 report_id, unsigned int buf_size, u8 *data)
{
	int ret = -1;
	u8 *recv = NULL;
	bool bRet = false;

	recv = kzalloc(buf_size, GFP_KERNEL);
	if (!recv) {

		goto out;
	}
	memset(recv, 0, buf_size);

	//printk("PIPE: %x\n", ((PIPE_CONTROL << 30) | __create_pipe(interface_to_usbdev(intf), 0) 
	//| USB_DIR_IN));

	ret = usb_control_msg(interface_to_usbdev(intf),
			      usb_rcvctrlpipe(interface_to_usbdev(intf), 0),
			      USB_REQ_GET_REPORT, USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			      (RTYPE_FEATURE << 8) | report_id, intf->altsetting[0].desc.bInterfaceNumber,
			      recv, buf_size, 100);
	if (ret < 0) {
		ERRPRINT(__func__, "Get feature error");
		goto err;
	}
	
	memcpy(data, recv, buf_size);

	bRet = true;
 err:
	kfree(recv);
	recv = NULL;

 out:
	return bRet;
}

static bool wacom_enter_boot(struct wacom_sysfs *wac_boot, int intf_index)
{
	u8 cmd_boot[2] = {0x02, 0x02};

	return wacom_set_feature(&wac_boot->intf[intf_index], RID_ENTER, 2, cmd_boot);
}

static bool wacom_exit_boot(struct wacom_sysfs *wac_boot, int intf_index)
{
	u8 cmd_exit[EXIT_SIZE] = {0};

	cmd_exit[0] = 0x01;
	cmd_exit[1] = 0x03;

	return wacom_set_feature(&wac_boot->intf[intf_index], RID_EXIT, EXIT_SIZE, cmd_exit);
}

static bool wacom_erase(struct wacom_sysfs *wac_boot, int intf_index)
{
	int sector;
	u8 cmd_erase[ERASE_SIZE] = {0};
	u8 result[4] = {0};
	bool bRet = false;

	cmd_erase[0] = 0x01;

	for (sector = 95; sector > 7; sector--) {

		/*Sending commands*/
		cmd_erase[2] = sector + 1; //echo
		cmd_erase[3] = sector;
		bRet = wacom_set_feature(&wac_boot->intf[intf_index], RID_ERASE, ERASE_SIZE, cmd_erase);
		if (!bRet) {
			ERRPRINT(__func__, "erase failed at 'S'et feature");
			goto out;
		}

		msleep(1000);

		/*Getting the result*/
		bRet = wacom_get_feature(&wac_boot->intf[intf_index], REPORT_ID_2, RESULT_SIZE, result);
		if (!bRet) {
			ERRPRINT(__func__, "erase failed at 'G'et feature");
			goto out;
		}

		if (result[1] != 0x00 || result[2] != cmd_erase[2] || result[3] != CMD_RES) {
			bRet = false;
			ERRPRINT(__func__, "resulting in erase fails");
			printk("result[0]: %x result[1]: %x result[2]: %x result[3]: %x \n",
			       result[0], result[1], result[2], result[3]);
			goto out;
		}
	}

	bRet = true;
 out:
	return bRet;
}

static bool wacom_write(struct wacom_sysfs *wac_boot, int intf_index, u8 *data)
{
	u32 addr = W9008_STARTADDR;
	u32 data_size_cnt = W9008_OFFSET;
	int i = 0, len = 0;
	u8 cmd_write[WRITE_SIZE] = {0};
	u8 result[4] = {0};
	bool bRet = false;
	cmd_write[0] = 0x01;
	cmd_write[1] = 0x01;

	for (addr = W9008_STARTADDR; addr <= W9008_ENDADDR; addr += W9008_BLOCKSIZE) {
		len = 2;
		/*Sending commands*/
		cmd_write[len++] = (u8)((data_size_cnt & 0xff) + 1); //echo
		cmd_write[len++] = (u8)(addr & 0x000000ff);
		cmd_write[len++] = (u8)((addr & 0x0000ff00) >> 8);
		cmd_write[len++] = (u8)((addr & 0x00ff0000) >> 16);
		cmd_write[len++] = (u8)((addr & 0xff000000) >> 24);	
		cmd_write[len++] = 0x10;	

		for (i = 0; i < W9008_PROGSIZE; i++) {
			cmd_write[len++] = data[data_size_cnt++];
		}
	
		bRet = wacom_set_feature(&wac_boot->intf[intf_index], RID_WRITE, WRITE_SIZE, cmd_write);
		if (!bRet) {
			ERRPRINT(__func__, "write failed at 'S'et feature");
			goto out;
		}

		/*Getting the result*/
		bRet = wacom_get_feature(&wac_boot->intf[intf_index], REPORT_ID_2, RESULT_SIZE, result);
		if (!bRet) {
			ERRPRINT(__func__, "write failed at 'G'et feature");
			goto out;
		}

		if (result[1] != 0x01 || result[2] != cmd_write[2] || result[3] != CMD_RES) {
			bRet = false;
			ERRPRINT(__func__, "resulting in write fails");
			printk("result[0]: %x result[1]: %x result[2]: %x result[3]: %x \n",
			       result[0], result[1], result[2], result[3]);
			goto out;
		}

	}

	bRet = true;
 out:
	return bRet;
}

static bool wacom_verify(struct wacom_sysfs *wac_boot, int index, u8 *data)
{
	u32 addr = W9008_STARTADDR;
	u32 data_size_cnt = W9008_OFFSET;
	int i = 0, len = 0;
	u8 cmd_verify[VERIFY_SIZE] = {0};
	u8 result[4] = {0};
	bool bRet = false;

	cmd_verify[0] = 0x01;
	cmd_verify[1] = 0x01;

	for (; addr <= W9008_ENDADDR; addr += W9008_BLOCKSIZE) {

		len = 2;
		/*Sending commands*/
		cmd_verify[len++] = (u8)((data_size_cnt & 0xff) + 1); //echo
		cmd_verify[len++] = (u8)(addr & 0x000000ff);
		cmd_verify[len++] = (u8)((addr & 0x0000ff00) >> 8);
		cmd_verify[len++] = (u8)((addr & 0x00ff0000) >> 16);
		cmd_verify[len++] = (u8)((addr & 0xff000000) >> 24);	
		cmd_verify[len++] = 0x10;	

		for (i = 0; i < W9008_PROGSIZE; i++) {
			cmd_verify[len++] = data[data_size_cnt++];
		}
	
		bRet = wacom_set_feature(&wac_boot->intf[index], RID_VERIFY, VERIFY_SIZE, cmd_verify);
		if (!bRet) {
			ERRPRINT(__func__, "verify failed at 'S'et feature");
			goto out;
		}

		/*Getting the result*/
		bRet = wacom_get_feature(&wac_boot->intf[index], REPORT_ID_2, RESULT_SIZE, result);
		if (!bRet) {
			ERRPRINT(__func__, "verify failed at 'G'et feature");
			goto out;
		}

		if (result[1] != 0x01 || result[2] != cmd_verify[2] || result[3] != CMD_RES) {
			bRet = false;
			ERRPRINT(__func__, "resulting in verify fails");
			printk("result[0]: %x result[1]: %x result[2]: %x result[3]: %x \n",
			       result[0], result[1], result[2], result[3]);
			goto out;
		}

	}

	bRet = true;
 out:
	return bRet;
}

/*Interface for Input device*/
int wacom_enter(struct wacom_sysfs *wacom, int intf_index)
{
	int ret = -1;
	bool bRet = false;
	bRet = wacom_enter_boot(wacom, intf_index);
	if (!bRet) {
		printk("Failed to enter boot mode \n");
		goto out;
	}

	ret = 0;
 out:
	return ret;
}

int wacom_out(struct wacom_sysfs *wacom, int intf_index)
{
	int ret = -1;
	bool bRet = false;
	bRet = wacom_exit_boot(wacom, intf_index);
	if (!bRet) {
		printk("Failed to exit boot mode \n");
		goto out;
	}

	ret = 0;
 out:
	return ret;
}

/*Interface for Boot device*/
int wacom_flash(struct wacom_sysfs *wac_boot, int index, u8 *data)
{
	int ret = -1;
	bool bRet = false;

	bRet = wacom_erase(wac_boot, index);
	if (!bRet) {
		ret = -ERR_ERASE;
		goto exit;
	}

	bRet = wacom_write(wac_boot, index, data);
	if (!bRet) {
		ret = -ERR_WRITE;
		goto out;
	}

	bRet = wacom_verify(wac_boot, index, data);
	if (!bRet) {
		ret = -ERR_VERIFY;
		goto out;
	}


 exit:
	bRet = wacom_exit_boot(wac_boot, index);
	if (!bRet) {
		ret = -ERR_EXIT_BOOT;
		goto out;
	}

	ret = 0;
 out:
	printk("%s flash finished \n", __func__);
	return ret;
}
