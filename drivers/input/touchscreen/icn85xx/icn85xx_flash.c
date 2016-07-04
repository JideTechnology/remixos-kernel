/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    icn85xx_flash.c
 Abstract:
               flash operation, read write etc.
 Author:       Zhimin Tian
 Date :        08 14,2013
 Version:      0.1[.revision]
 History :
     Change logs.  
 --*/
#include "icn85xx.h"

static struct file  *fp; 
static int g_status = R_OK;
static char fw_mode = 0;
static int fw_size = 0;
static unsigned char *fw_buf;
static char boot_mode = ICN85XX_WITH_FLASH_85;
void icn85xx_rawdatadump(short *mem, int size, char br)
{
    int i;
    for(i=0;i<size; i++)
    {
        if((i!=0)&&(i%br == 0))
            printk("\n");
        printk(" %5d", mem[i]);
    }
    printk("\n"); 
} 

void icn85xx_memdump(char *mem, int size)
{
    int i;
    for(i=0;i<size; i++)
    {
        if(i%16 == 0)
            printk("\n");
        printk(" 0x%2x", mem[i]);
    }
    printk("\n"); 
} 

int  icn85xx_checksum(int sum, char *buf, unsigned int size)
{
    int i;
    for(i=0; i<size; i++)
    {
        sum = sum + buf[i];
    }
    return sum;
}


int icn85xx_update_status(int status)
{
//  flash_info("icn85xx_update_status: %d\n", status);
    g_status = status;
    return 0;
}

int icn85xx_get_status(void)
{
    return  g_status;
}

void icn85xx_set_fw(int size, unsigned char *buf)
{
    fw_size = size;
    fw_buf = buf;
    
}

/***********************************************************************************************
Name    :   icn85xx_open_fw 
Input   :   *fw
            
Output  :   file size
function    :   open the fw file, and return total size
***********************************************************************************************/
int  icn85xx_open_fw( char *fw)
{
    int file_size;
    mm_segment_t fs; 
    struct inode *inode = NULL; 
    if(strcmp(fw, "icn85xx_firmware") == 0)
    {
        fw_mode = 1;  //use inner array
        return fw_size;
    }
    else
    {
        fw_mode = 0; //use file in file system
    }
    
    fp = filp_open(fw, O_RDONLY, 0); 
    if (IS_ERR(fp)) { 
        flash_error("read fw file error\n"); 
        return -1; 
    } 
    else
        flash_info("open fw file ok\n"); 
        
    inode = fp->f_dentry->d_inode;
    file_size = inode->i_size;  
//    flash_info("file size: %d\n", file_size); 

    fs = get_fs(); 
    set_fs(KERNEL_DS); 
    
    return  file_size;
    
}

/***********************************************************************************************
Name    :   icn85xx_read_fw 
Input   :   offset
            length, read length
            buf, return buffer
Output  :   
function    :   read data to buffer
***********************************************************************************************/
int  icn85xx_read_fw(int offset, int length, char *buf)
{
    loff_t  pos = offset;               
    if(fw_mode == 1)
    {
        memcpy(buf, fw_buf+offset, length);
    }
    else
    {                   
        vfs_read(fp, buf, length, &pos); 
    }
//  icn85xx_memdump(buf, length);
    return 0;       
}


/***********************************************************************************************
Name    :   icn85xx_close_fw 
Input   :   
Output  :   
function    :   close file
***********************************************************************************************/
int  icn85xx_close_fw(void)
{   
    if(fw_mode == 0)
    {
        filp_close(fp, NULL); 
    }
    
    return 0;
}
/***********************************************************************************************
Name    :   icn85xx_readVersion
Input   :   void
Output  :   
function    :   return version
***********************************************************************************************/
int icn85xx_readVersion(void)
{
    int err = 0;
    char tmp[2];    
    short CurVersion;
    err = icn85xx_i2c_rxdata(0x000c, tmp, 2);
    if (err < 0) {
        flash_error("%s failed: %d\n", __func__, err); 
        return 0;
    }       
    CurVersion = (tmp[0]<<8) | tmp[1];
    return CurVersion;  
}


