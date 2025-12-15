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
#include "lwip/opt.h"

#if LWIP_LLDP /* don't build if not configured for use in lwipopts.h */

/**
 * Include Application libraries
 */
#include "hal_data.h"
/**
 * Include lldp libraries
 */
#include "lwip_lldp.h"
#include "lwip_lldp_def.h"
#include "lwip_lldp_mib.h"
#include "lwip_lldp_transmit.h"
#include "lwip_lldp_receive.h"
/**
 * Include lwIP libraries
 */
#include "lwip/inet.h"
#include "lwip/netifapi.h"
#include "lwip/etharp.h"
#if LWIP_SNMP_APP
#include "examples/snmp/snmp_example.h"
#endif
#include "um_ether_netif_api.h"
/**
 * Inlcude FreeRTOS libraries
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
/**
 * Standard libraries
 */
#include <stdint.h>
#include <stdio.h>

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define LWIP_LLDP_TIMER_THREAD_STACK_BYTES      (8192)
#define LWIP_LLDP_TIMER_THREAD_PRIORITY         (configMAX_PRIORITIES - 4)
#define LWIP_LLDP_RECEIVE_THREAD_STACK_BYTES    (8192)
#define LWIP_LLDP_RECEIVE_THREAD_PRIORITY       (configMAX_PRIORITIES - 4)
#define LWIP_LLDP_GET_MIB_THREAD_STACK_BYTES    (8192)
#define LWIP_LLDP_GET_MIB_THREAD_PRIORITY       (configMAX_PRIORITIES - 4)
#define LWIP_LLDP_SET_MIB_THREAD_STACK_BYTES    (8192)
#define LWIP_LLDP_SET_MIB_THREAD_PRIORITY       (configMAX_PRIORITIES - 4)

/***********************************************************************************************************************
 * Private constants
 **********************************************************************************************************************/
/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/

static void LWIP_LLDP_timer_task (void * pvParameters);
static void LWIP_LLDP_receive_task (void * pvParameters);
static void LWIP_LLDP_get_mib_task (void * pvParameters);
static void LWIP_LLDP_set_mib_task (void * pvParameters);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

static bool glldp_port_enabled[BSP_FEATURE_GMAC_MAX_PORTS] = {0};
static QueueHandle_t gxReceiveQueue[BSP_FEATURE_GMAC_MAX_PORTS] = {0};
static QueueHandle_t gxGetMIBQueue[BSP_FEATURE_GMAC_MAX_PORTS] = {0};
static EventGroupHandle_t gxGetMIBEvent[BSP_FEATURE_GMAC_MAX_PORTS] = {0};
static QueueHandle_t gxSetMIBQueue[BSP_FEATURE_GMAC_MAX_PORTS] = {0};
static EventGroupHandle_t gxSetMIBEvent[BSP_FEATURE_GMAC_MAX_PORTS] = {0};
static SemaphoreHandle_t gxLLDPAgentMutex[BSP_FEATURE_GMAC_MAX_PORTS] = {0};
static const char LWIP_LLDP_timer_task_name[BSP_FEATURE_GMAC_MAX_PORTS][configMAX_TASK_NAME_LEN] =
{
    "LLDP_timer_1",
    "LLDP_timer_2",
    "LLDP_timer_3",
};
static const char LWIP_LLDP_receive_task_name[BSP_FEATURE_GMAC_MAX_PORTS][configMAX_TASK_NAME_LEN] =
{
    "LLDP_receive_1",
    "LLDP_receive_2",
    "LLDP_receive_3",
};
static const char LWIP_LLDP_get_mib_task_name[BSP_FEATURE_GMAC_MAX_PORTS][configMAX_TASK_NAME_LEN] =
{
    "LLDP_get_mib_1",
    "LLDP_get_mib_2",
    "LLDP_get_mib_3",
};
static const char LWIP_LLDP_set_mib_task_name[BSP_FEATURE_GMAC_MAX_PORTS][configMAX_TASK_NAME_LEN] =
{
    "LLDP_set_mib_1",
    "LLDP_set_mib_2",
    "LLDP_set_mib_3",
};

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
struct netif lldp_netif = {0};
uint8_t g_lwip_lldp_adminStatus[BSP_FEATURE_GMAC_MAX_PORTS] =
{
		LWIP_LLDP_enabledRxTx,  /* PORT 1 */
		LWIP_LLDP_enabledRxTx,  /* PORT 2 */
		LWIP_LLDP_disabled,     /* PORT 3 */
};

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Initializes LLDP agent.
 *
 * @param[in] netif      The network interface.
 *
 * @retval ERR_OK        Initialization was successful and each task has started.
 * @retval ERR_IF        Initialization was failed.
 **********************************************************************************************************************/
