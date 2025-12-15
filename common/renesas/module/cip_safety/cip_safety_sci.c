/***********************************************************************************************************************
 * Copyright [2020-2023] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
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
 * Include headers
 ***********************************************************************************************************************/

#if defined(OPENER_CIP_SAFETY)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cipcommon.h"
#include "cip_safety_sci.h"
#include "cipsafetysupervisor.h"
#include "cipsafetyvalidator.h"

/** FreeRTOS related definitions. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/** FSP module instances. */
#include "hal_data.h"
#include "common_data.h"
#include "main_thread.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define CIP_SAFETY_SCI_NUM_IMP_MSG_QUEUE 2

#define CIP_SAFETY_SCI_TX_COMPLETE_EVENT 0x1

#define CIP_SAFETY_SCI_RESET_EVENT_ERR 0x1
#define CIP_SAFETY_SCI_RESET_EVENT_RX  0x2

/* Length of reception buffer */
#define CIP_SAFETY_SCI_RX_BUF_LEN    2048

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
 * Private global variables
 **********************************************************************************************************************/
static unsigned long cip_safety_sci_rx_w = 0;
static unsigned long cip_safety_sci_rx_r = 0;
static unsigned long cip_safety_sci_rx_rem = 0;
static unsigned char cip_safety_sci_rx_buf[CIP_SAFETY_SCI_RX_BUF_LEN] = {0};
static unsigned char cip_safety_recieve_buf[CIP_SAFETY_SCI_RX_BUF_LEN] = {0};
static unsigned long cip_safety_recieve_buf_index = 0;
/* Start Up Message Queue */
static QueueHandle_t gxCipSafetyStartUpMsgQueue = 0;
/* Explicit Message Queue */
static QueueHandle_t gxCipSafetyExpMsgQueue = 0;
/* Implicit Message Queue */
static QueueHandle_t gxCipSafetyImpMsgQueue[CIP_SAFETY_SCI_NUM_IMP_MSG_QUEUE] = {0};
static unsigned long g_connection_id[CIP_SAFETY_SCI_NUM_IMP_MSG_QUEUE] = {0};
/* Data Acquisition Queue */
static QueueHandle_t gxCipSafetyDataAcqQueue = 0;
static EventGroupHandle_t xSCIResetEvent = NULL;
static EventGroupHandle_t xSCITXComEvent = NULL;
static QueueHandle_t gxCipSafetySCIRecvQueue = 0;
static SemaphoreHandle_t gxCipSafetySCITxMutex = 0;
static TaskHandle_t gCipSafetySCIRecvTask = NULL;
static TaskHandle_t gCipSafetySCISendTask = NULL;
static TaskHandle_t gCipSafetySCIResetTask = NULL;
static unsigned char g_start_up_seq = 0;
static cip_safety_sci_st_queue tx_queue = {0};
static cip_safety_sci_st_queue acq_rx_queue = {0};
static cip_safety_sci_st_queue start_up_rx_queue = {0};
static cip_safety_sci_st_queue exp_rx_queue = {0};
static cip_safety_sci_st_queue imp_rx_queue = {0};

static const unsigned char judge_data_tcp_o_to_t[3] = {0x06, 0xAF, 0x12}; /* TCP & Port Number 44818 in O to T Data */
static const unsigned char judge_data_udp_o_to_t[3] = {0x11, 0x08, 0xAE}; /* UDP & Port Number 2222 in O to T Data */
static const unsigned char judge_data_tcp_t_to_o[3] = {0x06, 0x12, 0xAF}; /* TCP & Port Number 44818 in T to O Data */
static const unsigned char judge_data_udp_t_to_o[3] = {0x11, 0xAE, 0x08}; /* UDP & Port Number 2222 in T to O Data */
static const unsigned char start_up_req[4] = {0x01, 0x01, 0x00, 0x00}; /* Start Up Message request */

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
uint8_t gu8_read_char;

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Transmits UART frame.
 *
 * @param[in] tx_buf      Pointer to transmit buffer.
 * @param[in] tx_num      Length of transmit buffer.
 *
 * @retval 0     Transmission was successful.
 * @retval -1    Transmission was failed.
 **********************************************************************************************************************/
