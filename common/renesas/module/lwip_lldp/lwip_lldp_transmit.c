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
#include "main_thread.h"
/**
 * Include lldp libraries
 */
#include "lwip_lldp.h"
#include "lwip_lldp_def.h"
#include "lwip_lldp_interface.h"
#include "lwip_lldp_oui.h"
#include "lwip_lldp_transmit.h"
/**
 * Include lwIP libraries
 */
#include "lwip/inet.h"
#include "lwip/netifapi.h"
#include "lwip/etharp.h"
#if LWIP_SNMP_APP
#include "examples/snmp/snmp_example.h"
#include "lwip/apps/snmp.h"
#endif

#if LWIP_LLDP_SUPPORT_CIP_TLV
/**
 * Include CIP libraries
 */
#include "cipidentity.h"
#include "ciplldpmanagement.h"
#include "cipethernetlink.h"
#endif

/**
 * Include FreeRTOS libraries
 */
#include "FreeRTOS.h"
#include "task.h"
/**
 * Standard libraries
 */
#include <stdint.h>
#include <stdio.h>

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define LWIP_LLDP_MAXIMUM_FRAME_SIZE    1536

/***********************************************************************************************************************
 * Private constants
 **********************************************************************************************************************/
/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/

static err_t lldp_mib_asn1_enc_oid(u8_t * pbuf, const u32_t * oid, u16_t oid_len);
static void lldp_mib_asn1_enc_oid_cnt(const u32_t * oid, u16_t oid_len, u16_t * octets_needed);
static uint32_t LWIP_LLDP_generate_packet (LWIP_LLDP_AGENT_VARIABLES * plldp_agent);
static void LWIP_LLDP_mibConstrInfoLLDPDU (LWIP_LLDP_AGENT_VARIABLES * plldp_agent);
static void LWIP_LLDP_mibConstrShutdownLLDPDU (LWIP_LLDP_AGENT_VARIABLES * plldp_agent);
static void LWIP_LLDP_txFrame (LWIP_LLDP_AGENT_VARIABLES * plldp_agent);
static void LWIP_LLDP_local_mib_verify (LWIP_LLDP_AGENT_VARIABLES * plldp_agent);
static void LWIP_LLDP_local_mib_update (LWIP_LLDP_AGENT_VARIABLES * plldp_agent);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

static uint8_t lldp_packet[LWIP_LLDP_MAXIMUM_FRAME_SIZE] = {0};
static uint8_t old_tx_state[BSP_FEATURE_GMAC_MAX_PORTS] = {0};

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/

extern permanentSysData_t permanentSystemData;

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Initializes the LLDP transmit module.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
void LWIP_LLDP_txInitializeLLDP (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    /* LLDP transmit module initialization */
    /* a) If applicable, either the non-volatile configuration for the LLDP local system MIB shall be retrieved */
    /* or the appropriate default values shall be assigned to all LLDP configuration variables. */
    LWIP_LLDP_local_mib_update(plldp_agent);
    /* Copies old LLDP local system MIB */
    memcpy((void *)&plldp_agent->tx.old_local_system_mib, (void *)&plldp_agent->tx.local_system_mib, sizeof(LWIP_LLDP_TX_TLV_VARIABLES));

    /* b) The internal (implementation specific) data structures shall be initialized with appropriate local */
    /* physical topology information. */
    plldp_agent->tx.tx_timers.txShutdownWhile = 0;
    plldp_agent->tx.tx_timers.txDelayWhile = 0;
    plldp_agent->tx.tx_timers.txTTR = 0;
    plldp_agent->tx.tx_variables.txTTL = 0;
    /* c) The variable somethingChangedLocal shall be set to FALSE */
    plldp_agent->tx.tx_variables.somethingChangedLocal = false;
    /* d) Appropriate values shall be set for the following timing parameters: */
    /* 1) reinitDelay. */
    plldp_agent->tx.tx_parameters.reinitDelay = LWIP_LLDP_TX_DEFAULT_REINITDELAY;
    /* 2) msgTxHold. */
#if defined(LWIP_LLDP_CIP_CONNECTED) && 1 != LWIP_LLDP_CIP_CONNECTED
    plldp_agent->tx.tx_parameters.msgTxHold = LWIP_LLDP_TX_DEFAULT_MSGTXHOLD;
#else
    plldp_agent->tx.tx_parameters.msgTxHold = &g_lldpmanagement.msgTxHold;
    *plldp_agent->tx.tx_parameters.msgTxHold = LWIP_LLDP_TX_DEFAULT_MSGTXHOLD;
#endif
    /* 3) msgTxInterval. */
#if defined(LWIP_LLDP_CIP_CONNECTED) && 1 != LWIP_LLDP_CIP_CONNECTED
    plldp_agent->tx.tx_parameters.msgTxInterval = LWIP_LLDP_TX_DEFAULT_MSGTXINTERVAL;
#else
    plldp_agent->tx.tx_parameters.msgTxInterval = &g_lldpmanagement.msgTxInterval;
    *plldp_agent->tx.tx_parameters.msgTxInterval = LWIP_LLDP_TX_DEFAULT_MSGTXINTERVAL;
#endif
    /* 4) txDelay. */
    plldp_agent->tx.tx_parameters.txDelay = LWIP_LLDP_TX_DEFAULT_TXDELAY;
}

