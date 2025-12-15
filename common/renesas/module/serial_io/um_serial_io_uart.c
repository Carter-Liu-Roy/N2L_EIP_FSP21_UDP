/***********************************************************************************************************************
 * Copyright [2020-2021] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics Corporation and/or its affiliates and may only
 * be used with products of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.
 * Renesas products are sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for
 * the selection and use of Renesas products and Renesas assumes no liability.  No license, express or implied, to any
 * intellectual property right is granted by Renesas.  This software is protected under all applicable laws, including
 * copyright laws. Renesas reserves the right to change or discontinue this software and/or this documentation.
 * THE SOFTWARE AND DOCUMENTATION IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND
 * TO THE FULLEST EXTENT PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY,
 * INCLUDING WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE
 * SOFTWARE OR DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR
 * DOCUMENTATION (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER,
 * INCLUDING, WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY
 * LOST PROFITS, OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "um_serial_io_cfg.h"
#include "um_serial_io_api.h"
#include "um_common_api.h"

/** For FSP error codes */
#include "fsp_common_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/** For FSP Ethernet module. */
#include "r_uart_api.h"
#include "r_sci_uart.h"
#include "r_sci_uart_cfg.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define SCI_UART_BAUDRATE    (115200)

/***********************************************************************************************************************
 * Private constants
 **********************************************************************************************************************/
/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Imported function prototypes
 **********************************************************************************************************************/
/** r_uart_api callback function set by r_uart_cfg. */
void user_uart_callback (uart_callback_args_t * p_args);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
static SemaphoreHandle_t  mutex_handle, tx_semaphore_handle;
static uart_instance_t const *p_ginst;

usr_err_t um_serial_io_uart_open(uart_instance_t const *p_inst);

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Initialize the controller.
 *
 * @param[in] p_ctrl				Pointer to the controller
 * @param[in] p_uart_instance		Pointer to the instance of uart interface
 *
 * @retval USR_SUCCESS					Process has been done successfully.
 * @retval USR_ERR_NOT_INITIALIZED		Initialization has been failed.
 **********************************************************************************************************************/
usr_err_t um_serial_io_uart_open(uart_instance_t const *p_inst)
{
	/** Error codes */
	fsp_err_t fsp_err;
#if 0 // Deleted with fsp2.2
	/** For setting baud rate */
	sci_uart_baud_calculation_t baud_target;
	sci_baud_setting_t baud_setting;
    /** Resolve control instance type. */
#endif

	/** Create a mutex for r_uart */
	mutex_handle = xSemaphoreCreateMutex();
	USR_ERROR_RETURN( mutex_handle, USR_ERR_NOT_INITIALIZED );

	/** Create a semaphore for transmission control. */
	tx_semaphore_handle = xSemaphoreCreateBinary();
	USR_ERROR_RETURN( tx_semaphore_handle, USR_ERR_NOT_INITIALIZED );
	(void) xSemaphoreGive( tx_semaphore_handle );

	/** Open the target interface. */
	fsp_err = p_inst->p_api->open( p_inst->p_ctrl, p_inst->p_cfg );
	USR_ERROR_RETURN( FSP_SUCCESS == fsp_err || FSP_ERR_ALREADY_OPEN == fsp_err , USR_ERR_NOT_INITIALIZED );
#if 0 // Deleted with fsp2.2
	/** Calculate baud rate using SCI UART specific function. */
	baud_target.baudrate = SCI_UART_BAUDRATE;
	baud_target.baud_rate_error_x_1000 = 5000;
	fsp_err = R_SCI_UART_BaudCalculate( &baud_target, SCI_UART_CLOCK_SOURCE_PCLKM, &baud_setting );
	USR_ERROR_RETURN( FSP_SUCCESS == fsp_err, USR_ERR_NOT_INITIALIZED );

	/** Set baud rate */
	fsp_err = p_inst->p_api->baudSet( p_inst->p_ctrl, (void *) &baud_setting);
	USR_ERROR_RETURN( FSP_SUCCESS == fsp_err, USR_ERR_NOT_INITIALIZED );
#endif
    /** Set callback */
#if defined(BSP_MCU_GROUP_RA6M3)
    fsp_err = p_ctrl->p_uart_instance->p_api->callbackSet( p_ctrl->p_uart_instance->p_ctrl,
                                                           user_uart_callback,
                                                           p_ctrl,
                                                           &p_ctrl->callback_memory );
    USR_ERROR_RETURN( FSP_SUCCESS == fsp_err, USR_ERR_NOT_INITIALIZED );
#endif // defined(BSP_MCU_GROUP_RA6M3)
    /** RZT2M FSP v1.0.0 does NOT support callbackSet API. */
    p_ginst = p_inst;

    /** Return success code */
	return USR_SUCCESS;
}
#if 0
/*******************************************************************************************************************//**
 * @brief Close the controller.
 *
 * @param[in] p_ctrl				Pointer to the controller
 *
 * @retval USR_SUCCESS					Process has been done successfully.
 **********************************************************************************************************************/
usr_err_t um_serial_io_uart_close(uart_ctrl_t * const p_ctrl )

