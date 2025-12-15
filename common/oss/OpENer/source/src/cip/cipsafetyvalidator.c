/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the Safety Validator Object
 *
 *  CIP Safety Validator Object
 *  ==============
 *
 *  This module implements the Safety Validator Object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  1: Safety Validator State
 *  - Attribute  2: Safety Validator Type
 *  - Attribute 12: Max Data Age
 *  - Attribute 13: Application Data Path
 *  - Attribute 15: Producer/Consumer Fault Counters
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributeSingle
 *  - SetAttributeSingle
 */
/* ********************************************************************
 * include files
 */
#if defined(OPENER_CIP_SAFETY)
#include "cipsafetyvalidator.h"

#include <string.h>

#include "cipcommon.h"
#include "opener_api.h"
#include "trace.h"
#include "endianconv.h"
#include "cip_safety_sci.h"

/* ********************************************************************
 * defines
 */

/* ********************************************************************
 * Type declarations
 */

/* ********************************************************************
 * module local variables
 */

/* ********************************************************************
 * global public variables
 */
CipUint g_safety_connection_fault_count = 0;
CipSafetyValidatorObject g_safetyvalidator[OPENER_SAFETYVALIDATOR_INSTANCE_CNT] = {
  {
    /* Multi-Cast Producer Connection Type 0x02 */
    .state = kSafetyValidatorStateUnallocated,
    .type = 0x02U,
    .max_data_age = 0,
    .app_data_path.path_size = 5,
    .app_data_path.class_id = 4,
    .app_data_path.instance_number = 0x0181,
    .app_data_path.attribute_number = 0,
    .fault_counter.size = 1,
    .fault_counter.counter = {0},
  },
  {
    /* Single Cast Consumer (Output) Connection Type 0x81 */
    .state = kSafetyValidatorStateUnallocated,
    .type = 0x81U,
    .max_data_age = 0,
    .app_data_path.path_size = 5,
    .app_data_path.class_id = 4,
    .app_data_path.instance_number = 0x01B1,
    .app_data_path.attribute_number = 0,
    .fault_counter.size = 1,
    .fault_counter.counter = {0},
  },
};
CipInstance g_safetyvalidator_instance[OPENER_SAFETYVALIDATOR_INSTANCE_CNT] = {0};
CipAttributeStruct g_safetyvalidator_attribute[OPENER_SAFETYVALIDATOR_INSTANCE_CNT][5] = {0};


/* ********************************************************************
 * local functions
 */
static void EncodeCipFaultCounters(const void *const data,
                                   ENIPMessage *const outgoing_message) {
  CipFaultCounters *fault_counter = (CipFaultCounters *)data;
  EncodeCipUsint(&fault_counter->size, outgoing_message);
  for(size_t i = 0; i < fault_counter->size; i++) {
    EncodeCipUsint(&fault_counter->counter[i], outgoing_message);
  }
}

static void EncodeCipAppDataPath(const void *const data,
                                 ENIPMessage *const outgoing_message) {
  CipEpath *app_data_path = (CipEpath *)data;
  EncodeCipPackedEPath(app_data_path, outgoing_message);
}

static EipStatus SafetyValidatorPreGetCallback(CipInstance *const instance,
                                               CipAttributeStruct *const attribute,
                                               CipByte service) {
  EipStatus eip_status = kEipStatusOkSend;
  (void) service; /* no unused parameter warnings */

  if (instance->instance_number == 0) {
    /* Class */
    if (attribute->attribute_number == 8) {
      /* Safety Connection Fault Count */
      if (0 != cip_safety_data_acq_wait()) {
        eip_status = kEipStatusError;
      }
    }
  } else {
    /* Instance */
    if (attribute->attribute_number == 12) {
      /* Max Data Age */
      if (0 != cip_safety_data_acq_wait()) {
        eip_status = kEipStatusError;
      }
    } else if (attribute->attribute_number == 15) {
      /* Producer/Consumer Fault Counters */
      if (0 != cip_safety_data_acq_wait()) {
        eip_status = kEipStatusError;
      }
    }
  }

  return eip_status;
}