/***********************************************************************************************
Name    :   icn85xx_goto_progmode 
Input   :   
Output  :   
function    :   change MCU to progmod
***********************************************************************************************/
int icn85xx_goto_progmode(void)
{
    int ret = -1;
    int retry = 3;
    unsigned char ucTemp;  
    
//    flash_info("icn85xx_goto_progmode\n"); 
    while(retry > 0)
    {       
        ucTemp = 0x5a;               
        ret = icn85xx_prog_i2c_txdata(0xcc3355, &ucTemp,1);             
        mdelay(2);             
        ucTemp = 01;           
        ret = icn85xx_prog_i2c_txdata(0x040400, &ucTemp,1);
        mdelay(2); 
        ret = icn85xx_check_progmod();
        if(ret == 0)
            return ret;
            
        retry--;
        mdelay(2); 
    }
    printk("icn85xx_goto_progmode over\n");
    if(retry == 0)
        return -1;
 
    return 0;
}

/***********************************************************************************************
Name    :   icn85xx_check_progmod 
Input   :   
Output  :   
function    :   check if MCU at progmode or not
***********************************************************************************************/
int icn85xx_check_progmod(void)
{
    int ret;
    unsigned char ucTemp = 0x0;
    ret = icn85xx_prog_i2c_rxdata(0x040002, &ucTemp, 1);
//    flash_info("icn85xx_check_progmod: 0x%x\n", ucTemp);
    if(ret < 0)
    {
        flash_error("icn85xx_check_progmod error, ret: %d\n", ret);
        return ret;
    }
    if(ucTemp == 0x85)
        return 0;
    else 
        return -1;

}

unsigned char FlashState(unsigned char State_Index)
{
    unsigned char ucTemp[4] = {0,0,0,0};

    ucTemp[2]=0x08;
    ucTemp[1]=0x10;
    ucTemp[0]=0x00;
    icn85xx_prog_i2c_txdata(0x4062c,ucTemp,3);

    if(State_Index==0)
    {
        ucTemp[0]=0x05;
    }
    else if(State_Index==1)
    {
        ucTemp[0]=0x35;
    }
    icn85xx_prog_i2c_txdata(0x40630,ucTemp,1);

    ucTemp[1]=0x00;
    ucTemp[0]=0x01;
    icn85xx_prog_i2c_txdata(0x40640,ucTemp,2);
       
    ucTemp[0]=1;
    icn85xx_prog_i2c_txdata(0x40644,ucTemp,1);
    while(ucTemp[0])
    {
        icn85xx_prog_i2c_rxdata(0x40644,ucTemp,1);
    }
  
    icn85xx_prog_i2c_rxdata(0x40648,ucTemp,1);
    return (unsigned char)(ucTemp[0]);
}

int  icn85xx_read_flashid(void)
{
    unsigned char ucTemp[4] = {0,0,0,0};
    int flashid=0;

    ucTemp[2]=0x08;
    ucTemp[1]=0x10;
    ucTemp[0]=0x00;
    icn85xx_prog_i2c_txdata(0x4062c,ucTemp,3);

    ucTemp[0]=0x9f;

    icn85xx_prog_i2c_txdata(0x40630,ucTemp,1);

    ucTemp[1] = 0x00;
    ucTemp[0] = 0x03;

    icn85xx_prog_i2c_txdata(0x40640,ucTemp,2);
       
    ucTemp[0]=1;
    icn85xx_prog_i2c_txdata(0x40644,ucTemp,1);
    while(ucTemp[0])
    {
        icn85xx_prog_i2c_rxdata(0x40644,ucTemp,1);
    }
  
    icn85xx_prog_i2c_rxdata(0x40648,(char *)&flashid,4);
    flashid=flashid&0x00ffffff;
    boot_mode = ICN85XX_WITHOUT_FLASH;
    return flashid;	
    if((MD25D40_ID1 == flashid) || (MD25D40_ID2 == flashid)
        ||(MD25D20_ID1 == flashid) || (MD25D20_ID1 == flashid)
        ||(GD25Q10_ID == flashid) || (MX25L512E_ID == flashid)
        || (MD25D05_ID == flashid)|| (MD25D10_ID == flashid))
    {
        boot_mode = ICN85XX_WITH_FLASH_85;
    }
    else
    {
        boot_mode = ICN85XX_WITHOUT_FLASH;
    }
   
    printk("flashid: 0x%x\n", flashid);
    return flashid;
}


