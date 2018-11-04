#include "hwz.h"
#include <xc.h>
#include <plib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/appio.h>

#include "sim.h"
#include "simglb.h"
#include "../hw.h"

extern const uint8_t rom_image[65536];
extern const uint8_t rd_image[131072];
extern const uint8_t rd_image2[ROMDISK2_SIZE];
uint8_t drive, sector, track,disk_temp_pointer;
uint8_t disk_temp[128],flash_buff[4096], conin_buffer[30], conin_buffer_pointer;

uint32_t last_addr = 0xFFFFF000;
uint8_t unwritten;

uint8_t fl_rdsr(void);
uint32_t fl_rdid(void);

#ifdef	USE_RAM_IMAGE_OLD
extern const uint8_t ram_image[65536];
#endif

#ifdef	USE_RAM_IMAGE_NEW
extern const uint8_t ram_image_a[3];
extern const uint8_t ram_image_b[0xFFFF-0xD800];
#endif

#ifdef	USE_RAMDISK
uint8_t ram_disk[RAMDISK_SIZE];
#endif

void reload_cpm_warm (void)
{
uint16_t i;
#ifdef	USE_RAM_IMAGE_OLD
	for (i=0xD400;i<(0xD400+0x1EFF);i++) ram[i] = ram_image[i];
#endif
#ifdef	USE_RAM_IMAGE_NEW
	for (i=0;i<0x1AFF;i++) ram[i+0xD800] = ram_image_b[i];
#endif
	}

//-------------------device at 0x68-----------------
uint8_t rxm_sta (void)
{
/*
if (U3BSTAbits.URXDA==1) return 0xFF;
	else return 0x00;
 */
}
uint8_t rxm_read (void)
{
/*
return U3BRXREG;
 */
}
void txm_write (uint8_t data)
{
/*
U3BTXREG = data;
while (U3BSTAbits.UTXBF==1);
 */
}

void set_drive (uint8_t dat)
{
drive = dat;
disk_temp_pointer = 0;
}
void set_sector (uint8_t dat)
{
sector = dat;
disk_temp_pointer = 0;
}
void set_track (uint8_t dat)
{
track = dat;
disk_temp_pointer = 0;
}

uint8_t read_disk_byte (void)
{
uint8_t temp;
uint32_t  base,ptr;
base = (((uint32_t )(track))*16) + sector;
if (drive==0)
	{
	base = base*128;
#ifdef USE_RAMDISK
	ptr = base + disk_temp_pointer;
	if (ptr<RAMDISK_SIZE)
		temp = ram_disk[ptr];
#endif
#ifndef	USE_RAMDISK
	temp = 0xA5;
#endif
	}
if (drive==1)
	{
	base = base*128;
#ifdef	USE_ROMDISK
	temp = rd_image[base + disk_temp_pointer];
#endif
	}
if (drive==2)
	{
#ifdef USE_EEPROM
	if (disk_temp_pointer==0) read_sector(disk_temp,base);
	temp = disk_temp[disk_temp_pointer];
#endif
	base = base*128;
#ifdef	USE_ROMDISK2
	temp = rd_image2[base + disk_temp_pointer];
#endif
	}

if (drive==3)
	{
	if (disk_temp_pointer==0) fl_read_128(base+(CPM1_DISK1_OFFSET),disk_temp);
	temp = disk_temp[disk_temp_pointer];
	}
if (drive==4)
	{
	if (disk_temp_pointer==0) fl_read_128(base+(CPM1_DISK2_OFFSET)+4096,disk_temp);
	temp = disk_temp[disk_temp_pointer];
	}
if (drive==5)
	{
	if (disk_temp_pointer==0) fl_read_128(base+(CPM1_DISK3_OFFSET)+(2*4096),disk_temp);
	temp = disk_temp[disk_temp_pointer];
	}
if (drive==6)
	{
	}

disk_temp_pointer++;
return temp;
}