/*******************************************************************************************************************//**
 * @brief Controls transmit state machine.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
void LWIP_LLDP_tx_state_machine (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    /* Updates timer */
    if (plldp_agent->tx.tx_timers.txShutdownWhile != 0) {
        plldp_agent->tx.tx_timers.txShutdownWhile--;
    }
    if (plldp_agent->tx.tx_timers.txDelayWhile != 0) {
        plldp_agent->tx.tx_timers.txDelayWhile--;
    }
    if (plldp_agent->tx.tx_timers.txTTR != 0) {
        plldp_agent->tx.tx_timers.txTTR--;
    }

    switch (plldp_agent->tx.tx_state) {
    case TX_LLDP_INITIALIZE:
        if ((plldp_agent->global.adminStatus == LWIP_LLDP_enabledRxTx) || 
            (plldp_agent->global.adminStatus == LWIP_LLDP_enabledTxOnly)) {
            /* LLDP transmit module initialization */
            /* e) The variable adminStatus shall be interrogated to determine whether initialization should proceed */
            /* further. If the value of adminStatus is ‘disabled’, LLDP transmit module initialization shall be halted */
            /* until the variable portEnabled is equal to TRUE and the value of adminStatus is either of the */
            /* following: */
            /* 1) enabledTxRx: the local LLDP agent can both transmit and receive LLDP frames. */
            /* 2) enabledTxOnly: the local LLDP agent can only transmit LLDP frames. */
            plldp_agent->tx.tx_state = TX_LLDP_IDLE;
        }
        break;
    case TX_LLDP_IDLE:
        LWIP_LLDP_local_mib_verify(plldp_agent);
        if ((plldp_agent->global.adminStatus == LWIP_LLDP_disabled) || 
            (plldp_agent->global.adminStatus == LWIP_LLDP_enabledRxOnly)) {
            plldp_agent->tx.tx_state = TX_LLDP_SHUTDOWN_FRAME;
        } else if ((plldp_agent->tx.tx_timers.txDelayWhile == 0) && ((plldp_agent->tx.tx_timers.txTTR == 0) || 
            (plldp_agent->tx.tx_variables.somethingChangedLocal == true))) {
            /* Frame transmission */
            /* a) An active LLDP agent enabled for transmission shall initiate an LLDP frame transmission whenever */
            /* either of the following events occur: */
            /* 1) Expiration of the transmission countdown timing counter, txTTR, associated with the LLDP */
            /* local system MIB. */
            /* 2) A condition (status or value) change in one or more objects in the LLDP local system MIB. */
            plldp_agent->tx.tx_state = TX_LLDP_INFO_FRAME;
        }
        break;
    case TX_LLDP_SHUTDOWN_FRAME:
        if (plldp_agent->tx.tx_timers.txShutdownWhile == 0) {
            plldp_agent->tx.tx_state = TX_LLDP_INITIALIZE;
        }
        break;
    case TX_LLDP_INFO_FRAME:
        break;
    default:
        break;
    }

    if (plldp_agent->global.portEnabled == false) {
        plldp_agent->tx.tx_state = TX_LLDP_INITIALIZE;
    }

    if (old_tx_state[plldp_agent->netif->lldp_port_id - 1] != plldp_agent->tx.tx_state) {
        switch (plldp_agent->tx.tx_state) {
        case TX_LLDP_INITIALIZE:
            LWIP_LLDP_txInitializeLLDP(plldp_agent);
            break;
        case TX_LLDP_IDLE:
#if defined(LWIP_LLDP_CIP_CONNECTED) && 1 != LWIP_LLDP_CIP_CONNECTED
            plldp_agent->tx.tx_variables.txTTL = (uint16_t)LWIP_MIN(65535, (plldp_agent->tx.tx_parameters.msgTxInterval * plldp_agent->tx.tx_parameters.msgTxHold));
        	plldp_agent->tx.tx_timers.txTTR = (uint16_t)plldp_agent->tx.tx_parameters.msgTxInterval;
#else
        	plldp_agent->tx.tx_variables.txTTL = (uint16_t)LWIP_MIN(65535, (*plldp_agent->tx.tx_parameters.msgTxInterval * *plldp_agent->tx.tx_parameters.msgTxHold));
        	plldp_agent->tx.tx_timers.txTTR = (uint16_t)*plldp_agent->tx.tx_parameters.msgTxInterval;
#endif
        	plldp_agent->tx.tx_variables.somethingChangedLocal = false;
            plldp_agent->tx.tx_timers.txDelayWhile = (uint16_t) plldp_agent->tx.tx_parameters.txDelay;
            break;
        case TX_LLDP_SHUTDOWN_FRAME:
            LWIP_LLDP_mibConstrShutdownLLDPDU(plldp_agent);
            LWIP_LLDP_txFrame(plldp_agent);
            plldp_agent->tx.tx_timers.txShutdownWhile = (uint16_t)plldp_agent->tx.tx_parameters.reinitDelay;
            break;
        case TX_LLDP_INFO_FRAME:
            LWIP_LLDP_mibConstrInfoLLDPDU(plldp_agent);
            LWIP_LLDP_txFrame(plldp_agent);
            /* UCT */
            plldp_agent->tx.tx_state = TX_LLDP_IDLE;
#if defined(LWIP_LLDP_CIP_CONNECTED) && 1 != LWIP_LLDP_CIP_CONNECTED
            plldp_agent->tx.tx_variables.txTTL = (uint16_t)LWIP_MIN(65535, (plldp_agent->tx.tx_parameters.msgTxInterval * plldp_agent->tx.tx_parameters.msgTxHold));
            plldp_agent->tx.tx_timers.txTTR = (uint16_t)plldp_agent->tx.tx_parameters.msgTxInterval;
#else
            plldp_agent->tx.tx_variables.txTTL = (uint16_t)LWIP_MIN(65535, (*plldp_agent->tx.tx_parameters.msgTxInterval * *plldp_agent->tx.tx_parameters.msgTxHold));
            plldp_agent->tx.tx_timers.txTTR = (uint16_t)*plldp_agent->tx.tx_parameters.msgTxInterval;
#endif
            plldp_agent->tx.tx_variables.somethingChangedLocal = false;
            plldp_agent->tx.tx_timers.txDelayWhile = (uint16_t)plldp_agent->tx.tx_parameters.txDelay;
            break;
        default:
            break;
        }
    }

    old_tx_state[plldp_agent->netif->lldp_port_id - 1] = plldp_agent->tx.tx_state;
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Encodes object identifier into a pbuf chained ASN1 msg.
 *
 * @param[out] pbuf      Pointer to a pbuf stream.
 * @param[in] oid        Pointer to object identifier array.
 * @param[in] oid_len    Object identifier array length.
 *
 * @return ERR_OK if successful, ERR_ARG if we can't (or won't) encode
 **********************************************************************************************************************/
