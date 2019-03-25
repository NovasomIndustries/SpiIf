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
/* use with
sudo rmmod ftdi_sio ; sleep 1; sudo LD_LIBRARY_PATH=libs:$LD_LIBRARY_PATH bin/Debug/SpiIf ->command>
*/
FT_HANDLE ftHandle;
unsigned char read_buf[DATA_SIZE];
ChannelConfig channelConf = {0};
unsigned char AC_BUSbit_Value = 0x0;
int vflag = 0;

ls1012_files flash_files[4] =
{
    {
/*        .file_name = "/Devel/NOVAsdk2019.01/Utils/nxp/ls1012components/rcw_N1.bin",*/
        .file_name = "/home/fil/novasis_sdk/tools/qspi_components/rcw_N1.bin",
        .address = 0x00000000,
        .max_len = (1024*1024)
    },
    {
        .file_name = "/home/fil/novasis_sdk/u-boot-2018.09/u-boot-dtb.bin",
        .address = 0x00100000,
        .max_len = (2048*1024)
    },
    /*
    {
        .file_name = "/Devel/NOVAsdk2019.01/Utils/nxp/ls1012components/ppa.itb",
        .address = 0x00400000,
        .max_len = (2048*1024)
    },
    */
    {
        .file_name = "/home/fil/novasis_sdk/tools/qspi_components/pfe_fw_sbl.itb",
        .address = 0x00a00000,
        .max_len = (3072*1024)
    },
    {
        .file_name = "unused",
        .address = 0xffa00000,
        .max_len = (2048*1024)
    },
};


extern  unsigned char s25fs512_id[8];

struct timeval t1, t2;
double elapsedTime;

void measure_time_start(void)
{
    gettimeofday(&t1, NULL);
}

void measure_time_end(void)
{
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
}

