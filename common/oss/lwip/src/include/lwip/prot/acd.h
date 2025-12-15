/**
 * @file
 * ACD protocol definitions
 */

/*
 *
 * Copyright (c) 2007 Dominik Spies <kontakt@dspies.de>
 * Copyright (c) 2018 Jasper Verschueren <jasper.verschueren@apart-audio.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Jasper Verschueren <jasper.verschueren@apart-audio.com>
 * Author: Dominik Spies <kontakt@dspies.de>
 */

#ifndef LWIP_HDR_PROT_ACD_H
#define LWIP_HDR_PROT_ACD_H

#ifdef __cplusplus
extern "C" {
#endif

/* RFC 5227 and RFC 3927 Constants */
#if LWIP_EIP_ACD
#define PROBE_WAIT           200      /* milliseconds (initial random delay)                                */
#define PROBE_MIN            200      /* milliseconds (minimum delay till repeated probe)                   */
#define PROBE_MAX            200      /* milliseconds (maximum delay till repeated probe)                   */
#define PROBE_NUM            4        /*              (number of probe packets)                             */
#define ANNOUNCE_NUM         2        /*              (number of announcement packets)                      */
#define ANNOUNCE_INTERVAL    2*1000   /* milliseconds (time between announcement packets)                   */
#define ANNOUNCE_WAIT        200      /* milliseconds (delay before announcing)                             */
#define DEFEND_ANC_NUM       2        /*              (number of announcement packets in DefendWithPolicyB) */
#define DEFEND_INTERVAL      2*1000   /* milliseconds (minimum interval between defensive ARPs)             */
#define ONGOING_PROBE_NUM    1        /*              (number of ongoing probe packets)                     */
#define ONGOING_PROBE_MIN    90*1000  /* milliseconds (minimum interval for ongoing probes)                 */
#define ONGOING_PROBE_MAX    150*1000 /* milliseconds (maximum interval for ongoing probes)                 */
#define SEMI_PROBE_NUM       2        /*              (number of probe packets in SemiActiveProbe)          */
#else /* LWIP_EIP_ACD */
#define PROBE_WAIT           1   /* second  (initial random delay)                    */
#define PROBE_MIN            1   /* second  (minimum delay till repeated probe)       */
#define PROBE_MAX            2   /* seconds (maximum delay till repeated probe)       */
#define PROBE_NUM            3   /*         (number of probe packets)                 */
#define ANNOUNCE_NUM         2   /*         (number of announcement packets)          */
#define ANNOUNCE_INTERVAL    2   /* seconds (time between announcement packets)       */
#define ANNOUNCE_WAIT        2   /* seconds (delay before announcing)                 */
#define MAX_CONFLICTS        10  /*         (max conflicts before rate limiting)      */
#define RATE_LIMIT_INTERVAL  60  /* seconds (delay between successive attempts)       */
#define DEFEND_INTERVAL      10  /* seconds (minimum interval between defensive ARPs) */
#endif /* LWIP_EIP_ACD */

/* ACD states */
typedef enum {
  /* ACD is module is off */
  ACD_STATE_OFF,
  /* Waiting before probing can be started */
  ACD_STATE_PROBE_WAIT,
  /* Probing the ipaddr */
  ACD_STATE_PROBING,
  /* Waiting before announcing the probed ipaddr */
  ACD_STATE_ANNOUNCE_WAIT,
  /* Announcing the new ipaddr */
  ACD_STATE_ANNOUNCING,
  /* Performing ongoing conflict detection with one defend within defend inferval */
  ACD_STATE_ONGOING,
#if LWIP_EIP_ACD
  /* Notification & FaultAction */
  ACD_STATE_FAULT,
   /* AcquireNewIpv4Parameters */
  ACD_STATE_ACQUIRE,
  /* DefendWithPolicyB */
  ACD_STATE_DEFEND,
  /* SemiActiveProbe */
  ACD_STATE_SEMI_ACT,
#else /* LWIP_EIP_ACD */
  /* Performing ongoing conflict detection but immediately back off and Release
   * the address when a conflict occurs. This state is used for LL addresses
   * that stay active even if the netif has a routable address selected.
   * In such a case, we cannot defend our address */
  ACD_STATE_PASSIVE_ONGOING,
  /* To many conflicts occured, we need to wait before restarting the selection
   * process */
  ACD_STATE_RATE_LIMIT
#endif /* LWIP_EIP_ACD */
} acd_state_enum_t;

typedef enum {
  ACD_IP_OK,            /* IP address is good, no conflicts found in checking state */
  ACD_RESTART_CLIENT,   /* Conflict found -> the client should try again */
  ACD_DECLINE           /* Decline the received IP address (rate limiting)*/
} acd_callback_enum_t;

#if LWIP_EIP_ACD
/* ADC Activity */
#define ACD_ACT_NO_CONF      0 /* NoConflictDetected */
#define ACD_ACT_PROBE        1 /* ProbeIpv4Address */
#define ACD_ACT_ONGOING      2 /* OngoingDetection */
#define ACD_ACT_SEMI_ACT     3 /* SemiActiveProbe */
#endif /* LWIP_EIP_ACD */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_PROT_ACD_H */
