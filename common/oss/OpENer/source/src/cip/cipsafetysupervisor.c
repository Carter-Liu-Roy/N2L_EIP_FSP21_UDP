/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the Safety Supervisor Object
 *
 *  CIP Safety Supervisor Object
 *  ==============
 *
 *  This module implements the Safety Supervisor Object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute 11: Device Status
 *  - Attribute 12: Exception Status
 *  - Attribute 13: Exception Detail Alarm
 *  - Attribute 14: Exception Detail Warning
 *  - Attribute 15: Alarm Enable
 *  - Attribute 16: Warning Enable
 *  - Attribute 24: Configuration Lock
 *  - Attribute 25: Configuration UNID
 *  - Attribute 26: Safety Configuration Identifier
 *  - Attribute 27: Target UNID
 *  - Attribute 28: Output Connection Point Owners
 *  - Attribute 29: Proposed UNID
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributeSingle
 *  - Apply_Attributes
 *  - SetAttributeSingle
 *  - Configure_Request
 *  - Validate_Configuration
 *  - Set_Password
 *  - Configuration_Lock/Unlock
 *  - Safety_Reset
 *  - Reset_Password
 *  - Propose_TUNID
 *  - Apply_TUNID
 */
/* ********************************************************************
 * include files
 */
#if defined(OPENER_CIP_SAFETY)
#include "cipsafetysupervisor.h"

#include <string.h>

#include "cipcommon.h"
#include "cipidentity.h"
#include "opener_api.h"
#include "trace.h"
#include "endianconv.h"
#include "generic_networkhandler.h"
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
CipSafetySupervisorObject g_safetysupervisor;  /**< definition of Safety Supervisor Object instance 1 data */
CipBool g_ns_led_flash_flag = false;
CipSafetyNetworkSegment g_safety_network_segment = {0};


/* ********************************************************************
 * local functions
 */
static bool IsUnidAll0x00(const CipUNID *unid) {
  if (unid->snn.time_of_day != 0x00000000) {
    return false;
  }
  if (unid->snn.date != 0x0000) {
    return false;
  }
  if (unid->nodeid != 0x00000000) {
    return false;
  }

  return true;
}

static bool IsUnidAll0xFF(const CipUNID *unid) {
  if (unid->snn.time_of_day != 0xFFFFFFFF) {
    return false;
  }
  if (unid->snn.date != 0xFFFF) {
    return false;
  }
  if (unid->nodeid != 0xFFFFFFFF) {
    return false;
  }

  return true;
}

static bool UnidCmp(const CipUNID *unid1, const CipUNID *unid2) {
  if (unid1->snn.time_of_day != unid2->snn.time_of_day) {
    return true;
  }
  if (unid1->snn.date != unid2->snn.date) {
    return true;
  }
  if (unid1->nodeid != unid2->nodeid) {
    return true;
  }

  return false;
}

static void UnidCpy(CipUNID *unid1, const CipUNID *unid2) {
  unid1->snn.time_of_day = unid2->snn.time_of_day;
  unid1->snn.date = unid2->snn.date;
  unid1->nodeid = unid2->nodeid;
}

static void SetUnidAll0xFF(CipUNID *unid) {
  unid->snn.time_of_day = 0xFFFFFFFF;
  unid->snn.date = 0xFFFF;
  unid->nodeid = 0xFFFFFFFF;
}

static bool IsScidAll0x00(const CipSCID *scid) {
  if (scid->sccrc != 0x00000000) {
    return false;
  }
  if (scid->scts.time_of_day != 0x00000000) {
    return false;
  }
  if (scid->scts.date != 0x0000) {
    return false;
  }

  return true;
}

static void SetScidAll0x00(CipSCID *scid) {
  scid->sccrc = 0x00000000;
  scid->scts.time_of_day = 0x00000000;
  scid->scts.date = 0x0000;
}

static void EncodeCipDetail(const void *const data,
                            ENIPMessage *const outgoing_message) {
  CipDetail *detail = (CipDetail *)data;
  EncodeCipUsint(&detail->size, outgoing_message);
  for(size_t i = 0; i < detail->size; i++) {
    EncodeCipByte(&detail->detail[i], outgoing_message);
  }
}

static void EncodeCipExceptionDetail(const void *const data,
                                     ENIPMessage *const outgoing_message) {
  CipExceptionDetail *exception_detail = (CipExceptionDetail *)data;
  EncodeCipDetail(&exception_detail->common, outgoing_message);
  EncodeCipDetail(&exception_detail->device, outgoing_message);
  EncodeCipDetail(&exception_detail->manufacturer, outgoing_message);
}

static void EncodeCipUNID(const void *const data,
                          ENIPMessage *const outgoing_message) {
  CipUNID *unid = (CipUNID *)data;
  EncodeCipUdint(&unid->snn.time_of_day, outgoing_message);
  EncodeCipUint(&unid->snn.date, outgoing_message);
  EncodeCipUdint(&unid->nodeid, outgoing_message);
}

static void EncodeCipSCID(const void *const data,
                          ENIPMessage *const outgoing_message) {
  CipSCID *scid = (CipSCID *)data;
  EncodeCipDword(&scid->sccrc, outgoing_message);
  EncodeCipUdint(&scid->scts.time_of_day, outgoing_message);
  EncodeCipUint(&scid->scts.date, outgoing_message);
}

static void EncodeCipOutputConnectionOwners(const void *const data,
                                            ENIPMessage *const outgoing_message) {
  CipOutputConnectionOwners *output_connection_owners = (CipOutputConnectionOwners *)data;
  EncodeCipUint(&output_connection_owners->num, outgoing_message);
  for(size_t i = 0; i < output_connection_owners->num; i++) {
    EncodeCipUNID(&output_connection_owners->output_owners[i].ocpunid, outgoing_message);
    EncodeCipPackedEPath(&output_connection_owners->output_owners[i].app_resource, outgoing_message);
  }
}

static EipStatus SafetySupervisorPreGetCallback(CipInstance *const instance,
                                                CipAttributeStruct *const attribute,
                                                CipByte service) {
  EipStatus eip_status = kEipStatusOkSend;
  (void) service; /* no unused parameter warnings */

  if (instance->instance_number == 1) {
    /* Instance 1 */
    if (attribute->attribute_number == 11) {
      /* Device Status */
      if (0 != cip_safety_data_acq_wait()) {
        eip_status = kEipStatusError;
      }
    }
    if (attribute->attribute_number == 12) {
      /* Exception Status */
      if (0 != cip_safety_data_acq_wait()) {
        eip_status = kEipStatusError;
      }
    }
  }

  return eip_status;
}

/* ********************************************************************
 * public functions
 */
