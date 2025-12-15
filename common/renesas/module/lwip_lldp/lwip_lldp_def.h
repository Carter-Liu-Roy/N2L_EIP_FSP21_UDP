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

#ifndef LWIP_LLDP_DEF_H_
#define LWIP_LLDP_DEF_H_

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "lwip/opt.h"
#include "lwipopts.h"
#if defined(LWIP_LLDP_CIP_CONNECTED) && LWIP_LLDP_CIP_CONNECTED
  #include "cipidentity.h"
#endif
#if LWIP_LLDP /* don't build if not configured for use in lwipopts.h */

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/* MAC address length */
#define LWIP_LLDP_MAC_ADD_LEN    6

/* EtherType */
#define LWIP_LLDP_ETH_TYPE    0x88CC /* LLDP EtherType */

/* Maximum TLV length */
#define LWIP_LLDP_TLV_MAX_LEN    511 /* 511 octets */

/* TLV header length */
#define LWIP_LLDP_TLV_HEADER_LEN    2 /* 2 octets */

/* TLV bit mask */
#define LWIP_LLDP_TLV_TYPE_MASK    0xFE00 /* Type : 7 bits */
#define LWIP_LLDP_TLV_LEN_MASK     0x01FF /* Length : 9 bits */

/* TLV types */
#define LWIP_LLDP_TLV_TYPE_0      0 /* End of LLDPDU : Mandatory */
#define LWIP_LLDP_TLV_TYPE_1      1 /* Chassis ID : Mandatory */
#define LWIP_LLDP_TLV_TYPE_2      2 /* Port ID : Mandatory */
#define LWIP_LLDP_TLV_TYPE_3      3 /* Time To Live : Mandatory */
#define LWIP_LLDP_TLV_TYPE_4      4 /* Port Description : Optional */
#define LWIP_LLDP_TLV_TYPE_5      5 /* System Name : Optional */
#define LWIP_LLDP_TLV_TYPE_6      6 /* System Description : Optional */
#define LWIP_LLDP_TLV_TYPE_7      7 /* System Capabilities : Optional */
#define LWIP_LLDP_TLV_TYPE_8      8 /* Management Address : Optional */
#define LWIP_LLDP_TLV_TYPE_127    127 /* Organizationally Specific TLVs : Optional */

/* Chassis ID TLV */
/* chassis ID subtype length */
#define LWIP_LLDP_CHASSIS_ID_SUBTYPE_LEN    1 /* 1 octet */
/* chassis ID subtypes */
#define LWIP_LLDP_CHASSIS_ID_SUBTYPE_1    1 /* Chassis component */
#define LWIP_LLDP_CHASSIS_ID_SUBTYPE_2    2 /* interface alias */
#define LWIP_LLDP_CHASSIS_ID_SUBTYPE_3    3 /* Port component */
#define LWIP_LLDP_CHASSIS_ID_SUBTYPE_4    4 /* MAC address */
#define LWIP_LLDP_CHASSIS_ID_SUBTYPE_5    5 /* Network address */
#define LWIP_LLDP_CHASSIS_ID_SUBTYPE_6    6 /* interface name */
#define LWIP_LLDP_CHASSIS_ID_SUBTYPE_7    7 /* Locally assigned */
/* Minimum chassis ID length */
#define LWIP_LLDP_CHASSIS_ID_MIN_LEN    1 /* 1 octet */
/* Maximum chassis ID length */
#define LWIP_LLDP_CHASSIS_ID_MAX_LEN    255 /* 255 octets */
/* Minimum Chassis ID TLV information string length */
#define LWIP_LLDP_CHASSIS_ID_TLV_MIN_LEN    2 /* 2 octets */
/* Maximum Chassis ID TLV information string length */
#define LWIP_LLDP_CHASSIS_ID_TLV_MAX_LEN    256 /* 256 octets */

/* Port ID TLV */
/* port ID subtype length */
#define LWIP_LLDP_PORT_ID_SUBTYPE_LEN    1 /* 1 octet */
/* port ID subtypes */
#define LWIP_LLDP_PORT_ID_SUBTYPE_1    1 /* interface alias */
#define LWIP_LLDP_PORT_ID_SUBTYPE_2    2 /* Port component */
#define LWIP_LLDP_PORT_ID_SUBTYPE_3    3 /* MAC address */
#define LWIP_LLDP_PORT_ID_SUBTYPE_4    4 /* Network address */
#define LWIP_LLDP_PORT_ID_SUBTYPE_5    5 /* interface name */
#define LWIP_LLDP_PORT_ID_SUBTYPE_6    6 /* Agent circuit ID */
#define LWIP_LLDP_PORT_ID_SUBTYPE_7    7 /* Locally assigned */
/* Minimum port ID length */
#define LWIP_LLDP_PORT_ID_MIN_LEN    1 /* 1 octet */
/* Maximum port ID length */
#define LWIP_LLDP_PORT_ID_MAX_LEN    255 /* 255 octets */
/* Minimum Port ID TLV information string length */
#define LWIP_LLDP_PORT_ID_TLV_MIN_LEN    2 /* 2 octets */
/* Maximum Port ID TLV information string length */
#define LWIP_LLDP_PORT_ID_TLV_MAX_LEN    256 /* 256 octets */

/* Time To Live TLV */
/* Time To Live TLV information string length */
#define LWIP_LLDP_TTL_TLV_LEN    2 /* 2 octets */

/* Port Description TLV */
/* Minimum port description length */
#define LWIP_LLDP_PORT_DESC_TLV_MIN_LEN    0 /* 0 octet */
/* Maximum port description length */
#define LWIP_LLDP_PORT_DESC_TLV_MAX_LEN    255 /* 255 octets */

/* System Name TLV */
/* Minimum System Name TLV information string length */
#define LWIP_LLDP_SYSTEM_NAME_TLV_MIN_LEN    0 /* 0 octet */
/* Maximum System Name TLV information string length */
#define LWIP_LLDP_SYSTEM_NAME_TLV_MAX_LEN    255 /* 255 octets */

/* System Description TLV */
/* Minimum System Description TLV information string length */
#define LWIP_LLDP_SYSTEM_DESC_TLV_MIN_LEN    0 /* 0 octet */
/* Maximum System Description TLV information string length */
#define LWIP_LLDP_SYSTEM_DESC_TLV_MAX_LEN    255 /* 255 octets */

/* System Capabilities TLV */
/* System capabilities */
#define LWIP_LLDP_SYSTEM_CAPA_BIT_0     0x01 /* Other */
#define LWIP_LLDP_SYSTEM_CAPA_BIT_1     (0x01 << 1) /* Repeater */
#define LWIP_LLDP_SYSTEM_CAPA_BIT_2     (0x01 << 2) /* Bridge */
#define LWIP_LLDP_SYSTEM_CAPA_BIT_3     (0x01 << 3) /* WLAN Access Point */
#define LWIP_LLDP_SYSTEM_CAPA_BIT_4     (0x01 << 4) /* Router */
#define LWIP_LLDP_SYSTEM_CAPA_BIT_5     (0x01 << 5) /* Telephone */
#define LWIP_LLDP_SYSTEM_CAPA_BIT_6     (0x01 << 6) /* DOCSIS cable device */
#define LWIP_LLDP_SYSTEM_CAPA_BIT_7     (0x01 << 7) /* Station Only */
/* System system capabilities length */
#define LWIP_LLDP_SYSTEM_CAPA_LEN    2 /* 2 octets */
/* System enabled capabilities string length */
#define LWIP_LLDP_ENABLED_CAPA_LEN    2 /* 2 octets */
/* System Capabilities TLV information string length */
#define LWIP_LLDP_SYSTEM_CAPA_TLV_LEN    4 /* 4 octets */