static err_t lldp_mib_asn1_enc_oid(u8_t * pbuf, const u32_t * oid, u16_t oid_len)
{
    if (oid_len > 1) {
        /* write compressed first two sub id's */
        u32_t compressed_byte = ((oid[0] * 40) + oid[1]);
        pbuf[0] = (u8_t)compressed_byte;
        oid_len -= 2;
        oid += 2;
    } else {
        /* @bug:  allow empty varbinds for symmetry (we must decode them for getnext), allow partial compression?? */
        /* ident_len <= 1, at least we need zeroDotZero (0.0) (ident_len == 2) */
        return ERR_ARG;
    }

    while (oid_len > 0) {
        u32_t sub_id;
        u8_t shift, tail;

        oid_len--;
        sub_id = *oid;
        tail = 0;
        shift = 28;
        while (shift > 0) {
            u8_t code;

            code = (u8_t)(sub_id >> shift);
            if ((code != 0) || (tail != 0)) {
                tail = 1;
                pbuf++;
                pbuf[0] = code | 0x80;
            }
            shift -= 7;
        }
        pbuf++;
        pbuf[0] = (u8_t)sub_id & 0x7F;

        /* proceed to next sub-identifier */
        oid++;
    }
    return ERR_OK;
}

/*******************************************************************************************************************//**
 * @brief Returns octet count for an object identifier.
 *
 * @param[in] oid               Pointer to object identifier array.
 * @param[in] oid_len           Object identifier array length.
 * @param[out] octets_needed    Pointer to the return value.
 **********************************************************************************************************************/