EipStatus ApplyAttributesSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size > 0) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    unsigned short rx_len = 0;
    CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

    if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
      message_router_response->general_status = kCipErrorResourceUnavailable;
    }
  }

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus SetAttributeSingleSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  CipAttributeStruct *attribute = GetCipAttribute(
    instance, message_router_request->request_path.attribute_number);
  EipUint16 attribute_number = message_router_request->request_path
                               .attribute_number;

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

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
        case 15: {

          CipBool alarm_enable = GetUsintFromMessage(
            &(message_router_request->data) );

          if(alarm_enable > 1) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {
            unsigned short rx_len = 0;
            CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

            if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
              message_router_response->general_status = kCipErrorResourceUnavailable;
            } else {
              if (message_router_response->general_status == kCipErrorSuccess) {
                OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

                if(attribute->data != NULL) {
                  CipUsint *data = (CipUsint *) attribute->data;

                  *(data) = alarm_enable;
                  message_router_response->general_status = kCipErrorSuccess;
                } else {
                  message_router_response->general_status = kCipErrorNotEnoughData;
                }
              }
            }
          }
        }
        break;

        case 16: {

          CipBool warning_enable = GetUsintFromMessage(
            &(message_router_request->data) );

          if(warning_enable > 1) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {
            unsigned short rx_len = 0;
            CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

            if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
              message_router_response->general_status = kCipErrorResourceUnavailable;
            } else {
              if (message_router_response->general_status == kCipErrorSuccess) {
                OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

                if(attribute->data != NULL) {
                  CipUsint *data = (CipUsint *) attribute->data;

                  *(data) = warning_enable;
                  message_router_response->general_status = kCipErrorSuccess;
                } else {
                  message_router_response->general_status = kCipErrorNotEnoughData;
                }
              }
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

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus ConfigureRequestSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  EipUint32 offset = 0;
  CipAttributeStruct *attribute = NULL;
  unsigned short rx_len = 0;
  CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size < 36) {
    message_router_response->general_status = kCipErrorNotEnoughData;
  } else if (message_router_request->request_path_size > 36) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    CipOctet password_req[16] = {0};
    CipUNID tunid_req = {0};
    CipUNID ounid_req = {0};
    if (message_router_request->request_path_size == 36) {
      offset = 0;
      memcpy(password_req, &message_router_request->data[offset], sizeof(password_req));
      offset += sizeof(password_req);
      memcpy(&tunid_req.snn.time_of_day, &message_router_request->data[offset], sizeof(tunid_req.snn.time_of_day));
      offset += sizeof(tunid_req.snn.time_of_day);
      memcpy(&tunid_req.snn.date, &message_router_request->data[offset], sizeof(tunid_req.snn.date));
      offset += sizeof(tunid_req.snn.date);
      memcpy(&tunid_req.nodeid, &message_router_request->data[offset], sizeof(tunid_req.nodeid));
      offset += sizeof(tunid_req.nodeid);
      memcpy(&ounid_req.snn.time_of_day, &message_router_request->data[offset], sizeof(ounid_req.snn.time_of_day));
      offset += sizeof(ounid_req.snn.time_of_day);
      memcpy(&ounid_req.snn.date, &message_router_request->data[offset], sizeof(ounid_req.snn.date));
      offset += sizeof(ounid_req.snn.date);
      memcpy(&ounid_req.nodeid, &message_router_request->data[offset], sizeof(ounid_req.nodeid));
      offset += sizeof(ounid_req.nodeid);
    }

    attribute = GetCipAttribute(instance, 11);
    OPENER_ASSERT(NULL != attribute);
    CipUsint *device_status = (CipUsint *) attribute->data;
    if ((((*device_status == kDeviceStatusConfiguring) ||
        (*device_status == kDeviceStatusIdle)) ||
        (*device_status == kDeviceStatusExecuting)) ||
        (*device_status == kDeviceStatusAbort)) {
      attribute = GetCipAttribute(instance, 24);
      OPENER_ASSERT(NULL != attribute);
      CipBool *config_lock = (CipBool *) attribute->data;
      if (*config_lock == false ) {
        if (!memcmp(g_safetysupervisor.password, password_req, 16)) {
          attribute = GetCipAttribute(instance, 27);
          OPENER_ASSERT(NULL != attribute);
          CipUNID *tunid = (CipUNID *) attribute->data;
          attribute = GetCipAttribute(instance, 25);
          OPENER_ASSERT(NULL != attribute);
          CipUNID *cfunid = (CipUNID *) attribute->data;
          if (!UnidCmp(tunid, &tunid_req)) {
            if (IsUnidAll0xFF(&ounid_req)) {
              /* Configure_Request (0xFF) *from SNCT */
              if (IsUnidAll0xFF(cfunid)) {
                /* Value of CFUNID is 0xFF */
                if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
                  message_router_response->general_status = kCipErrorResourceUnavailable;
                } else {
                  if (message_router_response->general_status == kCipErrorSuccess) {
                    /* Accept */
                  }
                }
              } else if (IsUnidAll0x00(cfunid)) {
                /* Value of CFUNID is 0x00 */
                if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
                  message_router_response->general_status = kCipErrorResourceUnavailable;
                } else {
                  if (message_router_response->general_status == kCipErrorSuccess) {
                    /* Accept Set CFUNID=0xFF */
                    SetUnidAll0xFF(cfunid);
                    /* Call the PostSetCallback if enabled. */
                    if (NULL != instance->cip_class->PostSetCallback) {
                      instance->cip_class->PostSetCallback(instance, 0, 0);
                    }
                  }
                }
              } else {
                /* Value of CFUNID is OUNID */
                /* Fail */
                message_router_response->general_status = kCipErrorInvalidParameterValue;
              }
            } else {
              /* Configure_Request (OUNID) */
             if (IsUnidAll0xFF(cfunid)) {
                /* Value of CFUNID is 0xFF */
                /* Fail */
                message_router_response->general_status = kCipErrorInvalidParameterValue;
              } else if (IsUnidAll0x00(cfunid)) {
                /* Value of CFUNID is 0x00 */
                if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
                  message_router_response->general_status = kCipErrorResourceUnavailable;
                } else {
                  if (message_router_response->general_status == kCipErrorSuccess) {
                    /* Accept Set CFUNID=OUNID */
                    UnidCpy(cfunid, &ounid_req);
                    /* Call the PostSetCallback if enabled. */
                    if (NULL != instance->cip_class->PostSetCallback) {
                      instance->cip_class->PostSetCallback(instance, 0, 0);
                    }
                  }
                }
              } else {
                /* Value of CFUNID is OUNID */
                if (!UnidCmp(cfunid, &ounid_req)) {
                  /* Accept only if CFUNID=OUNID */
                  if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
                    message_router_response->general_status = kCipErrorResourceUnavailable;
                  } else {
                    if (message_router_response->general_status == kCipErrorSuccess) {
                    }
                  }
                } else {
                  /* Fail otherwise */
                  message_router_response->general_status = kCipErrorInvalidParameterValue;
                }
              }
            }
          } else {
            /* SRS143 */
            message_router_response->general_status = kCipErrorInvalidParameterValue;
          }
        } else {
          /* SRS147 */
          message_router_response->general_status = kCipErrorPrivilegeViolation;
        }
      } else {
        /* SRS72 */
        message_router_response->general_status = kCipErrorObjectStateConflict;
      }
    } else {
      message_router_response->general_status = kCipErrorObjectStateConflict;
    }
  }

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus ValidateConfigurationSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  EipUint32 offset = 0;
  CipAttributeStruct *attribute = NULL;
  CipSCID scid_req = {0};
  CipSCID scid_res = {0};

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size < 10) {
    message_router_response->general_status = kCipErrorNotEnoughData;
  } else if (message_router_request->request_path_size > 10) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    if (message_router_request->request_path_size == 10) {
      offset = 0;
      memcpy(&scid_req.sccrc, &message_router_request->data[offset], sizeof(scid_req.sccrc));
      offset += sizeof(scid_req.sccrc);
      memcpy(&scid_req.scts.time_of_day, &message_router_request->data[offset], sizeof(scid_req.scts.time_of_day));
      offset += sizeof(scid_req.scts.time_of_day);
      memcpy(&scid_req.scts.date, &message_router_request->data[offset], sizeof(scid_req.scts.date));
      offset += sizeof(scid_req.scts.date);
    }

    attribute = GetCipAttribute(instance, 11);
    OPENER_ASSERT(NULL != attribute);
    CipUsint *device_status = (CipUsint *) attribute->data;
    if (*device_status == kDeviceStatusConfiguring) {
      attribute = GetCipAttribute(instance, 24);
      OPENER_ASSERT(NULL != attribute);
      CipBool *config_lock = (CipBool *) attribute->data;
      if (*config_lock == false ) {
        unsigned short rx_len = 0;
        CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

        if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
          message_router_response->general_status = kCipErrorResourceUnavailable;
        } else {
          if (message_router_response->general_status == kCipErrorSuccess) {
            memcpy(&scid_res.sccrc, &rx_buf[0], sizeof(scid_res.sccrc));
            memcpy(&scid_res.scts.time_of_day, &rx_buf[4], sizeof(scid_res.scts.time_of_day));
            memcpy(&scid_res.scts.date, &rx_buf[8], sizeof(scid_res.scts.date));

            attribute = GetCipAttribute(instance, 26);
            OPENER_ASSERT(NULL != attribute);
            CipSCID *scid = (CipSCID *) attribute->data;
            scid->sccrc = scid_res.sccrc;
            scid->scts.time_of_day = scid_res.scts.time_of_day;
            scid->scts.date = scid_res.scts.date;
            /* Call the PostSetCallback if enabled. */
            if (NULL != instance->cip_class->PostSetCallback) {
              instance->cip_class->PostSetCallback(instance, 0, 0);
            }
          }
        }
      } else {
        message_router_response->general_status = kCipErrorObjectStateConflict;
      }
    } else {
      message_router_response->general_status = kCipErrorObjectStateConflict;
    }
  }

  InitializeENIPMessage(&message_router_response->message);
  if (message_router_response->general_status == kCipErrorSuccess) {
    AddDintToMessage(scid_res.sccrc, &message_router_response->message);
    AddDintToMessage(scid_res.scts.time_of_day, &message_router_response->message);
    AddIntToMessage(scid_res.scts.date, &message_router_response->message); /* reserved */
  }

  return kEipStatusOkSend;
}

