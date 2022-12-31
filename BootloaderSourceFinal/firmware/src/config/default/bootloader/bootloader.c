/*******************************************************************************
  UART Bootloader Source File

  File Name:
    bootloader.c

  Summary:
    This file contains source code necessary to execute UART bootloader.

  Description:
    This file contains source code necessary to execute UART bootloader.
    It implements bootloader protocol which uses UART peripheral to download
    application firmware into internal flash from HOST-PC.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END
 #include "definitions.h"
#include "bootloader.h"
#include <device.h>
#include <string.h>
  
#define ERASE_BLOCK_SIZE        (4096UL) 
#define APP_START_ADDRESS      0x9D000000 + 0x1000 + 0x970  // (0x9D006000 + 0x1000) 
#define OFFSET_ALIGN_MASK       (~ERASE_BLOCK_SIZE + 1) 

#define PROGRAM_FLASH_END_ADRESS  0x9D07FFFF // (0x9D000000+BMXPFMSZ-1)
#define APP_FLASH_BASE_ADDRESS 	0x9D000000 
#define APP_FLASH_END_ADDRESS   PROGRAM_FLASH_END_ADRESS

 
#define DEV_CONFIG_REG_BASE_ADDRESS 0x9FC02FF0
#define DEV_CONFIG_REG_END_ADDRESS   0x9FC02FFF

/* Compare Value to achieve a 100Ms Delay */
#define TIMER_COMPARE_VALUE     (CORE_TIMER_FREQUENCY / 10)
  
static uint32_t CACHE_ALIGN input_buffer[800];//[WORDS(OFFSET_SIZE + DATA_SIZE)];

static uint8_t CACHE_ALIGN flash_data[800];//[WORDS(DATA_SIZE)];

static uint32_t flash_addr          = 0;
static uint8_t  input_command       = 0;

uint8_t rcv_start=0,packet_ready=0, rcv_reset=0 ; 
int esc=0; 
uint8_t *byte_buf = (uint8_t *)&input_buffer[0] , prg_successful=0 , prg_start=0;
uint32_t prg_len=0;
uint32_t buff_index=0;

uint8_t ExtLinAddressMB=0,ExtLinAddressUB=0,ExtLinAddressHB=0,ExtLinAddressLB=0;
uint8_t ExtSegAddressMB=0,ExtSegAddressUB=0,ExtSegAddressHB=0,ExtSegAddressLB=0;

/**
 * Static table used for the table_driven implementation.
 *****************************************************************************/
static const uint16_t crc_table[16] = 
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
};
 