{
	/** Unused parameter */
	(void) p_ctrl;

	/**
	 * TODO: Implement the sequence to close.
	 */

    /** Return success code */
	return USR_SUCCESS;
}
#endif
/*******************************************************************************************************************//**
 * @brief Output character via UART interface
 *
 * @param[in] p_ctrl		Pointer to the controller
 *
 * @retval USR_SUCCESS		Process has been done successfully.
 **********************************************************************************************************************/
usr_err_t um_serial_io_uart_write(serial_io_data_t * const p_data_buffer );
usr_err_t um_serial_io_uart_write(serial_io_data_t * const p_data_buffer )
{
#if 0  // Deleted with fsp2.2
	/** Error codes */
	fsp_err_t fsp_err;

	uint8_t offset;
	uint8_t tx_start;
	uint8_t chara;
	uint8_t cr = '\r';
#endif
	/** Enter lock section. */
	USR_LOCK_SECTION_START( mutex_handle );
	{
#if 1
        p_ginst->p_api->write( p_ginst->p_ctrl, p_data_buffer->p_buffer, p_data_buffer->length);
        (void) xSemaphoreTake( tx_semaphore_handle, portMAX_DELAY );

#else //Deleted with fsp2.2
		for( offset = tx_start = 0; offset < p_data_buffer->length; offset++ )
		{
			chara = p_data_buffer->p_buffer[offset];

			if( 0 != offset )
			{
				if ( '\n' == p_data_buffer->p_buffer[offset] )
				{
	    			/** write the character buffer. */
				    //p_ginst->p_api->write( p_ginst->p_ctrl, &cr, 1 );

				    p_ginst->p_api->write( p_ginst->p_ctrl, &p_data_buffer->p_buffer[tx_start],
				                           offset - tx_start  + 1);
	    			/** Wait until the transmission is completed. */
	    			(void) xSemaphoreTake( tx_semaphore_handle, portMAX_DELAY );
	    			tx_start = offset + 1;
				}
			}
			else
			{
	            if ( ( '\n' == p_data_buffer->p_buffer[offset] ) && ( '\r' != (p_data_buffer->p_buffer[offset-1]) ) )
	            {
	    			/** write the character buffer. */
	    			//p_ginst->p_api->write( p_ginst->p_ctrl, &cr, 1 );
	                p_data_buffer->p_buffer[offset-1] = '\n';
	                p_data_buffer->p_buffer[offset] = 0;
                    p_ginst->p_api->write( p_ginst->p_ctrl, &p_data_buffer->p_buffer[tx_start],
                                           offset - tx_start);
	    			/** Wait until the transmission is completed. */
	    			(void) xSemaphoreTake( tx_semaphore_handle, portMAX_DELAY );
                    tx_start = offset + 1;
				}
			}

			/** write the character buffer. */
			fsp_err = p_ginst->p_api->write( p_ginst->p_ctrl, &chara, 1 );
			USR_LOCK_ERROR_RETURN( mutex_handle, FSP_SUCCESS == fsp_err, USR_ERR_ABORTED );

			/** Wait until the transmission is completed. */
			(void) xSemaphoreTake( tx_semaphore_handle, portMAX_DELAY );
		}
#endif
	}
	USR_LOCK_SECTION_END( mutex_handle );

	/** Return success code. */
	return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Define callback function registered to uart interface
 *
 * @param[in] p_args				Pointer to the callback memory
 **********************************************************************************************************************/
void user_uart_callback (uart_callback_args_t * p_args)
{

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /** Handle the UART event */
    switch (p_args->event)
    {

	/** Detect transmission completed. */
	case UART_EVENT_TX_COMPLETE:
	{
		/** Release the resource for transmission. */
		(void) xSemaphoreGiveFromISR( tx_semaphore_handle, &xHigherPriorityTaskWoken );
		break;
	}

	/** Other cases are not handled. TODO: Implement the reception sequence. */
	default:
		break;
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************************************************//**
 * @brief Override write function of standard library.
 **********************************************************************************************************************/
#if defined(UM_SERIAL_IO_CFG_OVERRIDE_PRINTF_FUNCTION) && UM_SERIAL_IO_CFG_OVERRIDE_PRINTF_FUNCTION
#include <stdarg.h>
#include <stdio.h>
int printf (const char *__restrict format, ...)
{
    /** Resolve context */
    serial_io_data_t *tx_data = NULL;
    int32_t ret_len;

    /** Allocate memory */
    USR_HEAP_ALLOCATE( tx_data, sizeof(serial_io_data_t) );

    /** Resolve formats */
    va_list args;
    va_start (args, format);
    ret_len = vsprintf( (char *) tx_data->p_buffer, format, args);

    /** Check the error */
    if( ret_len < 0 ){
        return ret_len;
    }

    /** Request the task to write buffer. */
    tx_data->length = (uint32_t) ret_len;
    (void) um_serial_io_uart_write( tx_data );
    /** Close the va_list variable. */
    va_end(args);
    vPortFree(tx_data);

    /** return the length. */
    return (int) ret_len;
}
#endif
