/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V2.0
* | Date        :   2019-07-08
* | Info        :   Basic version
*
******************************************************************************/
#define _GNU_SOURCE
#include "DEV_Config.h"
#include <unistd.h>
#include <fcntl.h>

// uint32_t fd;
int INT_PIN;
/******************************************************************************
function:	Equipment Testing
parameter:
Info:   Only supports Jetson Nano and Raspberry Pi
******************************************************************************/
static int DEV_Equipment_Testing(void)
{
	int i;
	int fd;
	char value_str[20];
	fd = open("/etc/issue", O_RDONLY);
    printf("Current environment: ");
	while(1) {
		if (fd < 0) {
			return -1;
		}
		for(i=0;; i++) {
			if (read(fd, &value_str[i], 1) < 0) {
				return -1;
			}
			if(value_str[i] ==32) {
				printf("\r\n");
				break;
			}
			printf("%c",value_str[i]);
		}
		break;
	}

	if(i<5) {
		printf("Unrecognizable\r\n");
        return -1;
	} else {
		char RPI_System[10]   = {"Raspbian"};
		for(i=0; i<6; i++) {
			if(RPI_System[i] != value_str[i]) {
                    return 'J';
			}
		}
        return 'R';
	}
	return -1;
}


/******************************************************************************
function:	GPIO Function initialization and transfer
parameter:
Info:
******************************************************************************/
void DEV_GPIO_Mode(UWORD Pin, UWORD Mode)
{
    /*
        0:  INPT   
        1:  OUTP
    */
    SYSFS_GPIO_Export(Pin);
    if(Mode == 0 || Mode == SYSFS_GPIO_IN){
        SYSFS_GPIO_Direction(Pin, SYSFS_GPIO_IN);
    }else{
        SYSFS_GPIO_Direction(Pin, SYSFS_GPIO_OUT);
    }
// // #endif   
}

void DEV_Digital_Write(UWORD Pin, UBYTE Value)
{
    SYSFS_GPIO_Write(Pin, Value);
}

UBYTE DEV_Digital_Read(UWORD Pin)
{
    UBYTE Read_value = 0;
    Read_value = SYSFS_GPIO_Read(Pin);
    return Read_value;
}


/**
 * delay x ms
**/
void DEV_Delay_ms(UDOUBLE xms)
{
    UDOUBLE i;
    for(i=0; i < xms; i++){
        usleep(1000);
    }
}


void GPIO_Config(void)
{
    int Equipment = DEV_Equipment_Testing();
    if(Equipment=='R'){
        INT_PIN = 4;
    }else if(Equipment=='J'){
        INT_PIN = GPIO4;
    }else{
        printf("Device read failed or unrecognized!!!\r\n");
        while(1);
    }
    
    DEV_GPIO_Mode(INT_PIN, 0);
}

/******************************************************************************
function:	SPI Function initialization and transfer
parameter:
Info:
******************************************************************************/
void DEV_SPI_Init()
{
#if DEV_SPI
        printf("DEV SPI Device\r\n"); 
        DEV_HARDWARE_SPI_begin("/dev/spidev0.0");
#endif
}

void DEV_SPI_WriteByte(uint8_t Value)
{
#if DEV_SPI
        DEV_HARDWARE_SPI_TransferByte(Value);
#endif
}

void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len)
{
#if DEV_SPI
        DEV_HARDWARE_SPI_Transfer(pData, Len);
#endif
}
/******************************************************************************
function:	I2C Function initialization and transfer
parameter:
Info:
******************************************************************************/
void DEV_I2C_Init(uint8_t Add)
{
#if DEV_I2C
    printf("DEV I2C Device\r\n"); 
    DEV_HARDWARE_I2C_begin("/dev/i2c-10");//Mount I2C to I2C-10
    DEV_HARDWARE_I2C_setSlaveAddress(Add);
#endif
}

void I2C_Write_Byte(uint8_t Cmd, uint8_t value)
{
	// int ref;
#if DEV_I2C
    char wbuf[2]={Cmd, value};
    DEV_HARDWARE_I2C_write(wbuf, 2);
#endif
}

int I2C_Read_Byte(uint8_t Cmd)
{
	int ref;
#if DEV_I2C
        char rbuf[2]={0};
        DEV_HARDWARE_I2C_read(Cmd, rbuf, 1);
        ref = rbuf[0];
#endif
    return ref;
}

int I2C_Read_Word(uint8_t Cmd)
{
	int ref;
#if DEV_I2C
        char rbuf[2] = {0};
        DEV_HARDWARE_I2C_read(Cmd, rbuf, 2);
        ref = rbuf[1]<<8 | rbuf[0];
#endif
    return ref;
}

/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
UBYTE DEV_ModuleInit(void)
{
    printf("USE_DEV_LIB \r\n");
    GPIO_Config();
    DEV_I2C_Init(0x2F);//I2C adress 
    
    return 0;
}

/******************************************************************************
function:	Module exits, closes SPI and BCM2835 library
parameter:
Info:
******************************************************************************/
void DEV_ModuleExit(void)
{
    #if DEV_I2C
        DEV_HARDWARE_I2C_end();
    #endif
    #if DEV_SPI
        DEV_HARDWARE_SPI_end();
    #endif
}