void FlashWriteEnable(void)
{
    unsigned char ucTemp[4] = {0,0,0,0};

    ucTemp[2]=0x00;
    ucTemp[1]=0x10;
    ucTemp[0]=0x00;
    icn85xx_prog_i2c_txdata(0x4062c,ucTemp,3);
    
    ucTemp[0]=0x06; 
    icn85xx_prog_i2c_txdata(0x40630,ucTemp,1);

    ucTemp[0]=0x00;
    ucTemp[1]=0x00;    
    icn85xx_prog_i2c_txdata(0x40640,ucTemp,2);
    
    ucTemp[0]=1;
    icn85xx_prog_i2c_txdata(0x40644,ucTemp,1);
    while(ucTemp[0])
    {    
        icn85xx_prog_i2c_rxdata(0x40644,ucTemp,1);
    }
    
    ucTemp[0]=FlashState(0);
    while( (ucTemp[0]&0x02)!=0x02)
    {
        ucTemp[0]=FlashState(0);    
    }
}

#ifndef  QUAD_OUTPUT_ENABLE
void ClearFlashState(void)
{
    unsigned char ucTemp[4] = {0,0,0,0};
    icn85xx_prog_i2c_rxdata(0x40603,ucTemp,1);
    ucTemp[0]=(ucTemp[0]|0x20);
    icn85xx_prog_i2c_txdata(0x40603, ucTemp, 1 );
    
    FlashWriteEnable();   
    ////////////////////////////write comd to flash
    ucTemp[2]=0x00;
    ucTemp[1]=0x10;
    ucTemp[0]=0x10;
    icn85xx_prog_i2c_txdata(0x4062c,ucTemp,3);
    
    ucTemp[0]=0x01;
    icn85xx_prog_i2c_txdata(0x40630,ucTemp,1);
    
    ucTemp[0]=0x00;
    ucTemp[1]=0x00;
    icn85xx_prog_i2c_txdata(0x40640,ucTemp,2);

    ucTemp[0]=0x00;    
    icn85xx_prog_i2c_txdata(0x40638,ucTemp,1);
    
    ucTemp[0]=1;    
    icn85xx_prog_i2c_txdata(0x40644,ucTemp,1);
    while(ucTemp[0])
    {
        icn85xx_prog_i2c_rxdata(0x40644,ucTemp,1);
    }
    while(FlashState(0)&0x01);

}
#else
void ClearFlashState(void)
{
}
#endif


