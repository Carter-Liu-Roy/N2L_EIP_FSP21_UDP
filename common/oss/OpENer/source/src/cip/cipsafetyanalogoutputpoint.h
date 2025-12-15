/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

/** @file
 * @brief Declare public interface of the Safety Analog Output Point Object
 *
 */

#ifndef OPENER_CIPSAFETYANALOGOUTPUTPOINT_H_
#define OPENER_CIPSAFETYANALOGOUTPUTPOINT_H_

#if defined(OPENER_CIP_SAFETY)
#include "typedefs.h"
#include "ciptypes.h"

/**
 * @brief Set this define to determine the number of instantiated Safety Analog Output Point Objects
 */
#define OPENER_SAFETYANALOGOUTPUTPOINT_INSTANCE_CNT         (2)

/** @brief Safety Analog Output Point Object class code */
static const CipUint kCipSafetyAnalogOutputPointClassCode = 0x64U;

/** @brief Constants for the Output Range member of the Safety Analog Output Point object. */
typedef enum {
  kSafetyAnalogOutputPointOutputRangen10to10volts = 0U,
  kSafetyAnalogOutputPointOutputRange0to5volts = 1U,
  kSafetyAnalogOutputPointOutputRange0to10volts = 2U,
  kSafetyAnalogOutputPointOutputRange4to20milliamps = 3U,
  kSafetyAnalogOutputPointOutputRangen15to75millivolts = 4U,
  kSafetyAnalogOutputPointOutputRangen15to30millivolts = 5U,
  kSafetyAnalogOutputPointOutputRangen5to5volts = 6U,
  kSafetyAnalogOutputPointOutputRange1to5volts = 7U,
  kSafetyAnalogOutputPointOutputRange0to20milliamp = 8U,
  kSafetyAnalogOutputPointOutputRange0to50milliamp = 9U,
} CipSafetyAnalogOutputPointOutputRange;

/** @brief Constants for the Output Channel Mode member of the Safety Analog Output Point object. */
typedef enum {
  kSafetyAnalogOutputPointOutputModeNotUsed = 0U,
  kSafetyAnalogOutputPointOutputModeSafety = 1U,
  kSafetyAnalogOutputPointOutputModeStandard = 2U,
} CipSafetyAnalogOutputPointOutputMode;

/** @brief Constants for the Output Point Status member of the Safety Analog Output Point object. */
typedef enum {
  kSafetyAnalogOutputPointOutputStatusAlarm = 0U,
  kSafetyAnalogOutputPointOutputStatusOK = 1U,
} CipSafetyAnalogOutputPointOutputStatus;

/** @brief Constants for the Point Fault Reason member of the Safety Analog Output Point object. */
typedef enum {
  kSafetyAnalogOutputPointFaultReasonOperatingNormally = 1U,
  kSafetyAnalogOutputPointFaultReasonOverRange = 2U,
  kSafetyAnalogOutputPointFaultReasonUnderRange = 3U,
  kSafetyAnalogOutputPointFaultReasonSignalIntegrityFailure = 4U,
  kSafetyAnalogOutputPointFaultReasonDiscrepancyError = 5U,
  kSafetyAnalogOutputPointFaultReasonPartnerDiscrepancyError = 6U,
  kSafetyAnalogOutputPointFaultReasonCalibrationFailure = 7U,
  kSafetyAnalogOutputPointFaultReasonOutputSignalnotUsed = 8U,
} CipSafetyAnalogOutputPointFaultReason;

/* ********************************************************************
 * Type declarations
 */
/** @brief Type declaration for the Safety Analog Output Point Object
 *
 * This is the type declaration for the Safety Analog Output Point Object.
 */
typedef struct {
  CipInt value; /**< Attribute #2: */
  CipUsint range; /**< Attribute #3: */
  CipUsint mode; /**< Attribute #4: */
  CipBool status; /**< Attribute #5: */
  CipUsint fault_reason; /**< Attribute #6: */
} CipSafetyAnalogOutputPointObject;


/* ********************************************************************
 * global public variables
 */
extern CipSafetyAnalogOutputPointObject g_safetyanalogoutputpoint[];


/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the Safety Analog Output Point Object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipSafetyAnalogOutputPointInit(void);
#endif

#endif /* of OPENER_CIPSAFETYANALOGOUTPUTPOINT_H_ */