CipInstance *AddSafetyValidatorInstance(void) {
  CipClass *safetyvalidator_class = GetCipClass(kCipSafetyValidatorClassCode);
  CipInstance *ret_instance = NULL;
  CipInstance *next_instance = NULL;
  EipUint32 instance_id = 0;
  CipUint i = 0;

  if(NULL == safetyvalidator_class) {
    return NULL;
  }

  for(instance_id = 1; instance_id <= OPENER_SAFETYVALIDATOR_INSTANCE_CNT; instance_id++) {
    ret_instance = GetCipInstance(safetyvalidator_class, instance_id);

    if(ret_instance == NULL) {
      if(safetyvalidator_class->instances == NULL) {
        g_safetyvalidator_instance[instance_id - 1].attributes = &g_safetyvalidator_attribute[instance_id - 1][0];
        safetyvalidator_class->instances = &g_safetyvalidator_instance[instance_id - 1];
      } else {
        g_safetyvalidator_instance[instance_id - 1].attributes = &g_safetyvalidator_attribute[instance_id - 1][0];
        for(i = 0; i < OPENER_SAFETYVALIDATOR_INSTANCE_CNT; i++) {
          g_safetyvalidator_instance[i].next = NULL;
          if(g_safetyvalidator_instance[i].attributes != NULL) {
            if(next_instance != NULL) {
              next_instance->next = &g_safetyvalidator_instance[i];
            }
            next_instance = &g_safetyvalidator_instance[i];
          }
        }
      }
      ret_instance = &g_safetyvalidator_instance[instance_id - 1];
      safetyvalidator_class->number_of_instances++;
      break;
    }
  }

  return ret_instance;
}

CipInstance *RemoveSafetyValidatorInstance(const EipUint32 instance_id) {
  CipClass *safetyvalidator_class = GetCipClass(kCipSafetyValidatorClassCode);
  CipInstance *ret_instance = NULL;
  CipInstance *next_instance = NULL;
  CipUint i = 0;

  if(NULL == safetyvalidator_class) {
    return NULL;
  }

  if(instance_id > OPENER_SAFETYVALIDATOR_INSTANCE_CNT) {
    return NULL;
  }

  if(instance_id == 0) {
    return NULL;
  }

  ret_instance = GetCipInstance(safetyvalidator_class, instance_id);

  if(ret_instance != NULL) {
    safetyvalidator_class->instances = NULL;
    g_safetyvalidator_instance[instance_id - 1].attributes = NULL;
    memset((void *)&g_safetyvalidator[instance_id - 1], 0, sizeof(g_safetyvalidator[instance_id - 1]));
    for(i = 0; i < OPENER_SAFETYVALIDATOR_INSTANCE_CNT; i++) {
      g_safetyvalidator_instance[i].next = NULL;
      if(g_safetyvalidator_instance[i].attributes != NULL) {
        if(safetyvalidator_class->instances == NULL) {
          safetyvalidator_class->instances = &g_safetyvalidator_instance[i];
        }
        if(next_instance != NULL) {
          next_instance->next = &g_safetyvalidator_instance[i];
        }
        next_instance = &g_safetyvalidator_instance[i];
      }
    }
    ret_instance = &g_safetyvalidator_instance[instance_id - 1];
    safetyvalidator_class->number_of_instances--;
  }

  return ret_instance;
}

/* ********************************************************************
 * public functions
 */