/* Management Address TLV */
/* management address string length's length */
#define LWIP_LLDP_MANA_ADD_STR_LEN_LEN    1 /* 1 octet */
/* management address subtype length */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_LEN    1 /* 1 octet */
/* management address subtype */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_0     0  /* Reserved */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_1     1  /* IP (IP version 4) */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_2     2  /* IP6 (IP version 6) */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_3     3  /* NSAP */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_4     4  /* HDLC (8-bit multidrop) */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_5     5  /* BBN 1822 */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_6     6  /* 802 (includes all 802 media plus Ethernet "canonical format") */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_7     7  /* E.163 */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_8     8  /* E.164 (SMDS, Frame Relay, ATM) */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_9     9  /* F.69 (Telex) */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_10    10 /* X.121 (X.25, Frame Relay) */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_11    11 /* IPX */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_12    12 /* Appletalk */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_13    13 /* Decnet IV */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_14    14 /* Banyan Vines */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_15    15 /* E.164 with NSAP format subaddress */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_16    16 /* DNS (Domain Name System) */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_17    17 /* Distinguished Name */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_18    18 /* AS Number */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_19    19 /* XTP over IP version 4 */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_20    20 /* XTP over IP version 6 */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_21    21 /* XTP native mode XTP */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_22    22 /* Fibre Channel World-Wide Port Name */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_23    23 /* Fibre Channel World-Wide Node Name */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_24    24 /* GWID */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_25    25 /* AFI for L2VPN information */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_26    26 /* MPLS-TP Section Endpoint32_t Identifier */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_27    27 /* MPLS-TP LSP Endpoint32_t Identifier */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_28    28 /* MPLS-TP Pseudowire Endpoint32_t Identifier */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_29    29 /* MT IP: Multi-Topology IP version 4 */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_30    30 /* MT IPv6: Multi-Topology IP version 6 */
#define LWIP_LLDP_MANA_ADD_SUBTYPE_31    31 /* BGP SFC */
/* Minimum management address string length */
#define LWIP_LLDP_MANA_ADD_MIN_LEN    2 /* 2 octets */
/* Maximum management address string length */
#define LWIP_LLDP_MANA_ADD_MAX_LEN    32 /* 32 octets */
/* interface numbering subtype length */
#define LWIP_LLDP_INTERFACE_NUM_SUBTYPE_LEN    1 /* 1 octet */
/* interface numbering subtype */
#define LWIP_LLDP_INTERFACE_NUM_SUBTYPE_1    1 /* Unknown */
#define LWIP_LLDP_INTERFACE_NUM_SUBTYPE_2    2 /* ifIndex */
#define LWIP_LLDP_INTERFACE_NUM_SUBTYPE_3    3 /* system port number */
/* interface number length */
#define LWIP_LLDP_INTERFACE_NUM_LEN    4 /* 4 octets */
/* object identifier (OID) string length */
#define LWIP_LLDP_OID_STR_LEN    1 /* 1 octet */
/* Minimum object identifier length */
#define LWIP_LLDP_OID_MIN_LEN    0 /* 0 octets */
/* Maximum object identifier length */
#define LWIP_LLDP_OID_MAX_LEN    128 /* 128 octets */
/* Minimum Management Address TLV information string length */
#define LWIP_LLDP_MANA_ADD_TLV_MIN_LEN    9 /* 9 octets */
/* Maximum Management Address TLV information string length */
#define LWIP_LLDP_MANA_ADD_TLV_MAX_LEN    167 /* 167 octets */

/* Organizationally Specific TLVs */
/* Organizationally Specific TLV bit mask */
#define LWIP_LLDP_ORG_TLV_OUI_MASK        0xFFFFFF00 /* organizationally unique identifier (OUI) : 24bits */
#define LWIP_LLDP_ORG_TLV_SUBTYPE_MASK    0x000000FF /* organizationally defined subtype : 8 bits */
/* organizationally unique identifier (OUI) length */
#define LWIP_LLDP_OUI_LEN    3 /* 3 octets */
/* organizationally defined subtype length */
#define LWIP_LLDP_ORG_SUBTYPE_LEN    1 /* 1 octets */
/* Minimum organizationally defined information string length */
#define LWIP_LLDP_ORG_INFO_MIN_LEN    0 /* 0 octet */
/* Maximum organizationally defined information string length */
#define LWIP_LLDP_ORG_INFO_MAX_LEN    507 /* 507 octets */
/* Minimum Organizationally Specific TLV information string length */
#define LWIP_LLDP_ORG_TLV_MIN_LEN    4 /* 4 octets */
/* Maximum Organizationally Specific TLV information string length */
#define LWIP_LLDP_ORG_TLV_MAX_LEN    511 /* 511 octets */

/* The recommended default value for msgTxHold is 4. */
#define LWIP_LLDP_TX_DEFAULT_MSGTXHOLD    4
/* The recommended default value for msgTxInterval is 30 seconds. */
#define LWIP_LLDP_TX_DEFAULT_MSGTXINTERVAL    30
/* The recommended default value for reinitDelay is 2 seconds. */
#define LWIP_LLDP_TX_DEFAULT_REINITDELAY    2
/* The recommended default value for txDelay is 2 seconds. */
#define LWIP_LLDP_TX_DEFAULT_TXDELAY    2