EipStatus SetPasswordSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  EipUint32 offset = 0;

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size < 32) {
    message_router_response->general_status = kCipErrorNotEnoughData;
  } else if (message_router_request->request_path_size > 32) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    CipOctet current_password_req[16] = {0};
    CipOctet new_password_req[16] = {0};
    if (message_router_request->request_path_size == 32) {
      offset = 0;
      memcpy(current_password_req, &message_router_request->data[offset], sizeof(current_password_req));
      offset += sizeof(current_password_req);
      memcpy(new_password_req, &message_router_request->data[offset], sizeof(new_password_req));
      offset += sizeof(new_password_req);
    }

    if (!memcmp(g_safetysupervisor.password, current_password_req, 16)) {
      memcpy(g_safetysupervisor.password, new_password_req, 16);
      /* Call the PostSetCallback if enabled. */
      if (NULL != instance->cip_class->PostSetCallback) {
        instance->cip_class->PostSetCallback(instance, 0, 0);
      }
      message_router_response->general_status = kCipErrorSuccess;
    } else {
      /* SRS147 */
      message_router_response->general_status = kCipErrorPrivilegeViolation;
    }

    message_router_response->general_status = kCipErrorSuccess;
  }

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus ConfigurationLockUnlockSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  EipUint32 offset = 0;
  CipAttributeStruct *attribute = NULL;

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size < 27) {
    message_router_response->general_status = kCipErrorNotEnoughData;
  } else if (message_router_request->request_path_size > 27) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    CipOctet value = 0;
    CipOctet password_req[16] = {0};
    CipUNID tunid_req = {0};
    if (message_router_request->request_path_size == 27) {
      offset = 0;
      value = message_router_request->data[offset];
      offset += sizeof(value);
      memcpy(password_req, &message_router_request->data[offset], sizeof(password_req));
      offset += sizeof(password_req);
      memcpy(&tunid_req.snn.time_of_day, &message_router_request->data[offset], sizeof(tunid_req.snn.time_of_day));
      offset += sizeof(tunid_req.snn.time_of_day);
      memcpy(&tunid_req.snn.date, &message_router_request->data[offset], sizeof(tunid_req.snn.date));
      offset += sizeof(tunid_req.snn.date);
      memcpy(&tunid_req.nodeid, &message_router_request->data[offset], sizeof(tunid_req.nodeid));
      offset += sizeof(tunid_req.nodeid);
    }

    if (!memcmp(g_safetysupervisor.password, password_req, 16)) {
      attribute = GetCipAttribute(instance, 27);
      OPENER_ASSERT(NULL != attribute);
      CipUNID *tunid = (CipUNID *) attribute->data;
      if (!UnidCmp(tunid, &tunid_req)) {
        if (value > 1) {
          unsigned short rx_len = 0;
          CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

          if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
            message_router_response->general_status = kCipErrorResourceUnavailable;
          } else {
            if (message_router_response->general_status == kCipErrorSuccess) {
              attribute = GetCipAttribute(instance, 24);
              OPENER_ASSERT(NULL != attribute);
              CipBool *config_lock = (CipBool *) attribute->data;
              *config_lock = (CipBool)value;
              /* Call the PostSetCallback if enabled. */
              if (NULL != instance->cip_class->PostSetCallback) {
                instance->cip_class->PostSetCallback(instance, 0, 0);
              }
            }
          }
        } else {
          message_router_response->general_status = kCipErrorInvalidParameterValue;
        }
      } else {
        /* SRS145 */
        message_router_response->general_status = kCipErrorInvalidParameterValue;
      }
    } else {
      /* SRS149 */
      message_router_response->general_status = kCipErrorPrivilegeViolation;
    }
  }

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus SafetyResetSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  EipUint32 offset = 0;
  CipAttributeStruct *attribute = NULL;

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size < 27) {
    message_router_response->general_status = kCipErrorNotEnoughData;
  } else if (message_router_request->request_path_size > 28) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    CipUsint reset_type = 0;
    CipOctet password_req[16] = {0};
    CipUNID tunid_req = {0};
    CipUsint attribute_bit_map = 0;
    if (message_router_request->request_path_size == 27) {
      offset = 0;
      reset_type = message_router_request->data[offset];
      offset += sizeof(reset_type);
      memcpy(password_req, &message_router_request->data[offset], sizeof(password_req));
      offset += sizeof(password_req);
      memcpy(&tunid_req.snn.time_of_day, &message_router_request->data[offset], sizeof(tunid_req.snn.time_of_day));
      offset += sizeof(tunid_req.snn.time_of_day);
      memcpy(&tunid_req.snn.date, &message_router_request->data[offset], sizeof(tunid_req.snn.date));
      offset += sizeof(tunid_req.snn.date);
      memcpy(&tunid_req.nodeid, &message_router_request->data[offset], sizeof(tunid_req.nodeid));
      offset += sizeof(tunid_req.nodeid);
    }
    if (message_router_request->request_path_size == 28) {
      offset = 0;
      reset_type = message_router_request->data[offset];
      offset += sizeof(reset_type);
      memcpy(password_req, &message_router_request->data[offset], sizeof(password_req));
      offset += sizeof(password_req);
      memcpy(&tunid_req.snn.time_of_day, &message_router_request->data[offset], sizeof(tunid_req.snn.time_of_day));
      offset += sizeof(tunid_req.snn.time_of_day);
      memcpy(&tunid_req.snn.date, &message_router_request->data[offset], sizeof(tunid_req.snn.date));
      offset += sizeof(tunid_req.snn.date);
      memcpy(&tunid_req.nodeid, &message_router_request->data[offset], sizeof(tunid_req.nodeid));
      offset += sizeof(tunid_req.nodeid);
      attribute_bit_map = message_router_request->data[offset];
      offset += sizeof(attribute_bit_map);
    }

    if (!memcmp(g_safetysupervisor.password, password_req, 16)) {
      unsigned short rx_len = 0;
      CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

      if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
        message_router_response->general_status = kCipErrorResourceUnavailable;
      }

      if (message_router_response->general_status == kCipErrorSuccess) {
        switch (reset_type) {
          case 0: /* Reset type 0 -> Emulate as closely as possible cycling power on the device. */
            if ( kEipStatusError == ResetDevice() ) {
              message_router_response->general_status = kCipErrorInvalidParameter;
            } else {
              message_router_response->general_status = kCipErrorSuccess;
            }
            break;

          case 1: /* Reset type 1 -> Return as closely as possible to the default configuration, 
                     and then emulate cycling power as closely as possible.*/
            if ( kEipStatusError == ResetDeviceToInitialConfiguration(0) ) {
              message_router_response->general_status = kCipErrorInvalidParameter;
            } else {
              message_router_response->general_status = kCipErrorSuccess;
            }
            break;

          case 2: /* Reset type 2 -> Return as closely as possible to the out-of-box configuration 
                     except to preserve the parameters indicated by the Attribute Bit Map,
                     and then emulate cycling power as closely as possible.*/
            if ( kEipStatusError == ResetDeviceToInitialConfiguration(attribute_bit_map) ) {
              message_router_response->general_status = kCipErrorInvalidParameter;
            } else {
              message_router_response->general_status = kCipErrorSuccess;
            }
            break;

          default:
            message_router_response->general_status = kCipErrorInvalidParameter;
            break;
        }
      }
    } else {
      /* SRS150 */
      message_router_response->general_status = kCipErrorPrivilegeViolation;
    }
  }

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus ResetPasswordSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  EipUint32 offset = 0;

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size < 17) {
    message_router_response->general_status = kCipErrorNotEnoughData;
  } else if (message_router_request->request_path_size > 17) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    CipOctet data_size = 0;
    CipOctet password_req[16] = {0};
    if (message_router_request->request_path_size == 17) {
      offset = 0;
      data_size = message_router_request->data[offset];
      offset += sizeof(data_size);
      memcpy(password_req, &message_router_request->data[offset], sizeof(password_req));
      offset += sizeof(password_req);
    }

    if (data_size == 16) {
      CipOctet reset_password[16] = {0};
      if (!memcmp(reset_password, password_req, 16)) {
        /* Call the PostSetCallback if enabled. */
        if (NULL != instance->cip_class->PostSetCallback) {
          instance->cip_class->PostSetCallback(instance, 0, 0);
        }
        message_router_response->general_status = kCipErrorSuccess;
      } else {
        message_router_response->general_status = kCipErrorInvalidParameter;
      }
    } else {
      message_router_response->general_status = kCipErrorInvalidParameter;
    }
  }

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus ProposeTUNIDSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  EipUint32 offset = 0;
  CipAttributeStruct *attribute = NULL;

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size < 10) {
    message_router_response->general_status = kCipErrorNotEnoughData;
  } else if (message_router_request->request_path_size > 10) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    CipUNID tunid_req = {0};
    if (message_router_request->request_path_size == 10) {
      offset = 0;
      memcpy(&tunid_req.snn.time_of_day, &message_router_request->data[offset], sizeof(tunid_req.snn.time_of_day));
      offset += sizeof(tunid_req.snn.time_of_day);
      memcpy(&tunid_req.snn.date, &message_router_request->data[offset], sizeof(tunid_req.snn.date));
      offset += sizeof(tunid_req.snn.date);
      memcpy(&tunid_req.nodeid, &message_router_request->data[offset], sizeof(tunid_req.nodeid));
      offset += sizeof(tunid_req.nodeid);
    }

    unsigned short rx_len = 0;
    CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

    if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
      message_router_response->general_status = kCipErrorResourceUnavailable;
    } else {
      if (message_router_response->general_status == kCipErrorSuccess) {
        if (!IsUnidAll0xFF(&tunid_req) ) {
          if (tunid_req.nodeid == ntohl(g_network_status.ip_address)) {
            attribute = GetCipAttribute(instance, 29);
            OPENER_ASSERT(NULL != attribute);
            CipUNID *proposed_tunid = (CipUNID *) attribute->data;
            proposed_tunid->snn.time_of_day = tunid_req.snn.time_of_day;
            proposed_tunid->snn.date = tunid_req.snn.date;
            proposed_tunid->nodeid = tunid_req.nodeid;
            /* NET LED Flash sequence is started */
            g_ns_led_flash_flag = true;
            message_router_response->general_status = kCipErrorSuccess;
          } else {
            /* SRS195 */
            message_router_response->general_status = kCipErrorInvalidParameterValue;
          }
        } else {
          /* SRS196 */
          /* Stop NET LED Flash Sequence */
          g_ns_led_flash_flag = false;
          attribute = GetCipAttribute(instance, 29);
          OPENER_ASSERT(NULL != attribute);
          CipUNID *proposed_tunid = (CipUNID *) attribute->data;
          proposed_tunid->snn.time_of_day = tunid_req.snn.time_of_day;
          proposed_tunid->snn.date = tunid_req.snn.date;
          proposed_tunid->nodeid = tunid_req.nodeid;
          message_router_response->general_status = kCipErrorSuccess;
        }
      }
    }
  }

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus ApplyTUNIDSafetySupervisor(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  EipUint32 offset = 0;
  CipAttributeStruct *attribute = NULL;

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size < 10) {
    message_router_response->general_status = kCipErrorNotEnoughData;
  } else if (message_router_request->request_path_size > 10) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    CipUNID tunid_req = {0};
    if (message_router_request->request_path_size == 10) {
      offset = 0;
      memcpy(&tunid_req.snn.time_of_day, &message_router_request->data[offset], sizeof(tunid_req.snn.time_of_day));
      offset += sizeof(tunid_req.snn.time_of_day);
      memcpy(&tunid_req.snn.date, &message_router_request->data[offset], sizeof(tunid_req.snn.date));
      offset += sizeof(tunid_req.snn.date);
      memcpy(&tunid_req.nodeid, &message_router_request->data[offset], sizeof(tunid_req.nodeid));
      offset += sizeof(tunid_req.nodeid);
    }

    attribute = GetCipAttribute(instance, 11);
    OPENER_ASSERT(NULL != attribute);
    CipUsint *device_status = (CipUsint *) attribute->data;
    if (*device_status == kDeviceStatusWaitingforTUNID) {
      if (!IsUnidAll0xFF(&tunid_req) ) {
        attribute = GetCipAttribute(instance, 29);
        OPENER_ASSERT(NULL != attribute);
        CipUNID *proposed_tunid = (CipUNID *) attribute->data;
        if (!UnidCmp(proposed_tunid, &tunid_req) ) {
          unsigned short rx_len = 0;
          CipOctet rx_buf[CIP_SAFETY_CMD_CODE_LEN_0x0202] = {0};

          if (0 != cip_safety_exp_msg_wait(sizeof(rx_buf), &rx_len, rx_buf, message_router_response)) {
            message_router_response->general_status = kCipErrorResourceUnavailable;
          } else {
            if (message_router_response->general_status == kCipErrorSuccess) {
              /* SRS197 */
              /* Stop Flash Sequence */
              g_ns_led_flash_flag = false;
              attribute = GetCipAttribute(instance, 27);
              OPENER_ASSERT(NULL != attribute);
              CipUNID *tunid = (CipUNID *) attribute->data;
              tunid->snn.time_of_day = tunid_req.snn.time_of_day;
              tunid->snn.date = tunid_req.snn.date;
              tunid->nodeid = tunid_req.nodeid;
              /* Call the PostSetCallback if enabled. */
              if (NULL != instance->cip_class->PostSetCallback) {
                instance->cip_class->PostSetCallback(instance, 0, 0);
              }
              SetUnidAll0xFF(&proposed_tunid);
            }
          }
        } else {
          message_router_response->general_status = kCipErrorInvalidParameterValue;
        }
      } else {
        message_router_response->general_status = kCipErrorInvalidParameterValue;
      }
    } else {
      message_router_response->general_status = kCipErrorObjectStateConflict;
    }
  }

  InitializeENIPMessage(&message_router_response->message);
  return kEipStatusOkSend;
}

