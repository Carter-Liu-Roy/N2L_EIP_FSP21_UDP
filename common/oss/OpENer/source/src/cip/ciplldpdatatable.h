/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

/** @file
 * @brief Declare public interface of the LLDP Data Table Object
 *
 * @author Stefan Maetje <stefan.maetje@esd.eu>
 *
 */

#ifndef OPENER_CIPLLDPDATATABLE_H_
#define OPENER_CIPLLDPDATATABLE_H_

#include "typedefs.h"
#include "ciptypes.h"

/**
 * @brief Set this define to determine the number of instantiated LLDP Data Table Objects
 *
 * At least 8 instances shall be supported. It is recommended that 48 instances are supported.
 */
#define OPENER_LLDPDATATABLE_INSTANCE_CNT         (8)

/** @brief LLDP Data Table Object class code */
static const CipUint kCipLLDPDataTableClassCode = 0x010AU;

/* ********************************************************************
 * Type declarations
 */
/** @brief System Capabilities TLV information
 *
 * This is the System Capabilities TLV information.
 */
typedef struct {
  CipWord system_capabilities; /**< System Capabilities */
  CipWord enabled_capabilities; /**< Enabled Capabilities */
} CipSystemCapabilitiesTLV;

/** @brief IPv4 Management Addresses information
 *
 * This is the IPv4 Management Addresses information.
 */
typedef struct {
  CipUsint management_address_count; /**< Management Address Count */
  CipUdint management_address[256]; /**< Management Address */
} CipIPv4ManagementAddresses;

/** @brief CIP Identification information
 *
 * This is the CIP Identification information.
 */
typedef struct {
  CipUint vendor_id; /**< Vendor ID */
  CipUint device_type; /**< Device Type */
  CipUint product_code; /**< Product Code */
  CipByte major_revision; /**< Major Revision */
  CipUsint minor_revision; /**< Minor Revision */
  CipUdint cip_serial_number; /**< CIP Serial Number */
} CipCIPIdentification;

/** @brief Additional Ethernet Capabilities information
 *
 * This is the Additional Ethernet Capabilities information.
 */
typedef struct {
  CipBool preemption_support; /**< Preemption Support */
  CipBool preemption_status; /**< Preemption Status */
  CipBool preemption_active; /**< Preemption Active */
  CipUsint additional_fragment_size; /**< Additional Fragment Size */
} CipAdditionalEthernetCapabilities;

/** @brief Type declaration for the LLDP Data Table Object
 *
 * This is the type declaration for the LLDP Data Table Object.
 */
typedef struct {
  CipUint ethernet_link_instance_number; /**< Attribute #1: */
  CipUsint mac_address[6]; /**< Attribute #2: */
  CipShortString interface_label; /**< Attribute #3: */
  CipUint time_to_live; /**< Attribute #4: */
  CipSystemCapabilitiesTLV system_capabilities_tlv; /**< Attribute #5: */
  CipIPv4ManagementAddresses ipv4_management_addresses; /**< Attribute #6: */
  CipCIPIdentification cip_identification; /**< Attribute #7: */
  CipAdditionalEthernetCapabilities additional_ethernet_capabilities; /**< Attribute #8: */
  CipUdint last_change; /**< Attribute #9: */
} CipLLDPDataTableObject;


/* ********************************************************************
 * global public variables
 */
extern CipLLDPDataTableObject g_lldpdatatable[];


/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the LLDP Data Table Object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipLLDPDataTableInit(void);
CipInstance *DelLLDPDataTableInstance(const EipUint32 instance_id);
void ALLDelLLDPDataTableInstance(void);

#endif /* of OPENER_CIPLLDPDATATABLE_H_ */
