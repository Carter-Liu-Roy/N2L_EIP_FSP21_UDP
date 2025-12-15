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
#include "um_opener_port_api.h"
#include "um_opener_port_cfg.h"
#include "um_opener_port.h"
#include "um_opener_port_internal.h"
#include "um_common_api.h"

#include "um_ether_netif_api.h"

/** For getting extend configuration */
#if defined(BSP_MCU_GROUP_RZT2H) || defined(BSP_MCU_GROUP_RZN2H)
#include "r_gmac_b.h"
#else
#include "r_gmac.h"
#endif
#include "r_ether_phy.h"
#include "r_ether_phy_api.h"

/** Ethernet link object. */
#include "cipethernetlink.h"
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
#include "opener_api.h"
#include "renesas.h"
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */

#if defined(LWIP_SNMP) && LWIP_SNMP
/** lwIP SNMP **/
#include "snmp_core.h"
#include "snmp_core_priv.h"
#endif

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/**
 * For common and object-sepecific services
 */
#define _ETHLINK_GETATTR_ALL_SERV_CODE                     (0x01)  /** Get_Attributes_All Service code 0x01 */
#define _ETHLINK_GETANDCLR_SERV_CODE                       (0x4c)  /** Get_and_Clear Service code 0x4c */

/**
 * For instance attribute #2 interface capability
 */
#define _ETHLINK_IFFLAG_LINK_STATUS_BIT                    ((uint32_t)0x01<<0)   /** 1: Link Enable, 0: Link Disable */
#define _ETHLINK_IFFLAG_HALF_FULL_DUPLEX_BIT               ((uint32_t)0x01<<1)   /** 1: Full duplex, 0: Half duplex */
#define _ETHLINK_IFFLAG_NEGOTIATION_STATUS_BIT_SHIFT       (2)   /** See opener_port_negotiation_status_t */
#define _ETHLINK_IFFLAG_NEGOTIATION_STATUS_BIT_MASK        ((uint32_t)0x07<<_ETHLINK_IFFLAG_NEGOTIATION_STATUS_BIT_SHIFT)
#define _ETHLINK_IFFLAG_MANUAL_SETTING_REQUIRES_RESET_BIT  ((uint32_t)0x01<<5) /** Duplicated in Interface Capability #11 Attribute to retain backwards compatibility. */
#define _ETHLINK_IFFLAG_LOCAL_HARDWARE_FAULT_BIT           ((uint32_t)0x01<<6) /** 1: Hardware fault is detected (product specific), 0: not detected. */

/**
 * For instance attribute #4 Interface Counter
 */
#define _ETHLINK_IFCNTR_ATTR_NUM                           (4)  /** Attribute number #4 */
#define _ETHLINK_IFCNTR_CNTR_NUM                           (11) /** Counter number of Interface Counter, 11 */

#if defined(LWIP_SNMP) && LWIP_SNMP
#define _ETHLINK_IFCNTR_OID_LEN                            (11) /** OID Array Length */
#define _ETHLINK_IFCNTR_INDISCARDS_OID                     ((u32_t[]){1,3,6,1,2,1,2,2,1,13,0}) /** OID array of In Discards statistics */
#define _ETHLINK_IFCNTR_INUNKNOWNPROTOS_OID                ((u32_t[]){1,3,6,1,2,1,2,2,1,15,0}) /** OID array of In UnknownProtos statistics */
#endif

/**
 * For instance attribute #5 Media Counter
 */
#define _ETHLINK_MDRCNTR_ATTR_NUM                          (5)  /** Attribute number #5 */
#define _ETHLINK_MDRCNTR_CNTR_NUM                          (12) /** Counter number of Media Counter, 12 */

/**
 * Hardware dependent macro
 */
#define _ETHLINK_STATREG_OFFSET                            (0x400) /** Address offset of statistics registers */

/***********************************************************************************************************************
 * Private constants
 **********************************************************************************************************************/
