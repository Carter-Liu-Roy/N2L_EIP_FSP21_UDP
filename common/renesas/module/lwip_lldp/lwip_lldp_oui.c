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
#include "lwip_lldp_def.h"
#include "lwip_lldp_interface.h"
#include "lwip_lldp_oui.h"
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
#if LWIP_LLDP_SUPPORT_CIP_TLV
  #define LLDP_XODVA_CIPIDENT_SUBTYPE    0x09
  #define LLDP_XODVA_CIPMAC_SUBTYPE      0x02
  #define LLDP_XODVA_CIPIFLABEL_SUBTYPE  0x03
  #define LLDP_XODVA_CIPPOSID_SUBTYPE    0x04
  #define LLDP_XODVA_CIPT1SPHY_SUBTYPE   0x05
  #define LLDP_XODVA_ORG_CODE ((uint8_t[]){ 0x00, 0x21, 0x6c })
#endif
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
/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Validates Organizationally Specific TLVs.
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
int LWIP_LLDP_general_validation_oui (LWIP_LLDP_AGENT_VARIABLES * plldp_agent, uint8_t tlv_type, uint16_t tlv_length, uint8_t * ptlv_buf, LWIP_LLDP_RX_TLV_VARIABLES * p_rcv_tlv)
{
    uint8_t oui[LWIP_LLDP_OUI_LEN] = {0};
    uint8_t oui_zero[LWIP_LLDP_OUI_LEN] = {0};
    uint8_t org_sub = 0;
    uint8_t i = 0;
    uint32_t offset = 0;

    /* organizationally unique identifier (OUI) */
    memcpy((void *)&oui, (void *)&ptlv_buf[LWIP_LLDP_TLV_HEADER_LEN], LWIP_LLDP_OUI_LEN);
    /* organizationally defined subtype */
    org_sub = ptlv_buf[LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_OUI_LEN];

    /* General validation rules for all Organizationally Specific TLVs */
    switch (tlv_type) {
    /* Add Organizationally Specific TLVs */
    case LWIP_LLDP_TLV_TYPE_127:
#if LWIP_LLDP_SUPPORT_CIP_TLV
		if (0 == memcmp(LLDP_XODVA_ORG_CODE, &oui, LWIP_ARRAYSIZE(oui))) {
	        /* LLDP Ext-ODVA MIB, ODVA CIP Identification */
			if (LLDP_XODVA_CIPIDENT_SUBTYPE == org_sub) {
				offset = LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_OUI_LEN + LWIP_LLDP_ORG_SUBTYPE_LEN;
				//offset++;

				memcpy(&(p_rcv_tlv->cip_vendor_id), &(ptlv_buf[offset]), sizeof(p_rcv_tlv->cip_vendor_id));
				offset += sizeof(p_rcv_tlv->cip_vendor_id);
				memcpy(&(p_rcv_tlv->cip_device_type), &(ptlv_buf[offset]), sizeof(p_rcv_tlv->cip_device_type));
				offset += sizeof(p_rcv_tlv->cip_device_type);
				memcpy(&(p_rcv_tlv->cip_product_code), &(ptlv_buf[offset]), sizeof(p_rcv_tlv->cip_product_code));
				offset += sizeof(p_rcv_tlv->cip_product_code);
				memcpy(&(p_rcv_tlv->cip_revision), &(ptlv_buf[offset]), sizeof(p_rcv_tlv->cip_revision));
				offset += sizeof(p_rcv_tlv->cip_revision);
				memcpy(&(p_rcv_tlv->cip_serial_number), &(ptlv_buf[offset]), sizeof(p_rcv_tlv->cip_serial_number));
			}
            /* LLDP Ext-ODVA MIB, ODVA CIP MAC Address */
			else if (LLDP_XODVA_CIPMAC_SUBTYPE == org_sub) {
                offset = LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_OUI_LEN + LWIP_LLDP_ORG_SUBTYPE_LEN;

                memcpy(&p_rcv_tlv->cip_mac_address[0], &(ptlv_buf[offset]), LWIP_LLDP_CIP_MAC_TLV_LEN);
            }
            /* LLDP Ext-ODVA MIB, ODVA CIP Interface Label */
            else if (LLDP_XODVA_CIPIFLABEL_SUBTYPE == org_sub) {
                offset = LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_OUI_LEN + LWIP_LLDP_ORG_SUBTYPE_LEN;
                p_rcv_tlv->cip_if_label_len = tlv_length - LWIP_LLDP_OUI_LEN - LWIP_LLDP_ORG_SUBTYPE_LEN;

                memcpy(&p_rcv_tlv->cip_if_label[0], &(ptlv_buf[offset]), p_rcv_tlv->cip_if_label_len);
            }
            /* LLDP Ext-ODVA MIB, ODVA Position ID */
            else if (LLDP_XODVA_CIPPOSID_SUBTYPE == org_sub) {
                offset = LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_OUI_LEN + LWIP_LLDP_ORG_SUBTYPE_LEN;

                memcpy(&p_rcv_tlv->cip_posision_id, &(ptlv_buf[offset]), sizeof(CipUint));
            }
            /* LLDP Ext-ODVA MIB, ODVA T1S PHY Data */
            else if (LLDP_XODVA_CIPT1SPHY_SUBTYPE == org_sub) {
                offset = LWIP_LLDP_TLV_HEADER_LEN + LWIP_LLDP_OUI_LEN + LWIP_LLDP_ORG_SUBTYPE_LEN;

                CipUint temp;
                memcpy(&temp, &(ptlv_buf[offset]), sizeof(CipUint));
                p_rcv_tlv->cip_phy_mode = (CipUsint)temp;
                p_rcv_tlv->cip_phy_mode &= 0x0f;
                p_rcv_tlv->cip_phy_node_id = (CipUsint)(temp >> 8);
            }
	    	else {
				plldp_agent->rx.rx_counters.statsFramesDiscardedTotal++;
				plldp_agent->rx.rx_counters.statsFramesInErrorsTotal++;
				plldp_agent->rx.rx_variables.badFrame = true;
				return -1;
			}
		}
#endif
    	break;
    default:
        break;
    }

    for (i = 0; i < LWIP_LLDP_SUPPORT_RX_MAX_ORG_DEF_INFO; i++) {
        if (memcmp((void *)&p_rcv_tlv->rcv_org_def[i].oui[0], (void *)&oui_zero[0], LWIP_LLDP_OUI_LEN) == 0) {
            if (p_rcv_tlv->rcv_org_def[i].org_sub == 0) {
                memcpy((void *)&p_rcv_tlv->rcv_org_def[i].oui[0], (void *)&oui[0], LWIP_LLDP_OUI_LEN);
                p_rcv_tlv->rcv_org_def[i].org_sub = org_sub;
                p_rcv_tlv->rcv_org_def[i].org_info_len = (tlv_length - (LWIP_LLDP_OUI_LEN + LWIP_LLDP_ORG_SUBTYPE_LEN));
                memcpy((void *)&p_rcv_tlv->rcv_org_def[i].org_info[0], (void *)&ptlv_buf[LWIP_LLDP_TLV_HEADER_LEN + (LWIP_LLDP_OUI_LEN + LWIP_LLDP_ORG_SUBTYPE_LEN)], p_rcv_tlv->rcv_org_def[i].org_info_len);
                break;
            }
        }
    }

    return 0;
}

#endif

