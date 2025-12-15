/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

/** @file
 * @brief Declare public interface of the Safety Supervisor Object
 *
 */

#ifndef OPENER_CIPSAFETYSUPERVISOR_H_
#define OPENER_CIPSAFETYSUPERVISOR_H_

#if defined(OPENER_CIP_SAFETY)
#include "typedefs.h"
#include "ciptypes.h"

/**
 * @brief Set this define to determine the number of Password Bytes
 */
#define OPENER_SAFETYSUPERVISOR_NUM_PASSWORD        (16)

/**
 * @brief Set this define to determine the maximum number of Detail Bytes
 */
#define OPENER_SAFETYSUPERVISOR_MAX_DETAIL        (2)

/**
 * @brief Set this define to determine the number of Common Detail Bytes
 */
#define OPENER_SAFETYSUPERVISOR_NUM_COMMON_DETAIL        (2)

/**
 * @brief Set this define to determine the maximum number of Output Owners
 */
#define OPENER_SAFETYSUPERVISOR_MAX_OUTPUT_OWNER        (1)

/**
 * @brief Set this define to determine the instance number for Safety Configuration Assembly Instance
 */
#define OPENER_SAFETYSUPERVISOR_ASSEMBLY_CONFIG        (197)

/**
 * @brief Set this define to determine the number of bytes for Safety Configuration Assembly Instance
 */
#define OPENER_SAFETYSUPERVISOR_ASSEMBLY_CONFIG_BYTE        (64)

/* Attribute Bit Map Assignments */
#define OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_0        (1 << 0) /* When set, preserve Soft-set MacId */
#define OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_1        (1 << 1) /* When set, preserve Soft-set Baud Rate */
#define OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_2        (1 << 2) /* When set, preserve the TUNID */
#define OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_3        (1 << 3) /* When set, preserve the Password */
#define OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_4        (1 << 4) /* When set, preserve the CFUNID */
#define OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_5        (1 << 5) /* When set, preserve the OCPUNID */
#define OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_6        (1 << 6) /* Reserved, always 0 */
#define OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_7        (1 << 7) /* Use Extended Map (To be defined later) */

/** @brief Definition of CIP service codes
 *
 * An Enum with all CIP service codes. Common services codes range from 0x01 to
 *****0x1C
 *
 */
typedef enum {
  /* Start CIP object-specific services */
  kConfigureRequest = 0x4F,
  kValidateConfiguration = 0x50,
  kSetPassword = 0x51,
  kConfigurationLockUnlock = 0x52,
  kSafetyReset = 0x54,
  kResetPassword = 0x55,
  kProposeTUNID = 0x56,
  kApplyTUNID = 0x57,
  /* End CIP object-specific services */
} CIPSafetySupervisorServiceCode;

/** @brief Safety Supervisor Object class code */
static const CipUint kCipSafetySupervisorClassCode = 0x39U;

/** @brief Constants for the Device Status member of the Safety Supervisor object. */
typedef enum {
  kDeviceStatusUndefined = 0U,
  kDeviceStatusSelfTesting = 1U,
  kDeviceStatusIdle = 2U,
  kDeviceStatusSelfTestException = 3U,
  kDeviceStatusExecuting = 4U,
  kDeviceStatusAbort = 5U,
  kDeviceStatusCriticalFault = 6U,
  kDeviceStatusConfiguring = 7U,
  kDeviceStatusWaitingforTUNID = 8U,
} CipSafetySupervisorDeviceState;

/* Declare constants for exception status (#12) */
/** Indicates when device common alarm is reported. */
static const CipByte kExceptionStatusALARMDevicecommon = 0x01U;
/** Indicates when device specific alarm is reported. */
static const CipByte kExceptionStatusALARMDevicespecific = 0x02U;
/** Indicates when manufacturer specific alarm is reported. */
static const CipByte kExceptionStatusALARMManufacturerspecific = 0x04U;
/** Indicates when device common warning is reported. */
static const CipByte kExceptionStatusWARNINGdevicecommon = 0x10U;
/** Indicates when device specific warning is reported. */
static const CipByte kExceptionStatusWARNINGdevicespecific = 0x20U;
/** Indicates when manufacturer specific warning is reported. */
static const CipByte kExceptionStatusWARNINGmanufacturerspecific = 0x40U;
/** Indicates exception status is expanded method.*/
static const CipByte kExceptionStatusExpandedMethod = 0x80U;