err_t LWIP_LLDP_agent_init (struct netif *netif)
{
    BaseType_t xReturned = 0;
    LWIP_LLDP_AGENT_VARIABLES * plldp_agent = NULL;
    uint8_t * plldp_port_id = &netif->lldp_port_id;
    uint8_t i = 0;
    uint8_t array_pos = 0;

    /* Enable BPDU forwarding */
    R_ETHSW->MGMT_CONFIG |= (OPENER_LLDP_ETHER_SWITCH_MGMT_BPDU_FWD_ENABLE | OPENER_LLDP_ETHER_SWITCH_MGMT_BPDU_MGMT_ENABLE);
    /* Disable discarding BPDU frames */
    R_ETHSW->MGMT_CONFIG &= ~(uint32_t)(OPENER_LLDP_ETHER_SWITCH_MGMT_BPDU_DISCARD | OPENER_LLDP_ETHER_SWITCH_MGMT_BPDU_MGMT_DISCARD);

    for (i = 0; i < BSP_FEATURE_GMAC_MAX_PORTS; i++) {
        if (glldp_port_enabled[i] == false) {
            glldp_port_enabled[i] = true;
            *plldp_port_id = (uint8_t) (i + 1);
            break;
        }
    }

    if (*plldp_port_id == 0) {
        return ERR_IF;
    }

    if (*plldp_port_id > BSP_FEATURE_GMAC_MAX_PORTS) {
        return ERR_IF;
    }

    plldp_agent = pvPortMalloc(sizeof(LWIP_LLDP_AGENT_VARIABLES));
    if (plldp_agent == NULL) {
        return ERR_IF;
    }

    /* Initialization */
    memset((void *)plldp_agent, 0, sizeof(LWIP_LLDP_AGENT_VARIABLES));
    plldp_agent->netif = netif;

    array_pos = (uint8_t) (plldp_agent->netif->lldp_port_id - 1);

    /* Create mutex for LLDP agent */
    gxLLDPAgentMutex[array_pos] = xSemaphoreCreateMutex();
    if (gxLLDPAgentMutex[array_pos] == NULL) {
        vPortFree(plldp_agent);
        return ERR_IF;
    }

    /* Start for LLDP agent timer task */
    xReturned = xTaskCreate(LWIP_LLDP_timer_task,
                            &LWIP_LLDP_timer_task_name[array_pos][0],
                            (LWIP_LLDP_TIMER_THREAD_STACK_BYTES/sizeof(StackType_t)), /* In words, not bytes */
                            (void *)plldp_agent,
                            LWIP_LLDP_TIMER_THREAD_PRIORITY,
                            NULL);
    if (xReturned != pdPASS) {
        vPortFree(plldp_agent);
        return ERR_IF;
    }

    /* Start for LLDP agent receive task */
    xReturned = xTaskCreate(LWIP_LLDP_receive_task,
                            &LWIP_LLDP_receive_task_name[array_pos][0],
                            (LWIP_LLDP_RECEIVE_THREAD_STACK_BYTES/sizeof(StackType_t)), /* In words, not bytes */
                            (void *)plldp_agent,
                            LWIP_LLDP_RECEIVE_THREAD_PRIORITY,
                            NULL);
    if (xReturned != pdPASS) {
        vPortFree(plldp_agent);
        return ERR_IF;
    }

    /* Start for LLDP agent get mib task */
    xReturned = xTaskCreate(LWIP_LLDP_get_mib_task,
                            &LWIP_LLDP_get_mib_task_name[array_pos][0],
                            (LWIP_LLDP_GET_MIB_THREAD_STACK_BYTES/sizeof(StackType_t)), /* In words, not bytes */
                            (void *)plldp_agent,
                            LWIP_LLDP_GET_MIB_THREAD_PRIORITY,
                            NULL);
    if (xReturned != pdPASS) {
        vPortFree(plldp_agent);
        return ERR_IF;
    }

    /* Start for LLDP agent set mib task */
    xReturned = xTaskCreate(LWIP_LLDP_set_mib_task,
                            &LWIP_LLDP_set_mib_task_name[array_pos][0],
                            (LWIP_LLDP_SET_MIB_THREAD_STACK_BYTES/sizeof(StackType_t)), /* In words, not bytes */
                            (void *)plldp_agent,
                            LWIP_LLDP_SET_MIB_THREAD_PRIORITY,
                            NULL);
    if (xReturned != pdPASS) {
        vPortFree(plldp_agent);
        return ERR_IF;
    }

    return ERR_OK;
}