static int cip_safety_sci_send(unsigned char * tx_buf, unsigned long tx_num)
{
    fsp_err_t err = FSP_SUCCESS;
    EventBits_t uxBits = 0;
    int result = 0;

    /* Checks argments */
    if ((tx_buf == NULL) || (tx_num == 0)) {
        return -1;
    }

    /* Takes Mutex */
    if (xSemaphoreTake(gxCipSafetySCITxMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        /* Transmits UART frame */
        err = R_SCI_UART_Write(&g_uart_safety_ctrl, &tx_buf[0], tx_num);
        if (err == FSP_SUCCESS) {
            /* Waits transmit complete event */
            uxBits = xEventGroupWaitBits(xSCITXComEvent, CIP_SAFETY_SCI_TX_COMPLETE_EVENT, pdTRUE, pdFALSE, (1000 / portTICK_PERIOD_MS));
            if ((uxBits & CIP_SAFETY_SCI_TX_COMPLETE_EVENT) == 0) {
                /* Failed to get transmit complete event */
                result = -1;
            }
        } else {
            /* Failed to transmit UART frame */
            result = -1;
        }
        /* Gives Mutex */
        xSemaphoreGive(gxCipSafetySCITxMutex);
    } else {
        return -1;
    }

    return result;
}

/*******************************************************************************************************************//**
 * @brief Receives UART frame.
 *
 * @param[in] rx_buf          Pointer to receive buffer.
 * @param[in] rx_num          Length of receive buffer.
 * @param[in,out] rx_index    Pointer to receive buffer's offset.
 *
 * @retval 0     Reception was successful.
 * @retval -1    Reception was failed.
 **********************************************************************************************************************/
static int cip_safety_sci_receive(unsigned char * rx_buf, unsigned long rx_num, unsigned long * rx_index)
{
    unsigned long i = 0;

    if (cip_safety_sci_rx_rem) {
        i = cip_safety_sci_rx_rem;
        while (i--) {
            rx_buf[*rx_index] = cip_safety_sci_rx_buf[cip_safety_sci_rx_r++];
            if (cip_safety_sci_rx_r >= sizeof(cip_safety_sci_rx_buf)) {
                cip_safety_sci_rx_r = 0;
            }
            *rx_index += 1;
            if (*rx_index >= rx_num) {
                *rx_index = 0;
            }
            cip_safety_sci_rx_rem--;
        };
    } else {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief RTOS task.
 *
 * @param[in] pvParameter    Pointer to task parameters.
 **********************************************************************************************************************/
static void cip_safety_sci_rx_task (void * pvParameter)
{
    (void)pvParameter;
    volatile uint8_t data;
    unsigned long i = 0;
    unsigned long j = 0;
    unsigned long connection_id = 0;

    while (true) {
        /* Waits Queue */
        if (xQueueReceive(gxCipSafetySCIRecvQueue, (void *)&data, portMAX_DELAY) == pdPASS) {
            while (cip_safety_sci_receive(cip_safety_recieve_buf, CIP_SAFETY_SCI_RX_BUF_LEN, &cip_safety_recieve_buf_index) == 0){
                if (cip_safety_recieve_buf_index >= CIP_SAFETY_CMD_HEADER_SIZE) {
                    i = (cip_safety_recieve_buf[3] << 8) | cip_safety_recieve_buf[2];
                    if (cip_safety_recieve_buf_index >= (CIP_SAFETY_CMD_HEADER_SIZE + i)) {
                        tx_queue.cmd = (unsigned short)((cip_safety_recieve_buf[1] << 8) | cip_safety_recieve_buf[0]);
                        tx_queue.len = (unsigned short)((cip_safety_recieve_buf[3] << 8) | cip_safety_recieve_buf[2]);
                        memcpy(tx_queue.data, &cip_safety_recieve_buf[4], i);
                        switch(((cip_safety_recieve_buf[1] << 8) | cip_safety_recieve_buf[0])) {
                        case CIP_SAFETY_CMD_CODE_0x0101:
                            /* Send Start Up Message Queue */
                            xQueueSend(gxCipSafetyStartUpMsgQueue, (void *) &tx_queue, (TickType_t) 0);
                            cip_safety_start_up_res_send();
                            break;
                        case CIP_SAFETY_CMD_CODE_0x0202:
                            if (memcmp((void *)&judge_data_tcp_t_to_o[0], (void *)&cip_safety_recieve_buf[4], CIP_SAFETY_CMD_JUDGE_DATA_SIZE) == 0) {
                                /* Send Explicit Message Queue */
                                xQueueSend(gxCipSafetyExpMsgQueue, (void *) &tx_queue, (TickType_t) 0);
                            } else if (memcmp((void *)&judge_data_udp_t_to_o[0], (void *)&cip_safety_recieve_buf[4], CIP_SAFETY_CMD_JUDGE_DATA_SIZE) == 0) {
                                memcpy(&connection_id, &cip_safety_recieve_buf[8], CIP_SAFETY_CMD_CONNECTION_ID_SIZE);
                                for (j  = 0; j < CIP_SAFETY_SCI_NUM_IMP_MSG_QUEUE; j++) {
                                    if (g_connection_id[j] == connection_id) {
                                        /* Send Implicit Message Queue */
                                        xQueueSend(gxCipSafetyImpMsgQueue[j], (void *) &tx_queue, (TickType_t) 0);
                                        break;
                                    }
                                }
                            }
                            break;
                        case CIP_SAFETY_CMD_CODE_0x0302:
                            /* Send Data Acquisition Queue */
                            xQueueSend(gxCipSafetyDataAcqQueue, (void *) &tx_queue, (TickType_t) 0);
                            break;
                        default:
                            break;
                        }
                        cip_safety_recieve_buf_index = 0;
                    }
                }
            }
        }
    }
}

/*******************************************************************************************************************//**
 * @brief RTOS task.
 *
 * @param[in] pvParameter    Pointer to task parameters.
 **********************************************************************************************************************/
static void cip_safety_sci_tx_task (void * pvParameter)
{
    (void)pvParameter;

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while (true)
    {
        vTaskDelay((33 / portTICK_PERIOD_MS));
        cip_safety_data_acq_wait();
    }
}

/*******************************************************************************************************************//**
 * @brief RTOS task.
 *
 * @param[in] pvParameter    Pointer to task parameters.
 **********************************************************************************************************************/
static void cip_safety_sci_reset_task (void * pvParameter)
{
    (void)pvParameter;
    volatile EventBits_t uxBits;
    fsp_err_t err = FSP_SUCCESS;
    uint32_t remaining_bytes = 0;
    sci_baud_setting_t baud_setting = {0};
    sci_uart_baud_calculation_t baud_target = {7500000, true, 5000};

    while (true)
    {
        uxBits = xEventGroupWaitBits(xSCIResetEvent, (CIP_SAFETY_SCI_RESET_EVENT_ERR|CIP_SAFETY_SCI_RESET_EVENT_RX), pdTRUE, pdFALSE, portMAX_DELAY);
        if ((uxBits & CIP_SAFETY_SCI_RESET_EVENT_ERR) != 0) {
            err = R_SCI_UART_ReadStop(&g_uart_safety_ctrl, &remaining_bytes);
            if (err != FSP_SUCCESS) {
                while(1);
            }
            err = R_SCI_UART_Close(&g_uart_safety_ctrl);
            if (err != FSP_SUCCESS) {
                while(1);
            }
            err = R_SCI_UART_Open(&g_uart_safety_ctrl, &g_uart_safety_cfg);
            if (err != FSP_SUCCESS) {
                while(1);
            }
            err = R_SCI_UART_BaudCalculate(&baud_target, SCI_UART_CLOCK_SOURCE_SCI3ASYNCCLK , &baud_setting);
            if (err != FSP_SUCCESS) {
                while(1);
            }
            err = R_SCI_UART_BaudSet(&g_uart_safety_ctrl, (void *) &baud_setting);
            if (err != FSP_SUCCESS) {
                while(1);
            }
        }
        if ((uxBits & CIP_SAFETY_SCI_RESET_EVENT_RX) != 0) {
            err = R_SCI_UART_ReadStop(&g_uart_safety_ctrl, &remaining_bytes);
            if (err != FSP_SUCCESS) {
                while(1);
            }
        }
    }
}

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Initializes SCI.
 *
 * @retval 0     Initialization was successful.
 * @retval -1    Initialization was failed.
 **********************************************************************************************************************/
int cip_safety_sci_init(void)
{
    unsigned long i = 0;
    BaseType_t xReturned = 0;
    fsp_err_t err = FSP_SUCCESS;
    sci_baud_setting_t baud_setting = {0};
    sci_uart_baud_calculation_t baud_target = {7500000, true, 5000};

    xSCITXComEvent = xEventGroupCreate();
    if (xSCITXComEvent == NULL) {
        return -1;
    }

    xSCIResetEvent = xEventGroupCreate();
    if (xSCIResetEvent == NULL) {
        return -1;
    }

    gxCipSafetySCIRecvQueue = xQueueCreate(256, sizeof(uint8_t));
    if (gxCipSafetySCIRecvQueue == NULL) {
        return -1;
    }

    gxCipSafetySCITxMutex = xSemaphoreCreateMutex();
    if (gxCipSafetySCITxMutex == NULL) {
        return -1;
    }

    err = R_SCI_UART_Open(&g_uart_safety_ctrl, &g_uart_safety_cfg);
    if (err != FSP_SUCCESS) {
        return -1;
    }
    err = R_SCI_UART_BaudCalculate(&baud_target, SCI_UART_CLOCK_SOURCE_SCI3ASYNCCLK , &baud_setting);
    if (err != FSP_SUCCESS) {
        return -1;
    }
    err = R_SCI_UART_BaudSet(&g_uart_safety_ctrl, (void *) &baud_setting);
    if (err != FSP_SUCCESS) {
        return -1;
    }

    /* Create Start Up Message Queue */
    gxCipSafetyStartUpMsgQueue = xQueueCreate(1, sizeof(cip_safety_sci_st_queue));
    if (gxCipSafetyStartUpMsgQueue == NULL) {
        return -1;
    }

    /* Create Data Acquisition Queue */
    gxCipSafetyDataAcqQueue = xQueueCreate(1, sizeof(cip_safety_sci_st_queue));
    if (gxCipSafetyDataAcqQueue == NULL) {
        return -1;
    }

    /* Create Explicit Message Queue */
    gxCipSafetyExpMsgQueue = xQueueCreate(1, sizeof(cip_safety_sci_st_queue));
    if (gxCipSafetyExpMsgQueue == NULL) {
        return -1;
    }

    /* Create Implicit Message Queue */
    for (i  = 0; i < CIP_SAFETY_SCI_NUM_IMP_MSG_QUEUE; i++) {
        gxCipSafetyImpMsgQueue[i] = xQueueCreate(1, sizeof(cip_safety_sci_st_queue));
        if (gxCipSafetyImpMsgQueue[i] == NULL) {
            return -1;
        }
    }

    xReturned = xTaskCreate(cip_safety_sci_rx_task,
                            "cip_safety_sci_rx",
                            (1024/sizeof(configSTACK_DEPTH_TYPE)),
                            NULL,
                            (configMAX_PRIORITIES - 2),
                            &gCipSafetySCIRecvTask);
    if (xReturned != pdPASS) {
        return -1;
    }

    xReturned = xTaskCreate(cip_safety_sci_tx_task,
                            "cip_safety_sci_tx",
                            (1024/sizeof(configSTACK_DEPTH_TYPE)),
                            NULL,
                            (configMAX_PRIORITIES - 5),
                            &gCipSafetySCISendTask);
    if (xReturned != pdPASS) {
        return -1;
    }

    xReturned = xTaskCreate(cip_safety_sci_reset_task,
                            "cip_safety_sci_reset",
                            (1024/sizeof(configSTACK_DEPTH_TYPE)),
                            NULL,
                            (configMAX_PRIORITIES - 2),
                            &gCipSafetySCIResetTask);
    if (xReturned != pdPASS) {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief SCI Callback.
 *
 * @param[in] p_args    Pointer to SCI Callback.
 **********************************************************************************************************************/
void cip_safety_sci_callback(uart_callback_args_t * p_args)
{
    BaseType_t xResult = pdFAIL;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    unsigned char tmp_char = 0;

    /* Handle the UART event */
    switch (p_args->event)
    {
    /* Error event*/
    case UART_EVENT_ERR_OVERFLOW:
    case UART_EVENT_ERR_FRAMING:
    case UART_EVENT_ERR_PARITY:
    case UART_EVENT_BREAK_DETECT:
        xResult = xEventGroupSetBitsFromISR(xSCIResetEvent, CIP_SAFETY_SCI_RESET_EVENT_ERR, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        break;
    /* Received a character */
    case UART_EVENT_RX_CHAR:
        tmp_char = (unsigned char)p_args->data;
        if (g_start_up_seq < sizeof(start_up_req)) {
            /* Before getting a Start Up Message request  */
            if (tmp_char == start_up_req[g_start_up_seq]) {
                cip_safety_sci_rx_buf[g_start_up_seq] = tmp_char;
                g_start_up_seq++;

                if (g_start_up_seq == sizeof(start_up_req)) {
                    cip_safety_sci_rx_w += sizeof(start_up_req);
                    cip_safety_sci_rx_rem += sizeof(start_up_req);
                    xResult = xQueueSendFromISR(gxCipSafetySCIRecvQueue, &p_args->data, &xHigherPriorityTaskWoken);
                    if (xResult != pdFAIL) {
                        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                    }
                }
            } else {
                g_start_up_seq = 0;
            }
        } else {
            /* After getting a Start Up Message request */
            cip_safety_sci_rx_buf[cip_safety_sci_rx_w++] = tmp_char;
            if (cip_safety_sci_rx_w >= sizeof(cip_safety_sci_rx_buf)) {
                cip_safety_sci_rx_w = 0;
            }
            if (cip_safety_sci_rx_rem < sizeof(cip_safety_sci_rx_buf)) {
                cip_safety_sci_rx_rem++;
            }
            xResult = xQueueSendFromISR(gxCipSafetySCIRecvQueue, &p_args->data, &xHigherPriorityTaskWoken);
            if (xResult != pdFAIL) {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
        break;
    /* Receive complete */
    case UART_EVENT_RX_COMPLETE:
        xResult = xEventGroupSetBitsFromISR(xSCIResetEvent, CIP_SAFETY_SCI_RESET_EVENT_RX, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        break;
    /* Transmit complete */
    case UART_EVENT_TX_COMPLETE:
        xResult = xEventGroupSetBitsFromISR(xSCITXComEvent, CIP_SAFETY_SCI_TX_COMPLETE_EVENT, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        break;
        break;
    default:
        break;
    }
}

/*******************************************************************************************************************//**
 * @brief Waits Start Up Message request frame.
 *
 * @retval 0     Reception was successful.
 * @retval -1    Reception was failed.
 **********************************************************************************************************************/
int cip_safety_start_up_req_wait(void)
{
    fsp_err_t err = FSP_SUCCESS;

    err = R_SCI_UART_Read(&g_uart_safety_ctrl, &gu8_read_char, 0);
    if (err != FSP_SUCCESS) {
        return -1;
    }

    /* Waits Start Up Message Queue */
    if (xQueueReceive(gxCipSafetyStartUpMsgQueue, (void *)&start_up_rx_queue, (1000 / portTICK_PERIOD_MS)) != pdPASS) {
        return -1;
    }

    /* Checks Command Code */
    if (start_up_rx_queue.cmd != CIP_SAFETY_CMD_CODE_0x0101) {
        return -1;
    }

    /* Checks Length */
    if (start_up_rx_queue.len != 0) {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Finishs Start Up Message sequence.
 **********************************************************************************************************************/
void cip_safety_start_up_finish(void)
{
    g_start_up_seq = sizeof(start_up_req);
}

/*******************************************************************************************************************//**
 * @brief Transmits Start Up Message response frame.
 *
 * @param[in] data_len      Length of transmit data.
 * @param[in] data_buf      Pointer to transmit data buffer.
 *
 * @retval 0     Transmission was successful.
 * @retval -1    Transmission was failed.
 **********************************************************************************************************************/
int cip_safety_start_up_res_send(void)
{
    unsigned char tx_buf[(4 + CIP_SAFETY_CMD_CODE_LEN_0x0102)] = {0};
    unsigned short tx_cmd = CIP_SAFETY_CMD_CODE_0x0102;
    unsigned short tx_len = CIP_SAFETY_CMD_CODE_LEN_0x0102;
    unsigned short offset = 0;

    /* Sets Command Code */
    memcpy((void *)&tx_buf[offset], (void *)&tx_cmd, CIP_SAFETY_CMD_CODE_SIZE);
    offset += CIP_SAFETY_CMD_CODE_SIZE;

    /* Sets Length */
    memcpy((void *)&tx_buf[offset], (void *)&tx_len, CIP_SAFETY_CMD_LENGTH_SIZE);
    offset += CIP_SAFETY_CMD_LENGTH_SIZE;

    /* Transmits UART frame */
    if (cip_safety_sci_send(tx_buf, offset) != 0) {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Transmits Explicit Message frame.
 *
 * @param[in] data_len      Length of transmit data.
 * @param[in] data_buf      Pointer to transmit data buffer.
 *
 * @retval 0     Transmission was successful.
 * @retval -1    Transmission was failed.
 **********************************************************************************************************************/
int cip_safety_exp_msg_send(unsigned short data_len, unsigned char * data_buf)
{
    unsigned char tx_buf[(4 + CIP_SAFETY_CMD_CODE_LEN_0x0201)] = {0};
    unsigned short tx_cmd = CIP_SAFETY_CMD_CODE_0x0201;
    unsigned short tx_len = CIP_SAFETY_CMD_CODE_LEN_0x0201;
    unsigned short offset = 0;
#if 1 /* Not supported password */
    unsigned char cfg_req[] = {0x4F, 0x02, 0x20, 0x39, 0x24, 0x01};
    unsigned char cfg_lock[] = {0x52, 0x02, 0x20, 0x39, 0x24, 0x01};
    unsigned char safety_reset[] = {0x54, 0x02, 0x20, 0x39, 0x24, 0x01};
#endif

    if ((data_len > CIP_SAFETY_CMD_CIP_MSG_O_TO_T_SIZE) || (data_buf == NULL)) {
        return -1;
    }

#if 1 /* Not supported password */
    /* Configure_Request */
    if (memcmp((void *)&cfg_req[0], (void *)&data_buf[0], sizeof(cfg_req)) == 0) {
        if (CipSafetySupervisorCmpPassword((CipUsint *)&data_buf[6]) != 0) {
            return -1;
        }
    }
    /* Configuration_Lock/Unlock */
    if (memcmp((void *)&cfg_lock[0], (void *)&data_buf[0], sizeof(cfg_lock)) == 0) {
        if (CipSafetySupervisorCmpPassword((CipUsint *)&data_buf[7]) != 0) {
            return -1;
        }
    }
    /* Safety_Reset */
    if (memcmp((void *)&safety_reset[0], (void *)&data_buf[0], sizeof(safety_reset)) == 0) {
        if (CipSafetySupervisorCmpPassword((CipUsint *)&data_buf[7]) != 0) {
            return -1;
        }
    }
#endif

    /* Sets Command Code */
    memcpy((void *)&tx_buf[offset], (void *)&tx_cmd, CIP_SAFETY_CMD_CODE_SIZE);
    offset += CIP_SAFETY_CMD_CODE_SIZE;

    /* Sets Length */
    memcpy((void *)&tx_buf[offset], (void *)&tx_len, CIP_SAFETY_CMD_LENGTH_SIZE);
    offset += CIP_SAFETY_CMD_LENGTH_SIZE;

    /* Sets Judge_data */
    memcpy((void *)&tx_buf[offset], (void *)&judge_data_tcp_o_to_t[0], CIP_SAFETY_CMD_JUDGE_DATA_SIZE);
    offset += CIP_SAFETY_CMD_JUDGE_DATA_SIZE;

    /* Sets Data */
    memcpy((void *)&tx_buf[offset], (void *)data_buf, data_len);
    offset += CIP_SAFETY_CMD_CIP_MSG_O_TO_T_SIZE;

#if 1 /* Waits 23ms */
    vTaskDelay((23 / portTICK_PERIOD_MS));
#endif

    /* Clears Old Queue */
    while (xQueueReceive(gxCipSafetyExpMsgQueue, (void *)&exp_rx_queue, 0) == pdTRUE);

    /* Transmits UART frame */
    if (cip_safety_sci_send(tx_buf, offset) != 0) {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Waits Explicit Message frame.
 *
 * @param[in] rx_max_len                   Length of maximum receive buffer.
 * @param[out] rx_len                      Pointer to length of receive buffer.
 * @param[out] rx_buf                      Pointer to receive buffer.
 * @param[out] message_router_response     Pointer to CIP Message Router Response.
 *
 * @retval 0     Reception was successful.
 * @retval -1    Reception was failed.
 **********************************************************************************************************************/
int cip_safety_exp_msg_wait(unsigned short rx_max_len, unsigned short * rx_len, unsigned char * rx_buf, CipMessageRouterResponse *const message_router_response)
{
    unsigned short offset = 0;

    if ((rx_len == NULL) || (rx_buf == NULL)) {
        return -1;
    }

    /* Waits Explicit Message Queue */
    if (xQueueReceive(gxCipSafetyExpMsgQueue, (void *)&exp_rx_queue, (10000 / portTICK_PERIOD_MS)) != pdPASS) {
        return -1;
    }

    /* Checks Command Code */
    if (exp_rx_queue.cmd != CIP_SAFETY_CMD_CODE_0x0202) {
        return -1;
    }

    /* Checks Length */
    if (exp_rx_queue.len > rx_max_len) {
        return -1;
    }

    /* Checks Judge_data */
    if (memcmp((void *)&judge_data_tcp_t_to_o[0], (void *)&exp_rx_queue.data[offset], CIP_SAFETY_CMD_JUDGE_DATA_SIZE) != 0) {
        return -1;
    }
    offset += CIP_SAFETY_CMD_JUDGE_DATA_SIZE;

    /* Checks Message_size */
    if (exp_rx_queue.data[offset] > CIP_SAFETY_CMD_CIP_MSG_T_TO_O_SIZE) {
        return -1;
    }
    if (exp_rx_queue.data[offset] < 4) {
        return -1;
    }
    *rx_len = exp_rx_queue.data[offset];
    offset += CIP_SAFETY_CMD_MSG_SIZE_T_TO_O_SIZE;

    /* Checks Cip_Layer_Message */
    /* Checks Reply Service */
    if (message_router_response->reply_service != exp_rx_queue.data[offset]) {
        return -1;
    }
    offset++;
    *rx_len -= 1;

    /* Checks Reserved */
    if (exp_rx_queue.data[offset] != 0) {
        return -1;
    }
    offset++;
    *rx_len -= 1;

    /* Copies General Status */
    message_router_response->general_status = exp_rx_queue.data[offset];
    offset++;
    *rx_len -= 1;

    /* Checks Size of Additional/Extended Status */
    if (exp_rx_queue.data[offset] > MAX_SIZE_OF_ADD_STATUS) {
        return -1;
    }
    /* Copies Size of Additional/Extended Status */
    message_router_response->size_of_additional_status = exp_rx_queue.data[offset];
    offset++;
    *rx_len -= 1;

    /* Copies Additional/Extended Status */
    if (message_router_response->size_of_additional_status != 0) {
      memcpy(&message_router_response->additional_status[0], &exp_rx_queue.data[offset], (message_router_response->size_of_additional_status * 2));
      offset += (unsigned short)(message_router_response->size_of_additional_status * 2);
      *rx_len -= (unsigned short)(message_router_response->size_of_additional_status * 2);
    }

    /* Copies Response Data */
    memcpy((void *)&rx_buf[0], (void *)&exp_rx_queue.data[offset], *rx_len);
    offset += *rx_len;

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Transmits Implicit Message frame.
 *
 * @param[in] connection_id    Connection_ID.
 * @param[in] data_len         Length of transmit data.
 * @param[in] data_buf         Pointer to transmit data buffer.
 *
 * @retval 0     Transmission was successful.
 * @retval -1    Transmission was failed.
 **********************************************************************************************************************/
int cip_safety_imp_msg_send(unsigned long connection_id, unsigned short data_len, unsigned char * data_buf)
{
    unsigned char tx_buf[(4 + CIP_SAFETY_CMD_CODE_LEN_0x0201)] = {0};
    unsigned short tx_cmd = CIP_SAFETY_CMD_CODE_0x0201;
    unsigned short tx_len = CIP_SAFETY_CMD_CODE_LEN_0x0201;
    unsigned short offset = 0;

    if ((data_len > (CIP_SAFETY_CMD_CIP_MSG_O_TO_T_SIZE - CIP_SAFETY_CMD_CONNECTION_ID_SIZE)) || (data_buf == NULL)) {
        return -1;
    }

    /* Sets Command Code */
    memcpy((void *)&tx_buf[offset], (void *)&tx_cmd, CIP_SAFETY_CMD_CODE_SIZE);
    offset += CIP_SAFETY_CMD_CODE_SIZE;

    /* Sets Length */
    memcpy((void *)&tx_buf[offset], (void *)&tx_len, CIP_SAFETY_CMD_LENGTH_SIZE);
    offset += CIP_SAFETY_CMD_LENGTH_SIZE;

    /* Sets Judge_data */
    memcpy((void *)&tx_buf[offset], (void *)&judge_data_udp_o_to_t[0], CIP_SAFETY_CMD_JUDGE_DATA_SIZE);
    offset += CIP_SAFETY_CMD_JUDGE_DATA_SIZE;

    /* Sets Connection_ID */
    memcpy((void *)&tx_buf[offset], (void *)&connection_id, CIP_SAFETY_CMD_CONNECTION_ID_SIZE);
    offset += CIP_SAFETY_CMD_CONNECTION_ID_SIZE;

    /* Sets Data */
    memcpy((void *)&tx_buf[offset], (void *)data_buf, data_len);
    offset += (CIP_SAFETY_CMD_CIP_MSG_O_TO_T_SIZE - CIP_SAFETY_CMD_CONNECTION_ID_SIZE);

    /* Transmits UART frame */
    if (cip_safety_sci_send(tx_buf, offset) != 0) {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Waits Implicit Message frame.
 *
 * @param[in] connection_id    Connection_ID.
 * @param[in] rx_max_len       Length of maximum receive buffer.
 * @param[out] rx_len          Pointer to length of receive buffer.
 * @param[out] rx_buf          Pointer to receive buffer.
 *
 * @retval 0     Reception was successful.
 * @retval -1    Reception was failed.
 **********************************************************************************************************************/
int cip_safety_imp_msg_wait(unsigned long connection_id, unsigned short rx_max_len, unsigned short * rx_len, unsigned char * rx_buf)
{
    unsigned long i = 0;
    unsigned short offset = 0;

    if ((rx_len == NULL) || (rx_buf == NULL)) {
        return -1;
    }

    for (i  = 0; i < CIP_SAFETY_SCI_NUM_IMP_MSG_QUEUE; i++) {
        if (g_connection_id[i] == connection_id) {
            /* Waits Implicit Message Queue */
            if (xQueueReceive(gxCipSafetyImpMsgQueue[i], (void *)&imp_rx_queue, 0) != pdPASS) {
                return -1;
            }
            break;
        }
    }

    /* There is no matching Connection_ID */
    if (i >= CIP_SAFETY_SCI_NUM_IMP_MSG_QUEUE) {
        return -1;
    }

    /* Checks Command Code */
    if (imp_rx_queue.cmd != CIP_SAFETY_CMD_CODE_0x0202) {
        return -1;
    }

    /* Checks Length */
    if (imp_rx_queue.len > rx_max_len) {
        return -1;
    }

    /* Checks Judge_data */
    if (memcmp((void *)&judge_data_udp_t_to_o[0], (void *)&imp_rx_queue.data[offset], CIP_SAFETY_CMD_JUDGE_DATA_SIZE) != 0) {
        return -1;
    }
    offset += CIP_SAFETY_CMD_JUDGE_DATA_SIZE;

    /* Checks Message_size */
    if (imp_rx_queue.data[offset] > CIP_SAFETY_CMD_CIP_MSG_T_TO_O_SIZE) {
        return -1;
    }
    if (imp_rx_queue.data[offset] <= CIP_SAFETY_CMD_CONNECTION_ID_SIZE) {
        return -1;
    }
    *rx_len = imp_rx_queue.data[offset] - CIP_SAFETY_CMD_CONNECTION_ID_SIZE;
    offset += CIP_SAFETY_CMD_MSG_SIZE_T_TO_O_SIZE;

    /* Checks Connection_ID */
    if (memcmp((void *)&connection_id, (void *)&imp_rx_queue.data[offset], CIP_SAFETY_CMD_CONNECTION_ID_SIZE) != 0) {
        return -1;
    }
    offset += CIP_SAFETY_CMD_CONNECTION_ID_SIZE;

    /* Copies Cip_Layer_Message */
    memcpy((void *)&rx_buf[0], (void *)&imp_rx_queue.data[offset], *rx_len);
    offset += *rx_len;

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Sets Connection_ID for Implicit Message frame queue.
 *
 * @param[in] index            Index of Queue.
 * @param[in] connection_id    Connection_ID.
 *
 * @retval 0     Setting was successful.
 * @retval -1    Starting was failed.
 **********************************************************************************************************************/
int cip_safety_imp_msg_set_connection_id(unsigned long index, unsigned long connection_id)
{
    /* Checks Index */
    if (index >= CIP_SAFETY_SCI_NUM_IMP_MSG_QUEUE) {
        return -1;
    }

    g_connection_id[index] = connection_id;

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Waits Data Acquisition frame.
 *
 * @retval 0     Reception was successful.
 * @retval -1    Reception was failed.
 **********************************************************************************************************************/
int cip_safety_data_acq_wait(void)
{
    unsigned char tx_buf[(4 + CIP_SAFETY_CMD_CODE_LEN_0x0301)] = {0};
    unsigned short tx_cmd = CIP_SAFETY_CMD_CODE_0x0301;
    unsigned short tx_len = CIP_SAFETY_CMD_CODE_LEN_0x0301;
    unsigned short tx_offset = 0;
    unsigned short rx_offset = 0;
    unsigned short producer_max_data_age = 0;
    unsigned short consumer_max_data_age = 0;
    unsigned char producer_fault_counter_size = 0;
    unsigned char consumer_fault_counter_size = 0;
    unsigned char producer_fault_counter = 0;
    unsigned char consumer_fault_counter = 0;
    unsigned short safety_connection_fault_count = 0;
    unsigned char exception_status = 0;
    unsigned char device_status = 0;

    /* Sets Command Code */
    memcpy((void *)&tx_buf[tx_offset], (void *)&tx_cmd, CIP_SAFETY_CMD_CODE_SIZE);
    tx_offset += CIP_SAFETY_CMD_CODE_SIZE;

    /* Sets Length */
    memcpy((void *)&tx_buf[tx_offset], (void *)&tx_len, CIP_SAFETY_CMD_LENGTH_SIZE);
    tx_offset += CIP_SAFETY_CMD_LENGTH_SIZE;

    /* Clears Old Queue */
    while (xQueueReceive(gxCipSafetyDataAcqQueue, (void *)&acq_rx_queue, 0) == pdTRUE);

    /* Transmits UART frame */
    if (cip_safety_sci_send(tx_buf, tx_offset) != 0) {
        return -1;
    }

    /* Waits Data Acquisition Queue */
    if (xQueueReceive(gxCipSafetyDataAcqQueue, (void *)&acq_rx_queue, (1000 / portTICK_PERIOD_MS)) != pdPASS) {
        return -1;
    }

    /* Checks Command Code */
    if (acq_rx_queue.cmd != CIP_SAFETY_CMD_CODE_0x0302) {
        return -1;
    }

    /* Checks Length */
    if (acq_rx_queue.len != CIP_SAFETY_CMD_CODE_LEN_0x0302) {
        return -1;
    }

    /* Copies Data */
    /* Max Data Age */
    /* Producer */
    memcpy((void *)&producer_max_data_age, (void *)&acq_rx_queue.data[rx_offset], 2);
    rx_offset += 2;

    /* Consumer */
    memcpy((void *)&consumer_max_data_age, (void *)&acq_rx_queue.data[rx_offset], 2);
    rx_offset += 2;

    /* Producer/Consumer Counter Array Size */
    /* Producer */
    memcpy((void *)&producer_fault_counter_size, (void *)&acq_rx_queue.data[rx_offset], 1);
    rx_offset += 1;

    /* Consumer */
    memcpy((void *)&consumer_fault_counter_size, (void *)&acq_rx_queue.data[rx_offset], 1);
    rx_offset += 1;

    /* Producer/Consumer Fault Counter */
    /* Producer */
    memcpy((void *)&producer_fault_counter, (void *)&acq_rx_queue.data[rx_offset], 1);
    rx_offset += 1;

    /* Consumer */
    memcpy((void *)&consumer_fault_counter, (void *)&acq_rx_queue.data[rx_offset], 1);
    rx_offset += 1;

    /* Safety Connection Fault Count */
    memcpy((void *)&safety_connection_fault_count, (void *)&acq_rx_queue.data[rx_offset], 2);
    rx_offset += 2;

    /* Exception Status */
    memcpy((void *)&exception_status, (void *)&acq_rx_queue.data[rx_offset], 1);
    rx_offset += 1;

    /* Device Status */
    memcpy((void *)&device_status, (void *)&acq_rx_queue.data[rx_offset], 1);
    rx_offset += 1;

    /* Sets Data */
    /* Producer */
    if (CipSafetySafetyValidatorSetValidator(0, producer_max_data_age, producer_fault_counter_size, consumer_fault_counter) != 0) {
        return -1;
    }

    /* Consumer */
    if (CipSafetySafetyValidatorSetValidator(1, consumer_max_data_age, consumer_fault_counter_size, consumer_fault_counter) != 0) {
        return -1;
    }

    /* Safety Connection Fault Count */
    CipSafetySafetyValidatorSetSafetyConnectionFaultCount(safety_connection_fault_count);

    /* Device Status & Exception Status */
    CipSafetySupervisorSetStatus(device_status, exception_status);

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Starts to send Data Acquisition frame on cyclic.
 *
 * @retval 0     Starting was successful.
 * @retval -1    Starting was failed.
 **********************************************************************************************************************/
int cip_safety_data_acq_start_cyclic(void)
{
    xTaskNotifyGive(gCipSafetySCISendTask);

    return 0;
}
#endif