EipStatus CipSafetySupervisorInit(void) {
  CipClass *safetysupervisor_class = NULL;

  safetysupervisor_class = CreateCipClass(kCipSafetySupervisorClassCode,
                                          0, /* # class attributes */
                                          7, /* # highest class attribute number */
                                          1, /* # class services */
                                          29,/* # instance attributes */
                                          29,/* # of highest instance attribute */
                                          11,/* # instance services */
                                          1, /* # instances */
                                          "Safety Supervisor", /* object class name */
                                          1,  /* # class revision */
                                          NULL /* function pointer for initialization */
                                          );

  if(NULL == safetysupervisor_class) {
    return kEipStatusError;
  }

  /* Add services to the class */
  InsertService(safetysupervisor_class, kGetAttributeSingle,
                GetAttributeSingle, "GetAttributeSingle");
  InsertService(safetysupervisor_class, kApplyAttributes,
                ApplyAttributesSafetySupervisor,
                "ApplyAttributesSafetySupervisor");
  InsertService(safetysupervisor_class, kSetAttributeSingle,
                &SetAttributeSingleSafetySupervisor,
                "SetAttributeSingleSafetySupervisor");
  InsertService(safetysupervisor_class, kConfigureRequest,
                &ConfigureRequestSafetySupervisor,
                "ConfigureRequestSafetySupervisor");
  InsertService(safetysupervisor_class, kValidateConfiguration,
                &ValidateConfigurationSafetySupervisor,
                "ValidateConfigurationSafetySupervisor");
  InsertService(safetysupervisor_class, kSetPassword,
                &SetPasswordSafetySupervisor,
                "SetPasswordSafetySupervisor");
  InsertService(safetysupervisor_class, kConfigurationLockUnlock,
                &ConfigurationLockUnlockSafetySupervisor,
                "ConfigurationLockUnlockSafetySupervisor");
  InsertService(safetysupervisor_class, kSafetyReset,
                &SafetyResetSafetySupervisor,
                "SafetyResetSafetySupervisor");
  InsertService(safetysupervisor_class, kResetPassword,
                &ResetPasswordSafetySupervisor,
                "ResetPasswordSafetySupervisor");
  InsertService(safetysupervisor_class, kProposeTUNID,
                &ProposeTUNIDSafetySupervisor,
                "ProposeTUNIDSafetySupervisor");
  InsertService(safetysupervisor_class, kApplyTUNID,
                &ApplyTUNIDSafetySupervisor,
                "ApplyTUNIDSafetySupervisor");
  InsertGetSetCallback(safetysupervisor_class, SafetySupervisorPreGetCallback, kPreGetFunc);

  /* Bind attributes to the instance */
  CipInstance *safetysupervisor_instance = GetCipInstance(safetysupervisor_class, 1u);

  InsertAttribute(safetysupervisor_instance,
                  11,
                  kCipUsint,
                  EncodeCipUsint,
                  &g_safetysupervisor.device_status,
                  kGetableSingle|kPreGetFunc);
  InsertAttribute(safetysupervisor_instance,
                  12,
                  kCipByte,
                  EncodeCipByte,
                  &g_safetysupervisor.exception_status,
                  kGetableSingle|kPreGetFunc);
  InsertAttribute(safetysupervisor_instance,
                  13,
                  kCipAny,
                  EncodeCipExceptionDetail,
                  &g_safetysupervisor.alarm,
                  kGetableSingle);
  InsertAttribute(safetysupervisor_instance,
                  14,
                  kCipAny,
                  EncodeCipExceptionDetail,
                  &g_safetysupervisor.warning,
                  kGetableSingle);
  InsertAttribute(safetysupervisor_instance,
                  15,
                  kCipBool,
                  EncodeCipBool,
                  &g_safetysupervisor.alarm_enable,
                  kSetAndGetAble|kPreSetFunc|kPostSetFunc);
  InsertAttribute(safetysupervisor_instance,
                  16,
                  kCipBool,
                  EncodeCipBool,
                  &g_safetysupervisor.warning_enable,
                  kSetAndGetAble|kPreSetFunc|kPostSetFunc);
  InsertAttribute(safetysupervisor_instance,
                  24,
                  kCipBool,
                  EncodeCipBool,
                  &g_safetysupervisor.config_lock,
                  kGetableSingle);
  InsertAttribute(safetysupervisor_instance,
                  25,
                  kCipAny,
                  EncodeCipUNID,
                  &g_safetysupervisor.cfunid,
                  kGetableSingle);
  InsertAttribute(safetysupervisor_instance,
                  26,
                  kCipAny,
                  EncodeCipSCID,
                  &g_safetysupervisor.scid,
                  kGetableSingle);
  InsertAttribute(safetysupervisor_instance,
                  27,
                  kCipAny,
                  EncodeCipUNID,
                  &g_safetysupervisor.tunid,
                  kGetableSingle);
  InsertAttribute(safetysupervisor_instance,
                  28,
                  kCipAny,
                  EncodeCipOutputConnectionOwners,
                  &g_safetysupervisor.output_connection_owners,
                  kGetableSingle);
  InsertAttribute(safetysupervisor_instance,
                  29,
                  kCipAny,
                  EncodeCipUNID,
                  &g_safetysupervisor.proposed_tunid,
                  kGetableSingle);

  /* Set attributes to initial values */
  /* Password */
  memset(g_safetysupervisor.password, 0, OPENER_SAFETYSUPERVISOR_NUM_PASSWORD);
  /* Device Status */
  g_safetysupervisor.device_status = kDeviceStatusUndefined;
  /* Exception Status */
  g_safetysupervisor.exception_status = kExceptionStatusExpandedMethod;
  /* Exception Detail Alarm */
  g_safetysupervisor.alarm.common.size = OPENER_SAFETYSUPERVISOR_NUM_COMMON_DETAIL;
  memset(g_safetysupervisor.alarm.common.detail, 0, OPENER_SAFETYSUPERVISOR_MAX_DETAIL);
  g_safetysupervisor.alarm.device.size = 0;
  memset(g_safetysupervisor.alarm.device.detail, 0, OPENER_SAFETYSUPERVISOR_MAX_DETAIL);
  g_safetysupervisor.alarm.manufacturer.size = 0;
  memset(g_safetysupervisor.alarm.manufacturer.detail, 0, OPENER_SAFETYSUPERVISOR_MAX_DETAIL);
  /* Exception Detail Warning */
  g_safetysupervisor.warning.common.size = OPENER_SAFETYSUPERVISOR_NUM_COMMON_DETAIL;
  memset(g_safetysupervisor.warning.common.detail, 0, OPENER_SAFETYSUPERVISOR_MAX_DETAIL);
  g_safetysupervisor.warning.device.size = 0;
  memset(g_safetysupervisor.warning.device.detail, 0, OPENER_SAFETYSUPERVISOR_MAX_DETAIL);
  g_safetysupervisor.warning.manufacturer.size = 0;
  memset(g_safetysupervisor.warning.manufacturer.detail, 0, OPENER_SAFETYSUPERVISOR_MAX_DETAIL);
  /* Alarm Enable */
  g_safetysupervisor.alarm_enable = 0;
  /* Warning Enable */
  g_safetysupervisor.warning_enable = 0;
  /* Configuration Lock */
  g_safetysupervisor.config_lock = 0;
  /* Configuration UNID */
  g_safetysupervisor.cfunid.snn.time_of_day = 0;
  g_safetysupervisor.cfunid.snn.date = 0;
  g_safetysupervisor.cfunid.nodeid = 0;
  /* Safety Configuration Identifier */
  g_safetysupervisor.scid.sccrc = 0;
  g_safetysupervisor.scid.scts.time_of_day = 0;
  g_safetysupervisor.scid.scts.date = 0;
  /* Target UNID */
  g_safetysupervisor.tunid.snn.time_of_day = 0xFFFFFFFF;
  g_safetysupervisor.tunid.snn.date = 0xFFFF;
  g_safetysupervisor.tunid.nodeid = 0xFFFFFFFF;
  /* Output Connection Point Owners */
  g_safetysupervisor.output_connection_owners.num = 1;
  for(size_t i = 0; i < OPENER_SAFETYSUPERVISOR_MAX_OUTPUT_OWNER; i++) {
    g_safetysupervisor.output_connection_owners.output_owners[i].ocpunid.snn.time_of_day = 0;
    g_safetysupervisor.output_connection_owners.output_owners[i].ocpunid.snn.date = 0;
    g_safetysupervisor.output_connection_owners.output_owners[i].ocpunid.nodeid = 0;
    g_safetysupervisor.output_connection_owners.output_owners[i].app_resource.path_size = 5;
    g_safetysupervisor.output_connection_owners.output_owners[i].app_resource.class_id = 4;
    g_safetysupervisor.output_connection_owners.output_owners[i].app_resource.instance_number = 0x1B1;
    g_safetysupervisor.output_connection_owners.output_owners[i].app_resource.attribute_number = 0;
  }
  /* Proposed TUNID */
  g_safetysupervisor.proposed_tunid.snn.time_of_day = 0xFFFFFFFF;
  g_safetysupervisor.proposed_tunid.snn.date = 0xFFFF;
  g_safetysupervisor.proposed_tunid.nodeid = 0xFFFFFFFF;
  /* Config Data */
  memset(g_safetysupervisor.config_data, 0, OPENER_SAFETYSUPERVISOR_ASSEMBLY_CONFIG_BYTE);

  return kEipStatusOk;
}