/* Transmit LLDPDU */
/* Basic TLVs bit map */
#define LWIP_LLDP_BASIC_TLV_CAPA_BIT_0     0x01 /* portDesc(0) */
#define LWIP_LLDP_BASIC_TLV_CAPA_BIT_1     (0x01 << 1) /* sysName(1) */
#define LWIP_LLDP_BASIC_TLV_CAPA_BIT_2     (0x01 << 2) /* sysDesc(2) */
#define LWIP_LLDP_BASIC_TLV_CAPA_BIT_3     (0x01 << 3) /* sysCap(3) */

/* Support default basic TLVs */
#define LWIP_LLDP_DEFAULT_BASIC_TLV    (LWIP_LLDP_BASIC_TLV_CAPA_BIT_0 | LWIP_LLDP_BASIC_TLV_CAPA_BIT_1 | LWIP_LLDP_BASIC_TLV_CAPA_BIT_2 | LWIP_LLDP_BASIC_TLV_CAPA_BIT_3)

/* Support CIP TLVs */
#if defined(LWIP_LLDP_CIP_CONNECTED) && LWIP_LLDP_CIP_CONNECTED
  #define LWIP_LLDP_SUPPORT_CIP_TLV        true
  #define LWIP_LLDP_CIP_MAC_TLV_LEN        6
  #define LWIP_LLDP_CIP_LABEL_TLV_MAX_LEN  255 /* 255 octets */
  #define LWIP_LLDP_CIP_POSID_TLV_LEN      6
  #define LWIP_LLDP_CIP_T1SPHY_TLV_LEN     6
#else
  #define LWIP_LLDP_SUPPORT_CIP_TLV    false
#endif
/* Support maximum number of tlv */
#define LWIP_LLDP_SUPPORT_TX_MAX_TLV    16

/* Receive LLDPDU */
/* Support maximum number of neighbors */
#define LWIP_LLDP_SUPPORT_RX_MAX_NEIGHBORS    4

/* Support maximum number of unrecognized TLV informations */
#define LWIP_LLDP_SUPPORT_RX_MAX_UNKNOWN_TLV_INFO    4

/* Support maximum number of organizationally defined informations */
#define LWIP_LLDP_SUPPORT_RX_MAX_ORG_DEF_INFO    4

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/* adminStatus */
typedef enum {
    LWIP_LLDP_enabledTxOnly = 1,
    LWIP_LLDP_enabledRxOnly = 2,
    LWIP_LLDP_enabledRxTx = 3,
    LWIP_LLDP_disabled = 4
} LWIP_LLDP_ADMIN_STATUS;

/* Global variables */
typedef struct {
    int32_t adminStatus; /* adminStatus */
    bool snmpChangeStatus; /* Flag for adminStatus update by SNMP */
    bool portEnabled; /* portEnabled */
} LWIP_LLDP_GLOBAL_VARIABLES;

/* Transmission counters */
typedef struct {
    uint32_t statsFramesOutTotal; /* statsFramesOutTotal */
} LWIP_LLDP_TX_COUNTERS;

/* Reception counters */
typedef struct {
    uint32_t statsAgeoutsTotal; /* statsAgeoutsTotal */
    uint32_t statsFramesDiscardedTotal; /* statsFramesDiscardedTotal */
    uint32_t statsFramesInErrorsTotal; /* statsFramesInErrorsTotal */
    uint32_t statsFramesInTotal; /* statsFramesInTotal */
    uint32_t statsTLVsDiscardedTotal; /* statsTLVsDiscardedTotal */
    uint32_t statsTLVsUnrecognizedTotal; /* statsTLVsUnrecognizedTotal */
} LWIP_LLDP_RX_COUNTERS;

/* Transmit state machine timers */
typedef struct {
    uint16_t txShutdownWhile; /* txShutdownWhile */
    uint16_t txDelayWhile; /* txDelayWhile */
    uint16_t txTTR; /* txTTR */
} LWIP_LLDP_TX_SM_TIMERS;

/* Receive state machine timers */
typedef struct {
    uint16_t rxInfoTTL; /* rxInfoTTL */
    uint16_t TooManyNeighborsTimer; /* TooManyNeighborsTimer */
} LWIP_LLDP_RX_SM_TIMERS;