EipStatus SetAttributeSingleSafetyValidator(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  CipAttributeStruct *attribute = GetCipAttribute(
    instance, message_router_request->request_path.attribute_number);
  EipUint16 attribute_number = message_router_request->request_path
                               .attribute_number;
  (void) originator_address; /* no unused parameter warnings */
  (void) encapsulation_session; /* no unused parameter warnings */

  /* Check attribute exists and is not a dummy for GetAttributeAll */
  if(NULL != attribute && !(kGetableAllDummy & attribute->attribute_flags) ) {
    uint8_t set_bit_mask = (instance->cip_class->set_bit_mask[CalculateIndex(
                                                                attribute_number)
                            ]);
    if( set_bit_mask & ( 1 << ( (attribute_number) % 8 ) ) ) {

      if( (attribute->attribute_flags & kPreSetFunc)
           && instance->cip_class->PreSetCallback ) {
        instance->cip_class->PreSetCallback(instance,
                                            attribute,
                                            message_router_request->service);
      }

      switch(attribute_number) {
        case 12: {

          CipUint max_data_age = GetUintFromMessage(
            &(message_router_request->data) );

          if(max_data_age != 0) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {

            OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

            if(attribute->data != NULL) {
              CipUint *data = (CipUint *) attribute->data;

              *(data) = max_data_age;
              message_router_response->general_status = kCipErrorSuccess;
            } else {
              message_router_response->general_status = kCipErrorNotEnoughData;
            }
          }
        }
        break;

        default:
          message_router_response->general_status =
            kCipErrorAttributeNotSetable;
          break;
      }

      /* Call the PostSetCallback if enabled. */
      if( (attribute->attribute_flags & (kPostSetFunc | kNvDataFunc) )
           && NULL != instance->cip_class->PostSetCallback ) {
        CipUsint service = message_router_request->service;
        instance->cip_class->PostSetCallback(instance, attribute, service);
      }
    } else {
      message_router_response->general_status = kCipErrorAttributeNotSetable;
    }
  } else {
    /* we don't have this attribute */
    message_router_response->general_status = kCipErrorAttributeNotSupported;
  }

  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  return kEipStatusOkSend;
}

EipStatus CipSafetyValidatorInit(void) {
  CipClass *safetyvalidator_class = NULL;
  CipUint i = 0;

  safetyvalidator_class = CreateCipClass(kCipSafetyValidatorClassCode,
                                         1, /* # class attributes */
                                         8, /* # highest class attribute number */
                                         1, /* # class services */
                                         5,/* # instance attributes */
                                         15,/* # of highest instance attribute */
                                         2, /* # instance services */
                                         0, /* # instances */
                                         "Safety Validator", /* object class name */
                                         1,  /* # class revision */
                                         NULL /* function pointer for initialization */
                                         );

  if(NULL == safetyvalidator_class) {
    return kEipStatusError;
  }

  /* Add attributes to the class */
  InsertAttribute((CipInstance *)safetyvalidator_class,
                  8,
                  kCipUint,
                  EncodeCipUint,
                  &g_safety_connection_fault_count,
                  kGetableSingle|kPreGetFunc);

  /* Set attributes to initial values */
  g_safety_connection_fault_count = 0;

  /* Add services to the class */
  InsertService(safetyvalidator_class, kGetAttributeSingle,
                GetAttributeSingle, "GetAttributeSingle");
  InsertService(safetyvalidator_class, kSetAttributeSingle,
                &SetAttributeSingleSafetyValidator,
                "SetAttributeSingleSafetyValidator");
  InsertGetSetCallback(safetyvalidator_class->class_instance.cip_class, SafetyValidatorPreGetCallback, kPreGetFunc);
  InsertGetSetCallback(safetyvalidator_class, SafetyValidatorPreGetCallback, kPreGetFunc);

  /* Set attributes to initial values */
  for(i = 0; i < OPENER_SAFETYVALIDATOR_INSTANCE_CNT; i++) {
    g_safetyvalidator_instance[i].instance_number = i + 1;
    g_safetyvalidator_instance[i].cip_class = safetyvalidator_class;
  }

  return kEipStatusOk;
}

