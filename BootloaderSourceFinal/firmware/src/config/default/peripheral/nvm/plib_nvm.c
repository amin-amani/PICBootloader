/*******************************************************************************
  Non-Volatile Memory Controller(NVM) PLIB.

  Company:
    Microchip Technology Inc.

  File Name:
    plib_nvm.c

  Summary:
    Interface definition of NVM Plib.

  Description:
    This file defines the interface for the NVM Plib.
    It allows user to Program, Erase and lock the on-chip Non Volatile Flash
    Memory.
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

#include <string.h>
#include "sys/kmem.h"
#include "plib_nvm.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */
// *****************************************************************************

/*******************************************
 * Internal operation type
 ******************************************/
typedef enum
{
    PROGRAM_ERASE_OPERATION         = 0x05,
    PAGE_ERASE_OPERATION            = 0x04,
    ROW_PROGRAM_OPERATION           = 0x03,
    WORD_PROGRAM_OPERATION          = 0x01,
    NO_OPERATION                    = 0x00,
} NVM_OPERATION_MODE;

/*******************************************
 * Internal Flash Programming Unlock Keys
 ******************************************/
typedef enum
{
    NVM_UNLOCK_KEY1 = 0xAA996655,
    NVM_UNLOCK_KEY2 = 0x556699AA
} NVM_UNLOCK_KEYS;

#define NVM_INTERRUPT_ENABLE_MASK   0x1000000
#define NVM_INTERRUPT_FLAG_MASK     0x1000000

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

// *****************************************************************************
// *****************************************************************************
// Section: NVM Implementation
// *****************************************************************************
// *****************************************************************************

NVM_CALLBACK nvmCallbackFunc;

uintptr_t nvmContext;

void NVM_CallbackRegister( NVM_CALLBACK callback, uintptr_t context )
{
    /* Register callback function */
    nvmCallbackFunc    = callback;
    nvmContext         = context;
}

void NVM_InterruptHandler( void )
{
    IFS1CLR = NVM_INTERRUPT_FLAG_MASK;

    if(nvmCallbackFunc != NULL)
    {
        nvmCallbackFunc(nvmContext);
    }
}

void UART5_WriteByte( int data );
void CORETIMER_DelayMs (uint32_t delay_ms);
void CORETIMER_DelayUs (uint32_t delay_us);



static void NVM_StartOperationAtAddress( uint32_t address,  NVM_OPERATION_MODE operation )
{ 

    // Disable DMA & Disable Interrupts
	#ifdef _DMAC
	 __builtin_disable_interrupts();
//	susp = DmaSuspend();
	#else
	 __builtin_disable_interrupts(); 
	#endif	// _DMAC 

     NVMADDR = KVA_TO_PA(address);

    // NVMOP can be written only when WREN is zero. So, clear WREN.
    NVMCONCLR = _NVMCON_WREN_MASK;

    NVMCONCLR = _NVMCON_NVMOP_MASK;
    NVMCONSET = ( _NVMCON_NVMOP_MASK & (((uint32_t)operation) << _NVMCON_NVMOP_POSITION) );

    // Set WREN to enable writes to the WR bit and to prevent NVMOP modification
    NVMCONSET = _NVMCON_WREN_MASK;
    
    CORETIMER_DelayUs(7);
    // Write the unlock key sequence
    NVMKEY = 0x0;
    NVMKEY = NVM_UNLOCK_KEY1;
    NVMKEY = NVM_UNLOCK_KEY2;

    // Start the operation
    NVMCONSET = _NVMCON_WR_MASK;
    
   // __builtin_mtc0(12, 0, 0x00100000u);//processorStatus); 
    
   // IEC1SET   = NVM_INTERRUPT_ENABLE_MASK;
    
    /*
    // Enable Flash Write/Erase Operations
    NVMCON = NVMCON_WREN | operation;
    // Data sheet prescribes 6us delay for LVD to become stable.
    // To be on the safer side, we shall set 7us delay.
    CORETIMER_DelayUs(7);

    NVMKEY 		= 0xAA996655;
    NVMKEY 		= 0x556699AA;
    NVMCONSET 	= NVMCON_WR;

    // Wait for WR bit to clear
    while(NVMCON & NVMCON_WR);
    
    // Disable Flash Write/Erase operations
    NVMCONCLR = NVMCON_WREN;  
*/
	// Enable DMA & Enable Interrupts
	#ifdef _DMAC
//	DmaResume(susp);
	__builtin_enable_interrupts();
	#else
	__builtin_enable_interrupts();
	#endif // _DMAC

    
   // volatile uint32_t processorStatus;

    //processorStatus = __builtin_disable_interrupts();

    // Set the target Flash address to be operated on (destination).
  /*  NVMADDR = KVA_TO_PA(address);

    // NVMOP can be written only when WREN is zero. So, clear WREN.
    NVMCONCLR = _NVMCON_WREN_MASK;

    NVMCONCLR = _NVMCON_NVMOP_MASK;
    NVMCONSET = ( _NVMCON_NVMOP_MASK & (((uint32_t)operation) << _NVMCON_NVMOP_POSITION) );

    // Set WREN to enable writes to the WR bit and to prevent NVMOP modification
    NVMCONSET = _NVMCON_WREN_MASK;

    // Write the unlock key sequence
    NVMKEY = 0x0;
    NVMKEY = NVM_UNLOCK_KEY1;
    NVMKEY = NVM_UNLOCK_KEY2;

    // Start the operation
    NVMCONSET = _NVMCON_WR_MASK;

    __builtin_mtc0(12, 0, 0x00100000u);//processorStatus); 
    
    IEC1SET   = NVM_INTERRUPT_ENABLE_MASK;
   */
}

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

void NVM_Initialize( void )
{
    NVM_StartOperationAtAddress( NVMADDR,  NO_OPERATION );
}

bool NVM_Read( uint32_t *data, uint32_t length, const uint32_t address )
{
    memcpy((void *)data, (void *)KVA0_TO_KVA1(address), length);

    return true;
}

bool NVM_WordWrite( uint32_t data, uint32_t address )
{
    NVMDATA = (uint32_t )data;

    NVM_StartOperationAtAddress( address,  WORD_PROGRAM_OPERATION);

    return true;
}

bool NVM_RowWrite( uint32_t *data, uint32_t address )
{
   NVMSRCADDR = (uint32_t )KVA_TO_PA(data);

   NVM_StartOperationAtAddress( address,  ROW_PROGRAM_OPERATION);

   return true;
}

bool NVM_PageErase( uint32_t address )
{
   NVM_StartOperationAtAddress(address,  PAGE_ERASE_OPERATION);

   return true;
}

NVM_ERROR NVM_ErrorGet( void )
{
    // mask for WRERR and LVDERR bits
    return (NVMCON & (_NVMCON_LVDERR_MASK | _NVMCON_WRERR_MASK));
}

bool NVM_IsBusy( void )
{
    return (bool)NVMCONbits.WR;
}