/* Declare constants for Common Exception Detail 0 (#13 & #14) */
/** Indicates when internal diagnostic exception is reported. */
static const CipByte kCommonExceptionDetail0Diagnostic = 0x01U;
/** Indicates when microprocessor exception is reported. */
static const CipByte kCommonExceptionDetail0Microprocessor = 0x02U;
/** Indicates when eprom exception is reported. */
static const CipByte kCommonExceptionDetail0CodeMemory = 0x04U;
/** Indicates when eeprom exception is reported. */
static const CipByte kCommonExceptionDetail0NonVolatileMemory = 0x08U;
/** Indicates when ram exception is reported. */
static const CipByte kCommonExceptionDetail0DataMemory = 0x10U;
/** Indicates when internal real-time exception is reported. */
static const CipByte kCommonExceptionDetail0RealtimeFault = 0x40U;

/* Declare constants for Common Exception Detail 1 (#13 & #14) */
/** Indicates when power supply overcurrent is reported. */
static const CipByte kCommonExceptionDetail1PSOverCurrent = 0x01U;
/** Indicates when power supply output voltage is reported. */
static const CipByte kCommonExceptionDetail1PSOutputVoltage = 0x04U;
/** Indicates when power supply input voltage is reported. */
static const CipByte kCommonExceptionDetail1PSInputVoltage = 0x08U;
/** Indicates when scheduled maintenance due is reported. */
static const CipByte kCommonExceptionDetail1ScheduledMaintDue = 0x10U;
/** Indicates when notify manufacturer is reported. */
static const CipByte kCommonExceptionDetail1NotifyVendor = 0x20U;
/** Indicates when reset exception is reported. */
static const CipByte kCommonExceptionDetail1ResetException = 0x40U;

/* Declare constants for Safety Network Segment Format */
/* Safety Network Segment: Target Format (0x00) */
static const CipByte kSafetyNetworkSegmentFormatTargetFormat = 0x00U;
/* Safety Network Segment: Router Format (0x01) */
static const CipByte kSafetyNetworkSegmentFormatRouterFormat = 0x01U;
/* Safety Network Segment: Extended Format (0x02) */
static const CipByte kSafetyNetworkSegmentFormatExtendedFormat = 0x02U;

/* Declare constants for Network Segment Data Length */
/* Safety Network Segment: Target Format (0x00) */
static const CipByte kSafetyNetworkSegmentLengthTargetFormat = 0x1BU; /* 27 words */
/* Safety Network Segment: Router Format (0x01) */
static const CipByte kSafetyNetworkSegmentLengthRouterFormat = 0x06U; /* 6 words */
/* Safety Network Segment: Extended Format (0x02) */
static const CipByte kSafetyNetworkSegmentLengthExtendedFormat = 0x1EU; /* 30 words */

/* ********************************************************************
 * Type declarations
 */
/** @brief Detail
 *
 * This is the Detail.
 */
typedef struct {
  CipUsint size; /**< Size */
  CipByte detail[OPENER_SAFETYSUPERVISOR_MAX_DETAIL]; /**< Detail */
} CipDetail;

/** @brief Exception Detail
 *
 * This is the Exception Detail.
 */
typedef struct {
  CipDetail common; /**< Common Exception Detail */
  CipDetail device; /**< Device Exception Detail */
  CipDetail manufacturer; /**< Manufacturer Exception Detail */
} CipExceptionDetail;

/** @brief Date and Time
 *
 * This is the Date and Time.
 */
typedef struct {
  CipUdint time_of_day; /**< Time */
  CipUint date; /**< Date */
} CipDateAndTime;

/** @brief Unique Network Identifier
 *
 * This is the Unique Network Identifier.
 */
typedef struct {
  CipDateAndTime snn; /**< Safety Network Number */
  CipUdint nodeid; /**< NodeID */
} CipUNID;

/** @brief Safety Configuration Identifier
 *
 * This is the Safety Configuration Identifier.
 */
typedef struct {
  CipDword sccrc; /**< Safety Configuration CRC (SCCRC) */
  CipDateAndTime scts; /**< Safety Configuration Time Stamp (SCTS) */
} CipSCID;

/** @brief Output Owners
 *
 * This is the Output Owners.
 */
typedef struct {
  CipUNID ocpunid; /**< OCPUNID */
  CipEpath app_resource; /**< Application Resource */
} CipOutputOwners;

/** @brief Output Connection Point Owners
 *
 * This is the Output Connection Point Owners.
 */
typedef struct {
  CipUint num; /**< Number of Array Entries */
  CipOutputOwners output_owners[OPENER_SAFETYSUPERVISOR_MAX_OUTPUT_OWNER]; /**< Output Owners */
} CipOutputConnectionOwners;

/** @brief Type declaration for the Safety Supervisor Object
 *
 * This is the type declaration for the Safety Supervisor Object.
 */
typedef struct {
  CipOctet password[OPENER_SAFETYSUPERVISOR_NUM_PASSWORD]; /* Password */
  CipUsint device_status; /**< Attribute #11: */
  CipByte exception_status; /**< Attribute #12: */
  CipExceptionDetail alarm; /**< Attribute #13: */
  CipExceptionDetail warning; /**< Attribute #14: */
  CipBool alarm_enable; /**< Attribute #15: */
  CipBool warning_enable; /**< Attribute #16: */
  CipBool config_lock; /**< Attribute #24: */
  CipUNID cfunid; /**< Attribute #25: */
  CipSCID scid; /**< Attribute #26: */
  CipUNID tunid; /**< Attribute #27: */
  CipOutputConnectionOwners output_connection_owners; /**< Attribute #28: */
  CipUNID proposed_tunid; /**< Attribute #29: */
  CipOctet config_data[OPENER_SAFETYSUPERVISOR_ASSEMBLY_CONFIG_BYTE]; /* Config Data */
} CipSafetySupervisorObject;

/** @brief Safety Network Segment: Extended Format (0x02)
 *
 * This is the Safety Network Segment: Extended Format (0x02).
 */
typedef struct {
  CipByte reserved; /* Reserved */
  CipUdint sccrc; /* Configuration CRC (SCCRC) */
  CipDateAndTime scts; /* Configuration TimeStamp (SCTS) */
  CipUdint time_correction_epi; /* Time Correction EPI */
  CipWord time_correction_network_connection_parameters; /* Time Correction Network Connection Parameters */
  CipUNID tunid; /* Target_UNID (TUNID) */
  CipUNID ounid; /* Originator UNID (OUNID) */
  CipUint ping_interval_epi_multiplier; /* Ping_Interval_EPI_Multiplier */
  CipUint time_coord_msg_min_multiplier; /* Time_Coord_Msg_Min_Multiplier */
  CipUint network_time_expectation_multiplier; /* Network_Time_Expectation_Multiplier */
  CipUsint timeout_multiplier; /* Timeout_Multiplier */
  CipUsint max_consumer_number; /* Max_Consumer_Number */
  CipUint max_fault_number; /* Max_Fault_Number */
  CipUdint cpcrc; /* Connection Parameters CRC (CPCRC) */
  CipUdint time_correction_connection_id; /* Time Correction Connection ID */
  CipUint initial_time_stamp; /* Initial Time Stamp */
  CipUint initial_rollover_value; /* Initial Rollover Value */
} CipSafetyNetworkSegmentExtendedFormat;

/** @brief Safety Network Segment
 *
 * This is the Safety Network Segment.
 */
typedef struct {
  CipBool config_data_in; /* Config Data in Safety Open */
  CipBool output_device; /* Output Device */
  CipDword consumed_instance_id; /* Consumed Instance ID */
  CipByte length; /* Network Segment Data Length */
  CipUsint format; /* Safety Network Segment Format */
  CipSafetyNetworkSegmentExtendedFormat extended; /* Safety Network Segment: Extended Format (0x02) */
} CipSafetyNetworkSegment;


/* ********************************************************************
 * global public variables
 */
extern CipSafetySupervisorObject g_safetysupervisor;  /**< declaration of Safety Supervisor Object instance 1 data */


/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the Safety Supervisor Object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipSafetySupervisorInit(void);

/** @brief Starting the Safety Supervisor Object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipSafetySupervisorStart(void);

/** @brief Set Status of the Safety Supervisor Object
 */
void CipSafetySupervisorSetStatus(CipUsint device_status, CipByte exception_status);

/** @brief Get Device Status of the Safety Supervisor Object
 *
 * @return Device Status of the Safety Supervisor Object
 */
CipUsint CipSafetySupervisorGetDeviceStatus(void);

/** @brief Get NET LED flash of the Safety Supervisor Object
 *
 * @return NET LED flash of the Safety Supervisor Object
 */
CipBool CipSafetySupervisorGetNETLEDflash(void);

/** @brief Set Safety Network Number the Safety Supervisor Object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipSafetySupervisorGetSafetyNetworkNumber(CipUsint *p_safety_net_num);

/** @brief Process Safety Network Segment
 *
 * @return Network Segment Data Length
 */
size_t CipSafetySupervisorProcessSafetyNetworkSegment(const EipUint8 *message, CipBool config_data_in, CipBool output_device, CipDword consumed_instance_id);

/** @brief Process Safety Open
 */
void CipSafetySupervisorProcessSafetyOpen(void);

/** @brief Compare Password of the Safety Supervisor Object
 *
 * @return 0 on success, otherwise -1
 */
int CipSafetySupervisorCmpPassword(CipUsint * buf);
#endif

#endif /* of OPENER_CIPSAFETYSUPERVISOR_H_ */