CipInstance *CreateSafetyValidatorObject(const EipUint32 assembly_instance_id) {
  size_t idx = 0;
  CipClass *safetyvalidator_class = GetCipClass(kCipSafetyValidatorClassCode);

  if(NULL == safetyvalidator_class) {
    return NULL;
  }

  if(0 == assembly_instance_id) {
    return NULL;
  }

  for(idx = 0; idx < OPENER_SAFETYVALIDATOR_INSTANCE_CNT; ++idx) {
    if(g_safetyvalidator[idx].state == kSafetyValidatorStateUnallocated) {
      if(g_safetyvalidator[idx].app_data_path.instance_number == assembly_instance_id) {
        break;
      }
    }
  }

  if(idx == OPENER_SAFETYVALIDATOR_INSTANCE_CNT) {
    return NULL;
  }

  /* Bind attributes to the instance */
  CipInstance *const safetyvalidator_instance = AddSafetyValidatorInstance();
  if(0 == safetyvalidator_instance) {
    return NULL;
  }

  InsertAttribute(safetyvalidator_instance,
                  1,
                  kCipUsint,
                  EncodeCipUsint,
                  &g_safetyvalidator[idx].state,
                  kGetableSingle);
  InsertAttribute(safetyvalidator_instance,
                  2,
                  kCipUsint,
                  EncodeCipUsint,
                  &g_safetyvalidator[idx].type,
                  kGetableSingle);
  InsertAttribute(safetyvalidator_instance,
                  12,
                  kCipUint,
                  EncodeCipUint,
                  &g_safetyvalidator[idx].max_data_age,
                  kSetAndGetAble|kPreGetFunc|kPreSetFunc|kPostSetFunc);
  InsertAttribute(safetyvalidator_instance,
                  13,
                  kCipAny,
                  EncodeCipAppDataPath,
                  &g_safetyvalidator[idx].app_data_path,
                  kGetableSingle);
  InsertAttribute(safetyvalidator_instance,
                  15,
                  kCipAny,
                  EncodeCipFaultCounters,
                  &g_safetyvalidator[idx].fault_counter,
                  kGetableSingle|kPreGetFunc);

  /* Established */
  g_safetyvalidator[idx].state = kSafetyValidatorStateEstablished;

  return safetyvalidator_instance;
}

CipInstance *DeleteSafetyValidatorInstance(const EipUint32 assembly_instance_id) {
  size_t idx = 0;

  if(0 == assembly_instance_id) {
    return NULL;
  }

  for(idx = 0; idx < OPENER_SAFETYVALIDATOR_INSTANCE_CNT; ++idx) {
    if(g_safetyvalidator[idx].state == kSafetyValidatorStateEstablished) {
      if(g_safetyvalidator[idx].app_data_path.instance_number == assembly_instance_id) {
        break;
      }
    }
  }

  if(idx == OPENER_SAFETYVALIDATOR_INSTANCE_CNT) {
    return NULL;
  }

  /* Bind attributes to the instance */
  CipInstance *const safetyvalidator_instance = RemoveSafetyValidatorInstance(idx + 1);
  if(0 == safetyvalidator_instance) {
    return NULL;
  }

  /* Unallocated */
  g_safetyvalidator[idx].state = kSafetyValidatorStateUnallocated;

  return safetyvalidator_instance;
}

int CipSafetySafetyValidatorSetValidator(size_t idx, CipUint max_data_age, CipUsint fault_counter_size, CipUsint fault_counter) {
  if(idx >= OPENER_SAFETYVALIDATOR_INSTANCE_CNT) {
    return -1;
  }

  if(fault_counter_size > OPENER_SAFETYVALIDATOR_MAX_CNT_SIZE) {
    return -1;
  }

  g_safetyvalidator[idx].max_data_age = max_data_age;
  g_safetyvalidator[idx].fault_counter.size = fault_counter_size;
  g_safetyvalidator[idx].fault_counter.counter[0] = fault_counter;

  return 0;
}

void CipSafetySafetyValidatorSetSafetyConnectionFaultCount(CipUint safety_connection_fault_count) {
  g_safety_connection_fault_count = safety_connection_fault_count;
}
#endif