/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef enum e_opener_port_negotiation_status
{
    _NEGOTIATION_STATUS_IN_PROGRESS = 0,                     ///< OPENER_PORT_NEGOTIATION_STATUS_IN_PROGRESS
    _NEGOTIATION_STATUS_FAILED_AND_SPEED_DETECTION_FAILED,   ///< Using default values for speed and duplex.
    _NEGOTIATION_STATUS_FAILED_BUT_SPEED_DETECTION_SUCCESS,  ///< Using default values for speed and duplex.
    _NEGOTIATION_STATUS_SUCCESS,                             ///< OPENER_PORT_NEGOTIATION_STATUS_SUCCESS
    _NEGOTIATION_STATUS_IS_NOT_ATTEMPT,                      ///< OPENER_PORT_NEGOTIATION_STATUS_IS_NOT_ATTEMPT
} opener_port_negotiation_status_t;

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static usr_err_t _open_cip_ethlink_instance( opener_port_ether_netif_ctrl_t * const p_ctrl );
static usr_err_t _init_cip_ethlink_instance_attributes( opener_port_ether_netif_ctrl_t * const p_ctrl );
static usr_err_t _update_cip_ethlink_interface_speed_and_duplex_flags( ether_phy_instance_t const * p_ether_phy_instance,
                                                                       CipEthernetLinkObject * p_ethlink_cip_instance ,
                                                                       opener_port_negotiation_status_t * p_negotiation_status );
static usr_err_t _set_cip_ethlink_autonegotiaion_status( CipEthernetLinkObject * const p_ethlink_object, opener_port_negotiation_status_t status );
static void      _ether_netif_callback( ether_netif_callback_args_t * p_args );

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
static EipStatus _callback_ethernetlink_pre_get( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service );
static EipStatus _callback_ethernetlink_post_get( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service );
static EipStatus _get_interface_cntrs_from_ethsw(EipUint32 instance_num, CipEthernetLinkInterfaceCounters * p_interface_cntrs);
static EipStatus _get_media_cntrs_from_ethsw(EipUint32 instance_num, CipEthernetLinkMediaCounters * p_media_cntrs);
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
static CipEthernetLinkInterfaceCounters g_base_interface_counters[OPENER_ETHLINK_INSTANCE_CNT];
static CipEthernetLinkMediaCounters g_base_media_counters[OPENER_ETHLINK_INSTANCE_CNT];
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Initialize the controller.
 *
 * @param[in] p_ctrl                    Pointer to the controller
 *
 * @retval USR_SUCCESS                  Process has been done successfully.
 * @retval USR_ERR_NOT_INITIALIZED      Initialization has been failed.
 **********************************************************************************************************************/