EipStatus CipSafetySupervisorStart(void) {
  CipUsint i = 1;

  /* Initializes SCI for CIP Safety */
  if (cip_safety_sci_init() != 0) {
      return kEipStatusError;
  }

  /* Waits the first Start Up Message request frame */
  while (i--) {
    if (0 == cip_safety_start_up_req_wait()) {
      break;
    }
  }

  /* Finishs Start Up Message sequence */
  cip_safety_start_up_finish();

  /* Waits the first Data Acquisition frame */
  while (1) {
    if (0 == cip_safety_data_acq_wait()) {
      break;
    }
  }

  /* Starts to send Data Acquisition frame on cyclic */
  cip_safety_data_acq_start_cyclic();

  return kEipStatusOk;
}

void CipSafetySupervisorSetStatus(CipUsint device_status, CipByte exception_status) {
  if ((g_safetysupervisor.device_status != kDeviceStatusConfiguring) &&
      (device_status == kDeviceStatusConfiguring)){
      /* SRS198 */
      SetScidAll0x00(&g_safetysupervisor.scid);
  }

  g_safetysupervisor.device_status = device_status;
  g_safetysupervisor.exception_status = exception_status;
}

CipUsint CipSafetySupervisorGetDeviceStatus(void) {
  return g_safetysupervisor.device_status;
}

