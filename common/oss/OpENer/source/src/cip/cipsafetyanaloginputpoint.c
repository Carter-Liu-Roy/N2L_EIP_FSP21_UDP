/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the Safety Analog Input Point Object
 *
 *  CIP Safety Analog Input Point Object
 *  ==============
 *
 *  This module implements the Safety Analog Input Point Object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  2: Safety Analog Input Value
 *  - Attribute  3: Input Range
 *  - Attribute  4: Input Channel Mode
 *  - Attribute  5: Input Point Status
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
#include "cipsafetyanaloginputpoint.h"

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
CipSafetyAnalogInputPointObject g_safetyanaloginputpoint[OPENER_SAFETYANALOGINPUTPOINT_INSTANCE_CNT];


/* ********************************************************************
 * local functions
 */

/* ********************************************************************
 * public functions
 */
EipStatus SetAttributeSingleSafetyAnalogInputPoint(
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

EipStatus CipSafetyAnalogInputPointInit(void) {
  CipClass *safetyanaloginputpoint_class = NULL;

  safetyanaloginputpoint_class = CreateCipClass(kCipSafetyAnalogInputPointClassCode,
                                                0, /* # class attributes */
                                                7, /* # highest class attribute number */
                                                1, /* # class services */
                                                5,/* # instance attributes */
                                                6,/* # of highest instance attribute */
                                                2, /* # instance services */
                                                OPENER_SAFETYANALOGINPUTPOINT_INSTANCE_CNT, /* # instances */
                                                "Safety Analog Input Point", /* object class name */
                                                1,  /* # class revision */
                                                NULL /* function pointer for initialization */
                                                );

  if(NULL == safetyanaloginputpoint_class) {
    return kEipStatusError;
  }

  /* Add services to the class */
  InsertService(safetyanaloginputpoint_class, kGetAttributeSingle,
                GetAttributeSingle, "GetAttributeSingle");
  InsertService(safetyanaloginputpoint_class, kSetAttributeSingle,
                &SetAttributeSingleSafetyAnalogInputPoint,
                "SetAttributeSingleSafetyAnalogInputPoint");

  /* Bind attributes to the instance */
  for (size_t idx = 0; idx < OPENER_SAFETYANALOGINPUTPOINT_INSTANCE_CNT; ++idx) {
    CipInstance *safetyanaloginputpoint_instance = GetCipInstance(safetyanaloginputpoint_class, idx + 1);

    InsertAttribute(safetyanaloginputpoint_instance,
                    2,
                    kCipInt,
                    EncodeCipInt,
                    &g_safetyanaloginputpoint[idx].value,
                    kGetableSingle);
    InsertAttribute(safetyanaloginputpoint_instance,
                    3,
                    kCipUsint,
                    EncodeCipUsint,
                    &g_safetyanaloginputpoint[idx].range,
                    kSetAndGetAble|kPreSetFunc|kPostSetFunc);
    InsertAttribute(safetyanaloginputpoint_instance,
                    4,
                    kCipUsint,
                    EncodeCipUsint,
                    &g_safetyanaloginputpoint[idx].mode,
                    kSetAndGetAble|kPreSetFunc|kPostSetFunc);
    InsertAttribute(safetyanaloginputpoint_instance,
                    5,
                    kCipBool,
                    EncodeCipBool,
                    &g_safetyanaloginputpoint[idx].status,
                    kGetableSingle);
    InsertAttribute(safetyanaloginputpoint_instance,
                    6,
                    kCipUsint,
                    EncodeCipUsint,
                    &g_safetyanaloginputpoint[idx].fault_reason,
                    kGetableSingle);

    /* Set attributes to initial values */
    g_safetyanaloginputpoint[idx].value = 0;
    g_safetyanaloginputpoint[idx].range = kSafetyAnalogInputPointInputRange4to20milliamps;
    g_safetyanaloginputpoint[idx].mode = kSafetyAnalogInputPointInputModeNotUsed;
    g_safetyanaloginputpoint[idx].status = kSafetyAnalogInputPointInputStatusOK;
    g_safetyanaloginputpoint[idx].fault_reason = kSafetyAnalogInputPointFaultReasonInputSignalnotUsed;
  }

  return kEipStatusOk;
}
#endif
