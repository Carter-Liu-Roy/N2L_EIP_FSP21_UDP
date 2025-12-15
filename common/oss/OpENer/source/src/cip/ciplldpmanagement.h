/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

/** @file
 * @brief Declare public interface of the LLDP Management Object
 *
 */

#ifndef OPENER_CIPLLDPMANAGEMENT_H_
#define OPENER_CIPLLDPMANAGEMENT_H_

#include "typedefs.h"
#include "ciptypes.h"

/** @brief LLDP Management Object class code */
static const CipUint kCipLLDPManagementClassCode = 0x0109U;

/* ********************************************************************
 * Type declarations
 */
/** @brief LLDP enable information
 *
 * This is the LLDP enable information.
 */
typedef struct {
  CipUint length; /**< LLDP Enable Array Length */
  CipByte array; /**< LLDP Enable Array */
} CipLLDPEnable;

/** @brief Type declaration for the LLDP Management Object
 *
 * This is the type declaration for the LLDP Management Object.
 */
typedef struct {
  CipLLDPEnable lldp_enable; /**< Attribute #1: */
  CipUint msgTxInterval; /**< Attribute #2: */
  CipUsint msgTxHold; /**< Attribute #3: */
  CipWord lldp_datastore; /**< Attribute #4: */
  CipUdint last_change; /**< Attribute #5: */
} CipLLDPManagementObject;


/* ********************************************************************
 * global public variables
 */
extern CipLLDPManagementObject g_lldpmanagement;  /**< declaration of LLDP Management Object instance 1 data */


/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the LLDP Management Object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipLLDPManagementInit(void);

#endif /* of OPENER_CIPLLDPMANAGEMENT_H_ */