/* Transmit state machine timing parameters */
typedef struct {
#if defined(LWIP_LLDP_CIP_CONNECTED) && 1 != LWIP_LLDP_CIP_CONNECTED
    int32_t msgTxHold; /* msgTxHold */
    int32_t msgTxInterval; /* msgTxInterval */
#else
    CipUsint * msgTxHold; /* msgTxHold */
    CipUint * msgTxInterval; /* msgTxInterval */
#endif
    int32_t reinitDelay; /* reinitDelay */
    int32_t txDelay; /* txDelay */
} LWIP_LLDP_TX_SM_TIMING_PARAMETERS;

/* Transmit state machine variables */
typedef struct {
    bool somethingChangedLocal; /* somethingChangedLocal */
    uint16_t txTTL; /* txTTL */
} LWIP_LLDP_TX_SM_VARIABLES;

/* Receive state machine variables */
typedef struct {
    bool badFrame; /* badFrame */
    bool rcvFrame; /* rcvFrame */
    bool rxChanges; /* rxChanges */
    bool rxInfoAge; /* rxInfoAge */
    uint16_t rxTTL; /* rxTTL */
    bool somethingChangedRemote; /* somethingChangedRemote */
    bool tooManyNeighbors; /* tooManyNeighbors */
} LWIP_LLDP_RX_SM_VARIABLES;

/* TLV selection management */
typedef struct {
    uint8_t mibBasicTLVsTxEnable;     /* mibBasicTLVsTxEnable */
    bool mibMgmtAddrInstanceTxEnable; /* mibMgmtAddrInstanceTxEnable */
    bool mibCipInstanceTxEnable;      /* mibCipInstanceTxEnable */
} LWIP_LLDP_TLV_SELECTION_MANA;

/* Basic Organizationally Specific TLV format */
typedef struct {
    uint8_t oui[LWIP_LLDP_OUI_LEN]; /* organizationally unique identifier (OUI) */
    uint8_t org_sub; /* organizationally defined subtype */
    uint16_t org_info_len; /* length of organizationally defined information string */
    uint8_t org_info[LWIP_LLDP_ORG_INFO_MAX_LEN]; /* organizationally defined information string */
} LWIP_LLDP_RCV_OUI_INFO;

/* Transmit TLV required variables */
typedef struct {
    /* Chassis ID TLV objects */
    uint8_t chassis_id_subtype; /* chassis ID subtype */
    uint16_t chassis_id_len; /* length of chassis ID */
    uint8_t chassis_id[LWIP_LLDP_CHASSIS_ID_MAX_LEN]; /* chassis ID */
    /* Port ID TLV objects */
    uint8_t port_id_subtype; /* port ID subtype */
    uint16_t port_id_len; /* length of port ID */
    uint8_t port_id[LWIP_LLDP_PORT_ID_MAX_LEN]; /* port ID */
    /* Port description TLV object */
    uint16_t port_desc_len; /* length of port description */
    uint8_t port_desc[LWIP_LLDP_PORT_DESC_TLV_MAX_LEN]; /* port description */
    /* System name TLV object */
    uint16_t system_name_len; /* length of system name */
    uint8_t system_name[LWIP_LLDP_SYSTEM_NAME_TLV_MAX_LEN]; /* system name */
    /* System description TLV object */
    uint16_t system_desc_len; /* length of system description */
    uint8_t system_desc[LWIP_LLDP_SYSTEM_DESC_TLV_MAX_LEN]; /* system description */
    /* System capabilities TLV objects */
    uint16_t system_capa; /* system capabilities */
    uint16_t enabled_capa; /* enabled capabilities */
    /* Management address TLV objects */
    uint8_t mana_add_len; /* management address length */
    uint8_t mana_add_subtype; /* management address subtype */
    uint8_t mana_add[LWIP_LLDP_MANA_ADD_MAX_LEN]; /* management address */
    uint8_t interface_num_subtype; /* interface numbering subtype */
    uint32_t interface_num; /* interface number */
    uint16_t oid_len; /* OID length */
    uint8_t oid[LWIP_LLDP_OID_MAX_LEN]; /* OID */
#if LWIP_LLDP_SUPPORT_CIP_TLV
    /* CIP Identification TLV objects for EtherNet/IP ODVA */
    CipUint * cip_vendor_id;      /* Identity Class Attribute 1: Vendor ID               */
    CipUint * cip_device_type;    /* Identity Class Attribute 2: Device Type             */
    CipUint * cip_product_code;   /* Identity Class Attribute 3: Product Code            */
    CipRevision * cip_revision;   /* Identity Class Attribute 4: Revision / Major, Minor */
    CipUdint * cip_serial_number; /* Identity Class Attribute 6: Serial Number           */
    CipUsint cip_mac_address[LWIP_LLDP_CIP_MAC_TLV_LEN];    /* Ethernet Link Class Attribute  3: Physical Address  */
    CipUint  cip_if_label_len;                              /* Ethernet Link Class Attribute 10: Interface Label   */
    CipUsint cip_if_label[LWIP_LLDP_CIP_LABEL_TLV_MAX_LEN]; /* Ethernet Link Class Attribute 10: Interface Label   */
#endif
} LWIP_LLDP_TX_TLV_VARIABLES;


