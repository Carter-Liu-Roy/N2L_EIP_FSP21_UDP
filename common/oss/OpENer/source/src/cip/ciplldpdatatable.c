/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the LLDP Data Table Object
 * @author Stefan Maetje <stefan.maetje@esd.eu>
 *
 *  CIP LLDP Data Table Object
 *  ==============
 *
 *  This module implements the LLDP Data Table Object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  1: Ethernet Link Instance Number
 *  - Attribute  2: MAC Address
 *  - Attribute  3: Interface Label
 *  - Attribute  4: Time to Live
 *  - Attribute  5: System Capabilities TLV
 *  - Attribute  6: IPv4 Management Addresses
 *  - Attribute  7: CIP Identification
 *  - Attribute  8: Additional Ethernet Capabilities
 *  - Attribute  9: Last Change
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributeSingle
 *  - FindNextObjectInstance
 */
/* ********************************************************************
 * include files
 */
#include "ciplldpdatatable.h"

#include <string.h>

#include "cipcommon.h"
#include "opener_api.h"
#include "trace.h"
#include "endianconv.h"

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
CipLLDPDataTableObject g_lldpdatatable[OPENER_LLDPDATATABLE_INSTANCE_CNT] = {0};
EipByte g_lldpdatatable_interface_label_table[OPENER_LLDPDATATABLE_INSTANCE_CNT][256] = {0};
CipInstance g_lldpdatatable_instance[OPENER_LLDPDATATABLE_INSTANCE_CNT] = {0};
CipAttributeStruct g_lldpdatatable_attribute[OPENER_LLDPDATATABLE_INSTANCE_CNT][9] = {0};

/* ********************************************************************
 * local functions
 */
static void EncodeCipSystemCapabilitiesTLV(const void *const data,
                                 ENIPMessage *const outgoing_message) {
  CipSystemCapabilitiesTLV *system_capabilities_tlv = (CipSystemCapabilitiesTLV *)data;
  EncodeCipUint(&system_capabilities_tlv->system_capabilities, outgoing_message);
  EncodeCipUint(&system_capabilities_tlv->enabled_capabilities, outgoing_message);
}

static void EncodeCipIPv4ManagementAddresses(const void *const data,
                                 ENIPMessage *const outgoing_message) {
  CipUsint i = 0;

  CipIPv4ManagementAddresses *ipv4_management_addresses = (CipIPv4ManagementAddresses *)data;
  EncodeCipUsint(&ipv4_management_addresses->management_address_count, outgoing_message);
  for(i = 0; i < ipv4_management_addresses->management_address_count; i++) {
    EncodeCipUdint(&ipv4_management_addresses->management_address[i], outgoing_message);
  }
}

static void EncodeCipCIPIdentification(const void *const data,
                                 ENIPMessage *const outgoing_message) {
  CipCIPIdentification *cip_identification = (CipCIPIdentification *)data;
  EncodeCipUint(&cip_identification->vendor_id, outgoing_message);
  EncodeCipUint(&cip_identification->device_type, outgoing_message);
  EncodeCipUint(&cip_identification->product_code, outgoing_message);
  EncodeCipUsint(&cip_identification->major_revision, outgoing_message);
  EncodeCipUsint(&cip_identification->minor_revision, outgoing_message);
  EncodeCipUdint(&cip_identification->cip_serial_number, outgoing_message);
}

static void EncodeCipAdditionalEthernetCapabilities(const void *const data,
                                 ENIPMessage *const outgoing_message) {
  CipAdditionalEthernetCapabilities *additional_ethernet_capabilities = (CipAdditionalEthernetCapabilities *)data;
  EncodeCipBool(&additional_ethernet_capabilities->preemption_support, outgoing_message);
  EncodeCipBool(&additional_ethernet_capabilities->preemption_status, outgoing_message);
  EncodeCipBool(&additional_ethernet_capabilities->preemption_active, outgoing_message);
  EncodeCipUsint(&additional_ethernet_capabilities->additional_fragment_size, outgoing_message);
}

CipInstance *AddLLDPDataTableInstance(const EipUint32 instance_id) {
  CipClass *lldpdatatable_class = GetCipClass(kCipLLDPDataTableClassCode);
  CipInstance *ret_instance = NULL;
  CipInstance *next_instance = NULL;
  CipUint i = 0;

  if(NULL == lldpdatatable_class) {
    return NULL;
  }

  if(instance_id > OPENER_LLDPDATATABLE_INSTANCE_CNT) {
    return NULL;
  }

  if(instance_id == 0) {
    return NULL;
  }

  ret_instance = GetCipInstance(lldpdatatable_class, instance_id);

  if(ret_instance == NULL) {
    if(lldpdatatable_class->instances == NULL) {
      g_lldpdatatable_instance[instance_id - 1].attributes = &g_lldpdatatable_attribute[instance_id - 1][0];
      lldpdatatable_class->instances = &g_lldpdatatable_instance[instance_id - 1];
    } else {
      g_lldpdatatable_instance[instance_id - 1].attributes = &g_lldpdatatable_attribute[instance_id - 1][0];
      for(i = 0; i < OPENER_LLDPDATATABLE_INSTANCE_CNT; i++) {
        g_lldpdatatable_instance[i].next = NULL;
        if(g_lldpdatatable_instance[i].attributes != NULL) {
          if(next_instance != NULL) {
            next_instance->next = &g_lldpdatatable_instance[i];
          }
          next_instance = &g_lldpdatatable_instance[i];
        }
      }
    }
    ret_instance = &g_lldpdatatable_instance[instance_id - 1];
    lldpdatatable_class->number_of_instances++;
  }

  return ret_instance;
}

CipInstance *DelLLDPDataTableInstance(const EipUint32 instance_id) {
  CipClass *lldpdatatable_class = GetCipClass(kCipLLDPDataTableClassCode);
  CipInstance *ret_instance = NULL;
  CipInstance *next_instance = NULL;
  CipUint i = 0;

  if(NULL == lldpdatatable_class) {
    return NULL;
  }

  if(instance_id > OPENER_LLDPDATATABLE_INSTANCE_CNT) {
    return NULL;
  }

  if(instance_id == 0) {
    return NULL;
  }

  ret_instance = GetCipInstance(lldpdatatable_class, instance_id);

  if(ret_instance != NULL) {
    lldpdatatable_class->instances = NULL;
    g_lldpdatatable_instance[instance_id - 1].attributes = NULL;
    memset((void *)&g_lldpdatatable[instance_id - 1], 0, sizeof(g_lldpdatatable[instance_id - 1]));
    g_lldpdatatable[instance_id - 1].interface_label.string = &g_lldpdatatable_interface_label_table[instance_id - 1][0];
    for(i = 0; i < OPENER_LLDPDATATABLE_INSTANCE_CNT; i++) {
      g_lldpdatatable_instance[i].next = NULL;
      if(g_lldpdatatable_instance[i].attributes != NULL) {
        if(lldpdatatable_class->instances == NULL) {
          lldpdatatable_class->instances = &g_lldpdatatable_instance[i];
        }
        if(next_instance != NULL) {
          next_instance->next = &g_lldpdatatable_instance[i];
        }
        next_instance = &g_lldpdatatable_instance[i];
      }
    }
    ret_instance = &g_lldpdatatable_instance[instance_id - 1];
    lldpdatatable_class->number_of_instances--;
  }

  return ret_instance;
}

void ALLDelLLDPDataTableInstance(void) {
  EipUint32 instance_id;

  for(instance_id = 1; instance_id <= OPENER_LLDPDATATABLE_INSTANCE_CNT; instance_id++) {
    DelLLDPDataTableInstance(instance_id);
  }
}

/* ********************************************************************
 * public functions
 */
EipStatus FindNextObjectInstanceLLDPDataTable(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  CipClass *const lldpdatatable_class = GetCipClass(kCipLLDPDataTableClassCode);
  CipUint i = 0;
  CipUsint max_return_val_req = 0;
  CipUsint max_return_val_res = 0;
  CipUsint last_return_val_res = 0;
  CipUint instance_number = 0;
  CipUint number_of_instances = 0;
  CipInstance *ret_instance = NULL;

  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service = (0x80
                                            | message_router_request->service);

  instance_number = message_router_request->request_path.instance_number;
  number_of_instances = lldpdatatable_class->number_of_instances;
  max_return_val_req = GetUsintFromMessage(&(message_router_request->data));
  if(instance_number == 0) {
    if(max_return_val_req > number_of_instances) {
      last_return_val_res = number_of_instances;
    } else {
      last_return_val_res = max_return_val_req;
    }
    for(i = 0; i < last_return_val_res; i++) {
      ret_instance = GetCipInstance(lldpdatatable_class, (i + 1));
      if(ret_instance != NULL) {
        max_return_val_res++;
      }
    }
    /* the end of the list has been reached */
    if(max_return_val_req > number_of_instances) {
      max_return_val_res++;
    } else if(max_return_val_req > max_return_val_res) {
      max_return_val_res++;
    }
    AddSintToMessage(max_return_val_res, &message_router_response->message);
    for(i = 0; i < last_return_val_res; i++) {
      ret_instance = GetCipInstance(lldpdatatable_class, (i + 1));
      if(ret_instance != NULL) {
        AddIntToMessage((i + 1), &message_router_response->message);
      }
    }
    /* the end of the list has been reached */
    if(max_return_val_req > number_of_instances) {
      AddIntToMessage(0, &message_router_response->message);
    } else if(max_return_val_req > max_return_val_res) {
      AddIntToMessage(0, &message_router_response->message);
    }
  } else {
    message_router_response->general_status = kCipErrorPathDestinationUnknown;
    message_router_response->size_of_additional_status = 0;
  }

  return kEipStatusOkSend;
}