void EarseFlash(unsigned char erase_index,ulong flash_addr)
{
    unsigned char ucTemp[4] = {0,0,0,0};
    FlashWriteEnable();    
    if(erase_index==0)            //erase the chip
    {
        ucTemp[0]=0xc7;        
        icn85xx_prog_i2c_txdata(0x40630, ucTemp, 1 );
        ucTemp[2]=0x00;
        ucTemp[1]=0x10;
        ucTemp[0]=0x00;     
        icn85xx_prog_i2c_txdata(0x4062c, ucTemp, 3 );  
    }
    else if(erase_index==1)       //erase 32k space of the flash
    {
        ucTemp[0]=0x52;       
        icn85xx_prog_i2c_txdata(0x40630, ucTemp, 1);
        ucTemp[2]=0x00;
        ucTemp[1]=0x13;
        ucTemp[0]=0x00;        
        icn85xx_prog_i2c_txdata(0x4062c, ucTemp, 3);
    }
    else if(erase_index==2)     //erase 64k space of the flash
    {
        ucTemp[0]=0xd8;
        icn85xx_prog_i2c_txdata(0x40630, ucTemp,1);
        ucTemp[2]=0x00;
        ucTemp[1]=0x13;
        ucTemp[0]=0x00;
        icn85xx_prog_i2c_txdata(0x4062c, ucTemp, 3);
    } 
    else if(erase_index==3)
    {
        ucTemp[0]=0x20;         
        icn85xx_prog_i2c_txdata(0x40630, ucTemp, 1);
        ucTemp[2]=0x00;
        ucTemp[1]=0x13;
        ucTemp[0]=0x00;       
        icn85xx_prog_i2c_txdata(0x4062c, ucTemp, 3);
    }
    ucTemp[2]=(unsigned char)(flash_addr>>16);
    ucTemp[1]=(unsigned char)(flash_addr>>8);
    ucTemp[0]=(unsigned char)(flash_addr);
    icn85xx_prog_i2c_txdata(0x40634, ucTemp, 3);

    ucTemp[1]=0x00;
    ucTemp[0]=0x00;
    icn85xx_prog_i2c_txdata(0x40640, ucTemp, 2 );

    ucTemp[0]=1;
    icn85xx_prog_i2c_txdata(0x40644, ucTemp, 1);
    while(ucTemp[0])
    {
        icn85xx_prog_i2c_rxdata(0x40644,ucTemp,1);
    }      
            
}

/***********************************************************************************************
Name    :   icn85xx_erase_flash 
Input   :   
Output  :   
function    :   erase flash
***********************************************************************************************/
int  icn85xx_erase_flash(void)
{
    int i;
    ClearFlashState(); 
    while(FlashState(0)&0x01);
    FlashWriteEnable();
    EarseFlash(1,0);            
    while((FlashState(0)&0x01));  

    for(i=0; i<7; i++)
    {
        FlashWriteEnable();
        EarseFlash(3,0x8000+i*0x1000);            
        while((FlashState(0)&0x01)); 
    }
    for(i=0; i<4; i++)
    {
        FlashWriteEnable();
        EarseFlash(3,0x10000+i*0x1000);            
        while((FlashState(0)&0x01)); 
    }    
    return 0;
}


/***********************************************************************************************
Name    :   icn85xx_prog_buffer 
Input   :   
Output  :   
function    :   progm flash
***********************************************************************************************/
int  icn85xx_prog_buffer(unsigned int flash_addr,unsigned int sram_addr,unsigned int copy_length,unsigned char program_type)
{
    unsigned char ucTemp[4] = {0,0,0,0};
    unsigned char prog_state=0;

    unsigned int i=0;
    unsigned char program_commond=0;
    if(program_type == 0)
    {
        program_commond = 0x02;
    }
    else if(program_type == 1)
    {
        program_commond = 0xf2;
    }
    else
    {
        program_commond = 0x02;
    }
    
     
    for(i=0; i<copy_length; )
    {
        prog_state=(FlashState(0)&0x01);
        while(prog_state)
        {
            prog_state=(FlashState(0)&0x01);
        }
        FlashWriteEnable();           

        ucTemp[2]=0;
        ucTemp[1]=0x13;
        ucTemp[0]=0;
        icn85xx_prog_i2c_txdata(0x4062c, ucTemp, 3);

        ucTemp[2]=(unsigned char)(flash_addr>>16);
        ucTemp[1]=(unsigned char)(flash_addr>>8);
        ucTemp[0]=(unsigned char)(flash_addr);
        icn85xx_prog_i2c_txdata(0x40634, ucTemp, 3);
        
        ucTemp[2]=(unsigned char)(sram_addr>>16);
        ucTemp[1]=(unsigned char)(sram_addr>>8);
        ucTemp[0]=(unsigned char)(sram_addr);
        icn85xx_prog_i2c_txdata(0x4063c, ucTemp, 3);

        if(i+256<=copy_length)
        {
            ucTemp[1]=0x01;
            ucTemp[0]=0x00;
        }
        else
        {
            ucTemp[1]=(unsigned char)((copy_length-i)>>8);
            ucTemp[0]=(unsigned char)(copy_length-i);
        }
        icn85xx_prog_i2c_txdata(0x40640, ucTemp,2);

        ucTemp[0]=program_commond;
        icn85xx_prog_i2c_txdata(0x40630, ucTemp,1);

        ucTemp[0]=0x01;
        icn85xx_prog_i2c_txdata(0x40644, ucTemp,1);
        
        flash_addr+=256;
        sram_addr+=256;
        i+=256;
        while(ucTemp[0])
        {
            icn85xx_prog_i2c_rxdata(0x40644,ucTemp,1);
        }
    
    }     
  
    prog_state=(FlashState(0)&0x01);
    while(prog_state)
    {
        prog_state=(FlashState(0)&0x01);
    }
    return 0;   
}