CipBool CipSafetySupervisorGetNETLEDflash(void) {
  return g_ns_led_flash_flag;
}

EipStatus CipSafetySupervisorGetSafetyNetworkNumber(CipUsint *p_safety_net_num) {
  EipUint32 offset = 0;

  if (p_safety_net_num == NULL) {
      return kEipStatusError;
  }

  memcpy((void *)&p_safety_net_num[offset], (void *)&g_safetysupervisor.tunid.snn.time_of_day, sizeof(g_safetysupervisor.tunid.snn.time_of_day));
  offset += sizeof(g_safetysupervisor.tunid.snn.time_of_day);
  memcpy((void *)&p_safety_net_num[offset], (void *)&g_safetysupervisor.tunid.snn.date, sizeof(g_safetysupervisor.tunid.snn.date));
  offset += sizeof(g_safetysupervisor.tunid.snn.date);

  return kEipStatusOk;
}

size_t CipSafetySupervisorProcessSafetyNetworkSegment(const EipUint8 *message, CipBool config_data_in, CipBool output_device, CipDword consumed_instance_id) {
  EipUint32 offset = 0;

  memset((void *)&g_safety_network_segment, 0, sizeof(g_safety_network_segment));

  /* Config Data in Safety Open */
  g_safety_network_segment.config_data_in = config_data_in;
  /* Output Device */
  g_safety_network_segment.output_device = output_device;
  /* Consumed Instance ID */
  g_safety_network_segment.consumed_instance_id = consumed_instance_id;

  /* Safety Network Segment */
  if (message != NULL) {
    /* Network Segment Data Length */
    memcpy((void *)&g_safety_network_segment.length, (void *)&message[offset], sizeof(g_safety_network_segment.length));
    offset += sizeof(g_safety_network_segment.length);
    /* Safety Network Segment Format */
    memcpy((void *)&g_safety_network_segment.format, (void *)&message[offset], sizeof(g_safety_network_segment.format));
    offset += sizeof(g_safety_network_segment.format);
    /* Safety Network Segment Data */
    if ((g_safety_network_segment.format == kSafetyNetworkSegmentFormatExtendedFormat) &&
        (g_safety_network_segment.length == kSafetyNetworkSegmentLengthExtendedFormat)) {
        /* Safety Network Segment: Extended Format (0x02) */
        /* Reserved */
        memcpy((void *)&g_safety_network_segment.extended.reserved, (void *)&message[offset], sizeof(g_safety_network_segment.extended.reserved));
        offset += sizeof(g_safety_network_segment.extended.reserved);
        /* Configuration CRC (SCCRC) */
        memcpy((void *)&g_safety_network_segment.extended.sccrc, (void *)&message[offset], sizeof(g_safety_network_segment.extended.sccrc));
        offset += sizeof(g_safety_network_segment.extended.sccrc);
        /* Configuration TimeStamp (SCTS) */
        /* Time */
        memcpy((void *)&g_safety_network_segment.extended.scts.time_of_day, (void *)&message[offset], sizeof(g_safety_network_segment.extended.scts.time_of_day));
        offset += sizeof(g_safety_network_segment.extended.scts.time_of_day);
        /* Date */
        memcpy((void *)&g_safety_network_segment.extended.scts.date, (void *)&message[offset], sizeof(g_safety_network_segment.extended.scts.date));
        offset += sizeof(g_safety_network_segment.extended.scts.date);
        /* Time Correction EPI */
        memcpy((void *)&g_safety_network_segment.extended.time_correction_epi, (void *)&message[offset], sizeof(g_safety_network_segment.extended.time_correction_epi));
        offset += sizeof(g_safety_network_segment.extended.time_correction_epi);
        /* Time Correction Network Connection Parameters */
        memcpy((void *)&g_safety_network_segment.extended.time_correction_network_connection_parameters, (void *)&message[offset], sizeof(g_safety_network_segment.extended.time_correction_network_connection_parameters));
        offset += sizeof(g_safety_network_segment.extended.time_correction_network_connection_parameters);
        /* Target_UNID (TUNID) */
        /* Safety Network Number */
        /* Time */
        memcpy((void *)&g_safety_network_segment.extended.tunid.snn.time_of_day, (void *)&message[offset], sizeof(g_safety_network_segment.extended.tunid.snn.time_of_day));
        offset += sizeof(g_safety_network_segment.extended.tunid.snn.time_of_day);
        /* Date */
        memcpy((void *)&g_safety_network_segment.extended.tunid.snn.date, (void *)&message[offset], sizeof(g_safety_network_segment.extended.tunid.snn.date));
        offset += sizeof(g_safety_network_segment.extended.tunid.snn.date);
        /* NodeID */
        memcpy((void *)&g_safety_network_segment.extended.tunid.nodeid, (void *)&message[offset], sizeof(g_safety_network_segment.extended.tunid.nodeid));
        offset += sizeof(g_safety_network_segment.extended.tunid.nodeid);
        /* Originator UNID (OUNID) */
        /* Safety Network Number */
        /* Time */
        memcpy((void *)&g_safety_network_segment.extended.ounid.snn.time_of_day, (void *)&message[offset], sizeof(g_safety_network_segment.extended.ounid.snn.time_of_day));
        offset += sizeof(g_safety_network_segment.extended.ounid.snn.time_of_day);
        /* Date */
        memcpy((void *)&g_safety_network_segment.extended.ounid.snn.date, (void *)&message[offset], sizeof(g_safety_network_segment.extended.ounid.snn.date));
        offset += sizeof(g_safety_network_segment.extended.ounid.snn.date);
        /* NodeID */
        memcpy((void *)&g_safety_network_segment.extended.ounid.nodeid, (void *)&message[offset], sizeof(g_safety_network_segment.extended.ounid.nodeid));
        offset += sizeof(g_safety_network_segment.extended.ounid.nodeid);
        /* Ping_Interval_EPI_Multiplier */
        memcpy((void *)&g_safety_network_segment.extended.ping_interval_epi_multiplier, (void *)&message[offset], sizeof(g_safety_network_segment.extended.ping_interval_epi_multiplier));
        offset += sizeof(g_safety_network_segment.extended.ping_interval_epi_multiplier);
        /* Time_Coord_Msg_Min_Multiplier */
        memcpy((void *)&g_safety_network_segment.extended.time_coord_msg_min_multiplier, (void *)&message[offset], sizeof(g_safety_network_segment.extended.time_coord_msg_min_multiplier));
        offset += sizeof(g_safety_network_segment.extended.time_coord_msg_min_multiplier);
        /* Network_Time_Expectation_Multiplier */
        memcpy((void *)&g_safety_network_segment.extended.network_time_expectation_multiplier, (void *)&message[offset], sizeof(g_safety_network_segment.extended.network_time_expectation_multiplier));
        offset += sizeof(g_safety_network_segment.extended.network_time_expectation_multiplier);
        /* Timeout_Multiplier */
        memcpy((void *)&g_safety_network_segment.extended.timeout_multiplier, (void *)&message[offset], sizeof(g_safety_network_segment.extended.timeout_multiplier));
        offset += sizeof(g_safety_network_segment.extended.timeout_multiplier);
        /* Max_Consumer_Number */
        memcpy((void *)&g_safety_network_segment.extended.max_consumer_number, (void *)&message[offset], sizeof(g_safety_network_segment.extended.max_consumer_number));
        offset += sizeof(g_safety_network_segment.extended.max_consumer_number);
        /* Max_Fault_Number */
        memcpy((void *)&g_safety_network_segment.extended.max_fault_number, (void *)&message[offset], sizeof(g_safety_network_segment.extended.max_fault_number));
        offset += sizeof(g_safety_network_segment.extended.max_fault_number);
        /* Connection Parameters CRC (CPCRC) */
        memcpy((void *)&g_safety_network_segment.extended.cpcrc, (void *)&message[offset], sizeof(g_safety_network_segment.extended.cpcrc));
        offset += sizeof(g_safety_network_segment.extended.cpcrc);
        /* Time Correction Connection ID */
        memcpy((void *)&g_safety_network_segment.extended.time_correction_connection_id, (void *)&message[offset], sizeof(g_safety_network_segment.extended.time_correction_connection_id));
        offset += sizeof(g_safety_network_segment.extended.time_correction_connection_id);
        /* Initial Time Stamp */
        memcpy((void *)&g_safety_network_segment.extended.initial_time_stamp, (void *)&message[offset], sizeof(g_safety_network_segment.extended.initial_time_stamp));
        offset += sizeof(g_safety_network_segment.extended.initial_time_stamp);
        /* Initial Rollover Value */
        memcpy((void *)&g_safety_network_segment.extended.initial_rollover_value, (void *)&message[offset], sizeof(g_safety_network_segment.extended.initial_rollover_value));
        offset += sizeof(g_safety_network_segment.extended.initial_rollover_value);
    }
  }

  return (g_safety_network_segment.length * 2);
}