/* Receive TLV required variables */
typedef struct {
    /* Chassis ID TLV objects */
    uint8_t chassis_id_subtype; /* chassis ID subtype */
    uint16_t chassis_id_len; /* length of chassis ID */
    uint8_t chassis_id[LWIP_LLDP_CHASSIS_ID_MAX_LEN]; /* chassis ID */
    /* Port ID TLV objects */
    uint8_t port_id_subtype; /* port ID subtype */
    uint16_t port_id_len; /* length of port ID */
    uint8_t port_id[LWIP_LLDP_PORT_ID_MAX_LEN]; /* port ID */
    /* Port description TLV object */
    uint16_t port_desc_len; /* length of port description */
    uint8_t port_desc[LWIP_LLDP_PORT_DESC_TLV_MAX_LEN]; /* port description */
    /* System name TLV object */
    uint16_t system_name_len; /* length of system name */
    uint8_t system_name[LWIP_LLDP_SYSTEM_NAME_TLV_MAX_LEN]; /* system name */
    /* System description TLV object */
    uint16_t system_desc_len; /* length of system description */
    uint8_t system_desc[LWIP_LLDP_SYSTEM_DESC_TLV_MAX_LEN]; /* system description */
    /* System capabilities TLV objects */
    uint16_t system_capa; /* system capabilities */
    uint16_t enabled_capa; /* enabled capabilities */
    /* Management address TLV objects */
    uint8_t mana_add_len; /* management address length */
    uint8_t mana_add_subtype; /* management address subtype */
    uint8_t mana_add[LWIP_LLDP_MANA_ADD_MAX_LEN]; /* management address */
    uint8_t interface_num_subtype; /* interface numbering subtype */
    uint32_t interface_num; /* interface number */
    uint16_t oid_len; /* OID length */
    uint8_t oid[LWIP_LLDP_OID_MAX_LEN]; /* OID */
#if LWIP_LLDP_SUPPORT_CIP_TLV
    /* CIP Identification TLV objects for EtherNet/IP ODVA */
    CipUint cip_vendor_id;      /* Identity Class Attribute 1: Vendor ID               */
    CipUint cip_device_type;    /* Identity Class Attribute 2: Device Type             */
    CipUint cip_product_code;   /* Identity Class Attribute 3: Product Code            */
    CipRevision cip_revision;   /* Identity Class Attribute 4: Revision / Major, Minor */
    CipUdint cip_serial_number; /* Identity Class Attribute 6: Serial Number           */
    CipUsint cip_mac_address[LWIP_LLDP_CIP_MAC_TLV_LEN];     /* Ethernet Link Class Attribute  3: Physical Address */
    CipUint  cip_if_label_len;                               /* Ethernet Link Class Attribute 10: Interface Label  */
    CipUsint cip_if_label[LWIP_LLDP_CIP_LABEL_TLV_MAX_LEN];  /* Ethernet Link Class Attribute 10: Interface Label  */
    CipUint  cip_posision_id;   /* Line Link Class Attribute 1: Position ID            */
    CipUsint  cip_phy_mode;     /* Ethernet Link Class Attribute 16: PHY Mode, PHY Node ID for PLCA */
    CipUsint  cip_phy_node_id;  /* Ethernet Link Class Attribute 16: PHY Mode, PHY Node ID for PLCA */
#endif
    uint8_t rcv_unknown_tlv[LWIP_LLDP_SUPPORT_RX_MAX_UNKNOWN_TLV_INFO]; /* Received unrecognized TLV informations */
    LWIP_LLDP_RCV_OUI_INFO rcv_org_def[LWIP_LLDP_SUPPORT_RX_MAX_ORG_DEF_INFO]; /* Received organizationally defined informations */
} LWIP_LLDP_RX_TLV_VARIABLES;