void write_disk_byte (uint8_t dat)
{
uint8_t temp;
uint32_t base;
uint32_t  ptr;
base = (((unsigned int)(track))*16) + sector;
if (drive==0)
	{
#ifdef	USE_RAMDISK
	base = base*128;
	ptr = base + disk_temp_pointer;
	if (ptr<RAMDISK_SIZE)
		ram_disk[ptr] = dat;
#endif
	}
if (drive==1)
	{
	//rom disk, no writes allowed
	}
if (drive==2)
	{
#ifdef USE_EEPROM
	disk_temp[disk_temp_pointer] = dat;
	if (disk_temp_pointer==127) 
		{
		ee_wren();
		write_sector(disk_temp,base);
		}
#endif
	}
if (drive==3)
	{
	disk_temp[disk_temp_pointer] = dat;
	if (disk_temp_pointer==127) 
		{
		fl_write_128(base+(CPM1_DISK1_OFFSET),disk_temp);
		}
	}
if (drive==4)
	{
	disk_temp[disk_temp_pointer] = dat;
	if (disk_temp_pointer==127) 
		{
		fl_write_128(base+(CPM1_DISK2_OFFSET)+(1*4096),disk_temp);
		}
	}
if (drive==5)
	{
	disk_temp[disk_temp_pointer] = dat;
	if (disk_temp_pointer==127) 
		{
		fl_write_128(base+(CPM1_DISK3_OFFSET)+(2*4096),disk_temp);
		}
	}
if (drive==6)
	{
	}
disk_temp_pointer++;
}

uint8_t fl_rdsr(void)
{
volatile uint8_t temp;
CS_FLASH = 0;
SPI_dat(0x05);
temp = SPI_dat(0xFF);
CS_FLASH = 1;
return temp;
}


uint32_t fl_rdid(void)
{
uint8_t temp1,temp2,temp3;
uint32_t retval;
CS_FLASH = 0;
SPI_dat(0x9F);
temp3 = SPI_dat(0xFF);
temp2 = SPI_dat(0x55);
temp1 = SPI_dat(0xAA);
CS_FLASH = 1;
retval = (((uint32_t)(temp3))<<16)|(((uint32_t)(temp2))<<8)|(((uint32_t)(temp1))<<0);
return retval;
}

void fl_read_4k(uint32_t  addr, uint8_t * data)
{
uint16_t i;
CS_FLASH = 0;
SPI_dat(0x03);
SPI_dat((addr>>16)&0xFF);
SPI_dat((addr>>8)&0xFF);
SPI_dat((addr>>0)&0xFF);
for (i=0;i<4096;i++) *data++ = SPI_dat(0xFF);
CS_FLASH = 1;
}

void fl_read_nk(uint32_t  addr, uint8_t * data, uint16_t n)
{
uint16_t i;
CS_FLASH = 0;
SPI_dat(0x03);
SPI_dat((addr>>16)&0xFF);
SPI_dat((addr>>8)&0xFF);
SPI_dat((addr>>0)&0xFF);
for (i=0;i<n;i++) *data++ = SPI_dat(0xFF);
CS_FLASH = 1;
}

void fl_unlock(void)
{
uint16_t i;
fl_wren();
CS_FLASH = 0;
SPI_dat(0x98);
CS_FLASH = 1;
}

void fl_erase_4k(uint32_t  addr)
{
uint16_t i;
fl_wren();
CS_FLASH = 0;
SPI_dat(0x20);
SPI_dat((addr>>16)&0xFF);
SPI_dat((addr>>8)&0xFF);
SPI_dat((addr>>0)&0xFF);
CS_FLASH = 1;
while ((fl_rdsr())&0x01);
}


void fl_write(uint32_t  addr,uint8_t data)
{
uint16_t i;
fl_wren();
CS_FLASH = 0;
SPI_dat(0x02);
SPI_dat((addr>>16)&0xFF);
SPI_dat((addr>>8)&0xFF);
SPI_dat((addr>>0)&0xFF);
SPI_dat(data);
CS_FLASH = 1;
}

void fl_rst_pb(void)
{
/*
CS_FLASH = 0;
SPI_dat(0x50);
CS_FLASH = 1;
*/
fl_wren();
CS_FLASH = 0;
SPI_dat(0x01);
SPI_dat(0x00);
CS_FLASH = 1;
}

void fl_wren(void)
{
CS_FLASH = 0;
SPI_dat(0x06);
CS_FLASH = 1;
}