/***********************************************************************************************
Name    :   icn85xx_prog_data
Input   :   
Output  :   
function    :   write int data to flash
***********************************************************************************************/
int  icn85xx_prog_data(unsigned int flash_addr, unsigned int data)
{
    unsigned char ucTemp[4] = {0,0,0,0};
    
    ucTemp[3]=(unsigned char)(data>>24);
    ucTemp[2]=(unsigned char)(data>>16);
    ucTemp[1]=(unsigned char)(data>>8);
    ucTemp[0]=(unsigned char)(data);
    
    icn85xx_prog_i2c_txdata(0x2a000, ucTemp,4);
    icn85xx_prog_buffer(flash_addr , 0x2a000, 0x04,  0);
    return 0;
}

/***********************************************************************************************
Name    :   icn85xx_read_flash
Input   :   
Output  :   
function    :   read data from flash to sram
***********************************************************************************************/
void  icn85xx_read_flash(unsigned int sram_address,unsigned int flash_address,unsigned long copy_length,unsigned char i2c_wire_num)
{
    unsigned char ucTemp[4] = {0,0,0,0};

    if(i2c_wire_num==1)
    {
        ucTemp[2]=0x18;
        ucTemp[1]=0x13;
        ucTemp[0]=0x00;
    }
    else if(i2c_wire_num==2)
    {
        ucTemp[2]=0x1a;
        ucTemp[1]=0x13;
        ucTemp[0]=0x01;
    }
    else if(i2c_wire_num==4)
    {
        ucTemp[2]=0x19;
        ucTemp[1]=0x13;
        ucTemp[0]=0x01;
    }
    else
    {
        ucTemp[2]=0x18;
        ucTemp[1]=0x13;
        ucTemp[0]=0x01;
    }
    icn85xx_prog_i2c_txdata(0x4062c, ucTemp,3);

    if(i2c_wire_num==1)
    {           
        ucTemp[0]=0x03;
    }
    else if(i2c_wire_num==2)
    {
        ucTemp[0]=0x3b;
    }
    else if(i2c_wire_num==4)
    {
        ucTemp[0]=0x6b;
    }
    else
    {
        ucTemp[0]=0x0b;
    }   
    icn85xx_prog_i2c_txdata(0x40630, ucTemp,1);
    
    ucTemp[2]=(unsigned char)(flash_address>>16);
    ucTemp[1]=(unsigned char)(flash_address>>8);
    ucTemp[0]=(unsigned char)(flash_address);
    icn85xx_prog_i2c_txdata(0x40634, ucTemp,3);    

    ucTemp[2]=(unsigned char)(sram_address>>16);
    ucTemp[1]=(unsigned char)(sram_address>>8);
    ucTemp[0]=(unsigned char)(sram_address);
    icn85xx_prog_i2c_txdata(0x4063c, ucTemp,3);
    
    ucTemp[1]=(unsigned char)(copy_length>>8);
    ucTemp[0]=(unsigned char)(copy_length);
    icn85xx_prog_i2c_txdata(0x40640, ucTemp,2);
    
    ucTemp[0]=0x01;

    icn85xx_prog_i2c_txdata(0x40644, ucTemp,1);
    while(ucTemp[0])
    {
        icn85xx_prog_i2c_rxdata(0x40644,ucTemp,1);
    }

}