uint16_t CalculateCrc( uint32_t len)
{
    unsigned int i,cntr=0;
    uint16_t crc = 0;
    
    while(cntr<len)
    {
        i = (crc >> 12) ^ ((byte_buf[cntr])>> 4);
	    crc = crc_table[i & 0x0F] ^ (crc << 4);
	    i = (crc >> 12) ^ ((byte_buf[cntr]) >> 0);
	    crc = crc_table[i & 0x0F] ^ (crc << 4);
        cntr++;
	} 
    return (crc & 0xFFFF);
} 

 void WriteHexRecord2Flash( uint32_t totalHexRecLen)
{
     
	uint8_t Checksum = 0 , cntr=0;
//	uint32_t i;
	uint32_t WrData;
	uint32_t ProgAddress; 
	uint32_t nextRecStartPt = 0;
	uint8_t RecDataLen=0,RecType=0;
    uint8_t AddressMB=0,AddressUB=0,AddressHB=0,AddressLB=0;
    uint32_t AddressVal=0,ExtSegAddressVal=0,ExtLinAddressVal=0;
    prg_start=1;
	while(totalHexRecLen>=5) // A hex record must be atleast 5 bytes. (1 Data Len byte + 1 rec type byte+ 2 address bytes + 1 crc)
	{ 
          
  //  LED2_Set();
		RecDataLen = flash_data[0+nextRecStartPt];
		RecType = flash_data[3+nextRecStartPt];	
		// Hex Record checksum check.
		Checksum = 0; 
		Checksum=0;
	    if(Checksum != 0)
	    {
		    //Error. Hex record Checksum mismatch.
		} 
		else
		{
			// Hex record checksum OK.
			switch(RecType)
			{
				case 00:  //Record Type 00, data record.
					AddressMB = 0;
					AddressUB = 0;
					AddressHB = flash_data[1+nextRecStartPt];
					AddressLB = flash_data[2+nextRecStartPt];
					
					// Derive the address.
					ExtLinAddressVal = (ExtLinAddressMB<<24) + (ExtLinAddressUB<<16) + (ExtLinAddressHB<<8) + ExtLinAddressLB;
					ExtSegAddressVal = (ExtSegAddressMB<<24) + (ExtSegAddressUB<<16) + (ExtSegAddressHB<<8) + ExtSegAddressLB;
					AddressVal = (AddressMB<<24) + (AddressUB<<16) + (AddressHB<<8) + AddressLB + ExtLinAddressVal + ExtSegAddressVal; 
				    cntr=0;	 
					while(RecDataLen) // Loop till all bytes are done.
					{ 
                        
   // LED2_Clear();
						// Convert the Physical address to Virtual address.  
						ProgAddress = (uint32_t)PA_TO_KVA0(AddressVal);  
//						// Make sure we are not writing boot area and device configuration bits.
						if(((ProgAddress >= APP_FLASH_BASE_ADDRESS) && (ProgAddress <= APP_FLASH_END_ADDRESS))
						   && ((ProgAddress <DEV_CONFIG_REG_BASE_ADDRESS) || (ProgAddress > DEV_CONFIG_REG_END_ADDRESS)))
						{
							if(RecDataLen < 4)
							{ 
								// Sometimes record data length will not be in multiples of 4. Appending 0xFF will make sure that..
								// we don't write junk data in such cases.
								WrData = 0xFFFFFFFF;
                                WrData=(flash_data[7+nextRecStartPt+cntr]<<24)+(flash_data[6+nextRecStartPt+cntr]<<16)+(flash_data[5+nextRecStartPt+cntr]<<8)+flash_data[4+nextRecStartPt+cntr];
						
							}
							else
							{	
                                 WrData=(flash_data[7+nextRecStartPt+cntr]<<24)+(flash_data[6+nextRecStartPt+cntr]<<16)+(flash_data[5+nextRecStartPt+cntr]<<8)+flash_data[4+nextRecStartPt+cntr];
							
							}		
							// Write the data into flash.	
							NVM_WordWrite(WrData,(uint32_t)ProgAddress);	
							// Assert on error. This must be caught during debug phase.	
						}	 
						// Increment the address.
						AddressVal += 4;
						// Increment the data pointer.
                        cntr += 4;
						// Decrement data len.
						if(RecDataLen > 3)
						{
							RecDataLen -= 4;
						}	
						else
						{
							RecDataLen = 0;
						}	
					}
					break;
				
				case 0x02:  // Record Type 02, defines 4th to 19th bits of the data address.
                    
				    ExtSegAddressMB = 0;
					ExtSegAddressUB = flash_data[4+nextRecStartPt];
					ExtSegAddressHB = flash_data[5+nextRecStartPt];
					ExtSegAddressLB = 0;
					// Reset linear address.
					ExtLinAddressVal = 0;
					break;
					
				case 0x04:   // Record Type 04, defines 16th to 31st bits of the data address. 
                    
					ExtLinAddressMB = flash_data[4+nextRecStartPt];
					ExtLinAddressUB = flash_data[5+nextRecStartPt];
					ExtLinAddressHB = 0;
					ExtLinAddressLB = 0;
					// Reset segment address.
					ExtSegAddressVal = 0;
					break;
					
				case 01:  //Record Type 01, defines the end of file record.
				default: 
                    prg_successful=1;
					ExtSegAddressVal = 0;
					ExtLinAddressVal = 0;
					break;
			}		
		}	
        
		//Determine next record starting point.
        if(totalHexRecLen>=(flash_data[0+nextRecStartPt] + 5))
		    totalHexRecLen = totalHexRecLen - (flash_data[0+nextRecStartPt] + 5);
        else totalHexRecLen=0;
		nextRecStartPt = nextRecStartPt+ flash_data[0+nextRecStartPt] + 5;	
		// Decrement total hex record length by length of current record.
	}//while(1)	
    
   // LED2_Clear();
		
}	
 
/* Function to process the received command */
 
static void erase(void)
{
 
    int i=0;
    
    
    uint32_t ProgAddress=(uint32_t)(0x9D000000); 							
        for( i = 0; i < ((APP_FLASH_END_ADDRESS - APP_FLASH_BASE_ADDRESS + 1)/ERASE_BLOCK_SIZE); i++ )
        {
				
			NVM_PageErase( ProgAddress + (i*ERASE_BLOCK_SIZE) );	
         while(NVM_IsBusy() == true);
		}		
    
    

}