static void lldp_mib_asn1_enc_oid_cnt(const u32_t * oid, u16_t oid_len, u16_t * octets_needed)
{
  u32_t sub_id;

  *octets_needed = 0;
  if (oid_len > 1) {
    /* compressed prefix in one octet */
    (*octets_needed)++;
    oid_len -= 2;
    oid += 2;
  }
  while (oid_len > 0) {
    oid_len--;
    sub_id = *oid;

    sub_id >>= 7;
    (*octets_needed)++;
    while (sub_id > 0) {
      sub_id >>= 7;
      (*octets_needed)++;
    }
    oid++;
  }
}
/*******************************************************************************************************************//**
 * @brief Generates LLDP packet.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 *
 * @retval        The length of generated LLDP packet
 **********************************************************************************************************************/
static uint32_t LWIP_LLDP_generate_packet (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    uint32_t packet_offset = 0;
    uint8_t lldp_mac[] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e};
    uint16_t frametype = LWIP_LLDP_ETH_TYPE;
    uint16_t control_tag = 0;
    uint16_t control_data = 0;
    uint32_t control_data2 = 0;
    uint32_t i = 0;
    LWIP_LLDP_transmit_func * p_tx_func = (LWIP_LLDP_transmit_func *)plldp_agent->tx.p_tx_func;

    packet_offset = 0;
    /* Ethernet destination address */
    memcpy((void *)&lldp_packet[packet_offset], (void *)&lldp_mac[0], LWIP_LLDP_MAC_ADD_LEN);
    packet_offset += LWIP_LLDP_MAC_ADD_LEN;
    /* Ethernet source address */
    memcpy((void *)&lldp_packet[packet_offset], (void *)&plldp_agent->netif->hwaddr[0], LWIP_LLDP_MAC_ADD_LEN);
    packet_offset += LWIP_LLDP_MAC_ADD_LEN;
    if (ETHSW_SPECIFIC_TAG_ENABLE == ((ethsw_extend_cfg_t *) g_ethsw0_cfg.p_extend)->specific_tag) {
        /* ControlTag */
        control_tag = ((ethsw_extend_cfg_t *) g_ethsw0_cfg.p_extend)->specific_tag_id;
        control_tag = htons(control_tag);
        memcpy((void *)&lldp_packet[packet_offset], (void *)&control_tag, sizeof(control_tag));
        packet_offset += sizeof(control_tag);
        /* ControlData */
        control_data = 3;
        control_data = htons(control_data);
        memcpy((void *)&lldp_packet[packet_offset], (void *)&control_data, sizeof(control_data));
        packet_offset += sizeof(control_data);
        /* ControlData2 */
        if( plldp_agent->netif->lldp_port_id == 1 ){
            control_data2 = 1;
        } else if( plldp_agent->netif->lldp_port_id == 2 ){
            control_data2 = 2;
        } else if( plldp_agent->netif->lldp_port_id == 3 ){
            control_data2 = 4;
        }
        control_data2 = htonl(control_data2);
        memcpy((void *)&lldp_packet[packet_offset], (void *)&control_data2, sizeof(control_data2));
        packet_offset += sizeof(control_data2);
    }
    /* Ethernet frame type */
    frametype = htons(frametype);
    memcpy((void *)&lldp_packet[packet_offset], (void *)&frametype, sizeof(frametype));
    packet_offset += sizeof(frametype);

    while (p_tx_func[i] != NULL) {
        packet_offset += p_tx_func[i](plldp_agent, (uint8_t *)&lldp_packet[packet_offset]);
        i++;
    }

    return packet_offset;
}