/* Stats Rem Tables */
typedef struct {
    uint32_t last_change_time; /* lldpStatsRemTablesLastChangeTime */
    uint32_t inserts; /* lldpStatsRemTablesInserts */
    uint32_t deletes; /* lldpStatsRemTablesDeletes */
    uint32_t drops; /* lldpStatsRemTablesDrops */
    uint32_t ageouts; /* lldpStatsRemTablesAgeouts */
} LWIP_LLDP_STATS_REM_TABLES;

/* Receive variables */
typedef struct {
    int32_t interval; /* lldpNotificationInterval */
    bool enable; /* lldpPortConfigNotificationEnable */
} LWIP_LLDP_NOTIFICATION;

/* Transmit state */
typedef enum {
    TX_LLDP_INITIALIZE = 1,
    TX_LLDP_IDLE = 2,
    TX_LLDP_SHUTDOWN_FRAME = 3,
    TX_LLDP_INFO_FRAME = 4
} LWIP_LLDP_TX_STATE;

/* Receive state */
typedef enum {
    RX_LLDP_WAIT_PORT_OPERATIONAL = 1,
    RX_LLDP_DELETE_AGED_INFO = 2,
    RX_LLDP_INITIALIZE = 3,
    RX_LLDP_WAIT_FOR_FRAME = 4,
    RX_LLDP_FRAME = 5,
    RX_LLDP_DELETE_INFO = 6,
    RX_LLDP_UPDATE_INFO = 7
} LWIP_LLDP_RX_STATE;

/* Transmit variables */
typedef struct {
    uint8_t tx_state; /* Transmit state machine */
    LWIP_LLDP_TX_COUNTERS tx_counters; /* Transmission counters */
    LWIP_LLDP_TX_SM_TIMERS tx_timers; /* Transmit state machine timers */
    LWIP_LLDP_TX_SM_TIMING_PARAMETERS tx_parameters; /* Transmit state machine timing parameters */
    LWIP_LLDP_TX_SM_VARIABLES tx_variables; /* Transmit state machine variables */
    LWIP_LLDP_TX_TLV_VARIABLES local_system_mib; /* Transmit TLV required variables */
    LWIP_LLDP_TX_TLV_VARIABLES old_local_system_mib; /* Old transmit TLV required variables */
    void * p_tx_func[LWIP_LLDP_SUPPORT_TX_MAX_TLV]; /* Pointer of tlv function */
} LWIP_LLDP_TX_VARIABLES;

/* Receive variables */
typedef struct {
    uint8_t rx_state; /* Receive state machine */
    LWIP_LLDP_RX_COUNTERS rx_counters; /* Reception counters */
    LWIP_LLDP_RX_SM_TIMERS rx_timers; /* Receive state machine timers */
    LWIP_LLDP_RX_SM_VARIABLES rx_variables; /* Receive state machine variables */
    bool remote_system_mib_used[LWIP_LLDP_SUPPORT_RX_MAX_NEIGHBORS]; /* Receive system mib used */
    uint32_t rem_time_mark[LWIP_LLDP_SUPPORT_RX_MAX_NEIGHBORS]; /* Received timestamp */
    LWIP_LLDP_RX_TLV_VARIABLES remote_system_mib[LWIP_LLDP_SUPPORT_RX_MAX_NEIGHBORS]; /* Receive TLV required variables */
    LWIP_LLDP_STATS_REM_TABLES stats_rem_tables; /* Stats Rem Tables */
    LWIP_LLDP_NOTIFICATION notification; /* Notification */
} LWIP_LLDP_RX_VARIABLES;

/* LLDP agent variables */
typedef struct {
    struct netif *netif; /* The network interface */
    LWIP_LLDP_GLOBAL_VARIABLES global; /* Global variables */
    LWIP_LLDP_TLV_SELECTION_MANA tlv; /* TLV selection management */
    LWIP_LLDP_TX_VARIABLES tx; /* Transmit variables */
    LWIP_LLDP_RX_VARIABLES rx; /* Receive variables */
} LWIP_LLDP_AGENT_VARIABLES;

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global functions (to be accessed by other files)
 ***********************************************************************************************************************/

#endif

#endif /* LWIP_LLDP_DEF_H_ */