/* Function to receive application firmware via UART/USART */
static void input_task(void)
{ 
    uint16_t crc=0;
    uint8_t  input_data= 0; 
    if (UART1_ReceiverIsReady() == false)
    {
        return;
    } 
    input_data = UART1_ReadByte(); 
    if(rcv_start==1)
    { 
        if(input_data==0x10 || input_data==0x04 || input_data==0x01)
        {   
            if(esc==1)
            { 
                byte_buf[buff_index]=input_data;
                buff_index++;  
                prg_len++;
                esc=0;
            }
            else if(input_data==0x04){
                rcv_reset = 0; 
                  //  crc_L=byte_buf[buff_index-2];
                 //   crc_H=byte_buf[buff_index-1];
                packet_ready=1;  
            }
            else if(input_data==0x10)
                esc=1;
        }
        else
        { 
            esc=0; 
            byte_buf[buff_index]=input_data;
            buff_index++; 
            prg_len++;
        } 
    }     
    if(input_data==0x01 && rcv_reset==0)
    {
        rcv_reset = 1;
        esc=0; 
        buff_index=0; 
        prg_len=0;
        rcv_start=1;
        packet_ready=0;
    }
    if(packet_ready==1)
    {   
        packet_ready=0;
        rcv_start=0;
        esc=0;
        input_command   = (uint8_t)byte_buf[0]; 
        switch (input_command){
            case 0x01:
            { 
                byte_buf[0]=0x01; 
                byte_buf[1]=0x20; 
                byte_buf[2]=0x20; 
                buff_index=3;
                crc = CalculateCrc( (uint32_t)buff_index); 

                UART1_WriteByte(0x01);
                UART1_WriteByte(0x10); 
                UART1_WriteByte(byte_buf[0]); 
                UART1_WriteByte(byte_buf[1]); 
                UART1_WriteByte(byte_buf[2]); 
                if((crc&0x00FF)==0x10)
                    UART1_WriteByte(0x10);
                UART1_WriteByte(crc&0x00FF);
                if(((crc&0xFF00)>>8)==0x10)
                    UART1_WriteByte(0x10);
                UART1_WriteByte(((crc&0xFF00)>>8));  
                UART1_WriteByte(0x04); 
                break;
            }
            case 0x02:
            { 
                flash_addr = (input_buffer[0] & OFFSET_ALIGN_MASK);
                erase();

                byte_buf[0]=0x02;  
                buff_index=1;
                crc = CalculateCrc((uint32_t)buff_index); 

                UART1_WriteByte(0x01);
                UART1_WriteByte(byte_buf[0]);
                if((crc&0x00FF)==0x10)
                    UART1_WriteByte(0x10);
                UART1_WriteByte(crc&0x00FF);
                if(((crc&0xFF00)>>8)==0x10)
                    UART1_WriteByte(0x10);
                UART1_WriteByte(((crc&0xFF00)>>8)); 
                UART1_WriteByte(0x04);  
                break;
            }
            case 0x03:
            {
                for(int j=0;j<(prg_len-3);j++)flash_data[j]=byte_buf[j+1];
                byte_buf[0]=0x03;  
                buff_index=1;
                crc = CalculateCrc( (uint32_t)buff_index); 
                WriteHexRecord2Flash(prg_len-2);

                UART1_WriteByte(0x01);
                UART1_WriteByte(byte_buf[0]);
                if((crc&0x00FF)==0x10)
                    UART1_WriteByte(0x10);
                UART1_WriteByte(crc&0x00FF);
                if(((crc&0xFF00)>>8)==0x10)
                    UART1_WriteByte(0x10);
                UART1_WriteByte(((crc&0xFF00)>>8)); 
                UART1_WriteByte(0x04);  
                break;
            }
            case 0x08:
            {  
                UART1_WriteByte(byte_buf[0]);
                break;
            }
        }       
        buff_index=0; 
    }

   // CORETIMER_Start();
}
 
int run_Application(void)
{
    
    DWORD *AppPtr;
    void (*fptr)(void);
    __builtin_disable_interrupts();
    AppPtr = (DWORD *)APP_START_ADDRESS;
    if(*AppPtr == 0xFFFFFFFF)
	{
//        Buzzer_Set();
//        CORETIMER_DelayMs(500);
//        Buzzer_Clear();
//        CORETIMER_DelayMs(500);
    
    __builtin_enable_interrupts();
		return false;
	}
	else
	{
        Buzzer_Set();
        CORETIMER_DelayMs(50);
        Buzzer_Clear();
        CORETIMER_DelayMs(500);
        fptr = (void (*)(void))APP_START_ADDRESS;
        fptr();
		return true;
	}
    
    
    
//    uint32_t msp = *(uint32_t *)(APP_START_ADDRESS);
//    __builtin_disable_interrupts();
//    void (*fptr)(void);
//    
//        /* Set default to APP_RESET_ADDRESS */
//        fptr = (void (*)(void))APP_START_ADDRESS; 
//        if (msp == 0xffffffff)
//        { 
//        //    UART1_WriteByte(0xFF);
//            return false;
//        }
//
//        fptr();
//        
//    __builtin_enable_interrupts();
//        return true; 
}

void bootloader_Tasks(void)
{
   // CORETIMER_CompareSet(TIMER_COMPARE_VALUE);
   // UART1_WriteByte(0x01);
    input_task();  
    if(prg_successful==1)
    {
        UART1_Write("finished",8);
        LED2_Clear();
        LED2_Set();
        CORETIMER_DelayMs(500);
        LED2_Clear(); 
        
        CORETIMER_DelayMs(1000);
        prg_successful=0;  
        run_Application();
//        UART1_Write("finished",8);        
//        SYSKEY = 0x00000000;
//        SYSKEY = 0xAA996655;
//        SYSKEY = 0x556699AA;
//        RSWRSTSET = _RSWRST_SWRST_MASK;
//       (void)RSWRST;
    }  
}