void fl_write_4k(uint32_t  addr, uint8_t * data)
{
uint16_t i;
for (i=0;i<4096;i++) 
	{
	fl_write(addr+i,*data++);
	while ((fl_rdsr())&0x01);
	}
}

void fl_write_128(uint32_t sector,uint8_t * data)
{
uint32_t  addr;
uint8_t i;
addr = ((uint32_t )(sector))*128UL;
addr = addr&0xFFFFF000;
#ifdef	FLASH_BUFFERING	
if (last_addr!=addr)
	{
	if (last_addr!=0xFFFFF000)
		{
		fl_erase_4k(last_addr);
		fl_write_4k(last_addr,flash_buff);
		}
	fl_read_4k(addr,flash_buff);
	last_addr = addr;
	}
unwritten = 1;
#endif
#ifndef	FLASH_BUFFERING	
fl_read_4k(addr,flash_buff);
#endif
addr = ((uint32_t )(sector))*128UL;
addr = addr&0x00000FFF;
for (i=0;i<128;i++) flash_buff[addr+i] = data[i];
addr = ((uint32_t )(sector))*128UL;
addr = addr&0xFFFFF000;
#ifndef	FLASH_BUFFERING	
fl_erase_4k(addr);
fl_write_4k(addr,flash_buff);
#endif
}

void fl_read_128(uint32_t sector,uint8_t * data)
{
uint32_t  addr;
#ifdef	FLASH_BUFFERING	
if (unwritten == 1)
	{
	fl_erase_4k(last_addr);
	fl_write_4k(last_addr,flash_buff);
	unwritten = 0;	
	last_addr = 0xFFFFF000;
	}
#endif
addr = ((uint32_t )(sector))*128UL;
fl_read_nk(addr,data,128);
}

uint8_t init_first_x_sects (uint8_t i, uint8_t verify)			//format directory area
{
uint32_t j,k;
for (j=0;j<128;j++) disk_temp[j]=0xE5;
#ifdef USE_EEPROM
for (j=0;j<i;j++) 
	{
	ee_wren();
	write_sector(disk_temp,j);
	}
#endif
for (j=0;j<i;j++) fl_write_128(j+(CPM1_DISK1_OFFSET),disk_temp);
for (j=0;j<i;j++) fl_write_128(j+(CPM1_DISK2_OFFSET),disk_temp);
for (j=0;j<i;j++) fl_write_128(j+(CPM1_DISK3_OFFSET),disk_temp);

if (verify!=0)
	{
	for (j=0;j<i;j++) 
		{
		fl_read_128(j+(1*4096),disk_temp);
		for (k=0;k<128;k++) 
			{
			if (disk_temp[k]!=0xE5)
				{
				return 1;
				}
			}
		}
	}
return 0;
}



#ifdef USE_EEPROM
void write_sector (unsigned char *data, unsigned int addr)
{
unsigned char i,temp;
CS_MEM = 0;
SPI_dat(0x02);
temp = (addr>>9);
SPI_dat(temp);
temp = (addr>>1);
SPI_dat(temp);
temp = (addr<<7);
SPI_dat(temp);

for (i=0;i<128;i++) 
	SPI_dat(data[i]);

CS_MEM = 1;
temp = ee_rs();
temp = temp&0x01;

while (temp>0)
	{
	temp = ee_rs();
	temp = temp&0x01;
	}
}

void read_sector (unsigned char *data, unsigned int addr)
{
unsigned char i,temp;
CS_MEM = 0;
SPI_dat(0x03);
temp = (addr>>9);
SPI_dat(temp);
temp = (addr>>1);
SPI_dat(temp);
temp = (addr<<7);
SPI_dat(temp);


for (i=0;i<128;i++) 
	{
	*data = SPI_dat(0xFF);
	*data++;
	}

CS_MEM = 1;
}
unsigned char ee_rs (void)
{
unsigned char temp;
CS_MEM = 0;
SPI_dat(0x05);
temp = SPI_dat(0xFF);
CS_MEM = 1;
return temp;
}

void ee_wren (void)
{
CS_MEM = 0;
SPI_dat(0x06);
CS_MEM = 1;
}

void ee_wrdi (void)
{
CS_MEM = 0;
SPI_dat(0x04);
CS_MEM = 1;
}

#endif