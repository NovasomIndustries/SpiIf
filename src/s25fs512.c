#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ftd2xx.h"
#include "libMPSSE_spi.h"
#include "s25fs512.h"
#include "SpiIf.h"

unsigned char s25fs512_id[8] = {0x01, 0x02, 0x20, 0x4d, 0x00, 0x81, 0x30, 0x31};
extern  FT_HANDLE ftHandle;
extern  unsigned char read_buf[DATA_SIZE];
extern  int vflag;

unsigned char flash_IDR(unsigned char *reg_read)
{
unsigned char loc_tx_buff[16];
unsigned int sizeTransfered;

    /* WRITE_ENABLED */

    loc_tx_buff[0] = CMD_GET_IDENTIFICATION;                       //command
    if ( SPI_Write(ftHandle, loc_tx_buff, 1, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE ) )
    {
        printf("Error sending CMD_GET_IDENTIFICATION\n");
        exit(1);
    }

    if ( SPI_Read(ftHandle, reg_read, 16, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES| SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
        return 1;
    return 0;//okay
}

unsigned char flash_write_enable(void)
{
unsigned int sizeTransfered;
unsigned char loc_tx_buff[2];
    loc_tx_buff[0] = CMD_WRITE_ENABLED;
    if (SPI_Write(ftHandle, loc_tx_buff, 1, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
        exit (1);//ko
    return (0);
}

unsigned char flash_write_disable(void)
{
unsigned int sizeTransfered;
unsigned char loc_tx_buff[2];
    loc_tx_buff[0] = CMD_WRITE_DISABLED;
    if (SPI_Write(ftHandle, loc_tx_buff, 1, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
        return (1);//ko
    return (0);
}

unsigned char flash_enter4bam(void)
{
unsigned int sizeTransfered;
unsigned char loc_tx_buff[2];
    loc_tx_buff[0] = E4BAM_CMD;
    if (SPI_Write(ftHandle, loc_tx_buff, 1, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
        return (1);//ko
    return (0);
}

unsigned char flash_WRAR(unsigned int addr_reg_to_read, unsigned char data_to_write)
{
unsigned char loc_tx_buff[10];
unsigned int  rd_wr_len  = 0;

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+     Write "Read Latency" dummy cycle
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    /* 3byte add */
    loc_tx_buff[0] = WRAR;                       //command
    loc_tx_buff[4] =  addr_reg_to_read & 0xff;           //address
    loc_tx_buff[3] = (addr_reg_to_read >> 8)  & 0xff;
    loc_tx_buff[2] = (addr_reg_to_read >> 16) & 0xff;
    loc_tx_buff[1] = (addr_reg_to_read >> 24) & 0xff;

    loc_tx_buff[5] = data_to_write;                     //data to write!!!

    rd_wr_len      = 6;

    if (write_byte(loc_tx_buff, rd_wr_len))
        return 1;


    return 0;//okay
}

unsigned char flash_RDAR(unsigned int addr_reg_to_read, unsigned char *reg_read)
{
unsigned char loc_tx_buff[10];
unsigned int sizeTransfered;

    /* WRITE_ENABLED */

    loc_tx_buff[0] = RDAR;                       //command

    loc_tx_buff[4] =  addr_reg_to_read & 0xff;           //address
    loc_tx_buff[3] = (addr_reg_to_read >> 8)  & 0xff;
    loc_tx_buff[2] = (addr_reg_to_read >> 16) & 0xff;
    loc_tx_buff[1] = (addr_reg_to_read >> 24) & 0xff;      //address msb
    if ( SPI_Write(ftHandle, loc_tx_buff, 5, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE ) )
    {
        printf("Error sending RDAR\n");
        exit(1);
    }

    if ( SPI_Read(ftHandle, loc_tx_buff, 4, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES| SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
        return 1;
    *reg_read = loc_tx_buff[1];
    return 0;//okay
}

unsigned char flash_SingleRegRead(unsigned char single_reg)
{
unsigned char loc_tx_buff[10];
unsigned int sizeTransfered;

    /* WRITE_ENABLED */

    loc_tx_buff[0] = single_reg;                       //command

    if ( SPI_Write(ftHandle, loc_tx_buff, 1, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE ) )
    {
        printf("Error sending Single Reg Read CMD\n");
        exit(1);
    }

    if ( SPI_Read(ftHandle, loc_tx_buff, 1, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES| SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
        return 0;
    return loc_tx_buff[0];//okay
}

unsigned char flash_busy_and_error_check(void)
{
    return flash_SingleRegRead(CMD_READ_SR1) & 0x01;
}

unsigned char write_byte(unsigned char *buffer_out, unsigned int len_tx)
{
unsigned int sizeTransfered = 0;

    if (SPI_Write(ftHandle, buffer_out, len_tx, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
        return (1);//ko
    return (0);
}

void flash_getId(void)
{
unsigned int i;

    flash_IDR(read_buf);

    for (i=0;i<8;i++)
    {
        if ( read_buf[i] != s25fs512_id[i])
        {
            printf("Expected 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",s25fs512_id[0],s25fs512_id[1],s25fs512_id[2],s25fs512_id[3],s25fs512_id[4],s25fs512_id[5],s25fs512_id[6],s25fs512_id[7] );
            printf("Got      0x%02x%02x%02x%02x%02x%02x%02x%02x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3],read_buf[4],read_buf[5],read_buf[6],read_buf[7] );
            printf("Wrong flash type\n");
            set_Fault_Led_pin(1);
            exit(1);
        }
    }
    printf("Got 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3],read_buf[4],read_buf[5],read_buf[6],read_buf[7] );

}

void flash_setup(void)
{
unsigned char rd_special;

    flash_enter4bam();
    flash_WRAR(CR2V, 0x00);     // no latency
    flash_WRAR(CR3V, 0x00);     //

    flash_write_enable();
    if ( (flash_SingleRegRead(CMD_GET_STATUS) & 0x02 ) != 0x02 )
        printf("Unable to enable WEL\n");
    flash_write_disable();
    if ( (flash_SingleRegRead(CMD_GET_STATUS) & 0x02 )!= 0x00 )
        printf("Unable to disable WEL\n");
    if ( vflag == 1 )
    {
        flash_RDAR(SR1V,&rd_special);
        printf("SR1V 0x%02x\n",rd_special);
        printf("CR1V 0x%02x\n",flash_SingleRegRead(CMD_READ_CR));
        flash_RDAR(CR2V,&rd_special);
        printf("CR2V 0x%02x\n",rd_special);
        flash_RDAR(CR3V,&rd_special);
        printf("CR3V 0x%02x\n",rd_special);
        flash_RDAR(PPBL,&rd_special);
        printf("PPBL 0x%02x\n",rd_special);
    }
}

unsigned char flash_Write(unsigned char *write_buf , int len_data , unsigned int write_address)
{
#define PAGE_LEN    256
#define CMD_LEN     5

unsigned char   loc_tx_buff[PAGE_LEN+CMD_LEN];
unsigned int    i,sizeTransfered;
unsigned int    pagelen = PAGE_LEN;


	while (len_data > 0)
	{
        loc_tx_buff[0] = CMD_4PROGRAM_PAGE;
        loc_tx_buff[1] = (unsigned char)(write_address >> 24);
        loc_tx_buff[2] = (unsigned char)(write_address >> 16);
        loc_tx_buff[3] = (unsigned char)(write_address >> 8 );
        loc_tx_buff[4] = (unsigned char)(write_address);

        for (i = 0; i < PAGE_LEN; i++)
            loc_tx_buff[i + CMD_LEN] = *write_buf++;
            /*
        printf("Writing at address 0x%02x%02x%02x%02x\n",loc_tx_buff[1],loc_tx_buff[2],loc_tx_buff[3],loc_tx_buff[4]);
        for (i = 0; i < 32; i++)
            printf("0x%02x ",loc_tx_buff[i + CMD_LEN]);
        printf("\n");
    */
        while( flash_busy_and_error_check() )
            usleep(100);

        flash_write_enable();
//        flash_SingleRegRead(CMD_GET_STATUS);
        if (SPI_Write(ftHandle, loc_tx_buff, PAGE_LEN+CMD_LEN, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
            exit (1);
        while( flash_busy_and_error_check() )
            usleep(100);
//        flash_SingleRegRead(CMD_GET_STATUS);
//        flash_write_disable();

		write_address += pagelen;
		len_data -= pagelen;
	}
    return 0;
}

unsigned char flash_Read(unsigned int address, unsigned int len_buff, unsigned char *read_buffer)
{
unsigned char  cmd_buf[6];
unsigned int sizeTransfered = 0 , block_count = 0 , cnt = 0;
#define     BLOCK_SIZE      8192

    printf("Start Read %d\n",len_buff);
    while(flash_busy_and_error_check())
        usleep(100);
    while( len_buff > 0 )
    {
        cmd_buf[0] = CMD_4FAST_READ_PAGE;
        cmd_buf[1] = (unsigned char)(address >> 24);
        cmd_buf[2] = (unsigned char)(address >> 16);
        cmd_buf[3] = (unsigned char)(address >> 8);
        cmd_buf[4] = (unsigned char)address;
        if (SPI_Write(ftHandle, cmd_buf, 5, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE ) != FT_OK)
            exit (1);
        if (SPI_Read(ftHandle, read_buffer, BLOCK_SIZE, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) != FT_OK)
            exit (1);

        cnt++;
        if ( cnt >= 16 )
        {
            printf("0x%08x : %d %d , left to read : %d\n",address,sizeTransfered,block_count++,len_buff );
            cnt = 0;
        }
        /*
        printf("Reading at address : 0x%08x\n",address);
        for (i = 0; i < 32; i++)
            printf("0x%02x ",rdbuff[i]);
        printf("\n");
        */
        address += BLOCK_SIZE;
        read_buffer += BLOCK_SIZE;
        len_buff -= BLOCK_SIZE;
    }


    printf("Read done\n");
    return 0;//okay
}

unsigned char flash_BulkErase(void)
{
unsigned char loc_tx_buff[6];
unsigned int rd_wr_len  = 0;

    /* write enable */
    flash_write_enable();

    /* ERASE */
    loc_tx_buff[0] = CMD_BULK_ERASE;
    rd_wr_len      = 1;
    if (write_byte(loc_tx_buff, rd_wr_len))
         return 1;

    flash_write_disable();
    return 0;
}

unsigned char flash_SectorErase(int sector)
{
unsigned char loc_tx_buff[6];
unsigned int addr;
unsigned int    sizeTransfered;

    if  (sector == 0)
    {
        addr = 0;

        loc_tx_buff[0] = CMD_4P4ERASE_SECTOR;
        loc_tx_buff[1] = (unsigned char)(addr >> 24);
        loc_tx_buff[2] = (unsigned char)(addr >> 16);
        loc_tx_buff[3] = (unsigned char)(addr >> 8 );
        loc_tx_buff[4] = (unsigned char)(addr);
        flash_write_enable();
        if (SPI_Write(ftHandle, loc_tx_buff, 5, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
            exit (1);
        while( flash_busy_and_error_check() )
            usleep(100);
        flash_SingleRegRead(CMD_GET_STATUS);
        flash_write_disable();
    }
    addr = sector * SECTOR_SIZE;

    loc_tx_buff[0] = CMD_4ERASE_SECTOR;
    loc_tx_buff[1] = (unsigned char)(addr >> 24);
    loc_tx_buff[2] = (unsigned char)(addr >> 16);
    loc_tx_buff[3] = (unsigned char)(addr >> 8 );
    loc_tx_buff[4] = (unsigned char)(addr);
    flash_write_enable();
    if (SPI_Write(ftHandle, loc_tx_buff, 5, (uint32 *)&sizeTransfered,SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE) )
        exit (1);
    while( flash_busy_and_error_check() )
        usleep(100);
    flash_SingleRegRead(CMD_GET_STATUS);
    flash_write_disable();
    return 0;
}
