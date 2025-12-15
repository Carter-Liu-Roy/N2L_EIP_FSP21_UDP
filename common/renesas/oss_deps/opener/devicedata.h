/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

#ifndef DEVICE_DATA_H_
    #define DEVICE_DATA_H_

/**
 * These macros are used to initialize the identity object attributes defined as global variables in cip/cipidentity.c(.h)
 * If these macro values are changed, the corresponding fields in STC file and EDS file should also be changed.
 */
/** Modified for porting. */
#define OPENER_DEVICE_VENDOR_ID             (1105)     ///< Renesas Electronics (1105)
#if defined(OPENER_CIP_SAFETY)
#define OPENER_DEVICE_TYPE                  (42)       ///< Safety Analog I/O Device (42)
#define OPENER_DEVICE_NAME                  "RZ Adaptor Safety"
#else
#define OPENER_DEVICE_TYPE                  (12)       ///< Communications Adapter Device (12)
#define OPENER_DEVICE_NAME                  "Renesas OpENer Device"
#endif

/** Remain the values of OpENer sample application. */
#if defined(OPENER_CIP_SAFETY)
#define OPENER_DEVICE_PRODUCT_CODE          (21258)
#define OPENER_DEVICE_MAJOR_REVISION        (3)
#define OPENER_DEVICE_MINOR_REVISION        (7)
#else
#define OPENER_DEVICE_PRODUCT_CODE          (65001)
#define OPENER_DEVICE_MAJOR_REVISION        (2)
#define OPENER_DEVICE_MINOR_REVISION        (3)
#endif


#endif /* DEVICE_DATA_H_ */