/*******************************************************************************************************************//**
 * @brief Checks LLDP packet.
 *
 * @param[in] p_frame_packet        Pointer to the frame buffer to be set to the read Ethernet frame.
 *
 * @retval 0         LLDP packet
 * @retval -1        Not LLDP packet
 **********************************************************************************************************************/
int32_t LWIP_LLDP_receive_check (ether_netif_frame_t * p_frame_packet)
{
    uint16_t ethertype = 0;
    uint8_t lldp_multi[LWIP_LLDP_MAC_ADD_LEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E};
    uint8_t array_pos = 0;

    if (p_frame_packet->port == 0) {
        return -1;
    }

    if (p_frame_packet->port > BSP_FEATURE_GMAC_MAX_PORTS) {
        return -1;
    }

    array_pos = (uint8_t)(p_frame_packet->port - 1);

    /* LLDP frame recognition */
    /* a) The destination address value is the assigned LWIP_LLDP_Multicast address defined in 8.1. */
    if (memcmp((void *)&p_frame_packet->p_buffer[0], (void *)&lldp_multi[0], LWIP_LLDP_MAC_ADD_LEN) != 0) {
        return -1;
    }
    /* b) The Ethertype value is the LLDP Ethertype defined in 8.3 for the particular frame format. */
    memcpy((void *)&ethertype, (void *)&p_frame_packet->p_buffer[12], sizeof(ethertype));
    ethertype = htons(ethertype);
    if (ethertype != LWIP_LLDP_ETH_TYPE) {
        return -1;
    }

    if (gxReceiveQueue[array_pos] != NULL) {
        if (xQueueSend( gxReceiveQueue[array_pos], ( void * ) p_frame_packet, ( TickType_t ) 0 ) != pdPASS ) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Requests to get LLDP MIB.
 *
 * @param[in] lldp_port_id        The number of interface
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
int32_t LWIP_LLDP_mib_get_request (uint8_t lldp_port_id)
{
    uint8_t array_pos = 0;

    if (lldp_port_id == 0) {
        return -1;
    }

    if (lldp_port_id > BSP_FEATURE_GMAC_MAX_PORTS) {
        return -1;
    }

    array_pos = (uint8_t)(lldp_port_id - 1);

    if (gxGetMIBEvent[array_pos] != NULL) {
        xEventGroupSetBits(gxGetMIBEvent[array_pos], 1);
    } else {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Responses to get LLDP MIB.
 *
 * @param[in] lldp_port_id        The number of interface
 * @param[in] plldp_agent         Pointer to LLDP agent variables.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
int32_t LWIP_LLDP_mib_get_response (uint8_t lldp_port_id, LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    uint8_t array_pos = 0;

    if (lldp_port_id == 0) {
        return -1;
    }

    if (lldp_port_id > BSP_FEATURE_GMAC_MAX_PORTS) {
        return -1;
    }

    array_pos = (uint8_t)(lldp_port_id - 1);

    if (gxGetMIBQueue[array_pos] != NULL) {
        if (xQueueReceive(gxGetMIBQueue[array_pos], (void *)plldp_agent, (100 / portTICK_PERIOD_MS)) != pdPASS) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Requests to set LLDP MIB.
 *
 * @param[in] lldp_port_id        The number of interface
 * @param[in] plldp_agent         Pointer to LLDP agent variables.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
int32_t LWIP_LLDP_mib_set_request (uint8_t lldp_port_id, LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    uint8_t array_pos = 0;

    if (lldp_port_id == 0) {
        return -1;
    }

    if (lldp_port_id > BSP_FEATURE_GMAC_MAX_PORTS) {
        return -1;
    }

    array_pos = (uint8_t)(lldp_port_id - 1);

    if (gxSetMIBQueue[array_pos] != NULL) {
        xQueueSend(gxSetMIBQueue[array_pos], (void *)plldp_agent, (TickType_t)0);
    } else {
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Responses to set LLDP MIB.
 *
 * @param[in] lldp_port_id        The number of interface
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
int32_t LWIP_LLDP_mib_set_response (uint8_t lldp_port_id)
{
    uint8_t array_pos = 0;

    if (lldp_port_id == 0) {
        return -1;
    }

    if (lldp_port_id > BSP_FEATURE_GMAC_MAX_PORTS) {
        return -1;
    }

    array_pos = (uint8_t)(lldp_port_id - 1);

    if (gxSetMIBEvent[array_pos] != NULL) {
        if (xEventGroupWaitBits(gxSetMIBEvent[array_pos], 1, pdTRUE, pdFALSE, (100 / portTICK_PERIOD_MS)) != pdPASS) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Notifies the PTOPO MIB manager and MIB managers of other optional MIBs
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_notification_event (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    /* Add notifications, if PTOPO MIB manager and MIB managers of other optional MIBs are implemented. */
    LWIP_UNUSED_ARG(plldp_agent);
}

/*******************************************************************************************************************//**
 * @brief LLDP agent timer task.
 *
 * @param[in] pvParameters        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_timer_task (void * pvParameters)
{
    int32_t count = 0;
    LWIP_LLDP_AGENT_VARIABLES * plldp_agent = (LWIP_LLDP_AGENT_VARIABLES *)pvParameters;
    uint8_t array_pos = 0;

    /* Initialization */
    plldp_agent->global.adminStatus = LWIP_LLDP_enabledRxTx;
    plldp_agent->global.portEnabled = true;
    plldp_agent->tlv.mibBasicTLVsTxEnable = LWIP_LLDP_DEFAULT_BASIC_TLV;
    plldp_agent->tlv.mibMgmtAddrInstanceTxEnable = true;
    plldp_agent->tlv.mibCipInstanceTxEnable = LWIP_LLDP_SUPPORT_CIP_TLV;
    plldp_agent->tx.tx_state = TX_LLDP_INITIALIZE;
    LWIP_LLDP_txInitializeLLDP(plldp_agent);
    plldp_agent->rx.rx_state = RX_LLDP_WAIT_PORT_OPERATIONAL;
    plldp_agent->rx.notification.interval = 5;
    array_pos = (uint8_t) (plldp_agent->netif->lldp_port_id - 1);
    LWIP_LLDP_set_g_adminStatus(plldp_agent->netif->lldp_port_id, (uint8_t)plldp_agent->global.adminStatus);

    while (1) {
        xSemaphoreTake(gxLLDPAgentMutex[array_pos], portMAX_DELAY);
        if (plldp_agent->global.snmpChangeStatus == true)
        {
        	/* adminStatus is changed by SNMP, update the global adminStatus variable */
        	LWIP_LLDP_set_g_adminStatus(plldp_agent->netif->lldp_port_id, (uint8_t)plldp_agent->global.adminStatus);
        	plldp_agent->global.snmpChangeStatus = false;
        }
        else
        {
        	/* Update adminStatus in lldp_agent variables. */
        	/* Global adminStatus variable is used as a window to access from other module, so this get process is used to reflect it. */
        	plldp_agent->global.adminStatus = LWIP_LLDP_get_g_adminStatus(plldp_agent->netif->lldp_port_id);
        }
        LWIP_LLDP_tx_state_machine(plldp_agent);
        LWIP_LLDP_rx_state_machine(plldp_agent);
        if (plldp_agent->rx.notification.enable == true) {
            if (plldp_agent->rx.notification.interval <= count) {
                count = 0;
                if (plldp_agent->rx.rx_variables.somethingChangedRemote == true) {
                    LWIP_LLDP_notification_event(plldp_agent);
                }
            } else {
                count++;
            }
        }
        xSemaphoreGive(gxLLDPAgentMutex[array_pos]);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    };
}

/*******************************************************************************************************************//**
 * @brief LLDP agent receive task.
 *
 * @param[in] pvParameters        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_receive_task (void * pvParameters)
{
    LWIP_LLDP_AGENT_VARIABLES * plldp_agent = (LWIP_LLDP_AGENT_VARIABLES *)pvParameters;
    ether_netif_frame_t ether_netif_frame = {0};
    uint8_t array_pos = 0;

    array_pos = (uint8_t) (plldp_agent->netif->lldp_port_id - 1);

    gxReceiveQueue[array_pos] = xQueueCreate(LWIP_LLDP_SUPPORT_RX_MAX_NEIGHBORS, sizeof(ether_netif_frame_t));

    if (gxReceiveQueue[array_pos] == NULL) {
        return;
    }

    while (1) {
        if (xQueueReceive(gxReceiveQueue[array_pos], (void *)&ether_netif_frame, portMAX_DELAY) == pdPASS) {
            xSemaphoreTake(gxLLDPAgentMutex[array_pos], portMAX_DELAY);
            LWIP_LLDP_receive(plldp_agent, ether_netif_frame.length, &ether_netif_frame.p_buffer[0]);
            xSemaphoreGive(gxLLDPAgentMutex[array_pos]);
        }
    };
}

/*******************************************************************************************************************//**
 * @brief LLDP agent get mib task.
 *
 * @param[in] pvParameters        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_get_mib_task (void * pvParameters)
{
    LWIP_LLDP_AGENT_VARIABLES * plldp_agent = (LWIP_LLDP_AGENT_VARIABLES *)pvParameters;
    uint8_t array_pos = 0;

    array_pos = (uint8_t) (plldp_agent->netif->lldp_port_id - 1);

    gxGetMIBQueue[array_pos] = xQueueCreate(1, sizeof(LWIP_LLDP_AGENT_VARIABLES));

    if (gxGetMIBQueue[array_pos] == NULL) {
        return;
    }

    gxGetMIBEvent[array_pos] = xEventGroupCreate();

    if (gxGetMIBEvent[array_pos] == NULL) {
        return;
    }

    while (1) {
        if (xEventGroupWaitBits(gxGetMIBEvent[array_pos], 1, pdTRUE, pdFALSE, portMAX_DELAY) == pdPASS) {
            xSemaphoreTake(gxLLDPAgentMutex[array_pos], portMAX_DELAY);
            xQueueSend(gxGetMIBQueue[array_pos], (void *)plldp_agent, (TickType_t)0);
            xSemaphoreGive(gxLLDPAgentMutex[array_pos]);
        }
    };
}


/*******************************************************************************************************************//**
 * @brief LLDP agent set mib task.
 *
 * @param[in] pvParameters        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_set_mib_task (void * pvParameters)
{
    LWIP_LLDP_AGENT_VARIABLES * plldp_agent = (LWIP_LLDP_AGENT_VARIABLES *)pvParameters;
    LWIP_LLDP_AGENT_VARIABLES * p_set_mib_lldp_agent = NULL;
    uint8_t array_pos = 0;

    array_pos = (uint8_t) (plldp_agent->netif->lldp_port_id - 1);

    p_set_mib_lldp_agent = pvPortMalloc(sizeof(LWIP_LLDP_AGENT_VARIABLES));

    if (p_set_mib_lldp_agent == NULL) {
        return;
    }

    gxSetMIBQueue[array_pos] = xQueueCreate(1, sizeof(LWIP_LLDP_AGENT_VARIABLES));

    if (gxSetMIBQueue[array_pos] == NULL) {
        return;
    }

    gxSetMIBEvent[array_pos] = xEventGroupCreate();

    if (gxSetMIBEvent[array_pos] == NULL) {
        return;
    }

    while (1) {
        if (xQueueReceive(gxSetMIBQueue[array_pos], (void *)p_set_mib_lldp_agent, portMAX_DELAY) == pdPASS) {
            xSemaphoreTake(gxLLDPAgentMutex[array_pos], portMAX_DELAY);
#if defined(LWIP_LLDP_CIP_CONNECTED) && 1 != LWIP_LLDP_CIP_CONNECTED
            if ((plldp_agent->tx.tx_parameters.msgTxHold != p_set_mib_lldp_agent->tx.tx_parameters.msgTxHold) ||
                (plldp_agent->tx.tx_parameters.msgTxInterval != p_set_mib_lldp_agent->tx.tx_parameters.msgTxInterval)) {
                p_set_mib_lldp_agent->tx.tx_variables.txTTL = (uint16_t)LWIP_MIN(65535, (p_set_mib_lldp_agent->tx.tx_parameters.msgTxInterval * p_set_mib_lldp_agent->tx.tx_parameters.msgTxHold));
                p_set_mib_lldp_agent->tx.tx_variables.somethingChangedLocal = true;
            }
#else
            if ((*plldp_agent->tx.tx_parameters.msgTxHold != *p_set_mib_lldp_agent->tx.tx_parameters.msgTxHold) ||
                (*plldp_agent->tx.tx_parameters.msgTxInterval != *p_set_mib_lldp_agent->tx.tx_parameters.msgTxInterval)) {
                p_set_mib_lldp_agent->tx.tx_variables.txTTL = (uint16_t)LWIP_MIN(65535, (*p_set_mib_lldp_agent->tx.tx_parameters.msgTxInterval * *p_set_mib_lldp_agent->tx.tx_parameters.msgTxHold));
                p_set_mib_lldp_agent->tx.tx_variables.somethingChangedLocal = true;
            }
#endif
            memcpy((void *)plldp_agent, (void *)p_set_mib_lldp_agent, sizeof(LWIP_LLDP_AGENT_VARIABLES));
            xSemaphoreGive(gxLLDPAgentMutex[array_pos]);
            xEventGroupSetBits(gxSetMIBEvent[array_pos], 1);
        }
    };
}

/*******************************************************************************************************************//**
 * @brief Set global adminStatus variable of specified port
 *
 * @param[in] port_id        Port number
 * @param[in] adminStatus    Set value to global adminStatus variable
 **********************************************************************************************************************/
void LWIP_LLDP_set_g_adminStatus(uint8_t port_id, uint8_t set_value)
{
	g_lwip_lldp_adminStatus[port_id -1] = set_value;
}

/*******************************************************************************************************************//**
 * @brief Get global adminStatus variable
 *
 * @param[in] port_id                   Port number
 * @param[in] lldp_global_enable        Pointer of LLDP global variables
 **********************************************************************************************************************/
int32_t LWIP_LLDP_get_g_adminStatus(uint8_t port_id)
{
	return (int32_t) g_lwip_lldp_adminStatus[port_id -1];
}

#endif