/*******************************************************************************************************************//**
 * @brief Transmits LLDP for lwIP.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_transmit (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    fsp_err_t err = FSP_SUCCESS;
    uint32_t length;

    length = LWIP_LLDP_generate_packet(plldp_agent);
    err = g_ether0.p_api->write(g_ether0.p_ctrl, (void *)lldp_packet, length);
    if (err != FSP_SUCCESS) {
        return;
    }
}

/*******************************************************************************************************************//**
 * @brief Transmits End Of LLDPDU.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_end (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
	/* Unused parameter */
    LWIP_UNUSED_ARG(plldp_agent);

    uint16_t offset = 0;
    uint16_t type_length = 0;

    offset += 2;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_0 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits Chassis ID.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_chassis_id (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint16_t type_length = 0;

    offset += 2;
    /* chassis ID subtype */
    pbuf[offset] = plldp_agent->tx.local_system_mib.chassis_id_subtype;
    offset++;

    /* chassis ID */
    memcpy((void *)&pbuf[offset], (void *)&plldp_agent->tx.local_system_mib.chassis_id[0], plldp_agent->tx.local_system_mib.chassis_id_len);
    offset += plldp_agent->tx.local_system_mib.chassis_id_len;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_1 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits Port ID.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_port_id (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint16_t type_length = 0;

    offset += 2;
    /* port ID subtype */
    pbuf[offset] = plldp_agent->tx.local_system_mib.port_id_subtype;
    offset++;

    /* port ID */
    memcpy((void *)&pbuf[offset], (void *)&plldp_agent->tx.local_system_mib.port_id[0], plldp_agent->tx.local_system_mib.port_id_len);
    offset += plldp_agent->tx.local_system_mib.port_id_len;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_2 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits Time To Live.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_ttl (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint16_t ttl = 0;
    uint16_t type_length = 0;

    offset += 2;
    /* time to live (TTL) */
    ttl = plldp_agent->tx.tx_variables.txTTL;
    ttl = htons(ttl);
    memcpy((void *)&pbuf[offset], &ttl, sizeof(ttl));
    offset += sizeof(ttl);

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_3 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits Time To Live with zero.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_ttl_zero (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
	/* Unused parameter */
    LWIP_UNUSED_ARG(plldp_agent);

    uint16_t offset = 0;
    uint16_t ttl = 0;
    uint16_t type_length = 0;

    offset += 2;
    /* time to live (TTL) */
    ttl = 0;
    ttl = htons(ttl);
    memcpy((void *)&pbuf[offset], &ttl, sizeof(ttl));
    offset += sizeof(ttl);

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_3 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits Port Description.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_port_desc (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint16_t type_length = 0;

    offset += 2;
    /* port description */
    memcpy((void *)&pbuf[offset], (void *)&plldp_agent->tx.local_system_mib.port_desc[0], plldp_agent->tx.local_system_mib.port_desc_len);
    offset += plldp_agent->tx.local_system_mib.port_desc_len;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_4 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits System Name.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_system_name (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint16_t type_length = 0;

    offset += 2;
    /* system name */
    memcpy((void *)&pbuf[offset], (void *)&plldp_agent->tx.local_system_mib.system_name[0], plldp_agent->tx.local_system_mib.system_name_len);
    offset += plldp_agent->tx.local_system_mib.system_name_len;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_5 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits System Description.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_system_desc (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint16_t type_length = 0;

    offset += 2;
    /* port description */
    memcpy((void *)&pbuf[offset], (void *)&plldp_agent->tx.local_system_mib.system_desc[0], plldp_agent->tx.local_system_mib.system_desc_len);
    offset += plldp_agent->tx.local_system_mib.system_desc_len;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_6 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits System Capabilities.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_system_capa (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint16_t system_capa = 0;
    uint16_t enabled_capa = 0;
    uint16_t type_length = 0;

    offset += 2;
    /* system capabilities */
    system_capa = htons(plldp_agent->tx.local_system_mib.system_capa);
    memcpy((void *)&pbuf[offset], &system_capa, sizeof(system_capa));
    offset += sizeof(system_capa);

    /* enabled capabilities */
    enabled_capa = htons(plldp_agent->tx.local_system_mib.enabled_capa);
    memcpy((void *)&pbuf[offset], &enabled_capa, sizeof(enabled_capa));
    offset += sizeof(enabled_capa);

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_7 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmits Management Address.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[out] pbuf              Pointer to ethernet frame buffer.
 *
 * @retval        The offset of this tlv
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_mana_add (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint16_t type_length = 0;
    uint32_t system_port_number = 0;

    offset += 2;
    /* management address string length */
    pbuf[offset] = plldp_agent->tx.local_system_mib.mana_add_len;
    offset++;
    /* management address subtype */
    pbuf[offset] = plldp_agent->tx.local_system_mib.mana_add_subtype;
    offset++;
    /* management address */
    memcpy((void *)&pbuf[offset], (void *)&plldp_agent->tx.local_system_mib.mana_add[0], (plldp_agent->tx.local_system_mib.mana_add_len - LWIP_LLDP_MANA_ADD_SUBTYPE_LEN));
    offset += (uint16_t) (plldp_agent->tx.local_system_mib.mana_add_len - LWIP_LLDP_MANA_ADD_SUBTYPE_LEN);

    /* interface numbering subtype */
    pbuf[offset] = plldp_agent->tx.local_system_mib.interface_num_subtype;
    offset++;
    /* interface number */
    system_port_number = htonl(plldp_agent->tx.local_system_mib.interface_num);
    memcpy((void *)&pbuf[offset], (void *)&system_port_number, sizeof(system_port_number));
    offset += sizeof(system_port_number);

    /* object identifier (OID) string length */
    pbuf[offset] = (uint8_t) plldp_agent->tx.local_system_mib.oid_len;
    offset++;

    /* object identifier */
    memcpy((void *)&pbuf[offset], (void *)&plldp_agent->tx.local_system_mib.oid[0], plldp_agent->tx.local_system_mib.oid_len);
    offset += plldp_agent->tx.local_system_mib.oid_len;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_8 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

#if LWIP_LLDP_SUPPORT_CIP_TLV
/*******************************************************************************************************************//**
 * @brief Transmit CIP Identification.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_odva_cip_id (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint8_t oui[3] = {0x00, 0x21, 0x6C}; //ODVA = 00:21:6c
    uint16_t type_length = 0;
    uint32_t val;

    offset += 2;

    /* OUI */
    memcpy((void *)&pbuf[offset], (void *)&oui, sizeof(oui));
    offset += sizeof(oui);

    /* Subtype */
    pbuf[offset] = 9;  //CIP Identification = 9
    offset++;

    /* CIP Electronic Key */
    /* Get the values from the local MIB and change to Big-endian. */
    val = *plldp_agent->tx.local_system_mib.cip_vendor_id;
    val = htons((uint16_t)val);
    memcpy((void *)&pbuf[offset], &val, sizeof(&plldp_agent->tx.local_system_mib.cip_vendor_id));
    offset += sizeof(CipUint);
    val = *plldp_agent->tx.local_system_mib.cip_device_type;
    val = htons((uint16_t)val);
    memcpy((void *)&pbuf[offset], &val, sizeof(&plldp_agent->tx.local_system_mib.cip_device_type));
    offset += sizeof(CipUint);
    val = *plldp_agent->tx.local_system_mib.cip_product_code;
    val = htons((uint16_t)val);
    memcpy((void *)&pbuf[offset], &val, sizeof(&plldp_agent->tx.local_system_mib.cip_product_code));
    offset += sizeof(CipUint);
    memcpy((void *)&pbuf[offset], plldp_agent->tx.local_system_mib.cip_revision, sizeof(&plldp_agent->tx.local_system_mib.cip_revision));
    offset += sizeof(CipRevision);
    val = *plldp_agent->tx.local_system_mib.cip_serial_number;
    val = htonl(val);
    memcpy((void *)&pbuf[offset], &val, sizeof(&plldp_agent->tx.local_system_mib.cip_serial_number));
    offset += sizeof(CipUdint);

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_127 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmit CIP MAC Address.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_odva_cip_mac (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint8_t oui[3] = {0x00, 0x21, 0x6C}; //ODVA = 00:21:6c
    uint16_t type_length = 0;

    offset += 2;

    /* OUI */
    memcpy((void *)&pbuf[offset], (void *)&oui, sizeof(oui));
    offset += sizeof(oui);

    /* Subtype */
    pbuf[offset] = 2;  //CIP MAC Address = 2
    offset++;

    /* MAC Address */
    memcpy((void *)&pbuf[offset], &plldp_agent->tx.local_system_mib.cip_mac_address[0], LWIP_LLDP_CIP_MAC_TLV_LEN);
    offset += LWIP_LLDP_CIP_MAC_TLV_LEN;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_127 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}

/*******************************************************************************************************************//**
 * @brief Transmit CIP Interface Label.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static uint16_t LWIP_LLDP_transmit_odva_cip_if_label (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t * pbuf)
{
    uint16_t offset = 0;
    uint8_t oui[3] = {0x00, 0x21, 0x6C}; //ODVA = 00:21:6c
    uint16_t type_length = 0;

    offset += 2;

    /* OUI */
    memcpy((void *)&pbuf[offset], (void *)&oui, sizeof(oui));
    offset += sizeof(oui);

    /* Subtype */
    pbuf[offset] = 3;  //CIP MAC Address = 3
    offset++;

    /* Interface Label */
    memcpy((void *)&pbuf[offset], (void *)&plldp_agent->tx.local_system_mib.cip_if_label[0], plldp_agent->tx.local_system_mib.cip_if_label_len);
    offset += plldp_agent->tx.local_system_mib.cip_if_label_len;

    /* TLV type */
    type_length = (LWIP_LLDP_TLV_TYPE_127 << 9);
    type_length &= LWIP_LLDP_TLV_TYPE_MASK;

    /* TLV information string length */
    type_length |= ((offset - 2) & LWIP_LLDP_TLV_LEN_MASK);

    type_length = htons(type_length);
    memcpy((void *)&pbuf[0], (void *)&type_length, sizeof(type_length));

    return(offset);
}
#endif

/*******************************************************************************************************************//**
 * @brief Constructs an information LLDPDU.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_mibConstrInfoLLDPDU (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    uint16_t offset = 0;

    /* Normal LLDPDU construction */
    memset((void *)&plldp_agent->tx.p_tx_func[0], (int) NULL, sizeof(plldp_agent->tx.p_tx_func));
    /* a) Three mandatory TLVs as specified in 9.2: */
    /* 1) Chassis ID TLV (see 9.5.2). */
    plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_chassis_id;
    /* 2) Port ID TLV (see 9.5.3). */
    plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_port_id;
    /* 3) Time To Live TLV, with the TTL value set equal to txTTL (see 9.5.4). */
    plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_ttl;
    /* b) Additional optional TLVs from either the basic management set or from one or more */
    /* organizationally specific sets, as allowed by LLDPDU length restrictions and as selected in the */
    /* LLDP local system MIB by network management (see Table 9-1, Table F-1, and Table G-1). */
    if (plldp_agent->tlv.mibBasicTLVsTxEnable & LWIP_LLDP_BASIC_TLV_CAPA_BIT_0) {
        plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_port_desc;
    }
    if (plldp_agent->tlv.mibBasicTLVsTxEnable & LWIP_LLDP_BASIC_TLV_CAPA_BIT_1) {
        plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_system_name;
    }
    if (plldp_agent->tlv.mibBasicTLVsTxEnable & LWIP_LLDP_BASIC_TLV_CAPA_BIT_2) {
        plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_system_desc;
    }
    if (plldp_agent->tlv.mibBasicTLVsTxEnable & LWIP_LLDP_BASIC_TLV_CAPA_BIT_3) {
        plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_system_capa;
    }
    if (plldp_agent->tlv.mibMgmtAddrInstanceTxEnable == true) {
        plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_mana_add;
    }
#if LWIP_LLDP_SUPPORT_CIP_TLV
    if (plldp_agent->tlv.mibCipInstanceTxEnable == true) {
        plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_odva_cip_id;
        plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_odva_cip_mac;
        plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_odva_cip_if_label;
    }
#endif
    /* c) An End Of LLDPDU TLV. */
    plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_end;
}

/*******************************************************************************************************************//**
 * @brief Constructs a shutdown LLDPDU.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_mibConstrShutdownLLDPDU (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    uint16_t offset = 0;

    /* Shutdown LLDPDU construction */
    memset((void *)&plldp_agent->tx.p_tx_func[0], (int) NULL, sizeof(plldp_agent->tx.p_tx_func));
    /* 1) The Chassis ID and Port ID TLVs. */
    plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_chassis_id;
    plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_port_id;
    /* 2) The Time To Live TLV with the TTL field set to zero. */
    plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_ttl_zero;
    /* 3) An End Of LLDPDU TLV. */
    plldp_agent->tx.p_tx_func[offset++] = (void *)LWIP_LLDP_transmit_end;
}

/*******************************************************************************************************************//**
 * @brief Prepends the source and destinations addresses and the LLDP Ethertype to each LLDPDU.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_txFrame (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    LWIP_LLDP_transmit(plldp_agent);
    /* LLDP frame formatting/transmission */
    /* c) Increment the statsFramesOutTotal counter. */
    plldp_agent->tx.tx_counters.statsFramesOutTotal++;
}

/*******************************************************************************************************************//**
 * @brief Verifies that the LLDP Local System MIB has changed.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_local_mib_verify (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    LWIP_LLDP_local_mib_update(plldp_agent);

    if (memcmp((void *)&plldp_agent->tx.old_local_system_mib, (void *)&plldp_agent->tx.local_system_mib, sizeof(LWIP_LLDP_TX_TLV_VARIABLES)) != 0) {
        /* the LLDP local system MIB has changed. */
        plldp_agent->tx.tx_variables.somethingChangedLocal = true;

#if defined(LWIP_LLDP_CIP_CONNECTED) && LWIP_LLDP_CIP_CONNECTED
        /* The macro OPENER_HAS_LLDP_FUNC is defined for OpENer (EtherNet/IP protocol stack).   */
        /* And the variable g_lldpmanagement is prepared by OpENer.								*/
        /* This following code is valid if OpENer exists.										*/
        g_lldpmanagement.last_change = (sys_now() / 10);
#endif
        /* Copies old LLDP local system MIB */
        memcpy((void *)&plldp_agent->tx.old_local_system_mib, (void *)&plldp_agent->tx.local_system_mib, sizeof(LWIP_LLDP_TX_TLV_VARIABLES));
    }
}

/*******************************************************************************************************************//**
 * @brief Updates the LLDP Local System MIB.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_local_mib_update (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    const struct snmp_obj_id *oid = snmp_get_device_enterprise_oid();

    memset((void *)&plldp_agent->tx.local_system_mib, 0, sizeof(plldp_agent->tx.local_system_mib));
    /* Chassis ID TLV objects */
    plldp_agent->tx.local_system_mib.chassis_id_subtype = LWIP_LLDP_CHASSIS_ID_SUBTYPE_4;
    plldp_agent->tx.local_system_mib.chassis_id_len = LWIP_LLDP_MAC_ADD_LEN;
    memcpy( (void *)&plldp_agent->tx.local_system_mib.chassis_id[0], (void *)&plldp_agent->netif->hwaddr[0], plldp_agent->tx.local_system_mib.chassis_id_len );
    /* Port ID TLV objects */
    plldp_agent->tx.local_system_mib.port_id_subtype = LWIP_LLDP_PORT_ID_SUBTYPE_5;
    plldp_agent->tx.local_system_mib.port_id_len = (uint16_t)strlen(plldp_agent->netif->name);
    memcpy( (void *)&plldp_agent->tx.local_system_mib.port_id[0], (void *)&plldp_agent->netif->name[0], plldp_agent->tx.local_system_mib.port_id_len );
    /* Port description TLV object */
    plldp_agent->tx.local_system_mib.port_desc_len = (uint16_t)strlen(plldp_agent->netif->name);
    memcpy( (void *)&plldp_agent->tx.local_system_mib.port_desc[0], (void *)&plldp_agent->netif->name[0], plldp_agent->tx.local_system_mib.port_desc_len );
    /* System name TLV object */
    plldp_agent->tx.local_system_mib.system_name_len = (uint8_t)strlen((const char *)permanentSystemData.snmp_mib_sys_name);
    memcpy( (void *)&plldp_agent->tx.local_system_mib.system_name[0], (void *)&permanentSystemData.snmp_mib_sys_name[0], plldp_agent->tx.local_system_mib.system_name_len );
    /* System description TLV object */
    plldp_agent->tx.local_system_mib.system_desc_len = (uint8_t)strlen((const char *)permanentSystemData.snmp_mib_sys_descr);
    memcpy( (void *)&plldp_agent->tx.local_system_mib.system_desc[0], (void *)&permanentSystemData.snmp_mib_sys_descr[0], plldp_agent->tx.local_system_mib.system_desc_len );
    /* System capabilities TLV objects */
    plldp_agent->tx.local_system_mib.system_capa = (LWIP_LLDP_SYSTEM_CAPA_BIT_0 | LWIP_LLDP_SYSTEM_CAPA_BIT_2);
    plldp_agent->tx.local_system_mib.enabled_capa = (LWIP_LLDP_SYSTEM_CAPA_BIT_0 | LWIP_LLDP_SYSTEM_CAPA_BIT_2);
    /* Management address TLV objects */
    plldp_agent->tx.local_system_mib.mana_add_len = sizeof(plldp_agent->netif->ip_addr.addr) + LWIP_LLDP_MANA_ADD_SUBTYPE_LEN;
    plldp_agent->tx.local_system_mib.mana_add_subtype = LWIP_LLDP_MANA_ADD_SUBTYPE_1;
    memcpy( (void *)&plldp_agent->tx.local_system_mib.mana_add[0], (void *)&plldp_agent->netif->ip_addr.addr, (plldp_agent->tx.local_system_mib.mana_add_len - LWIP_LLDP_MANA_ADD_SUBTYPE_LEN) );
    plldp_agent->tx.local_system_mib.interface_num_subtype = LWIP_LLDP_INTERFACE_NUM_SUBTYPE_3;
    plldp_agent->tx.local_system_mib.interface_num = (uint32_t)plldp_agent->netif->lldp_port_id;
#if defined(LWIP_LLDP_CIP_CONNECTED) && LWIP_LLDP_CIP_CONNECTED
    /* ODVA CIP Identification TLV objects */
    plldp_agent->tx.local_system_mib.cip_vendor_id = &g_identity.vendor_id;
	plldp_agent->tx.local_system_mib.cip_device_type = &g_identity.device_type;
	plldp_agent->tx.local_system_mib.cip_product_code = &g_identity.product_code;
	plldp_agent->tx.local_system_mib.cip_revision = &g_identity.revision;
	plldp_agent->tx.local_system_mib.cip_serial_number = &g_identity.serial_number;
    memcpy( (void *)&plldp_agent->tx.local_system_mib.cip_mac_address[0], (void *)&plldp_agent->netif->hwaddr[0], LWIP_LLDP_CIP_MAC_TLV_LEN );
    plldp_agent->tx.local_system_mib.cip_if_label_len = (uint16_t)strlen(plldp_agent->netif->name);
    memcpy( (void *)&plldp_agent->tx.local_system_mib.cip_if_label[0], (void *)&plldp_agent->netif->name[0], plldp_agent->tx.local_system_mib.cip_if_label_len );
#endif
    if (lldp_mib_asn1_enc_oid((u8_t *)&plldp_agent->tx.local_system_mib.oid[0], oid->id, oid->len) == ERR_OK) {
        lldp_mib_asn1_enc_oid_cnt(oid->id, oid->len, (u16_t *)&plldp_agent->tx.local_system_mib.oid_len);
    } else {
        plldp_agent->tx.local_system_mib.oid_len = 0;
    }
}


#endif

