/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

/** @file
 * @brief Declare public interface of the Safety Validator Object
 *
 */

#ifndef OPENER_CIPSAFETYVALIDATOR_H_
#define OPENER_CIPSAFETYVALIDATOR_H_

#if defined(OPENER_CIP_SAFETY)
#include "typedefs.h"
#include "ciptypes.h"

/**
 * @brief Set this define to determine the number of instantiated Safety Validator Objects
 */
#define OPENER_SAFETYVALIDATOR_INSTANCE_CNT         (2)

/**
 * @brief Set this define to determine the maximum number of Counter Array Size
 *
 * Size of array equals Max Consumer number for multicast producers and 
 * 1 and 1 for single-cast and multicast consumer.
 */
#define OPENER_SAFETYVALIDATOR_MAX_CNT_SIZE         (1)

/** @brief Safety Validator Object class code */
static const CipUint kCipSafetyValidatorClassCode = 0x3AU;

/** @brief Constants for the Safety Validator State member of the Safety Supervisor object. */
typedef enum {
  kSafetyValidatorStateUnallocated = 0U,
  kSafetyValidatorStateInitializing = 1U,
  kSafetyValidatorStateEstablished = 2U,
  kSafetyValidatorStateConnectionFailed = 3U,
} CipSafetyValidatorState;

/* Declare constants for Safety Validator Type (#2) */
/** Indicates Safety Connection Type is Unallocated. */
static const CipByte kSafetyValidatorTypeUnallocated = 0x00U;
/** Indicates Safety Connection Type is Single-cast. */
static const CipByte kSafetyValidatorTypeSinglecast = 0x01U;
/** Indicates Safety Connection Type is Multi-cast. */
static const CipByte kSafetyValidatorTypeMulticast = 0x02U;
/** Indicates Safety Validator Type is Producer(client). */
static const CipByte kSafetyValidatorTypeProducer = 0x00U;
/** Indicates Safety Validator Type is Consumer(server). */
static const CipByte kSafetyValidatorTypeConsumer = 0x80U;

/** Indicates Single Cast Producer (Input) Connection Type 0x01 */
static const CipByte kSafetyValidatorTypeSingleProducer = 0x01U;
/** Indicates Single Cast Consumer (Output) Connection Type 0x81 */
static const CipByte kSafetyValidatorTypeSingleConsumer = 0x81U;
/** Indicates Multi-Cast Producer Connection Type 0x02 */
static const CipByte kSafetyValidatorTypeMultiProducer = 0x02U;
/** Indicates Multi-Cast Consumer Connection Type 0x82 */
static const CipByte kSafetyValidatorTypeMultiConsumer = 0x82U;


/* ********************************************************************
 * Type declarations
 */
/** @brief Producer/Consumer Fault Counters
 *
 * This is the Producer/Consumer Fault Counters.
 */
typedef struct {
  CipUsint size; /**< Producer/Consumer Counter Array Size */
  CipUsint counter[OPENER_SAFETYVALIDATOR_MAX_CNT_SIZE]; /**< Producer/Consumer Fault Counter */
} CipFaultCounters;

/** @brief Type declaration for the Safety Validator Object
 *
 * This is the type declaration for the Safety Validator Object.
 */
typedef struct {
  CipUsint state; /**< Attribute #1: */
  CipUsint type; /**< Attribute #2: */
  CipUint max_data_age; /**< Attribute #12: */
  CipEpath app_data_path; /**< Attribute #13: */
  CipFaultCounters fault_counter; /**< Attribute #15: */
} CipSafetyValidatorObject;


/* ********************************************************************
 * global public variables
 */
extern CipSafetyValidatorObject g_safetyvalidator[];


/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the Safety Validator Object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipSafetyValidatorInit(void);

/** @brief Create the instance of the Safety Validator Object
 *
 * @return Pointer of created instance on success, otherwise NULL
 */
CipInstance *CreateSafetyValidatorObject(const EipUint32 assembly_instance_id);

/** @brief Delete the instance of the Safety Validator Object
 *
 * @return Pointer of deleted instance on success, otherwise NULL
 */
CipInstance *DeleteSafetyValidatorInstance(const EipUint32 assembly_instance_id);

/** @brief Set Max Data Age & Fault Counter of the Safety Validator Object
 *
 * @return 0 on success, otherwise -1
 */
int CipSafetySafetyValidatorSetValidator(size_t idx, CipUint max_data_age, CipUsint fault_counter_size, CipUsint fault_counter);

/** @brief Set Safety Connection Fault Count of the Safety Validator Object
 */
void CipSafetySafetyValidatorSetSafetyConnectionFaultCount(CipUint safety_connection_fault_count);
#endif

#endif /* of OPENER_CIPSAFETYVALIDATOR_H_ */