void CipSafetySupervisorProcessSafetyOpen(void) {
  CipBool nv_set_flag = false;
  CipClass *safetysupervisor_class = NULL;
  CipInstance *instance = NULL;

  if (g_safety_network_segment.config_data_in == true) {
    /* Type 1 SafetyOpen */
    nv_set_flag = true;
    /* CFUNID == 0? */
    if (IsUnidAll0x00(&g_safetysupervisor.cfunid)) {
      /* Set CFUNID=OUNID */
      UnidCpy(&g_safetysupervisor.cfunid, &g_safety_network_segment.extended.ounid);
      nv_set_flag = true;
    }
    /* Output Device? */
    if (g_safety_network_segment.output_device == true) {
      for(size_t i = 0; i < OPENER_SAFETYSUPERVISOR_MAX_OUTPUT_OWNER; i++) {
        if(g_safetysupervisor.output_connection_owners.output_owners[i].app_resource.instance_number == g_safety_network_segment.consumed_instance_id) {
          /* Set OCPUNID=OUNID */
          UnidCpy(&g_safetysupervisor.output_connection_owners.output_owners[i].ocpunid, &g_safety_network_segment.extended.ounid);
          nv_set_flag = true;
          break;
        }
      }
    }
  } else {
    /* Type 2 SafetyOpenType 2 SafetyOpen */
    /* Output Device? */
    if (g_safety_network_segment.output_device == true) {
      for(size_t i = 0; i < OPENER_SAFETYSUPERVISOR_MAX_OUTPUT_OWNER; i++) {
        if(g_safetysupervisor.output_connection_owners.output_owners[i].app_resource.instance_number == g_safety_network_segment.consumed_instance_id) {
          /* Set OCPUNID=OUNID */
          UnidCpy(&g_safetysupervisor.output_connection_owners.output_owners[i].ocpunid, &g_safety_network_segment.extended.ounid);
          nv_set_flag = true;
          break;
        }
      }
    }
  }

  if (nv_set_flag == true) {
    safetysupervisor_class = GetCipClass(kCipSafetySupervisorClassCode);
    if (NULL != safetysupervisor_class) {
      instance = GetCipInstance(safetysupervisor_class, 1);
      /* Call the PostSetCallback if enabled. */
      if (NULL != instance->cip_class->PostSetCallback) {
        instance->cip_class->PostSetCallback(instance, 0, 0);
      }
    }
  }
}

int CipSafetySupervisorCmpPassword(CipUsint * buf) {
  if (memcmp((void *)&g_safetysupervisor.password[0], (void *)&buf[0], OPENER_SAFETYSUPERVISOR_NUM_PASSWORD) == 0) {
    return 0;
  }

  return -1;
}
#endif
