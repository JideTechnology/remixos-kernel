/******************************************************************************
 *
 *	Copyright(c) O2Micro, 2012. All rights reserved.
 *
 *	Description:
 *		this is header file for OZ8806 register definition 
 *
 *****************************************************************************/

 #ifndef _OZ8806_REGDEF_H_
 #define _OZ8806_REGDEF_H_

/*****************************************************************************
 * #include section
 * add #include here
 *****************************************************************************/


/*****************************************************************************
 * const section
 * add const #define here
 *****************************************************************************/

/* Operation register map define */
#define	OZ8806_OP_IDREV				0X00	//CHIP ID and Reversion
	#define	CHIP_ID_MASK				0xF8		//default CHIP_ID[7:3]=0x07
	#define CHIP_VER_MASK				0x07		//default CHIP_VER[2:0]=0x00 for 8806

#define OZ8806_OP_VD23				0x03	//

#define OZ8806_OP_I2CCONFIG			0x04	//I2C config register
	#define	I2C_ADDRESS_MASK			0x0E		//default 0X07

#define OZ8806_OP_PEC_CTRL			0x08	//PEC control register
	#define	PEC_EN						0x04		//PEC control register[2]: 1: PEC enable 0: PEC disable

#define	OZ8806_OP_CTRL				0x09	//Status control register
	#define	SOFT_RESET					0x80		//Status control register[7]: 1: reset
	#define SLEEP_MODE					0x40		//Status control register[6]: 1: sleep 0:active
	#define SLEEP_OCV_EN				0x20
	#define	CHARGE_ON					0x10
	#define	VME_SEL						0x0c
	#define	CHG_SEL						0x02
	#define	BI_STATUS					0x01
	

#define	OZ8806_OP_VOLT_OFFSET		0X0A	
	#define VOLT_OFFSET_MASK			0xFC		

#define OZ8806_OP_BATTERY_ID		0x0B	            
	                                                       
#define OZ8806_OP_CELL_TEMP		0x0C	//cell temperature low byte,[7:4]->[3:0]of cell temp
	#define CELL_TEMP_MASK				0x0FFF

#define OZ8806_OP_CELL_TEMP_HIGH	0x0D	//cell temperature high byte,[7:0]->[11:4]of cell temp
	                                                       
#define OZ8806_OP_CELL_VOLT		0x0E	//cell voltage low byte,[7:4]->[3:0]of cell volt
	#define CELL_VOLT_MASK				0x0FFF

#define OZ8806_OP_CELL_VOLT_HIGH	0x0f	//cell voltage high byte,[7:0]->[11:4]of cell volt
	                                                       
#define OZ8806_OP_OCV_LOW			0x10	//OCV low byte,[7:4]->[3:0]of OCV
	#define OCV_LOW_MASK				0xF0
	#define	SLEEP_OCV_FLAG				0X02
	#define	POWER_OCV_FLAG				0X01

#define OZ8806_OP_OCV_HIGH			0x11	//OCV high byte,[7:0]->[11:4]of OCV

#define OZ8806_OP_CURRENT		0x12	//CURRENT low byte,[7:1]->[5:0]of CURRENT


#define OZ8806_OP_CAR			0x14	//CAR low byte

#define OZ8806_OP_BOARD_OFFSET  0x18


	
/*****************************************************************************
 * function prototype section
 * add function prototype here
 *****************************************************************************/




#endif