usr_err_t um_opener_port_ether_netif_open( opener_port_ether_netif_ctrl_t * const p_ctrl,
                                           ether_netif_instance_t const * const p_ether_netif_instance )
{
    /** Error codes*/
    usr_err_t usr_err;

    /** Temporary link status */
    uint32_t link_status;

    /** Set target module instance. */
    p_ctrl->p_ether_netif_instance = p_ether_netif_instance;

    /** Open target module instance. */
    usr_err = p_ctrl->p_ether_netif_instance->p_api->open(p_ctrl->p_ether_netif_instance->p_ctrl, p_ctrl->p_ether_netif_instance->p_cfg);
    USR_ERROR_RETURN( USR_SUCCESS == usr_err || USR_ERR_ALREADY_OPEN == usr_err || USR_ERR_ALREADY_RUNNING == usr_err, USR_ERR_NOT_INITIALIZED);

    /** Setup CIP Ethernet link object*/
    usr_err = _open_cip_ethlink_instance( p_ctrl );
    USR_ERROR_RETURN( USR_SUCCESS == usr_err, USR_ERR_NOT_INITIALIZED );

    /** Initialize CIP Ethernet link object attributes . */
    usr_err = _init_cip_ethlink_instance_attributes( p_ctrl );
    USR_ERROR_RETURN( USR_SUCCESS == usr_err, USR_ERR_NOT_INITIALIZED );

    /** Add callback function. */
    p_ctrl->callback_node.p_func = _ether_netif_callback;
    p_ctrl->callback_node.p_memory = &(p_ctrl->callback_memory);
    p_ctrl->callback_node.p_context = p_ctrl;
    usr_err = p_ctrl->p_ether_netif_instance->p_api->callbackAdd( p_ctrl->p_ether_netif_instance->p_ctrl, &p_ctrl->callback_node );
    USR_ERROR_RETURN( USR_SUCCESS == usr_err, USR_ERR_NOT_INITIALIZED );

    /** Get link status with checking callback. */
    usr_err = p_ctrl->p_ether_netif_instance->p_api->linkStatusGet( p_ctrl->p_ether_netif_instance->p_ctrl, &link_status, true );
    USR_ERROR_RETURN( USR_SUCCESS == usr_err, USR_ERR_NOT_INITIALIZED );

    /** Return success code. */
    return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Start target module.
 *
 * @param[in] p_ctrl                    Pointer to the controller
 *
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
usr_err_t um_opener_port_ether_netif_start( opener_port_ether_netif_ctrl_t * const p_ctrl )
{
    /** Error codes. */
    usr_err_t usr_err;

    /** Start target module */
    usr_err = p_ctrl->p_ether_netif_instance->p_api->start( p_ctrl->p_ether_netif_instance->p_ctrl );
    USR_ERROR_RETURN( USR_SUCCESS == usr_err, USR_SUCCESS );

    /** Wait for first callback. */
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

    /** Return success code. */
    return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Stop target module.
 *
 * @param[in] p_ctrl                    Pointer to the controller
 *
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
usr_err_t um_opener_port_ether_netif_stop( opener_port_ether_netif_ctrl_t * const p_ctrl )
{
    /** Error codes. */
    usr_err_t usr_err;

    /** Stop target module. */
    usr_err = p_ctrl->p_ether_netif_instance->p_api->stop(p_ctrl->p_ether_netif_instance->p_ctrl);
    USR_ERROR_RETURN( USR_SUCCESS == usr_err, USR_SUCCESS );

    /** Return success code. */
    return USR_SUCCESS;
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Setup CIP Ethernet link instance with module controller.
 *
 * @param[in] p_ctrl                    Pointer to the controller
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
static usr_err_t _open_cip_ethlink_instance( opener_port_ether_netif_ctrl_t * const p_ctrl )
{
    /** For scanning ports */
    uint8_t port = 0;
    uint8_t cnt_instance = 0;

    /** Get gmac extend configuration. */
#if defined(BSP_MCU_GROUP_RZT2H) || defined(BSP_MCU_GROUP_RZN2H)
    gmac_b_extend_cfg_t const * p_gmac_b_extend_cfg = p_ctrl->p_ether_netif_instance->p_cfg->p_ether_instance->p_cfg->p_extend;
#else
    gmac_extend_cfg_t const * p_gmac_extend_cfg = p_ctrl->p_ether_netif_instance->p_cfg->p_ether_instance->p_cfg->p_extend;
#endif

    /** Set the Ethernet Link object address to controller */
    for( port = 0; port < BSP_FEATURE_GMAC_MAX_PORTS; port++ )
    {
        /** Get PHY instance */
#if defined(BSP_MCU_GROUP_RZT2H) || defined(BSP_MCU_GROUP_RZN2H)
        p_ctrl->p_ether_phy_instance[port] = (*p_gmac_b_extend_cfg->pp_phy_instance)[port];
#else
        p_ctrl->p_ether_phy_instance[port] = (*p_gmac_extend_cfg->pp_phy_instance)[port];
#endif

        /** If the PHY instance is NULL, */
        if( NULL == p_ctrl->p_ether_phy_instance[port] )
        {
            /** Set the port to NULL not to assign the object. */
            p_ctrl->p_ethlink_cip_instance[port]  = NULL;
            continue;
        }

        /** Set the pointer of Ethernet Link CIP object instance */
        p_ctrl->p_ethlink_cip_instance[port] = &g_ethernet_link[cnt_instance];

        /** Increment index of Ethernet Link instance. */
        cnt_instance++;

        /** If instances do NOT remains, */
        if( cnt_instance >= OPENER_ETHLINK_INSTANCE_CNT )
        {
            /** Stop the assignment of Ethernet link object. */
            break;
        }
    }

    /** Return success code. */
    return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Initialize CIP Ethernet link attributes
 *
 * NOTE:
 *  This function initialize all attributes even if the attributes have already initialized on definition or on CipStackInit() function.
 *  This function should be called after the CIP stack initialization to overwrite attributes by the CIP initialization.
 *
 * @param[in] p_ctrl                    Pointer to the controller
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
static usr_err_t _init_cip_ethlink_instance_attributes( opener_port_ether_netif_ctrl_t * const p_ctrl )
{
    /** For scanning ports */
    uint32_t port;

    /** For resolving types */
    ether_phy_instance_t const * p_ether_phy_instance;
    ether_phy_extend_cfg_t const * p_ether_phy_extend_cfg;
    CipEthernetLinkObject * p_ethlink_cip_instance;

    /** Scan all ports. */
    for( port = 0; port < BSP_FEATURE_GMAC_MAX_PORTS ; port++ )
    {
        /** If the pointer is NULL, continue. */
        if( NULL == p_ctrl->p_ether_phy_instance[port] || NULL == p_ctrl->p_ethlink_cip_instance[port] )
        {
            continue;
        }

        /** Resolve the types */
        p_ether_phy_instance = p_ctrl->p_ether_phy_instance[port];
        p_ethlink_cip_instance = p_ctrl->p_ethlink_cip_instance[port];

        /** Resolve extended configuration of PHY instance. */
        p_ether_phy_extend_cfg = p_ether_phy_instance->p_cfg->p_extend;

        /** Initialize the interface speed (#1) and flags (#2) which are update in callback function. */
        p_ethlink_cip_instance->interface_speed = 0;
        p_ethlink_cip_instance->interface_flags = 0;

        /** Set the MAC address (#3) */
        (void) CipEthernetLinkSetMac( (EipUint8 *) p_ctrl->p_ether_netif_instance->p_cfg->p_ether_instance->p_cfg->p_mac_address );

        /** Set the interface type (#7) */
        p_ethlink_cip_instance->interface_type = kEthLinkIfTypeTwistedPair;

#if ( (OPENER_ETHLINK_INSTANCE_CNT > (1)) || ((1) == OPENER_ETHLINK_LABEL_ENABLE) )
        /**
         * TODO: Explicitly initialize g_ethernet_link[port].interface_label (#10) when multiple ports are enabled (OPENER_ETHLINK_INSTANCE_CNT > 1)
         */
#endif

        /** Initialize the interface capability (#11), and set Auto MDX capability. */
        p_ethlink_cip_instance->interface_caps.capability_bits = kEthLinkCapAutoMDX;  /** TODO: is it right? */
        p_ethlink_cip_instance->interface_caps.speed_duplex_selector = 0;             /** Set to 0 because the #6 is NOT supported. */

        /** Settings with/without auto-negotiation. */
        switch ( p_ether_phy_extend_cfg->auto_negotiation )
        {
        case ETHER_PHY_AUTO_NEGOTIATION_ON:
            /** Add the auto-negotiation capability to the interface capability (#11). */
            p_ethlink_cip_instance->interface_caps.capability_bits |= kEthLinkCapAutoNeg;
            break;

        case ETHER_PHY_AUTO_NEGOTIATION_OFF:
            /** Set auto-negotiation status. */
            (void) _set_cip_ethlink_autonegotiaion_status(&g_ethernet_link[port], _NEGOTIATION_STATUS_IS_NOT_ATTEMPT);
            break;
        }
    }

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
    /** Get pointer to Ethernet Link class  */
    CipClass * p_ethernetlink_class = GetCipClass(kCipEthernetLinkClassCode);
    /** Set callback functions of Ethernet Link class to update and clear counters */
    InsertGetSetCallback(p_ethernetlink_class, _callback_ethernetlink_pre_get, kPreGetFunc);
    InsertGetSetCallback(p_ethernetlink_class, _callback_ethernetlink_post_get, kPostGetFunc);
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */

    /** Return success code. */
    return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Callback handler for Ethernet network interface module
 *
 * @param[in] p_args                    Pointer to arguments of callback.
 **********************************************************************************************************************/
void _ether_netif_callback( ether_netif_callback_args_t * p_args )
{
    /** Resolve user context */
    opener_port_ether_netif_ctrl_t * p_ctrl = ( opener_port_ether_netif_ctrl_t * ) p_args->p_context;
    (void) p_ctrl;

    /** For scanning ports */
    uint8_t port = 0;

    /** For resolving types */
    ether_phy_instance_t const * p_ether_phy_instance;
    ether_phy_extend_cfg_t const * p_ether_phy_extend_cfg;
    CipEthernetLinkObject * p_ethlink_cip_instance;
    opener_port_negotiation_status_t negotiation_status;

    /** Check event */
    switch( p_args->event )
    {
    case ETHER_NETIF_CALLBACK_EVENT_ANY_PORT_LINK_STATUS_CHANGED:

        for( port = 0; port < BSP_FEATURE_GMAC_MAX_PORTS ; port++ )
        {
            /** If the pointer is NULL, continue */
            if( NULL == p_ctrl->p_ether_phy_instance[port] || NULL == p_ctrl->p_ethlink_cip_instance[port] )
            {
                continue;
            }

            /** Resolve the types */
            p_ether_phy_instance = p_ctrl->p_ether_phy_instance[port];
            p_ethlink_cip_instance = p_ctrl->p_ethlink_cip_instance[port];

            /** Resolve extended configuration of PHY instance. */
            p_ether_phy_extend_cfg = p_ether_phy_instance->p_cfg->p_extend;

            if( p_args->port_link_status & ETHER_NETIF_PORT_BIT(port) )
            {
                /** Enable link status flag (in #2) */
                p_ethlink_cip_instance->interface_flags |= _ETHLINK_IFFLAG_LINK_STATUS_BIT;

                /** Update the interface speed and flags (#1 and #2) from actual data. */
                _update_cip_ethlink_interface_speed_and_duplex_flags(p_ether_phy_instance, p_ethlink_cip_instance, &negotiation_status);

                /** Set auto-negotiation status (in #2) to success code. */
                if( ETHER_PHY_AUTO_NEGOTIATION_ON == p_ether_phy_extend_cfg->auto_negotiation )
                {
                    (void) _set_cip_ethlink_autonegotiaion_status(p_ethlink_cip_instance, negotiation_status);

                    /**
                     * TODO: For getting auto-negotiaion fail status, r_ether_phy API should be expanded.
                     */
                }
            }
            else
            {
                /** Disable link status flag (in #2) */
                p_ethlink_cip_instance->interface_flags &= ~(_ETHLINK_IFFLAG_LINK_STATUS_BIT);

                /** Set auto-negotiation status (in #2) to in progress code. */
                if( ETHER_PHY_AUTO_NEGOTIATION_ON == p_ether_phy_extend_cfg->auto_negotiation )
                {
                    (void) _set_cip_ethlink_autonegotiaion_status(p_ethlink_cip_instance, _NEGOTIATION_STATUS_IN_PROGRESS);
                }
            }
        }
        break;

    case ETHER_NETIF_CALLBACK_EVENT_RECEIVE_ETHER_FRAME:
        /** Discards packet. */
        USR_HEAP_RELEASE( p_args->p_frame_packet );
        break;
    }
}

/*******************************************************************************************************************//**
 * @brief Update CIP Ethernet link interface speed (#1) and duplex flags (in #2)
 *
 * @param[in] p_ether_phy_instance      Instance information of an interface.
 * @param[in] p_ethlink_cip_instance    Data of an CIP Ethernet Link object.
 * @param[out] p_negotiation_status     Negotiation status for OpENer port.
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
static usr_err_t _update_cip_ethlink_interface_speed_and_duplex_flags( ether_phy_instance_t const * p_ether_phy_instance,
                                                                       CipEthernetLinkObject * p_ethlink_cip_instance ,
                                                                       opener_port_negotiation_status_t * p_negotiation_status )
{
    /** Error code */
    fsp_err_t fsp_err;

    /** Get register address of Ethernet subsystem */
    uint32_t link_speed_duplex;
    uint32_t local_pause_bits;
    uint32_t partner_pause_bits;

    /** Unused parameters */
    (void) local_pause_bits;
    (void) partner_pause_bits;

    /**
     * Get target ability.
     * TODO:
     *  This process should be done by um_ether_netif module and
     *  the abilities should be passed from um_ether_netif module callback.
     */
    fsp_err = p_ether_phy_instance->p_api->linkPartnerAbilityGet( p_ether_phy_instance->p_ctrl,
                                                                  &link_speed_duplex,
                                                                  &local_pause_bits,
                                                                  &partner_pause_bits );
    FSP_ERROR_RETURN( FSP_SUCCESS == fsp_err, USR_ERR_ABORTED );

    /** Set to the interface flag and speed*/
    switch( link_speed_duplex )
    {
    case ETHER_PHY_LINK_SPEED_1000F:
        p_ethlink_cip_instance->interface_flags |= _ETHLINK_IFFLAG_HALF_FULL_DUPLEX_BIT;
        p_ethlink_cip_instance->interface_speed = 1000;
        *p_negotiation_status = _NEGOTIATION_STATUS_SUCCESS;
        break;
    case ETHER_PHY_LINK_SPEED_1000H:
        p_ethlink_cip_instance->interface_flags &= ~(_ETHLINK_IFFLAG_HALF_FULL_DUPLEX_BIT);
        p_ethlink_cip_instance->interface_speed = 1000;
        *p_negotiation_status = _NEGOTIATION_STATUS_SUCCESS;
        break;
    case ETHER_PHY_LINK_SPEED_100F:
        p_ethlink_cip_instance->interface_flags |= _ETHLINK_IFFLAG_HALF_FULL_DUPLEX_BIT;
        p_ethlink_cip_instance->interface_speed = 100;
        *p_negotiation_status = _NEGOTIATION_STATUS_SUCCESS;
        break;
    case ETHER_PHY_LINK_SPEED_100H:
        p_ethlink_cip_instance->interface_flags &= ~(_ETHLINK_IFFLAG_HALF_FULL_DUPLEX_BIT);
        p_ethlink_cip_instance->interface_speed = 100;
        *p_negotiation_status = _NEGOTIATION_STATUS_FAILED_BUT_SPEED_DETECTION_SUCCESS;
        break;
    case ETHER_PHY_LINK_SPEED_10F:
        p_ethlink_cip_instance->interface_flags |= _ETHLINK_IFFLAG_HALF_FULL_DUPLEX_BIT;
        p_ethlink_cip_instance->interface_speed = 10;
        *p_negotiation_status = _NEGOTIATION_STATUS_FAILED_AND_SPEED_DETECTION_FAILED;
        break;
    case ETHER_PHY_LINK_SPEED_10H:
        p_ethlink_cip_instance->interface_flags &= ~(_ETHLINK_IFFLAG_HALF_FULL_DUPLEX_BIT);
        p_ethlink_cip_instance->interface_speed = 10;
        *p_negotiation_status = _NEGOTIATION_STATUS_FAILED_AND_SPEED_DETECTION_FAILED;
        break;
    default:
        p_ethlink_cip_instance->interface_speed = 0;
        *p_negotiation_status = _NEGOTIATION_STATUS_IS_NOT_ATTEMPT;
        break;
    }

    /** Return success code. */
    return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Update CIP Ethernet link interface negotiation status flags (in #2)
 *
 * @param[in] p_ethlink_cip_instance    Data of an CIP Ethernet Link object.
 * @param[in] status                    Negotiation status for OpENer port.
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
static usr_err_t _set_cip_ethlink_autonegotiaion_status( CipEthernetLinkObject * p_ethlink_cip_instance,  opener_port_negotiation_status_t status )
{
    /** Clear with bit mask */
    p_ethlink_cip_instance->interface_flags &= ~(_ETHLINK_IFFLAG_NEGOTIATION_STATUS_BIT_MASK);

    /** Set status */
    p_ethlink_cip_instance->interface_flags |= ((uint32_t) status << _ETHLINK_IFFLAG_NEGOTIATION_STATUS_BIT_SHIFT) & _ETHLINK_IFFLAG_NEGOTIATION_STATUS_BIT_MASK;

    /** Return success code */
    return USR_SUCCESS;
}

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
/*******************************************************************************************************************//**
 * @brief Update Ethernet Link Object Attribute #4, #5
 *
 * * NOTE:
 *  This function is registered as callback function for Ethernet Link Object.
 *  The function is called before GetAttributeSingle processing.
 *
 * @param[in] p_instance                Pointer of CIP instance data
 * @param[in] p_attribute               Pointer of CIP attribute data
 * @param service                       Service code received from requester
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
static EipStatus _callback_ethernetlink_pre_get( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service )
{
	/** Resolve parameters */
	EipUint32 instance_num = (*p_instance).instance_number;
	EipUint16 attr = (*p_attribute).attribute_number;

	/** For storing the values of counters */
	uint32_t * p_counter_val;
	uint32_t * p_base_counter_val;

	if ((_ETHLINK_IFCNTR_ATTR_NUM == attr)||(_ETHLINK_GETATTR_ALL_SERV_CODE == service)){
		/** Get Interface Counters values from ethsw */
		_get_interface_cntrs_from_ethsw(instance_num, &(g_ethernet_link[instance_num - 1].interface_cntrs));
		/** Subtract base line values from the Interface Counters */
		p_counter_val = (uint32_t *) &(g_ethernet_link[instance_num - 1].interface_cntrs);
		p_base_counter_val =  (uint32_t *) &(g_base_interface_counters[instance_num - 1]);
		for (uint8_t i=0; i < _ETHLINK_IFCNTR_CNTR_NUM; i++){
			*p_counter_val -= *p_base_counter_val;
			p_counter_val ++;
			p_base_counter_val ++;
		}
	}
	if ((_ETHLINK_MDRCNTR_ATTR_NUM == attr)||(_ETHLINK_GETATTR_ALL_SERV_CODE == service)){
		/** Get Media Counters values from ethsw */
		_get_media_cntrs_from_ethsw(instance_num, &(g_ethernet_link[instance_num - 1].media_cntrs));
		/** Subtract base line values from the Media Counters */
		p_counter_val = (uint32_t *) &(g_ethernet_link[instance_num - 1].media_cntrs);
		p_base_counter_val = (uint32_t *) &(g_base_media_counters[instance_num - 1]);
		for (uint8_t i=0; i < _ETHLINK_MDRCNTR_CNTR_NUM; i++){
			*p_counter_val -= *p_base_counter_val;
			p_counter_val ++;
			p_base_counter_val ++;
		}
	}

    /** Return success code */
    return kEipStatusOk;
}

/*******************************************************************************************************************//**
 * @brief Clear Ethernet Link Object Attribute #4, #5
 *
 * * NOTE:
 *  This function is registered as callback function for Ethernet Link Object.
 *  The function is called after GetAttributeSingle processing.
 *
 * @param[in] p_instance                Pointer of CIP instance data
 * @param p_attribute                   Pointer of CIP attribute data
 * @param[in] service                   Service code received from requester
 * @retval USR_SUCCESS                  Process has been done successfully.
 * @retval USR_ERROR                    Invalid Attribute number
 **********************************************************************************************************************/
static EipStatus _callback_ethernetlink_post_get( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service )
{
	/** Unused parameter */
	(void) p_attribute;

	/** Resolve parameters */
	EipUint32 instance_num = (*p_instance).instance_number;
	EipUint16 attr = (*p_attribute).attribute_number;

	/** Clear Interface Counters and Media Counters if Service is Get_and_Clear Service */
	if (_ETHLINK_GETANDCLR_SERV_CODE == service)
	{
		switch(attr){
		case _ETHLINK_IFCNTR_ATTR_NUM:
			/** Take a snapshot for values of Interface Counters */
			_get_interface_cntrs_from_ethsw(instance_num, &(g_base_interface_counters[instance_num-1]));
			break;
		case _ETHLINK_MDRCNTR_ATTR_NUM:
			/** Take a snapshot for values of Media Counters */
			_get_media_cntrs_from_ethsw(instance_num, &(g_base_media_counters[instance_num-1]));
			break;
		default:
		    /** Return error code */
			return kEipStatusError;
			break;
		}
	}

    /** Return success code */
    return kEipStatusOk;
}

/*******************************************************************************************************************//**
 * @brief Get Interface Counters from ethsw
 *
 * @param[in] instance_num              Instance number
 * @param[in] p_interface_cntrs         Pointer of an Interface Counters array
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
static EipStatus _get_interface_cntrs_from_ethsw(EipUint32 instance_num, CipEthernetLinkInterfaceCounters * p_interface_cntrs)
{
#if defined(LWIP_SNMP) && LWIP_SNMP
	s32_t error_status = SNMP_ERR_NOERROR;
	struct snmp_obj_id oid;
	struct snmp_node_instance node_instance;
#endif

	/** Set a register base line */
	R_ETHSW_Type * p_ethsw_reg;
	p_ethsw_reg = (R_ETHSW_Type *) R_ETHSW_BASE;

	/** For storing the values of counters */
	uint32_t * p_reg_address;

	/** Store the values of Interface Counters */
	p_reg_address = (uint32_t *) &(p_ethsw_reg->AOCTETSRECEIVEDOK_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.in_octets= *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFINUCASTPKTS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.in_ucast = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFINMULTICASTPKTS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.in_nucast = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFINBROADCASTPKTS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.in_nucast += *p_reg_address;

#if defined(LWIP_SNMP) && LWIP_SNMP
	memcpy(oid.id, _ETHLINK_IFCNTR_INDISCARDS_OID, _ETHLINK_IFCNTR_OID_LEN * sizeof(u32_t)/sizeof(u8_t));
	oid.id[10] = instance_num;
	oid.len = _ETHLINK_IFCNTR_OID_LEN;
	error_status = snmp_get_node_instance_from_oid(&oid.id[0], oid.len, &node_instance);
	if (error_status == SNMP_ERR_NOERROR)
	{
		node_instance.get_value(&node_instance, &p_interface_cntrs->ul.in_discards);
	}
	else
	{
		p_interface_cntrs->ul.in_discards = 0x0000;
	}
#else
	p_interface_cntrs->ul.in_discards = 0x0000; /** In Discards statistics is got from TCP/IP stack */
#endif

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFINERRORS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.in_errors = *p_reg_address;

#if defined(LWIP_SNMP) && LWIP_SNMP
	memcpy(oid.id, _ETHLINK_IFCNTR_INUNKNOWNPROTOS_OID, _ETHLINK_IFCNTR_OID_LEN * sizeof(u32_t)/sizeof(u8_t));
	oid.id[10] = instance_num;
	oid.len = _ETHLINK_IFCNTR_OID_LEN;
	error_status = snmp_get_node_instance_from_oid(&oid.id[0], oid.len, &node_instance);
	if (error_status == SNMP_ERR_NOERROR)
	{
		node_instance.get_value(&node_instance, &p_interface_cntrs->ul.in_unknown_protos);
	}
	else
	{
		p_interface_cntrs->ul.in_unknown_protos = 0x0000;
	}
#else
	p_interface_cntrs->ul.in_unknown_protos = 0x0000; /** Unknown protocol packets statistics is got from TCP/IP stack */
#endif

	p_reg_address = (uint32_t *) &(p_ethsw_reg->AOCTETSTRANSMITTEDOK_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.out_octets = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFOUTUCASTPKTS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.out_ucast = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFOUTMULTICASTPKTS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.out_nucast = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFOUTBROADCASTPKTS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.out_nucast += *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFOUTDISCARDS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.out_discards = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFOUTERRORS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_interface_cntrs->ul.out_errors = *p_reg_address;

    /** Return success code */
    return kEipStatusOk;
}

/*******************************************************************************************************************//**
 * @brief Get Media Counters from ethsw
 *
 * @param[in] instance_num              Instance number
 * @param[in] p_media_cntrs             Pointer of an Media Counters array
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
static EipStatus _get_media_cntrs_from_ethsw(EipUint32 instance_num, CipEthernetLinkMediaCounters * p_media_cntrs)
{
	/** Set a register base line */
    R_ETHSW_Type * p_ethsw_reg;
    p_ethsw_reg = (R_ETHSW_Type *) R_ETHSW_BASE;

	/** For storing the values of counters */
	uint32_t * p_reg_address;

	/** Store the values of Media Counters */
	p_reg_address = (uint32_t *) &(p_ethsw_reg->AALIGNMENTERRORS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.align_errs = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->AFRAMECHECKSEQUENCEERRORS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.fcs_errs = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->ASINGLECOLLISIONS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.single_coll = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->AMULTIPLECOLLISIONS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.multi_coll = *p_reg_address;

	p_media_cntrs->ul.sqe_test_errs = 0x0; /** Not supported */

	p_reg_address = (uint32_t *) &(p_ethsw_reg->ADEFERRED_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.def_trans = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->ALATECOLLISIONS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.late_coll = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->AEXCESSIVECOLLISIONS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.exc_coll = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFOUTERRORS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.mac_tx_errs = *p_reg_address;

	p_reg_address = (uint32_t *) &(p_ethsw_reg->ACARRIERSENSEERRORS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.crs_errs = *p_reg_address;

	p_media_cntrs->ul.frame_too_long = 0x0; /** Not supported */

	p_reg_address = (uint32_t *) &(p_ethsw_reg->IFINERRORS_P0);
	p_reg_address += _ETHLINK_STATREG_OFFSET * (instance_num-1) / sizeof(uint32_t);
	p_media_cntrs->ul.mac_rx_errs = *p_reg_address;

    /** Return success code */
    return kEipStatusOk;
}
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */
