/*
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
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 *
 */

#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_mib2.h"
#include "lwip/apps/snmpv3.h"
#include "lwip/apps/snmp_snmpv2_framework.h"
#include "lwip/apps/snmp_snmpv2_usm.h"
#if LWIP_LLDP
#include "lwip_lldp_def.h"
#include "examples/snmp/snmp_private_mib/lldp-mib.h"
#if LWIP_LLDP_SUPPORT_CIP_TLV
#include "examples/snmp/snmp_private_mib/lldp-ext-odva-v01.h"
#endif
#endif
#include "snmp_example.h"

#if LWIP_SNMP
static const struct snmp_mib *mibs[] = {
  &mib2,
#if LWIP_LLDP
  &lldpmib,
#if LWIP_LLDP_SUPPORT_CIP_TLV
  &lldpxodvamib,
#endif
#endif
};
#endif /* LWIP_SNMP */

permanentSysData_t permanentSystemData = {
  SNMP_LWIP_MIB2_SYSDESC,
  SNMP_LWIP_MIB2_SYSCONTACT,
  SNMP_LWIP_MIB2_SYSNAME,
  SNMP_LWIP_MIB2_SYSLOCATION,
};

void
snmp_example_init(void)
{
#if LWIP_SNMP
#if SNMP_LWIP_MIB2
#if SNMP_USE_NETCONN
  snmp_threadsync_init(&snmp_mib2_lwip_locks, snmp_mib2_lwip_synchronizer);
#endif /* SNMP_USE_NETCONN */
  snmp_mib2_set_sysdescr((const u8_t*)permanentSystemData.snmp_mib_sys_descr, NULL);
  snmp_mib2_set_syscontact((u8_t *)permanentSystemData.snmp_mib_sys_contact, NULL, CFG_SNMP_MIB_SYS_CONTACT_LEN);
  snmp_mib2_set_sysname((u8_t *)permanentSystemData.snmp_mib_sys_name, NULL, CFG_SNMP_MIB_SYS_NAME_LEN);
  snmp_mib2_set_syslocation((u8_t *)permanentSystemData.snmp_mib_sys_location, NULL, CFG_SNMP_MIB_SYS_LOCATION_LEN);
#endif /* SNMP_LWIP_MIB2 */

#if LWIP_SNMP_V3
  snmpv3_dummy_init();
#endif

  snmp_set_mibs(mibs, LWIP_ARRAYSIZE(mibs));
  snmp_init();
#endif /* LWIP_SNMP */
}
