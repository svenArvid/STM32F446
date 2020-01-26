/**
  ******************************************************************************
  * @file    LwIP/LwIP_TCP_Echo_Server/Src/main.c 
  * @author  MCD Application Team
  * @brief   This sample code implements a TCP Echo Server application based on 
  *          Raw API of LwIP stack. This application uses STM32F4xx the 
  *          ETH HAL API to transmit and receive data. 
  *          The communication is done with a web browser of a remote PC.
  */
/* Includes ------------------------------------------------------------------*/
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "app_ethernet.h"
#include "tcp_echoserver.h"
#include "Network.h"
#include "Uart.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct netif gnetif; /* network interface structure */

/* Private function prototypes -----------------------------------------------*/
static void Netif_Config(void);

/* Private functions ---------------------------------------------------------*/

void Network_Init(void)
{   
  /* Initialize the LwIP stack */
  lwip_init();
  
  /* Configure the Network interface */
  Netif_Config();
  
  /* tcp echo server Init */
  tcp_echoserver_init();
  
  /* Notify user about the netwoek interface config */
  User_notification(&gnetif);
}

void Network_1ms(void)
{
  /* Read a received packet from the Ethernet buffers and send it to the lwIP for handling */
  ethernetif_input(&gnetif);

  /* Handle timeouts */
  sys_check_timeouts();

  static uint32_t count = 0;
  count++;
  if (count % 100 == 0)
  {
    uint8_t i = 0;
    ip4_addr_t *ipaddr;
    struct netif *netif;
    struct eth_addr *eth_ret;
    uint8_t r = 0;
    UART_PRINTF("ARP table entries:\r\n");
    for (i = 0; i < 5; i++) {
      if (etharp_get_entry(i, &ipaddr, &netif, &eth_ret))
      {
        UART_PRINTF("MAC: %02X-%02X-%02X-%02X-%02X-%02X\r\n",
          eth_ret->addr[0], eth_ret->addr[1], eth_ret->addr[2], eth_ret->addr[3], eth_ret->addr[4], eth_ret->addr[5]);
        UART_PRINTF("IP: %d.%d.%d.%d\r\n", ip4_addr1(ipaddr), ip4_addr2(ipaddr), ip4_addr3(ipaddr), ip4_addr4(ipaddr));
      }

    }
    


  }

#ifdef USE_DHCP
  /* handle periodic timers for LwIP */
  DHCP_Periodic_Handle(&gnetif);
#endif 
}

/**
  * @brief  Configurates the network interface
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  
#ifdef USE_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
  IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
  IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif /* USE_DHCP */
  
  /* Add the network interface */    
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);
  
  /* Registers the default network interface */
  netif_set_default(&gnetif);
  
  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
