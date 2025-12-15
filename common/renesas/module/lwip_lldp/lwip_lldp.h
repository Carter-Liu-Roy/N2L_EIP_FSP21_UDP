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

#ifndef LWIP_LLDP_H_
#define LWIP_LLDP_H_

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "lwip/opt.h"
#include "lwip/err.h"

#if LWIP_LLDP /* don't build if not configured for use in lwipopts.h */

#include "lwip_lldp_def.h"
#include "um_common_api.h"
#include "um_ether_netif_api.h"

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/* MGMT_CONFIG Register Bit Definitions */
#define OPENER_LLDP_ETHER_SWITCH_MGMT_BPDU_FWD_ENABLE         (1 << 6)
#define OPENER_LLDP_ETHER_SWITCH_MGMT_BPDU_DISCARD            (1 << 7)
#define OPENER_LLDP_ETHER_SWITCH_MGMT_BPDU_MGMT_ENABLE        (1 << 8)
#define OPENER_LLDP_ETHER_SWITCH_MGMT_BPDU_MGMT_DISCARD       (1 << 9)

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 ***********************************************************************************************************************/
extern struct netif lldp_netif;

/***********************************************************************************************************************
 * Exported global functions (to be accessed by other files)
 ***********************************************************************************************************************/
err_t LWIP_LLDP_agent_init (struct netif *netif);
int32_t LWIP_LLDP_receive_check (ether_netif_frame_t * p_frame_packet);
void LWIP_LLDP_set_g_adminStatus(uint8_t port, uint8_t adminStatus);
int32_t LWIP_LLDP_get_g_adminStatus(uint8_t port_id);

#endif

#endif /* LWIP_LLDP_H_ */