/***********************************************************************************************
Name    :   icn85xx_read_fw_Ver 
Input   :   fw
Output  :   
function    :   read fw version
***********************************************************************************************/

short  icn85xx_read_fw_Ver(char *fw)
{
    short FWversion;
    char tmp[2];
    int file_size;
    file_size = icn85xx_open_fw(fw);
    if(file_size < 0)
    {
        return -1;  
    }   
    icn85xx_read_fw(0x100, 2, &tmp[0]);
    
    icn85xx_close_fw();
    FWversion = (tmp[1]<<8)|tmp[0];
//    flash_info("FWversion: 0x%x\n", FWversion);
    return FWversion;

   
}

#define ENABLE_BYTE_CHECK 1
/***********************************************************************************************
Name    :   icn85xx_fw_download 
Input   :   
Output  :   
function    :   download code to sram
***********************************************************************************************/
int  icn85xx_fw_download(unsigned int offset, unsigned char * buffer, unsigned int size)
{
#ifdef ENABLE_BYTE_CHECK      
    unsigned char testb[B_SIZE]={0};
    int i =0;
#endif

    icn85xx_prog_i2c_txdata(offset,buffer,size);   
#ifdef ENABLE_BYTE_CHECK  
    icn85xx_prog_i2c_rxdata(offset,testb,size);   
    for(i = 0; i < size; i++)
    {
        if(buffer[i] != testb[i])
        {
            flash_error("buffer[%d]:%x  testb[%d]:%x\n",i,buffer[i],i,testb[i]);
            return -1;
        }  
    }
//    pr_err("same data\n");
#endif 
    return 0;   
}

/***********************************************************************************************
Name    :   icn85xx_bootfrom_flash
Input   :   
Output  :   
function    :
***********************************************************************************************/
int  icn85xx_bootfrom_flash(int ictype)
{
    int ret = -1;
    unsigned char ucTemp;    
    flash_info("20141114 icn85xx_bootfrom_flash\n");
    if(ictype == ICN85XX_WITH_FLASH_85)
    {
        ucTemp=0x7f;            
        ret = icn85xx_prog_i2c_txdata(0x40004, &ucTemp, 1 );        //ICN85XX chip reset 
        if (ret < 0) {
            flash_error("1 %s failed: %d\n", __func__, ret); 
            return ret;
        }
    }
    else if(ictype == ICN85XX_WITH_FLASH_86)
    {
        ucTemp=0x7f;            
        ret = icn85xx_prog_i2c_txdata(0x4046c, &ucTemp, 1 );        //ICN85EX chip reset 
        if (ret < 0) {
            flash_error("2 %s failed: %d\n", __func__, ret); 
            return ret;
        }    
    }
    return ret;
}

/***********************************************************************************************
Name    :   icn85xx_bootfrom_sram
Input   :   
Output  :   
function    :
***********************************************************************************************/
int  icn85xx_bootfrom_sram(void)
{
    int ret = -1;   
    unsigned char ucTemp = 0x03;                      
    unsigned long addr = 0x40400;   
    flash_info("icn85xx_bootfrom_sram\n");
    ret = icn85xx_prog_i2c_txdata(addr, &ucTemp, 1 );           //change bootmode from sram   
    return ret;
}
/***********************************************************************************************
Name    :   icn85xx_crc_calc
Input   :   
Output  :   
function    :
***********************************************************************************************/
/*
unsigned int icn85xx_crc_calc(unsigned crc_in, char *buf, int len)
{
    int pos;
    unsigned int crc_result;
    unsigned char in_data_8b;
    unsigned int    crc_reg_32b;
    unsigned char   i;
    unsigned char   xor_flag;

    crc_result = crc_in;
    for(pos=0;pos<len;pos++)
    {
        in_data_8b = *(buf+pos);
        crc_reg_32b = crc_result ; 
        for(i=0; i<32; i++) 
        { 
            if(crc_reg_32b & 0x00000001)
            {
                crc_reg_32b ^= 0x04C11DB7;
                crc_reg_32b >>= 1;
                crc_reg_32b |= 0x80000000;
            }
            else 
            {
                crc_reg_32b >>= 1;
            } 
        } 
 
        for(i=0; i<40; i++) 
        {
            xor_flag = (crc_reg_32b>>31);
            crc_reg_32b = (crc_reg_32b<<1) + (in_data_8b>>7);
            in_data_8b = (in_data_8b<<1);
            if(xor_flag)
            {
                crc_reg_32b = crc_reg_32b ^ 0x04C11DB7;
            }
        } 
        crc_result = crc_reg_32b;
    }
    return crc_result;
}
*/


