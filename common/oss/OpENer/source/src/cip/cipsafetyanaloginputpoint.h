/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

/** @file
 * @brief Declare public interface of the Safety Analog Input Point Object
 *
 */

#ifndef OPENER_CIPSAFETYANALOGINPUTPOINT_H_
#define OPENER_CIPSAFETYANALOGINPUTPOINT_H_

#if defined(OPENER_CIP_SAFETY)
#include "typedefs.h"
#include "ciptypes.h"

/**
 * @brief Set this define to determine the number of instantiated Safety Analog Input Point Objects
 */
#define OPENER_SAFETYANALOGINPUTPOINT_INSTANCE_CNT         (2)

/** @brief Safety Analog Input Point Object class code */
static const CipUint kCipSafetyAnalogInputPointClassCode = 0x49U;

/** @brief Constants for the Input Range member of the Safety Analog Input Point object. */
typedef enum {
  kSafetyAnalogInputPointInputRangen10to10volts = 0U,
  kSafetyAnalogInputPointInputRange0to5volts = 1U,
  kSafetyAnalogInputPointInputRange0to10volts = 2U,
  kSafetyAnalogInputPointInputRange4to20milliamps = 3U,
  kSafetyAnalogInputPointInputRangen15to75millivolts = 4U,
  kSafetyAnalogInputPointInputRangen15to30millivolts = 5U,
  kSafetyAnalogInputPointInputRangen5to5volts = 6U,
  kSafetyAnalogInputPointInputRange1to5volts = 7U,
  kSafetyAnalogInputPointInputRange0to20milliamp = 8U,
  kSafetyAnalogInputPointInputRange0to50milliamp = 9U,
} CipSafetyAnalogInputPointInputRange;

/** @brief Constants for the Input Channel Mode member of the Safety Analog Input Point object. */
typedef enum {
  kSafetyAnalogInputPointInputModeNotUsed = 0U,
  kSafetyAnalogInputPointInputModeSafety = 1U,
  kSafetyAnalogInputPointInputModeStandard = 2U,
} CipSafetyAnalogInputPointInputMode;

/** @brief Constants for the Input Point Status member of the Safety Analog Input Point object. */
typedef enum {
  kSafetyAnalogInputPointInputStatusAlarm = 0U,
  kSafetyAnalogInputPointInputStatusOK = 1U,
} CipSafetyAnalogInputPointInputStatus;

/** @brief Constants for the Point Fault Reason member of the Safety Analog Input Point object. */
typedef enum {
  kSafetyAnalogInputPointFaultReasonOperatingNormally = 1U,
  kSafetyAnalogInputPointFaultReasonOverRange = 2U,
  kSafetyAnalogInputPointFaultReasonUnderRange = 3U,
  kSafetyAnalogInputPointFaultReasonSignalIntegrityFailure = 4U,
  kSafetyAnalogInputPointFaultReasonDiscrepancyError = 5U,
  kSafetyAnalogInputPointFaultReasonPartnerDiscrepancyError = 6U,
  kSafetyAnalogInputPointFaultReasonCalibrationFailure = 7U,
  kSafetyAnalogInputPointFaultReasonInputSignalnotUsed = 8U,
} CipSafetyAnalogInputPointFaultReason;

/* ********************************************************************
 * Type declarations
 */
/** @brief Type declaration for the Safety Analog Input Point Object
 *
 * This is the type declaration for the Safety Analog Input Point Object.
 */
typedef struct {
  CipInt value; /**< Attribute #2: */
  CipUsint range; /**< Attribute #3: */
  CipUsint mode; /**< Attribute #4: */
  CipBool status; /**< Attribute #5: */
  CipUsint fault_reason; /**< Attribute #6: */
} CipSafetyAnalogInputPointObject;


/* ********************************************************************
 * global public variables
 */
extern CipSafetyAnalogInputPointObject g_safetyanaloginputpoint[];


/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the Safety Analog Input Point Object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipSafetyAnalogInputPointInit(void);
#endif

#endif /* of OPENER_CIPSAFETYANALOGINPUTPOINT_H_ */
