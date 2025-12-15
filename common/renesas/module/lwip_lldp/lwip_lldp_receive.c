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
#include "lwip_lldp_receive.h"
/**
 * Include lwIP libraries
 */
#include "lwip/inet.h"
#include "lwip/netifapi.h"
#include "lwip/etharp.h"
#if LWIP_SNMP_APP
#include "lwip/snmp.h"
#include "examples/snmp/snmp_example.h"
#endif
/**
 * Inlcude FreeRTOS libraries
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
/***********************************************************************************************************************
 * Private constants
 **********************************************************************************************************************/
/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/

static int LWIP_LLDP_frame_recognition (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint32_t len, uint8_t * pbuf);
static int LWIP_LLDP_LLDPDU_validation (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint32_t len, uint8_t * pbuf, LWIP_LLDP_RX_TLV_VARIABLES * p_rcv_tlv);
static int LWIP_LLDP_TLV_MIB_object_value_comparison (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint32_t len, uint8_t * pbuf, LWIP_LLDP_RX_TLV_VARIABLES * p_rcv_tlv);
static void LWIP_LLDP_mibDeleteObjects (LWIP_LLDP_AGENT_VARIABLES * plldp_agent);
static void LWIP_LLDP_rxInitializeLLDP (LWIP_LLDP_AGENT_VARIABLES * plldp_agent);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

static uint8_t old_rx_state[BSP_FEATURE_GMAC_MAX_PORTS] = {0}; /* Old receive state machine */

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Receives LLDP frame.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[in] len                The length of ethernet frame.
 * @param[in] pbuf               Pointer to ethernet frame buffer.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
