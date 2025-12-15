/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the Safety Analog Output Point Object
 *
 *  CIP Safety Analog Output Point Object
 *  ==============
 *
 *  This module implements the Safety Analog Output Point Object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  2: Safety Analog Output Value
 *  - Attribute  3: Output Range
 *  - Attribute  4: Output Channel Mode
 *  - Attribute  5: Output Point Status
 *  - Attribute  6: Point Fault Reason
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
#include "cipsafetyanalogoutputpoint.h"

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
CipSafetyAnalogOutputPointObject g_safetyanalogoutputpoint[OPENER_SAFETYANALOGOUTPUTPOINT_INSTANCE_CNT];


/* ********************************************************************
 * local functions
 */

/* ********************************************************************
 * public functions
 */
EipStatus SetAttributeSingleSafetyAnalogOutputPoint(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  CipAttributeStruct *attribute = GetCipAttribute(
    instance, message_router_request->request_path.attribute_number);
  EipUint16 attribute_number = message_router_request->request_path
                               .attribute_number;

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
        case 3: {

          CipUsint range = GetUsintFromMessage(
            &(message_router_request->data) );

          if(range > 9) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {

            OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

            if(attribute->data != NULL) {
              CipUsint *data = (CipUsint *) attribute->data;

              *(data) = range;
              message_router_response->general_status = kCipErrorSuccess;
            } else {
              message_router_response->general_status = kCipErrorNotEnoughData;
            }
          }
        }
        break;

        case 4: {

          CipUsint mode = GetUsintFromMessage(
            &(message_router_request->data) );

          if(mode > 2) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {

            OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

            if(attribute->data != NULL) {
              CipUsint *data = (CipUsint *) attribute->data;

              *(data) = mode;
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

EipStatus CipSafetyAnalogOutputPointInit(void) {
  CipClass *safetyanalogoutputpoint_class = NULL;

  safetyanalogoutputpoint_class = CreateCipClass(kCipSafetyAnalogOutputPointClassCode,
                                                 0, /* # class attributes */
                                                 7, /* # highest class attribute number */
                                                 1, /* # class services */
                                                 5,/* # instance attributes */
                                                 6,/* # of highest instance attribute */
                                                 2, /* # instance services */
                                                 OPENER_SAFETYANALOGOUTPUTPOINT_INSTANCE_CNT, /* # instances */
                                                 "Safety Analog Output Point", /* object class name */
                                                 1,  /* # class revision */
                                                 NULL /* function pointer for initialization */
                                                 );

  if(NULL == safetyanalogoutputpoint_class) {
    return kEipStatusError;
  }

  /* Add services to the class */
  InsertService(safetyanalogoutputpoint_class, kGetAttributeSingle,
                GetAttributeSingle, "GetAttributeSingle");
  InsertService(safetyanalogoutputpoint_class, kSetAttributeSingle,
                &SetAttributeSingleSafetyAnalogOutputPoint,
                "SetAttributeSingleSafetyAnalogOutputPoint");

  /* Bind attributes to the instance */
  for (size_t idx = 0; idx < OPENER_SAFETYANALOGOUTPUTPOINT_INSTANCE_CNT; ++idx) {
    CipInstance *safetyanalogoutputpoint_instance = GetCipInstance(safetyanalogoutputpoint_class, idx + 1);

    InsertAttribute(safetyanalogoutputpoint_instance,
                    2,
                    kCipInt,
                    EncodeCipInt,
                    &g_safetyanalogoutputpoint[idx].value,
                    kGetableSingle);
    InsertAttribute(safetyanalogoutputpoint_instance,
                    3,
                    kCipUsint,
                    EncodeCipUsint,
                    &g_safetyanalogoutputpoint[idx].range,
                    kSetAndGetAble|kPreSetFunc|kPostSetFunc);
    InsertAttribute(safetyanalogoutputpoint_instance,
                    4,
                    kCipUsint,
                    EncodeCipUsint,
                    &g_safetyanalogoutputpoint[idx].mode,
                    kSetAndGetAble|kPreSetFunc|kPostSetFunc);
    InsertAttribute(safetyanalogoutputpoint_instance,
                    5,
                    kCipBool,
                    EncodeCipBool,
                    &g_safetyanalogoutputpoint[idx].status,
                    kGetableSingle);
    InsertAttribute(safetyanalogoutputpoint_instance,
                    6,
                    kCipUsint,
                    EncodeCipUsint,
                    &g_safetyanalogoutputpoint[idx].fault_reason,
                    kGetableSingle);

    /* Set attributes to initial values */
    g_safetyanalogoutputpoint[idx].value = 0;
    g_safetyanalogoutputpoint[idx].range = kSafetyAnalogOutputPointOutputRange4to20milliamps;
    g_safetyanalogoutputpoint[idx].mode = kSafetyAnalogOutputPointOutputModeNotUsed;
    g_safetyanalogoutputpoint[idx].status = kSafetyAnalogOutputPointOutputStatusOK;
    g_safetyanalogoutputpoint[idx].fault_reason = kSafetyAnalogOutputPointFaultReasonOutputSignalnotUsed;
  }

  return kEipStatusOk;
}
#endif
