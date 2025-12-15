/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the LLDP Management Object
 *
 *  CIP LLDP Management Object
 *  ==============
 *
 *  This module implements the LLDP Management Object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  1: LLDP Enable
 *  - Attribute  2: msgTxInterval
 *  - Attribute  3: msgTxHold
 *  - Attribute  4: LLDP Datastore
 *  - Attribute  5: Last Change
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributeSingle
 *  - SetAttributeSingle
 */
/* ********************************************************************
 * include files
 */
#include "ciplldpmanagement.h"

#include <string.h>

#include "cipcommon.h"
#include "opener_api.h"
#include "trace.h"
#include "endianconv.h"

/* ********************************************************************
 * defines
 */
/* Definition of LLDP Enable Array Length */
#ifndef OPENER_ETHLINK_INSTANCE_CNT
  #define OPENER_LLDPMAN_LLDPEN_LEN    2
#else
  #define OPENER_LLDPMAN_LLDPEN_LEN    OPENER_ETHLINK_INSTANCE_CNT + 1
#endif
/* Mask Definition of LLDP Enable Array */
#if OPENER_LLDPMAN_LLDPEN_LEN == 2
  #define OPENER_LLDPMAN_LLDPEN_MASK   0x03
#elif OPENER_LLDPMAN_LLDPEN_LEN == 3
  #define OPENER_LLDPMAN_LLDPEN_MASK   0x07
#elif OPENER_LLDPMAN_LLDPEN_LEN == 4
  #define OPENER_LLDPMAN_LLDPEN_MASK   0x0F
#elif OPENER_LLDPMAN_LLDPEN_LEN == 5
  #define OPENER_LLDPMAN_LLDPEN_MASK   0x1F
  /* If subsequent definitions are needed, please add them as appropriate. */
#endif

/* ********************************************************************
 * Type declarations
 */

/* ********************************************************************
 * module local variables
 */

/* ********************************************************************
 * global public variables
 */
CipLLDPManagementObject g_lldpmanagement;  /**< definition of LLDP Management Object instance 1 data */


/* ********************************************************************
 * local functions
 */
static void EncodeCipLLDPEnable(const void *const data,
                                 ENIPMessage *const outgoing_message) {
  CipLLDPEnable *lldp_enable = (CipLLDPEnable *)data;
  EncodeCipUint(&lldp_enable->length, outgoing_message);
  EncodeCipByte(&lldp_enable->array, outgoing_message);
}


/* ********************************************************************
 * public functions
 */
EipStatus SetAttributeSingleLLDPManagement(
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
        case 1: {

          CipUint length = GetUintFromMessage(
            &(message_router_request->data) );
          CipByte array = GetUsintFromMessage(
            &(message_router_request->data) );

          if(length != OPENER_LLDPMAN_LLDPEN_LEN) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else if((array & ~OPENER_LLDPMAN_LLDPEN_MASK) != 0) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {

            OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

            if(attribute->data != NULL) {
              CipUint *data1 = (CipUint *) attribute->data;
              CipByte *data2 = (CipByte *) attribute->data + sizeof(CipUint);

              *(data1) = length;
              *(data2) = array;
              message_router_response->general_status = kCipErrorSuccess;
            } else {
              message_router_response->general_status = kCipErrorNotEnoughData;
            }
          }
        }
        break;

        case 2: {

          CipUint msgTxInterval = GetUintFromMessage(
            &(message_router_request->data) );

          if(msgTxInterval > 3600) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else if(msgTxInterval < 5) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {

            OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

            if(attribute->data != NULL) {
              CipUint *data = (CipUint *) attribute->data;

              *(data) = msgTxInterval;
              message_router_response->general_status = kCipErrorSuccess;
            } else {
              message_router_response->general_status = kCipErrorNotEnoughData;
            }
          }
        }
        break;

        case 3: {

          CipUsint msgTxHold = GetUsintFromMessage(
            &(message_router_request->data) );

          if(msgTxHold > 100) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else if(msgTxHold == 0) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {

            OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

            if(attribute->data != NULL) {
              CipUsint *data = (CipUsint *) attribute->data;

              *(data) = msgTxHold;
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

EipStatus CipLLDPManagementInit(void) {
  CipClass *lldpmanagement_class = NULL;

  lldpmanagement_class = CreateCipClass(kCipLLDPManagementClassCode,
                             0, /* # class attributes */
                             7, /* # highest class attribute number */
                             2, /* # class services */
                             5,/* # instance attributes */
                             5,/* # of highest instance attribute */
                             2, /* # instance services */
                             1, /* # instances */
                             "LLDP Management", /* object class name */
                             1,  /* # class revision */
                             NULL /* function pointer for initialization */
                             );

  if(NULL == lldpmanagement_class) {
    return kEipStatusError;
  }

  /* Add services to the class */
  InsertService(lldpmanagement_class, kGetAttributeSingle,
                GetAttributeSingle, "GetAttributeSingle");
  InsertService(lldpmanagement_class, kSetAttributeSingle,
                &SetAttributeSingleLLDPManagement,
                "SetAttributeSingleLLDPManagement");

  /* Bind attributes to the instance */
  CipInstance *lldpmanagement_instance = GetCipInstance(lldpmanagement_class, 1u);

  InsertAttribute(lldpmanagement_instance,
                  1,
                  kCipAny,
                  EncodeCipLLDPEnable,
                  &g_lldpmanagement.lldp_enable,
                  kSetAndGetAble|kPreGetFunc|kPostSetFunc);
  InsertAttribute(lldpmanagement_instance,
                  2,
                  kCipUint,
                  EncodeCipUint,
                  &g_lldpmanagement.msgTxInterval,
                  kSetAndGetAble|kPreGetFunc|kPostSetFunc);
  InsertAttribute(lldpmanagement_instance,
                  3,
                  kCipUsint,
                  EncodeCipUsint,
                  &g_lldpmanagement.msgTxHold,
                  kSetAndGetAble|kPreGetFunc|kPostSetFunc);
  InsertAttribute(lldpmanagement_instance,
                  4,
                  kCipWord,
                  EncodeCipWord,
                  &g_lldpmanagement.lldp_datastore,
                  kGetableSingle);
  InsertAttribute(lldpmanagement_instance,
                  5,
                  kCipUdint,
                  EncodeCipUdint,
                  &g_lldpmanagement.last_change,
                  kGetableSingle|kPreGetFunc);

  /* Set attributes to initial values */
  g_lldpmanagement.lldp_enable.length = 2;
  g_lldpmanagement.lldp_enable.array = 0x03;
  g_lldpmanagement.msgTxInterval = 30; /* Recommended default value is 30. */
  g_lldpmanagement.msgTxHold = 1;
  g_lldpmanagement.lldp_datastore = 0x1; /* LLDPData Table Object */
  g_lldpmanagement.last_change = 0;

  return kEipStatusOk;
}
