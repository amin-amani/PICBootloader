/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for bootloader project.

  Description:
    This file contains the "main" function for bootloader project.  The
    "main" function calls the "SYS_Initialize" function to initialize
    all modules in the system.
    It calls "bootloader_start" once system is initialized.
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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
 
// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

void wait_for_PC()
{
    long int i=0;
    char packet;
    char input_data[7]="0",input_data2[7] ="0";
    int packet_start=0, packet_start2=0;
    int command_cntr=0,command_cntr2=0;
    //    UART1_Write("Waiting\n",8);
    for(i=0;i<12000000;i++)
    { 
        CORETIMER_DelayUs(10); 
        if (UART1_ReceiverIsReady() == true)
        {
            packet=UART1_ReadByte(); 
            if(packet=='b')
            {   
                packet_start=1;
                input_data[0] = packet;
                command_cntr=0;
            }
            else if(packet_start==1 && command_cntr<6)
            { 
                command_cntr++;  
                input_data[command_cntr] =packet;  
                if(command_cntr==6)
                {
                    command_cntr=0;
                    packet_start=0;
                }
                
            }
            if(input_data[1]=='t' && input_data[2]=='l' && input_data[3]=='m' && input_data[4]=='a' && input_data[5]=='i' && input_data[6]=='n' )
            {
                UART1_Write("btlmain",7);
                while(1) 
                    bootloader_Tasks();
            }  
            
            
            if(packet=='r')
            {   
                packet_start2=1;
                input_data2[0] = packet;
                command_cntr2=0;
            }
            else if(packet_start2==1 && command_cntr2<6)
            { 
                command_cntr2++;  
                input_data2[command_cntr2] =packet;  
                if(command_cntr2==6)
                {
                    command_cntr2=0;
                    packet_start2=0;
                }
                
            }
            if(input_data2[1]=='u' && input_data2[2]=='n' && input_data2[3]=='m' && input_data2[4]=='a' && input_data2[5]=='i' && input_data2[6]=='n' )
            {
                UART1_Write("runmain",7);
                CORETIMER_DelayMs(1000); 
                while(1) 
                    run_Application();
            }
        }  
    }
    UART1_Write("StopWaiting\n",12);
}

int main ( void )
{ 
 
    SYS_Initialize ( NULL ); 
    LED2_OutputEnable();
    Buzzer_OutputEnable();
    Buzzer_Clear();
    UART1_WriteByte('h');  
    UART1_WriteByte('i');   
    LED2_Toggle();
    CORETIMER_DelayMs(500); 
    LED2_Toggle(); 
    wait_for_PC();
    while(1)
    {
        run_Application();   
        bootloader_Tasks();    
    }
    /* Execution should not come here during normal operation */
    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