/* 
    This polynomial (0x04c11db7) is used at: AUTODIN II, Ethernet, & FDDI 
*/
static unsigned int crc32table[256] = {    
 0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,    
 0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,    
 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,    
 0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,    
 0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,    
 0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,    
 0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,    
 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,    
 0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,    
 0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,    
 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,    
 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,    
 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,    
 0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,    
 0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,    
 0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,    
 0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,    
 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,    
 0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,    
 0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,    
 0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,    
 0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,    
 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,    
 0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,    
 0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,    
 0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,    
 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,    
 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,    
 0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,    
 0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,    
 0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,    
 0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,    
 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,    
 0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,    
 0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,    
 0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,    
 0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,    
 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,    
 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,    
 0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,    
 0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,    
 0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,    
 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

unsigned int icn85xx_crc_calc(unsigned crc_in, char *buf, int len)  
{
    int i;    
    unsigned int crc = crc_in;        
    for(i = 0; i < len; i++)        
        crc = (crc << 8) ^ crc32table[((crc >> 24) ^ *buf++) & 0xFF];        
    return crc;
}


/***********************************************************************************************
Name    :   icn85xx_crc_enable
Input   :   
Output  :   
function    :control crc control
***********************************************************************************************/
int  icn85xx_crc_enable(unsigned char enable)
{
    unsigned char ucTemp;
    int ret = 0;
    if(enable==1)
    {
        ucTemp = 1;
        ret = icn85xx_prog_i2c_txdata(0x40028, &ucTemp, 1 );
    }
    else if(enable==0)
    {
        ucTemp = 0;
        ret = icn85xx_prog_i2c_txdata(0x40028, &ucTemp, 1 );
    } 
    return ret;
}
/***********************************************************************************************
Name    :   icn85xx_crc_check
Input   :   
Output  :   
function    :chec crc right or not
***********************************************************************************************/
int  icn85xx_crc_check(unsigned int crc, unsigned int len)
{
    int ret;
    unsigned int crc_len;
    unsigned int crc_result;    
    unsigned char ucTemp[4] = {0,0,0,0};     
    
    ret= icn85xx_prog_i2c_rxdata(0x4002c, ucTemp, 4 );
    crc_result = ucTemp[3]<<24 | ucTemp[2]<<16 | ucTemp[1] << 8 | ucTemp[0];
//    flash_info("crc_result: 0x%x\n", crc_result);
    
    ret = icn85xx_prog_i2c_rxdata(0x40034, ucTemp, 2);
    crc_len = ucTemp[1] << 8 | ucTemp[0];
//    flash_info("crc_len: %d\n", crc_len);

    if((crc_result == crc) && (crc_len == len))
        return 0;
    else 
    {
        flash_info("crc_fw: 0x%x\n", crc);
        flash_info("crc_result: 0x%x\n", crc_result);
        flash_info("crc_len: %d\n", crc_len);
        return -1;
    }
    
}


/***********************************************************************************************
Name    :   icn85xx_fw_update 
Input   :   fw
Output  :   
function    :   upgrade fw
***********************************************************************************************/
//E_UPGRADE_ERR_TYPE icn85xx_fw_update(void *fw)
int  icn85xx_fw_update(void *fw)
{
    int file_size, last_length;
    int j, num;
    char temp_buf[B_SIZE];
    unsigned int crc_fw;

    file_size = icn85xx_open_fw(fw);
    if(file_size < 0)
    {
        icn85xx_update_status(R_FILE_ERR);
        return R_FILE_ERR;  
    }
    if(icn85xx_goto_progmode() != 0)
    {
        icn85xx_update_status(R_PROGRAM_ERR);
        flash_error("icn85xx_goto_progmode() != 0 error\n");
        return R_STATE_ERR; 
    } 
    msleep(1); 
    icn85xx_crc_enable(1);
    flash_info("20141014 icn85xx_fw_download start\n");
    num = file_size/B_SIZE;   
    crc_fw = 0;
    for(j=0; j < num; j++)
    {
        icn85xx_read_fw(j*B_SIZE, B_SIZE, temp_buf);        
        crc_fw = icn85xx_crc_calc(crc_fw, temp_buf, B_SIZE);
        if(icn85xx_fw_download(j*B_SIZE, temp_buf, B_SIZE) != 0)
        {
            flash_error("error j:%d\n",j);
            icn85xx_update_status(R_PROGRAM_ERR);
            icn85xx_close_fw();
            return R_PROGRAM_ERR;
        }    
        icn85xx_update_status(5+(int)(60*j/num));
    }
    last_length = file_size - B_SIZE*j;
    if(last_length > 0)
    {
        icn85xx_read_fw(j*B_SIZE, last_length, temp_buf);     
        crc_fw = icn85xx_crc_calc(crc_fw, temp_buf, last_length);
        if(icn85xx_fw_download(j*B_SIZE, temp_buf, last_length) != 0)
        {
            flash_error("error last length\n");
            icn85xx_update_status(R_PROGRAM_ERR);
            icn85xx_close_fw();
            return R_PROGRAM_ERR;
        }
    }    
    icn85xx_close_fw(); 
    icn85xx_update_status(65);
//    flash_info("crc_fw: 0x%x\n", crc_fw);
//    msleep(1);
    icn85xx_crc_enable(0);
    if(icn85xx_crc_check(crc_fw, file_size) != 0)
    {
        icn85xx_update_status(R_VERIFY_ERR);
        flash_info("down fw error, crc error\n");
        return R_PROGRAM_ERR;
    }
    else
    {
        //flash_info("downoad fw ok, crc ok\n");
    }
    icn85xx_update_status(70);
   flash_info("icn85xx_fw_download ok\n"); 
    if(ICN85XX_WITH_FLASH_85 == boot_mode)
    {
        icn85xx_erase_flash();
    //    return R_PROGRAM_ERR;
        icn85xx_update_status(75);

        FlashWriteEnable();

        icn85xx_prog_buffer( 0, 0, file_size,0);

        icn85xx_update_status(85);

        while((FlashState(0)&0x01));         
        FlashWriteEnable();

        icn85xx_prog_data(FLASH_CRC_ADDR, crc_fw);
        icn85xx_prog_data(FLASH_CRC_ADDR+4, file_size);

        icn85xx_update_status(90);

        
        icn85xx_crc_enable(1);   
        icn85xx_read_flash( 0,  0, file_size,  2);    
        icn85xx_crc_enable(0);
        if(icn85xx_crc_check(crc_fw, file_size) != 0)
        {
            flash_info("read flash data error, crc error\n");
            return R_PROGRAM_ERR;
        }
        else
        {
            flash_info("read flash data ok, crc ok\n");
        }                  
        while((FlashState(0)&0x01));
        icn85xx_update_status(95);

       // if(icn85xx_bootfrom_flash() == 0)
        if(icn85xx_bootfrom_sram() == 0)	//code already in ram
        {
            flash_error("icn85xx_bootfrom_flash error\n");
            icn85xx_update_status(R_STATE_ERR);
            return R_STATE_ERR;
        }
    }
    else if(ICN85XX_WITHOUT_FLASH == boot_mode)
    {
        if(icn85xx_bootfrom_sram() == 0)
        {
            flash_error("icn85xx_bootfrom_sram error\n");
            icn85xx_update_status(R_STATE_ERR);
            return R_STATE_ERR;
        } 
    }
    msleep(50);
    icn85xx_update_status(R_OK);
    flash_info("icn85xx upgrade ok\n");
    return R_OK;
}