int LWIP_LLDP_receive (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint32_t len, uint8_t * pbuf)
{
    LWIP_LLDP_RX_TLV_VARIABLES rcv_tlv = {0};

    /* Receive state machine checks */
    if (plldp_agent->rx.rx_state != RX_LLDP_WAIT_FOR_FRAME) {
        return -1;
    }

    /* LLDP frame recognition */
    if (LWIP_LLDP_frame_recognition(plldp_agent, len, pbuf) != 0) {
        return -1;
    }

    /* rcvFrame checks */
    if (plldp_agent->rx.rx_variables.rcvFrame != true) {
        return -1;
    }
    /* RX_FRAME */
    plldp_agent->rx.rx_state = RX_LLDP_FRAME;
    plldp_agent->rx.rx_variables.rxChanges = false;
    plldp_agent->rx.rx_variables.rcvFrame = false;

    /* LLDPDU validation */
    if (LWIP_LLDP_LLDPDU_validation(plldp_agent, len, pbuf, &rcv_tlv) != 0) {
        plldp_agent->rx.rx_state = RX_LLDP_WAIT_FOR_FRAME;
        plldp_agent->rx.rx_variables.badFrame = false;
        plldp_agent->rx.rx_variables.rxInfoAge = false;
        plldp_agent->rx.rx_variables.somethingChangedRemote = false;
        return -1;
    }

    /* TLV/MIB object value comparison */
    if (LWIP_LLDP_TLV_MIB_object_value_comparison(plldp_agent, len, pbuf, &rcv_tlv) != 0) {
        plldp_agent->rx.rx_state = RX_LLDP_WAIT_FOR_FRAME;
        plldp_agent->rx.rx_variables.badFrame = false;
        plldp_agent->rx.rx_variables.rxInfoAge = false;
        plldp_agent->rx.rx_variables.somethingChangedRemote = false;
        return -1;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Controls receive state machine.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
void LWIP_LLDP_rx_state_machine (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    /* Update timer */
    if (plldp_agent->rx.rx_timers.rxInfoTTL != 0) {
        plldp_agent->rx.rx_timers.rxInfoTTL--;
        /* Checks timeout Timer */
        if (plldp_agent->rx.rx_timers.rxInfoTTL == 0) {
            plldp_agent->rx.rx_variables.rxInfoAge = true;
        }
    }
    if (plldp_agent->rx.rx_timers.TooManyNeighborsTimer != 0) {
        plldp_agent->rx.rx_timers.TooManyNeighborsTimer--;
    }

    switch (plldp_agent->rx.rx_state) {
    case RX_LLDP_WAIT_PORT_OPERATIONAL:
        if (plldp_agent->rx.rx_variables.rxInfoAge == true) {
            plldp_agent->rx.rx_state = RX_LLDP_DELETE_AGED_INFO;
        } else if (plldp_agent->global.portEnabled == true) {
            plldp_agent->rx.rx_state = RX_LLDP_INITIALIZE;
        }
        break;
    case RX_LLDP_DELETE_AGED_INFO:
        break;
    case RX_LLDP_INITIALIZE:
        /* LLDP receive module initialization */
        /* b) The adminStatus object shall be interrogated to determine whether initialization should proceed */
        /* further. If the value of adminStatus is ‘disabled’, LLDP receive initialization shall be halted until the */
        /* value of adminStatus is one of the following: */
        /* 1) enabledTxRx: the local LLDP agent can both transmit and receive LLDP frames. */
        /* 2) enabledRxOnly: the local LLDP agent can only receive LLDP frames. */
        if ((plldp_agent->global.adminStatus == LWIP_LLDP_enabledRxTx) ||
            (plldp_agent->global.adminStatus == LWIP_LLDP_enabledRxOnly)) {
            plldp_agent->rx.rx_state = RX_LLDP_WAIT_FOR_FRAME;
        }
        break;
    case RX_LLDP_WAIT_FOR_FRAME:
        if (plldp_agent->rx.rx_variables.rxInfoAge == true) {
            plldp_agent->rx.rx_state = RX_LLDP_DELETE_INFO;
            plldp_agent->rx.rx_counters.statsAgeoutsTotal++;
            plldp_agent->rx.stats_rem_tables.ageouts++;
            MIB2_COPY_SYSUPTIME_TO((u32_t *)&plldp_agent->rx.stats_rem_tables.last_change_time);
        } else if ((plldp_agent->global.adminStatus == LWIP_LLDP_disabled) ||
            (plldp_agent->global.adminStatus == LWIP_LLDP_enabledTxOnly)) {
            plldp_agent->rx.rx_state = RX_LLDP_INITIALIZE;
        }
        break;
    default:
        break;
    }

    if ((plldp_agent->rx.rx_variables.rxInfoAge == false) &&
        (plldp_agent->global.portEnabled == false)) {
        plldp_agent->rx.rx_state = RX_LLDP_WAIT_PORT_OPERATIONAL;
    }

    if (old_rx_state[plldp_agent->netif->lldp_port_id - 1] != plldp_agent->rx.rx_state) {
        switch (plldp_agent->rx.rx_state) {
        case RX_LLDP_WAIT_PORT_OPERATIONAL:
            break;
        case RX_LLDP_DELETE_AGED_INFO:
            plldp_agent->rx.rx_variables.somethingChangedRemote = false;
            LWIP_LLDP_mibDeleteObjects(plldp_agent);
            plldp_agent->rx.rx_variables.rxInfoAge = false;
            plldp_agent->rx.rx_variables.somethingChangedRemote = true;
            /* UCT */
            plldp_agent->rx.rx_state = RX_LLDP_WAIT_PORT_OPERATIONAL;
            break;
        case RX_LLDP_INITIALIZE:
            LWIP_LLDP_rxInitializeLLDP(plldp_agent);
            plldp_agent->rx.rx_variables.rcvFrame = false;
            break;
        case RX_LLDP_WAIT_FOR_FRAME:
            plldp_agent->rx.rx_variables.badFrame = false;
            plldp_agent->rx.rx_variables.rxInfoAge = false;
            plldp_agent->rx.rx_variables.somethingChangedRemote = false;
            break;
        case RX_LLDP_DELETE_INFO:
            LWIP_LLDP_mibDeleteObjects(plldp_agent);
            plldp_agent->rx.rx_variables.somethingChangedRemote = true;
            /* UCT */
            plldp_agent->rx.rx_state = RX_LLDP_WAIT_FOR_FRAME;
            plldp_agent->rx.rx_variables.badFrame = false;
            plldp_agent->rx.rx_variables.rxInfoAge = false;
            plldp_agent->rx.rx_variables.somethingChangedRemote = false;
            break;
        default:
            break;
        }
    }

    old_rx_state[plldp_agent->netif->lldp_port_id - 1] = plldp_agent->rx.rx_state;
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Recognizes ethernet frame.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[in] len                The length of ethernet frame.
 * @param[in] pbuf               Pointer to ethernet frame buffer.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
static int LWIP_LLDP_frame_recognition (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint32_t len, uint8_t * pbuf)
{
    uint16_t ethertype = 0;
    uint8_t lldp_multi[LWIP_LLDP_MAC_ADD_LEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E};

    LWIP_UNUSED_ARG(len);

    /* LLDP frame recognition */
    /* a) The destination address value is the assigned LWIP_LLDP_Multicast address defined in 8.1. */
    if (memcmp((void *)&pbuf[0], (void *)&lldp_multi[0], LWIP_LLDP_MAC_ADD_LEN) != 0) {
        return -1;
    }
    /* b) The Ethertype value is the LLDP Ethertype defined in 8.3 for the particular frame format. */
    memcpy((void *)&ethertype, (void *)&pbuf[12], sizeof(ethertype));
    ethertype = htons(ethertype);
    if (ethertype != LWIP_LLDP_ETH_TYPE) {
        return -1;
    }

    /* If both of the above conditions are TRUE, the global variable rcvFrame shall be set to TRUE and the frame */
    /* shall be sent to the LLDP receive module for validation. */
    plldp_agent->rx.rx_variables.rcvFrame = true;

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Validates Basic management TLV.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[in] tlv_type           TLV type
 * @param[in] tlv_length         TLV information string length
 * @param[in] ptlv_buf           Pointer to LLDPPDU TLV buffer.
 * @param[out] p_rcv_tlv         Pointer to received LLDP TLV variables.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
static int LWIP_LLDP_general_validation (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t tlv_type, uint16_t tlv_length, uint8_t * ptlv_buf, LWIP_LLDP_RX_TLV_VARIABLES * p_rcv_tlv)
{
    uint16_t offset = 0;
    uint16_t system_capa = 0;
    uint16_t enabled_capa = 0;
    uint8_t mana_add_str_len = 0;
    uint8_t interface_num_subtype = 0;
    uint32_t interface_num = 0;
    uint8_t oid_len = 0;

    switch (tlv_type) {
    case LWIP_LLDP_TLV_TYPE_0:
        break;
    case LWIP_LLDP_TLV_TYPE_1:
    case LWIP_LLDP_TLV_TYPE_2:
    case LWIP_LLDP_TLV_TYPE_3:
        /* a) If the LLDPDU contains more than one Chassis ID TLV, Port ID TLV, or Time To Live TLV: */
        /* 1) The LLDPDU shall be discarded. */
        /* 2) The statsFramesDiscardedTotal and statsFramesInErrorsTotal counters shall both be */
        /* incremented. */
        plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
        plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
        plldp_agent->rx.rx_variables.badFrame = true;
        return -1;
    case LWIP_LLDP_TLV_TYPE_4:
        if (tlv_length > LWIP_LLDP_PORT_DESC_TLV_MAX_LEN) {
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
            plldp_agent->rx.rx_variables.badFrame = true;
            return -1;
        }
        /* Port description TLV object */
        p_rcv_tlv->port_desc_len = tlv_length;
        memcpy((void *)&p_rcv_tlv->port_desc[0], (void *)&ptlv_buf[LWIP_LLDP_TLV_HEADER_LEN], p_rcv_tlv->port_desc_len);
        break;
    case LWIP_LLDP_TLV_TYPE_5:
        if (tlv_length > LWIP_LLDP_SYSTEM_NAME_TLV_MAX_LEN) {
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
            plldp_agent->rx.rx_variables.badFrame = true;
            return -1;
        }
        /* System name TLV object */
        p_rcv_tlv->system_name_len = tlv_length;
        memcpy((void *)&p_rcv_tlv->system_name[0], (void *)&ptlv_buf[LWIP_LLDP_TLV_HEADER_LEN], p_rcv_tlv->system_name_len);
        break;
    case LWIP_LLDP_TLV_TYPE_6:
        if (tlv_length > LWIP_LLDP_SYSTEM_DESC_TLV_MAX_LEN) {
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
            plldp_agent->rx.rx_variables.badFrame = true;
            return -1;
        }
        /* System description TLV object */
        p_rcv_tlv->system_desc_len = tlv_length;
        memcpy((void *)&p_rcv_tlv->system_desc[0], (void *)&ptlv_buf[LWIP_LLDP_TLV_HEADER_LEN], p_rcv_tlv->system_desc_len);
        break;
    case LWIP_LLDP_TLV_TYPE_7:
        if (tlv_length < LWIP_LLDP_SYSTEM_CAPA_TLV_LEN) {
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
            plldp_agent->rx.rx_variables.badFrame = true;
            return -1;
        }
        /* System capabilities TLV objects */
        memcpy((void *)&system_capa, (void *)&ptlv_buf[LWIP_LLDP_TLV_HEADER_LEN], LWIP_LLDP_SYSTEM_CAPA_LEN);
        p_rcv_tlv->system_capa = htons(system_capa);
        memcpy((void *)&enabled_capa, (void *)&ptlv_buf[LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_SYSTEM_CAPA_LEN], LWIP_LLDP_ENABLED_CAPA_LEN);
        p_rcv_tlv->enabled_capa = htons(enabled_capa);
        break;
    case LWIP_LLDP_TLV_TYPE_8:
        if ((tlv_length < LWIP_LLDP_MANA_ADD_TLV_MIN_LEN) || (tlv_length > LWIP_LLDP_MANA_ADD_TLV_MAX_LEN)) {
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
            plldp_agent->rx.rx_variables.badFrame = true;
            return -1;
        }
        offset += LWIP_LLDP_TLV_HEADER_LEN;
        /* management address string length */
        mana_add_str_len = ptlv_buf[offset];
        if ((mana_add_str_len < LWIP_LLDP_MANA_ADD_MIN_LEN) || (mana_add_str_len > LWIP_LLDP_MANA_ADD_MAX_LEN)) {
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
            plldp_agent->rx.rx_variables.badFrame = true;
            return -1;
        }
        /* Management address TLV objects */
        p_rcv_tlv->mana_add_len = (mana_add_str_len - LWIP_LLDP_MANA_ADD_SUBTYPE_LEN);
        p_rcv_tlv->mana_add_subtype = ptlv_buf[offset + LWIP_LLDP_MANA_ADD_STR_LEN_LEN];
        memcpy((void *)&p_rcv_tlv->mana_add[0], (void *)&ptlv_buf[offset + (LWIP_LLDP_MANA_ADD_STR_LEN_LEN + LWIP_LLDP_MANA_ADD_SUBTYPE_LEN)], p_rcv_tlv->mana_add_len);
        offset += (uint16_t) (LWIP_LLDP_MANA_ADD_STR_LEN_LEN + mana_add_str_len);
        /* interface numbering subtype */
        interface_num_subtype = ptlv_buf[offset];
        if (((interface_num_subtype != LWIP_LLDP_INTERFACE_NUM_SUBTYPE_1) &&
            (interface_num_subtype != LWIP_LLDP_INTERFACE_NUM_SUBTYPE_2)) &&
            (interface_num_subtype != LWIP_LLDP_INTERFACE_NUM_SUBTYPE_3)) {
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
            plldp_agent->rx.rx_variables.badFrame = true;
            return -1;
        }
        /* Management address TLV objects */
        p_rcv_tlv->interface_num_subtype = interface_num_subtype;
        memcpy((void *)&interface_num, (void *)&ptlv_buf[offset + LWIP_LLDP_INTERFACE_NUM_SUBTYPE_LEN], LWIP_LLDP_INTERFACE_NUM_LEN);
        p_rcv_tlv->interface_num = htonl(interface_num);
        offset += (LWIP_LLDP_INTERFACE_NUM_SUBTYPE_LEN + LWIP_LLDP_INTERFACE_NUM_LEN);
        /* OID string length */
        oid_len = ptlv_buf[offset];
        if (oid_len > LWIP_LLDP_OID_MAX_LEN) {
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
            plldp_agent->rx.rx_variables.badFrame = true;
            return -1;
        }
        /* Management address TLV objects */
        p_rcv_tlv->oid_len = oid_len;
        memcpy((void *)&p_rcv_tlv->oid[0], (void *)&ptlv_buf[offset + LWIP_LLDP_OID_STR_LEN], p_rcv_tlv->oid_len);
        break;
    default:
        break;
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Validates LLDPDU.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[in] len                The length of ethernet frame.
 * @param[in] pbuf               Pointer to ethernet frame buffer.
 * @param[out] p_rcv_tlv         Pointer to received LLDP TLV variables.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
static int LWIP_LLDP_LLDPDU_validation (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint32_t len, uint8_t * pbuf, LWIP_LLDP_RX_TLV_VARIABLES * p_rcv_tlv)
{
    uint16_t offset = 0;
    uint16_t type_length = 0;
    uint8_t tlv_type = 0;
    uint16_t tlv_length = 0;
    uint16_t ttl = 0;
    uint8_t i = 0;

    /* The receive module shall process each incoming LLDPDU as it is received. The statsFramesInTotal counter */
    /* for the port shall be incremented and the LLDPDU shall be checked to verify the presence of the three */
    /* mandatory TLVs at the beginning of the LLDPDU as defined in 9.2. */
    plldp_agent->rx.rx_counters.statsFramesInTotal++;

    /* a) The first TLV shall be extracted: */
    offset = 14;
    memcpy((void *)&type_length, (void *)&pbuf[offset], sizeof(type_length));
    type_length = htons(type_length);
    tlv_type = (uint8_t)((type_length & LWIP_LLDP_TLV_TYPE_MASK) >> 9);
    tlv_length = (type_length & LWIP_LLDP_TLV_LEN_MASK);
    /* 1) If the extracted TLV type value does not equal 1, the TLV is not a Chassis ID TLV: */
    if (tlv_type != LWIP_LLDP_TLV_TYPE_1) {
        /* i) The LLDPDU shall be discarded. */
        /* ii) The statsFramesDiscardedTotal and statsFramesInErrorsTotal counters shall both be */
        /* incremented. */
        plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
        plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
        /* iii) The variable badFrame shall be set to TRUE. */
        plldp_agent->rx.rx_variables.badFrame = true;
        /* iv) The procedure rxProcessFrame() shall be terminated. */
        return -1;
    }

    /* 2) If the extracted TLV type value equals 1, and the chassis ID TLV information string length is */
    /* not within the range 2 < n < 256: */
    if ((tlv_length < LWIP_LLDP_CHASSIS_ID_TLV_MIN_LEN) || (tlv_length > LWIP_LLDP_CHASSIS_ID_TLV_MAX_LEN)) {
        /* i) The LLDPDU shall be discarded. */
        /* ii) The statsFramesDiscardedTotal and statsFramesInErrorsTotal counters shall both be */
        /* incremented. */
        plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
        plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
        /* iii) The variable badFrame shall be set to TRUE. */
        plldp_agent->rx.rx_variables.badFrame = true;
        /* iv) The procedure rxProcessFrame() shall be terminated. */
        return -1;
    }

    /* Chassis ID TLV objects */
    p_rcv_tlv->chassis_id_subtype = pbuf[offset + LWIP_LLDP_TLV_HEADER_LEN];
    p_rcv_tlv->chassis_id_len = (tlv_length - LWIP_LLDP_CHASSIS_ID_SUBTYPE_LEN);
    memcpy((void *)&p_rcv_tlv->chassis_id[0], (void *)&pbuf[offset + (LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_CHASSIS_ID_SUBTYPE_LEN)], p_rcv_tlv->chassis_id_len);

    /* 3) If the extracted TLV type value equals 1, and the chassis ID TLV information string length is */
    /* within the range 2 < n < 256, the chassis ID value shall be extracted to become the first part of */
    /* the MSAP identifier. */
    offset += (uint16_t) (LWIP_LLDP_TLV_HEADER_LEN + tlv_length);

    /* b) The second TLV shall be extracted: */
    memcpy((void *)&type_length, (void *)&pbuf[offset], sizeof(type_length));
    type_length = htons(type_length);
    tlv_type = (uint8_t)((type_length & LWIP_LLDP_TLV_TYPE_MASK) >> 9);
    tlv_length = (type_length & LWIP_LLDP_TLV_LEN_MASK);
    /* 1) If the extracted TLV type value does not equal 2, the TLV is not a Port ID TLV: */
    if (tlv_type != LWIP_LLDP_TLV_TYPE_2) {
        /* 1) If the extracted TLV type value does not equal 2, the TLV is not a Port ID TLV: */
        /* i) The LLDPDU shall be discarded. */
        /* ii) The statsFramesDiscardedTotal and statsFramesInErrorsTotal counters shall both be */
        /* incremented. */
        plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
        plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
        /* iii) The variable badFrame shall be set to TRUE. */
        plldp_agent->rx.rx_variables.badFrame = true;
        /* iv) The procedure rxProcessFrame() shall be terminated. */
        return -1;
    }

    /* 2) If the extracted TLV type value equals 2, and the port ID TLV information string length is not */
    /* within the range 2 < n < 256: */
    if ((tlv_length < LWIP_LLDP_PORT_ID_TLV_MIN_LEN) || (tlv_length > LWIP_LLDP_PORT_ID_TLV_MAX_LEN)) {
        /* i) The LLDPDU shall be discarded. */
        /* ii) The statsFramesDiscardedTotal and statsFramesInErrorsTotal counters shall both be */
        /* incremented. */
        plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
        plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
        /* iii) The variable badFrame shall be set to TRUE. */
        plldp_agent->rx.rx_variables.badFrame = true;
        /* iv) The procedure rxProcessFrame() shall be terminated. */
        return -1;
    }

    /* Port ID TLV objects */
    p_rcv_tlv->port_id_subtype = pbuf[offset + LWIP_LLDP_TLV_HEADER_LEN];
    p_rcv_tlv->port_id_len = (tlv_length - LWIP_LLDP_PORT_ID_SUBTYPE_LEN);
    memcpy((void *)&p_rcv_tlv->port_id[0], (void *)&pbuf[offset + (LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_PORT_ID_SUBTYPE_LEN)], p_rcv_tlv->port_id_len);

    /* 3) If the extracted TLV type value equals 2, and the port ID TLV information string length is */
    /* within the range 2 < n < 256, the port ID value shall be extracted and appended to the */
    /* chassis ID value to complete construction of the MSAP identifier. */
    offset += (uint16_t) (LWIP_LLDP_TLV_HEADER_LEN + tlv_length);

    /* c) The third TLV shall be extracted: */
    memcpy((void *)&type_length, (void *)&pbuf[offset], sizeof(type_length));
    type_length = htons(type_length);
    tlv_type = (uint8_t)((type_length & LWIP_LLDP_TLV_TYPE_MASK) >> 9);
    tlv_length = (type_length & LWIP_LLDP_TLV_LEN_MASK);
    /* 1) If the extracted TLV type value does not equal 3, the TLV is not a Time To Live TLV. */
    if (tlv_type != LWIP_LLDP_TLV_TYPE_3) {
        /* i) The LLDPDU shall be discarded. */
        /* ii) The statsFramesDiscardedTotal and statsFramesInErrorsTotal counters shall both be */
        /* incremented. */
        plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
        plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
        /* iii) The variable badFrame shall be set to TRUE. */
        plldp_agent->rx.rx_variables.badFrame = true;
        /* iv) The procedure rxProcessFrame() shall be terminated. */
        return -1;
    }

    /* 2) If the extracted TLV type value equals 3, and the Time To Live TLV information string length */
    /* is less than 2: */
    if (tlv_length < LWIP_LLDP_TTL_TLV_LEN) {
        /* i) The LLDPDU shall be discarded. */
        /* ii) The statsFramesDiscardedTotal and statsFramesInErrorsTotal counters shall both be */
        /* incremented. */
        plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
        plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
        /* iii) The variable badFrame shall be set to TRUE. */
        plldp_agent->rx.rx_variables.badFrame = true;
        /* iv) The procedure rxProcessFrame() shall be terminated. */
        return -1;
    }

    /* 3) If the extracted TLV type value equals 3, and the Time To Live TLV information string length */
    /* is greater than or equal to 2, the first two octets of the TLV information string shall be extracted */
    /* and rxTTL shall be set to this value: */
    memcpy((void *)&ttl, (void *)&pbuf[offset + 2], sizeof(ttl));
    ttl = htons(ttl);
    plldp_agent->rx.rx_variables.rxTTL = ttl;
    if (ttl == 0) {
        /* i) If rxTTL equals zero, a shutdown frame has been received. The MSAP identifier and */
        /* rxTTL shall be passed up to the LLDP MIB manager, and further LLDPDU validation */
        /* shall be terminated. */
        return 0;
    }
    /* ii) If rxTTL is non-zero, LLDPDU validation shall continue and the remaining TLVs shall be */
    /* validated. */
    offset += (uint16_t) (LWIP_LLDP_TLV_HEADER_LEN + tlv_length);

    /* Each of the remaining TLV information elements shall be decoded in succession as required by their */
    /* particular TLV format definitions: */
    while (offset < len) {
        memcpy((void *)&type_length, (void *)&pbuf[offset], sizeof(type_length));
        type_length = htons(type_length);
        tlv_type = (uint8_t)((type_length & LWIP_LLDP_TLV_TYPE_MASK) >> 9);
        tlv_length = (type_length & LWIP_LLDP_TLV_LEN_MASK);
        if (tlv_type == LWIP_LLDP_TLV_TYPE_0) {
            /* d) If the TLV type value equals 0, the TLV is the End Of LLDPDU TLV. The MSAP identifier, */
            /* rxTTL, and all validated TLVs shall be passed to the LLDP manager for LLDP remote systems MIB */
            /* updating. */
            return 0;
        } else if ((tlv_type >= LWIP_LLDP_TLV_TYPE_1) && (tlv_type <= LWIP_LLDP_TLV_TYPE_8)) {
            /* e) If (0 < TLV_type_value < 8) the TLV is a member of the basic management set and shall be */
            /* validated according to the general rules for all TLVs defined in 10.3.2.1 as well as any specific rules */
            /* defined for the particular TLVs defined in 9.5. */
            if (LWIP_LLDP_general_validation(plldp_agent, tlv_type, tlv_length, &pbuf[offset], p_rcv_tlv) != 0) {
                return -1;
            }
        } else if (tlv_type == LWIP_LLDP_TLV_TYPE_127) {
            /* g) If the TLV type value is 127, the TLV is an Organizationally Specific TLV: */
            /* 1) If the TLV’s OUI and organizationally defined subtype are recognized, the TLV shall be */
            /* validated according to the general rules for all TLVs defined in 10.3.2.1 as well as the general */
            /* rules for Organizationally Specific TLVs defined in 10.3.2.2 plus any specific rules defined for */
            /* the particular TLV (see Annex F for IEEE 802.1 TLVs and Annex G for IEEE 802.3 TLVs). */
            if (LWIP_LLDP_general_validation_oui(plldp_agent, tlv_type, tlv_length, &pbuf[offset], p_rcv_tlv) != 0) {
                /* 2) If the TLV’s OUI and/or organizationally defined subtype are not recognized, the */
                /* statsTLVsUnrecognizedTotal counter shall be incremented, and the TLV shall be assumed to */
                /* be validated. */
                plldp_agent->rx.rx_counters.statsTLVsUnrecognizedTotal++;
            }
        } else {
            /* f) If TLV_type_value is in the range of reserved TLV types in Table 9-1, the TLV is unrecognized and */
            /* may be a basic TLV from a later LLDP version. The statsTLVsUnrecognizedTotal counter shall be */
            /* incremented, and the TLV shall be assumed to be validated. */
            plldp_agent->rx.rx_counters.statsTLVsUnrecognizedTotal++;
            for (i = 0; i < LWIP_LLDP_SUPPORT_RX_MAX_UNKNOWN_TLV_INFO; i++) {
                if (p_rcv_tlv->rcv_unknown_tlv[i] == 0) {
                    p_rcv_tlv->rcv_unknown_tlv[i] = tlv_type;
                }
            }
        }
        offset += (uint16_t) (LWIP_LLDP_TLV_HEADER_LEN + tlv_length);
    }

    /* h) If the end of the LLDPDU has been reached, the MSAP identifier, rxTTL, and all validated TLVs */
    /* shall be passed to the LLDP manager for LLDP remote systems MIB updating. */
    return 0;
}

/*******************************************************************************************************************//**
 * @brief Updates the MIB objects corresponding to the TLVs contained in the received LLDPDU for the LLDP remote system.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[in] r_mib_pos          The position of LLDP remote systems MIB.
 * @param[in] len                The length of ethernet frame.
 * @param[in] pbuf               Pointer to ethernet frame buffer.
 * @param[out] p_rcv_tlv         Pointer to received LLDP TLV variables.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
static int LWIP_LLDP_mibUpdateObjects (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t r_mib_pos, uint32_t len, uint8_t * pbuf, LWIP_LLDP_RX_TLV_VARIABLES * p_rcv_tlv)
{
	LWIP_UNUSED_ARG(len);
	LWIP_UNUSED_ARG(pbuf);

    /* RX_LLDP_UPDATE_INFO */
    plldp_agent->rx.rx_state = RX_LLDP_UPDATE_INFO;
    /* LLDP remote systems MIB update */
    plldp_agent->rx.remote_system_mib_used[r_mib_pos] = true;
    memcpy((void *)&plldp_agent->rx.remote_system_mib[r_mib_pos], (void *)p_rcv_tlv, sizeof(LWIP_LLDP_RX_TLV_VARIABLES));
    MIB2_COPY_SYSUPTIME_TO((u32_t *)&plldp_agent->rx.rem_time_mark);
    /* b) Set the timing counter rxInfoTTL associated with the MSAP identifier to rxTTL. */
    plldp_agent->rx.rx_timers.rxInfoTTL = plldp_agent->rx.rx_variables.rxTTL;
    /* c) Set the flag variable somethingChangedRemote associated with the MSAP identifier to TRUE to */
    /* notify the managers of PTOPO and other optional MIBs that something has changed in the LLDP */
    /* remote systems MIB associated with that MSAP identifier. */
    plldp_agent->rx.rx_variables.somethingChangedRemote = true;
    plldp_agent->rx.stats_rem_tables.inserts++;
    MIB2_COPY_SYSUPTIME_TO((u32_t *)&plldp_agent->rx.stats_rem_tables.last_change_time);
    return 0;
}

/*******************************************************************************************************************//**
 * @brief Deletes all information in the LLDP remote systems MIB associated with the MSAP identifier
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_mibDeleteObjects (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    memset((void *)&plldp_agent->rx.remote_system_mib_used[0], 0, sizeof(plldp_agent->rx.remote_system_mib_used));
    memset((void *)&plldp_agent->rx.rem_time_mark[0], 0, sizeof(plldp_agent->rx.rem_time_mark));
    memset((void *)&plldp_agent->rx.remote_system_mib[0], 0, sizeof(plldp_agent->rx.remote_system_mib));
    plldp_agent->rx.stats_rem_tables.deletes++;
    MIB2_COPY_SYSUPTIME_TO((u32_t *)&plldp_agent->rx.stats_rem_tables.last_change_time);
}

/*******************************************************************************************************************//**
 * @brief Performs the “Too many neighbors” process.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[in] len                The length of ethernet frame.
 * @param[in] pbuf               Pointer to ethernet frame buffer.
 * @param[out] p_rcv_tlv         Pointer to received LLDP TLV variables.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
static int LWIP_LLDP_Too_many_neighbors (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint32_t len, uint8_t * pbuf, LWIP_LLDP_RX_TLV_VARIABLES * p_rcv_tlv)
{
	LWIP_UNUSED_ARG(plldp_agent);
    LWIP_UNUSED_ARG(len);
    LWIP_UNUSED_ARG(pbuf);
    LWIP_UNUSED_ARG(p_rcv_tlv);

    /* a) Ignore and not process the new neighbor’s information. */
    return -1;
}

/*******************************************************************************************************************//**
 * @brief Compares TLV/MIB object value.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 * @param[in] len                The length of ethernet frame.
 * @param[in] pbuf               Pointer to ethernet frame buffer.
 * @param[out] p_rcv_tlv         Pointer to received LLDP TLV variables.
 *
 * @retval 0         OK
 * @retval -1        NG
 **********************************************************************************************************************/
static int LWIP_LLDP_TLV_MIB_object_value_comparison (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint32_t len, uint8_t * pbuf, LWIP_LLDP_RX_TLV_VARIABLES * p_rcv_tlv)
{
    uint8_t i = 0;

    /* TLV/MIB object value comparison */
    /* a) Set the flag variable somethingChangedRemote associated with the MSAP identifier to FALSE. */
    plldp_agent->rx.rx_variables.somethingChangedRemote = false;
    if (plldp_agent->rx.rx_variables.rxTTL == 0) {
        /* b) If rxTTL is zero, delete all information associated with the MSAP identifier from the LLDP remote */
        /* systems MIB, set the variable somethingChangedRemote to TRUE, and wait for the next LLDPDU. */
        /* RX_LLDP_DELETE_INFO */
        plldp_agent->rx.rx_state = RX_LLDP_DELETE_INFO;
        LWIP_LLDP_mibDeleteObjects(plldp_agent);
        plldp_agent->rx.rx_variables.somethingChangedRemote = true;
        /* UCT */
        plldp_agent->rx.rx_state = RX_LLDP_WAIT_FOR_FRAME;
        plldp_agent->rx.rx_variables.badFrame = false;
        plldp_agent->rx.rx_variables.rxInfoAge = false;
        plldp_agent->rx.rx_variables.somethingChangedRemote = false;
    } else {
        for (i = 0; i < LWIP_LLDP_SUPPORT_RX_MAX_NEIGHBORS; i++) {
            if (plldp_agent->rx.remote_system_mib_used[i] == true) {
                if (((plldp_agent->rx.remote_system_mib[i].chassis_id_subtype == p_rcv_tlv->chassis_id_subtype) &&
                    (plldp_agent->rx.remote_system_mib[i].chassis_id_len == p_rcv_tlv->chassis_id_len)) && 
                    (memcmp((void *)&plldp_agent->rx.remote_system_mib[i].chassis_id[0], (void *)&p_rcv_tlv->chassis_id[0], LWIP_LLDP_CHASSIS_ID_MAX_LEN) == 0)) {
                    if (((plldp_agent->rx.remote_system_mib[i].port_id_subtype == p_rcv_tlv->port_id_subtype) &&
                        (plldp_agent->rx.remote_system_mib[i].port_id_len == p_rcv_tlv->port_id_len)) && 
                        (memcmp((void *)&plldp_agent->rx.remote_system_mib[i].port_id[0], (void *)&p_rcv_tlv->port_id[0], LWIP_LLDP_PORT_ID_MAX_LEN) == 0)) {
                        /* d) If rxTTL is non-zero and the LLDPDU’s MSAP identifier is associated with an existing LLDP */
                        /* remote systems MIB, compare all current information in the LLDP remote systems MIB with the */
                        /* information in the TLVs just received: */
                        if (memcmp((void *)plldp_agent->rx.remote_system_mib, (void *)p_rcv_tlv, sizeof(LWIP_LLDP_RX_TLV_VARIABLES)) == 0) {
                            /* 1) If no differences are found, set the control variable rxChanges to FALSE, set the timing counter */
                            /* rxInfoTTL associated with the MSAP identifier to rxTTL, and wait for the next LLDPDU. */
                            plldp_agent->rx.rx_variables.rxChanges = false;
                            plldp_agent->rx.rx_timers.rxInfoTTL = plldp_agent->rx.rx_variables.rxTTL;
                        } else {
                            /* 2) If any differences are found and there is sufficient space in the LLDP remote systems MIB to */
                            /* store the new LLDPDU, set the control variable rxChanges to TRUE and perform the LLDP */
                            /* remote systems MIB update process defined in 10.3.5. */
                            plldp_agent->rx.rx_variables.rxChanges = true;
                            LWIP_LLDP_mibUpdateObjects(plldp_agent, i, len, pbuf, p_rcv_tlv);
                        }
                        /* UCT */
                        plldp_agent->rx.rx_state = RX_LLDP_WAIT_FOR_FRAME;
                        plldp_agent->rx.rx_variables.badFrame = false;
                        plldp_agent->rx.rx_variables.rxInfoAge = false;
                        plldp_agent->rx.rx_variables.somethingChangedRemote = false;
                        return 0;
                    }
                }
            }
        }
        for (i = 0; i < LWIP_LLDP_SUPPORT_RX_MAX_NEIGHBORS; i++) {
            if (plldp_agent->rx.remote_system_mib_used[i] == false) {
                /* c) If rxTTL is non-zero and the LLDPDU’s MSAP identifier is not associated with an existing LLDP */
                /* remote systems MIB, check to determine if sufficient space exists in the LLDP remote systems MIB */
                /* to accommodate the current LLDPDU: */
                /* 1) If sufficient space exists, perform the LLDP remote systems MIB update procedure defined in */
                /* 10.3.5. */
                plldp_agent->rx.rx_variables.rxChanges = true;
                LWIP_LLDP_mibUpdateObjects(plldp_agent, i, len, pbuf, p_rcv_tlv);
                /* UCT */
                plldp_agent->rx.rx_state = RX_LLDP_WAIT_FOR_FRAME;
                plldp_agent->rx.rx_variables.badFrame = false;
                plldp_agent->rx.rx_variables.rxInfoAge = false;
                plldp_agent->rx.rx_variables.somethingChangedRemote = false;
                return 0;
            }
        }
        /* 2) If sufficient space does not exist, perform the “Too many neighbors” process defined in 10.3.4. */
        if (LWIP_LLDP_Too_many_neighbors(plldp_agent, len, pbuf, p_rcv_tlv) != 0) {
            /* d) Set the flag variable tooManyNeighbors to TRUE. */
            plldp_agent->rx.rx_variables.tooManyNeighbors = true;
            /* e) If the information selected to be discarded is the information in the current LLDPDU: */
            /* 1) Set the tooManyNeighborsTimer as follows: */
            /* tooManyNeighborsTimer = max (tooManyNeighborsTimer, rxTTL) (1) */
            /* where rxTTL is the TTL value in the current LLDPDU */
            plldp_agent->rx.rx_timers.TooManyNeighborsTimer = LWIP_MAX(plldp_agent->rx.rx_timers.TooManyNeighborsTimer, plldp_agent->rx.rx_variables.rxTTL);
            /* 2) Discard the current LLDPDU and increment the statsFramesDiscardedTotal counter. */
            plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
            plldp_agent->rx.stats_rem_tables.drops++;
            MIB2_COPY_SYSUPTIME_TO((u32_t *)&plldp_agent->rx.stats_rem_tables.last_change_time);
            /* 3) Wait for the next LLDPDU. */
            return -1;
        }
    }

    return 0;
}

/*******************************************************************************************************************//**
 * @brief Initializes the LLDP receive module.
 *
 * @param[in] plldp_agent        Pointer to LLDP agent variables.
 **********************************************************************************************************************/
static void LWIP_LLDP_rxInitializeLLDP (LWIP_LLDP_AGENT_VARIABLES * plldp_agent)
{
    /* LLDP receive module initialization */
    /* a) The variable tooManyNeighbors shall be set to FALSE. */
    plldp_agent->rx.rx_variables.tooManyNeighbors = false;
    /* c) All information in the remote systems MIB associated with this port shall be deleted. */
    memset((void *)&plldp_agent->rx.remote_system_mib_used[0], 0, sizeof(plldp_agent->rx.remote_system_mib_used));
    memset((void *)&plldp_agent->rx.rem_time_mark[0], 0, sizeof(plldp_agent->rx.rem_time_mark));
    memset((void *)&plldp_agent->rx.remote_system_mib[0], 0, sizeof(plldp_agent->rx.remote_system_mib));
}

#endif

