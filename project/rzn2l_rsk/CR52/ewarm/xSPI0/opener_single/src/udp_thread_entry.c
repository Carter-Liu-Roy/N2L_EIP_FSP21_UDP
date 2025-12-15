#include "udp_thread.h"
#include "udp.h"
#include "sockets.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/timeouts.h"
#include "lwip/init.h"
#include "lwip/ip4.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include <lwip/sockets.h>

#include "lwip/api.h"

#define CLIENT_UDP_PORT     61000


/* UDP Thread entry function */
/* pvParameters contains TaskHandle_t */
void udp_thread_entry(void * pvParameters)
{
  FSP_PARAMETER_NOT_USED(pvParameters);
  uint32_t err;
  uint32_t len;
  struct netbuf *buf;
  void *data;
  ip4_addr_t client_ipaddr;
  uint16_t client_port;
  uint8_t recvbuf[64]= {0};
  uint8_t sendbuf[64]= {0};
  uint32_t cnt = 0;
  void *UDP_Search_RX_Buffer;
  static struct netconn *udp_server_conn;
  
  vTaskDelay(2000);
  udp_server_conn = netconn_new(NETCONN_UDP);
  if (udp_server_conn != NULL)
  {
    err = netconn_bind(udp_server_conn, IP_ADDR_ANY, CLIENT_UDP_PORT);
    if (err == ERR_OK)
    {
      while (1)
      {
        err = netconn_recv(udp_server_conn, &buf);
        if (err == ERR_OK)
        {
          netbuf_data(buf, &data, &len);
          memcpy(recvbuf, data, len);
          client_port = buf->port;
          client_ipaddr = buf->addr;
          netbuf_delete(buf);
        }
        else
        {
          // APP_PRINT("netconn_recv error : %d \n", err);
        }
        
        buf = netbuf_new();   // Create a new netbuf
        netbuf_ref(buf, recvbuf, strlen(recvbuf));  // refer the netbuf to the data to be sent
        err = netconn_sendto(udp_server_conn, buf, &client_ipaddr,client_port);
        
        netbuf_delete(buf);  // delete the netbuf
        
        vTaskDelay(200);
      }
    }
    else
    {
      
    }
  }
  else
  {
   
  }
  
}