void InitializeLLDPDataTable(CipClass *class) {

  CipClass *meta_class = class->class_instance.cip_class;

  InsertAttribute( (CipInstance *) class, 1, kCipUint, EncodeCipUint,
                   (void *) &class->revision, kGetableSingleAndAll );                                                /* revision */
  InsertAttribute( (CipInstance *) class, 2, kCipUint, EncodeCipUint,
                   (void *) &class->number_of_instances, kGetableSingleAndAll );                                                /*  largest instance number */
  InsertAttribute( (CipInstance *) class, 3, kCipUint, EncodeCipUint,
                   (void *) &class->number_of_instances, kGetableSingleAndAll );                                                /* number of instances currently existing*/
  InsertAttribute( (CipInstance *) class, 4, kCipUint, EncodeCipUint,
                   (void *) &kCipUintZero, kGetableAllDummy );                                                /* optional attribute list - default = 0 */
  InsertAttribute( (CipInstance *) class, 5, kCipUint, EncodeCipUint,
                   (void *) &kCipUintZero, kNotSetOrGetable );                                                /* optional service list - default = 0 */
  InsertAttribute( (CipInstance *) class, 6, kCipUint, EncodeCipUint,
                   (void *) &meta_class->highest_attribute_number,
                   kGetableSingle );                                                                                                /* max class attribute number*/
  InsertAttribute( (CipInstance *) class, 7, kCipUint, EncodeCipUint,
                   (void *) &class->highest_attribute_number,
                   kGetableSingle );                                                                                           /* max instance attribute number*/

  InsertService(meta_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll");                                                 /* bind instance services to the metaclass*/
  InsertService(meta_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle");
  InsertService(meta_class,
                kFindNextObjectInstance,
                &FindNextObjectInstanceLLDPDataTable,
                "FindNextObjectInstanceLLDPDataTable");
}

EipStatus CipLLDPDataTableInit(void) {
  CipClass *lldpdatatable_class = NULL;
  CipUint i = 0;

  lldpdatatable_class = CreateCipClass(kCipLLDPDataTableClassCode,
                             0, /* # class attributes */
                             7, /* # highest class attribute number */
                             3, /* # class services */
                             9,/* # instance attributes */
                             9,/* # of highest instance attribute */
                             1, /* # instance services */
                             0, /* # instances */
                             "LLDP Data Table", /* object class name */
                             1,  /* # class revision */
                             &InitializeLLDPDataTable /* function pointer for initialization */
                             );

  if(NULL == lldpdatatable_class) {
    return kEipStatusError;
  }

  /* Add services to the class */
  InsertService(lldpdatatable_class, kGetAttributeSingle,
  GetAttributeSingle, "GetAttributeSingle");

  /* Set attributes to initial values */
  for(i = 0; i < OPENER_LLDPDATATABLE_INSTANCE_CNT; i++) {
    g_lldpdatatable[i].interface_label.string = &g_lldpdatatable_interface_label_table[i][0];
    g_lldpdatatable_instance[i].instance_number = i + 1;
    g_lldpdatatable_instance[i].cip_class = lldpdatatable_class;
  }

  return kEipStatusOk;
}

CipInstance *CreateLLDPDataTableObject(const EipUint32 instance_id) {
  CipClass *lldpdatatable_class = GetCipClass(kCipLLDPDataTableClassCode);

  if(NULL == lldpdatatable_class) {
    return NULL;
  }

  if(0 == instance_id) {
    return NULL;
  }

  /* Bind attributes to the instance */
  CipInstance *const lldpdatatable_instance = AddLLDPDataTableInstance(instance_id);

  InsertAttribute(lldpdatatable_instance,
                1,
                kCipUint,
                EncodeCipUint,
                &g_lldpdatatable[instance_id - 1].ethernet_link_instance_number,
                kGetableSingle);
  InsertAttribute(lldpdatatable_instance,
                2,
                kCip6Usint,
                EncodeCipEthernetLinkPhyisicalAddress,
                &g_lldpdatatable[instance_id - 1].mac_address,
                kGetableSingle);
  InsertAttribute(lldpdatatable_instance,
                3,
                kCipShortString,
                EncodeCipShortString,
                &g_lldpdatatable[instance_id - 1].interface_label,
                kGetableSingle);
  InsertAttribute(lldpdatatable_instance,
                4,
                kCipUint,
                EncodeCipUint,
                &g_lldpdatatable[instance_id - 1].time_to_live,
                kGetableSingle);
  InsertAttribute(lldpdatatable_instance,
                5,
                kCipAny,
                EncodeCipSystemCapabilitiesTLV,
                &g_lldpdatatable[instance_id - 1].system_capabilities_tlv,
                kGetableSingle);
  InsertAttribute(lldpdatatable_instance,
                6,
                kCipAny,
                EncodeCipIPv4ManagementAddresses,
                &g_lldpdatatable[instance_id - 1].ipv4_management_addresses,
                kGetableSingle);
  InsertAttribute(lldpdatatable_instance,
                7,
                kCipAny,
                EncodeCipCIPIdentification,
                &g_lldpdatatable[instance_id - 1].cip_identification,
                kGetableSingle);
  InsertAttribute(lldpdatatable_instance,
                8,
                kCipAny,
                EncodeCipAdditionalEthernetCapabilities,
                &g_lldpdatatable[instance_id - 1].additional_ethernet_capabilities,
                kGetableSingle);
  InsertAttribute(lldpdatatable_instance,
                9,
                kCipUdint,
                EncodeCipUdint,
                &g_lldpdatatable[instance_id - 1].last_change,
                kGetableSingle);

  return lldpdatatable_instance;
}