int main(int argc, char **argv)
{
unsigned int i = 0 , j , k;
int opflag=0 , file_len , block_num=-1;
char *cvalue = NULL;
int c;
FILE    *fp;
char    strc = '|' , filearg[256];;
unsigned int sector_num;
//char filename1[128] , filename2[128] , filename3[128];

    while ((c = getopt (argc, argv, "bw:r:s:WR:v?z1:2:3:o")) != -1)
    {
        switch (c)
        {
            case 'b':   opflag = OP_BULKERASE;
                        break;
            case 'w':   opflag = OP_WRITEBLOB;
                        sprintf(filearg,"%s\n", optarg);
                        break;
            case 'W':   opflag = OP_WRITERCW;
                        //cvalue = optarg;
                        break;
            case 'r':   opflag = OP_READFLASH;
                        cvalue = optarg;
                        break;
            case 'R':   opflag = OP_READRCW;
                        cvalue = optarg;
                        break;
            case 's':   opflag = OP_SECTORERASE;
                        block_num = atoi(optarg);
                        break;
            case 'v':   vflag = 1;
                        break;
            case 'z':   opflag = 5;
                        break;
            case 'o':   opflag = 6;
                        break;
            case '?':   printf ("Options ?\nwas %c\n",c);
                        exit (1);
            default:
                        exit(1);
        }
    }
    if ( opflag == 0 )
    {
        printf("Otions are : b (bulk erase )  , w ( write file ), r <file> ( read file ) , v ( verbose )\n");
        printf ("c = %d , opflag = %d , cvalue = %s\n", c , opflag, cvalue);
        exit(1);
    }

    if (( opflag == OP_READFLASH ) && (strlen(cvalue) < 2))
    {
        printf("Unspecified read file!\n");
        exit (1);
    }
    if (( opflag == OP_SECTORERASE ) && (block_num == -1))
    {
        printf("Wrong sector number!\n");
        exit (1);
    }
    if (( opflag == OP_READRCW ) && (strlen(cvalue) < 2))
    {
        printf("Unspecified read file!\n");
        exit (1);
    }
    if (( opflag == OP_WRITEBLOB ) && (strlen(filearg) > 0))
    {
        if ( strncmp(filearg,"default",sizeof("default")-1) == 0)
            printf("Using default files\n");
        else
        {
            char delim[] = " ";
            char *ptr = strtok(filearg, delim);

            i=0;
            while(ptr != NULL)
            {
                sprintf(flash_files[i].file_name,"%s",ptr);
                ptr = strtok(NULL, delim);
                i++;
            }
        }
    }

    printf("Using :\n");
    for(i=0;i<3;i++)
        printf("%s\n", flash_files[i].file_name);
    ftdi_init();


    //flash_getId();
    flash_setup();

    switch ( opflag )
    {
        case OP_SECTORERASE :
                                printf("Erasing sector %d ( address 0x%08x )\n",flash_files[i].address / SECTOR_SIZE,flash_files[i].address);
                                measure_time_start();
                                flash_SectorErase(block_num);
                                measure_time_end();
                                printf("Sector %d erased in %f mSec.\n",flash_files[i].address / SECTOR_SIZE,elapsedTime);
                                break;
        case OP_BULKERASE :
                                printf("Bulk erasing\n");
                                measure_time_start();
                                measure_time_end();
                                printf("Bulk erased in %f mSec.\n",elapsedTime);
                                flash_BulkErase();
                                while (flash_busy_and_error_check())
                                {
                                    switch (strc)
                                    {
                                        case '|' : strc = '/';break;
                                        case '/' : strc = '-';break;
                                        case '-' : strc = '\\';break;
                                        case '\\' : strc = '|';break;
                                        default : strc = '|' ; break;
                                    }
                                    printf("%c\r", strc); fflush(stdout);
                                    usleep(500000);
                                }
                                break;
        case OP_WRITEBLOB :
                                for(i=0;i<3;i++)
                                {
                                    for(j=0;j<flash_files[i].max_len;j++)
                                        read_buf[j] = 0xff;
                                    fp = fopen(flash_files[i].file_name, "rb");
                                    if ( fp )
                                    {
                                        file_len = fread(read_buf, 1 , flash_files[i].max_len, fp);
                                        fclose(fp);
                                    }
                                    else
                                    {
                                        printf("File %s not found\n",flash_files[i].file_name);
                                        exit(1);
                                    }
                                    sector_num = (file_len / 0x40000) + 1;

                                    for ( k=0; k<sector_num; k++)
                                    {
                                        printf("Erasing sector %d ( address 0x%08x )\n",flash_files[i].address / SECTOR_SIZE + k,flash_files[i].address);
                                        measure_time_start();
                                        flash_SectorErase(flash_files[i].address / SECTOR_SIZE + k);
                                        measure_time_end();
                                        printf("Sector %d erased in %f mSec.\n",flash_files[i].address / SECTOR_SIZE + k,elapsedTime);
                                    }
                                    printf("Writing file %s at 0x%08x, %d bytes long ( ETA %d sec. )\n",flash_files[i].file_name,flash_files[i].address,file_len,(file_len/256)/14);
                                    measure_time_start();
                                    if (flash_Write(read_buf , file_len , flash_files[i].address ) )
                                    {
                                        printf("flash_Write error\n");
                                        exit(1);
                                    }
                                    measure_time_end();
                                    printf("File %s written at address 0x%08x, took %f mSec.\n",flash_files[i].file_name,flash_files[i].address,elapsedTime);
                                }
                                break;
        case OP_WRITERCW :
                                for(i=0;i<1;i++)
                                {
                                    for(j=0;j<flash_files[i].max_len;j++)
                                        read_buf[j] = 0x5a;
                                    fp = fopen(flash_files[i].file_name, "rb");
                                    if ( fp )
                                    {
                                        file_len = fread(read_buf, 1 , flash_files[i].max_len, fp);
                                        fclose(fp);
                                    }
                                    else
                                    {
                                        printf("File %s not found\n",flash_files[i].file_name);
                                        exit(1);
                                    }
                                    printf("Erasing sector %d ( address 0x%08x )\n",flash_files[i].address / SECTOR_SIZE,flash_files[i].address);
                                    measure_time_start();
                                    flash_SectorErase(flash_files[i].address / SECTOR_SIZE);
                                    measure_time_end();
                                    printf("Sector %d erased in %f mSec.\n",flash_files[i].address / SECTOR_SIZE,elapsedTime);
                                    printf("Writing file %s at address 0x%08x\n",flash_files[i].file_name,flash_files[i].address);
                                    measure_time_start();
                                    if (flash_Write(read_buf , file_len , flash_files[i].address ) )
                                    {
                                        printf("flash_Write error\n");
                                        exit(1);
                                    }
                                    measure_time_end();
                                    printf("File %s written at address 0x%08x, took %f mSec.\n",flash_files[i].file_name,flash_files[i].address,elapsedTime);
                                }
                                break;

        case OP_READFLASH :
                                for(j=0;j<DATA_SIZE;j++)
                                    read_buf[j] = 0xff;
                                //if (flash_Read(0x00, (unsigned int )DATA_SIZE, read_buf))
                                if (flash_Read(0x00, 8192, read_buf))
                                {
                                    printf("\n Error reading Flash!!!\n");
                                    exit(1);
                                }
                                fp = fopen(cvalue, "wb");
                                fwrite(read_buf, sizeof(read_buf), 1, fp);
                                fclose(fp);
                                break;

        case OP_READRCW :
                                for(j=0;j<RCW_DATA_SIZE;j++)
                                    read_buf[j] = 0xff;
                                if (flash_Read(0x00, (unsigned int )RCW_DATA_SIZE, read_buf))
                                {
                                    printf("\n Error reading Flash!!!\n");
                                    exit(1);
                                }
                                fp = fopen(cvalue, "wb");
                                fwrite(read_buf, sizeof(read_buf), 1, fp);
                                fclose(fp);
                                break;

        default :               exit (1);

    }


    ftdi_close();
	printf("Closed\n");
}
