/**************************************************************************//**
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "wifi_cli_lwip.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/apps/httpd.h"
#include "lwip/netifapi.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "sl_wfx_task.h"
#include "sl_wfx_host.h"

/*******************************************************************************
 ******************   Wi-Fi CLI lwIP App Task Configuration   *****************
 ******************************************************************************/
/* LWIP task priority */
#define WFX_CLI_LWIP_TASK_PRIO      32u
/* LWIP task stack size */
#define WFX_CLI_LWIP_TASK_STK_SIZE  800u

/* LWIP task stack */
static CPU_STK  wfx_cli_lwip_task_stk[WFX_CLI_LWIP_TASK_STK_SIZE];
/* LWIP task tcb */
static OS_TCB   wfx_cli_lwip_task_tcb;

/************************ Private variables ***********************************/
static void *iperf_server_session = NULL;
static void *iperf_client_session = NULL;
static bool iperf_client_is_foreground_mode = false;

static uint32_t last_client_bytes_transferred = 0;
static uint32_t last_client_ms_duration = 0;
static uint32_t last_client_bandwidth_kbitpsec = 0;

static uint16_t ping_nb_packet_received = 0;
static uint16_t ping_nb_packet_sent = 0;
static uint32_t ping_echo_total_time = 0;
static uint32_t ping_echo_min_time = 0xFFFFFFFF;
static uint32_t ping_echo_max_time = 0;
static uint16_t ping_seq_num = 0;
static uint32_t ping_time = 0;

/**************************************************************************//**
 * @brief Static TCP function prototypes
 *****************************************************************************/
static err_t start_tcp_server_impl(const ip_addr_t *local_addr, 
                                  u16_t local_port,
                                  tcp_state_t *state);
static err_t tcp_srv_accepted_cb(void *arg, 
                                struct tcp_pcb *newpcb, 
                                err_t err);
static void tcp_send_tmr_cb (void  *p_tmr, void  *p_arg);

static void tcp_srv_timer_cb(void *timer, void *data);
static err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);
static RTOS_ERR create_tcp_timer(tcp_state_t *state);

/**************************************************************************//**
 * @brief start TCP server
 *****************************************************************************/
err_t start_tcp_server(const ip_addr_t *local_addr,
                       u16_t local_port,
                       tcp_state_t *state)
{
  if (local_addr == NULL || state == NULL) {
    return ERR_ARG;
  }
  return start_tcp_server_impl(local_addr, local_port, state);
}

/**************************************************************************//**
 * @brief Implementation of TCP server
 *****************************************************************************/
static err_t start_tcp_server_impl(const ip_addr_t *local_addr, 
                                  u16_t local_port,
                                  tcp_state_t *state)
{
  err_t err;
  struct tcp_pcb *pcb;

  /* Allocate a new TCP pcb */
  pcb = tcp_new_ip_type(IP_GET_TYPE(*local_addr));
  if (pcb == NULL) {
    return ERR_MEM;
  }

  /* Binding to local address & port */
  err = tcp_bind(pcb, local_addr, local_port);
  if (err != ERR_OK) {
    printf("Bind error %d\r\n", err);
    tcp_close(pcb);
    return err;
  }

  /* Listening  with incomming queue limit */
  state->server_pcb = tcp_listen_with_backlog(pcb, 1); // pcb will be freed
  if (state->server_pcb == NULL) {
    if (pcb != NULL) {
      tcp_close(pcb);
    }
    return ERR_MEM;
  }
  pcb = NULL;

  /* Setup callback function */
  tcp_arg(state->server_pcb, state);
  tcp_accept(state->server_pcb, tcp_srv_accepted_cb);
  printf("Successfully start TCP server\r\n");
  return ERR_OK;
}

/**************************************************************************//**
 * @brief Timer callback of TCP server timer for periodically sending
 *****************************************************************************/
void tcp_srv_timer_cb(void *timer, void *data) 
{
  PP_UNUSED_PARAM(timer);
  tcp_state_t *s;
  s = (tcp_state_t *)data;
  err_t wr_err = ERR_MEM;
  
  while ((wr_err == ERR_MEM) &&
        (s->p_send_buf != NULL) &&
        s->state != SRV_CLOSING)
  {
    wr_err = tcp_write(s->conn_pcb, s->p_send_buf,
                      s->msg_size, TCP_WRITE_FLAG_COPY);

    if (wr_err == ERR_MEM) {
      /* we are low on memory, try later / harder, defer to poll */
      printf("ERR_MEM\r\n");
    } else {
      wr_err = tcp_output(s->conn_pcb);
      if (wr_err != ERR_OK) {
          printf("tcp_output error\r\n");
      } else {
        s->time_started = sys_now();
      }
    }
  }
}

/**************************************************************************//**
 * @brief Called after accepting a connection from the client
 *****************************************************************************/
err_t tcp_srv_accepted_cb(void *arg, struct tcp_pcb *newpcb, err_t err) 
{
  tcp_state_t *s;

  if ((err != ERR_OK) || (newpcb == NULL) || (arg == NULL)) {
    return ERR_VAL;
  }

  printf("Client %s:%d connected!\r\n", 
          ip4addr_ntoa(&newpcb->remote_ip), newpcb->remote_port);

  s = (tcp_state_t *)arg;

  s->state = SRV_ACCEPTED;
  s->conn_pcb = newpcb;
  s->p_recv_buf = NULL;

  tcp_arg(newpcb, s);
  tcp_sent(newpcb, tcp_sent_cb);

  // Start timer
  s->tcp_tmr_cb = tcp_srv_timer_cb;
  create_tcp_timer(s);
  return ERR_OK;
}

/**************************************************************************//**
 * @brief allocate the sending message string based on the msg_size
 *****************************************************************************/
void* allocate_sending_msg(u16_t msg_sz) 
{
  char *msg_buf, *p_char;

  msg_buf = (char *)malloc(msg_sz + 3);
  if (msg_buf == NULL) {
      printf("Failed to allocate memory for sending message\r\n");
  } else {
      p_char = msg_buf;
      for (int i = 0; i < msg_sz; i++) {
          *p_char++ = 'H';
      }
      *p_char++ = '\r';
      *p_char++ = '\n';
      *p_char   = '\0';     // NULL string
  }
  return msg_buf;
}

/**************************************************************************//**
 * @brief Called after ack from remote host, compute the sending time
 *****************************************************************************/
err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  LWIP_UNUSED_ARG(tpcb);

  tcp_state_t *s = (tcp_state_t *)arg;
  u32_t diff_ms = sys_now() - s->time_started;
  printf("\r\nTime elapsed : %"PRIu32" ms\r\n", diff_ms);
  printf("\r\nAcknowledge length = %u \r\n", len);
  return ERR_OK;
}

/**************************************************************************//**
 * @brief Called after connected to the remot server. Start timer for sending
 *****************************************************************************/
err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) 
{
  LWIP_UNUSED_ARG(tpcb);
  LWIP_UNUSED_ARG(err);
  tcp_state_t *s;
  RTOS_ERR tmr_err;

  s = (tcp_state_t *)arg;
  printf("\r\nSuccessfully connected! \r\n");

  // start timer
  tmr_err = create_tcp_timer(s);
  if (tmr_err.Code != RTOS_ERR_NONE ) {
      printf("Failed to start TCP client sending timer\r\n");
      return ERR_USE;
  }
  return ERR_OK;
}

/**************************************************************************//**
 * @brief Close all TCP connection & listenning
 *****************************************************************************/
void close_tcp(tcp_state_t **state) 
{
  err_t err;
  tcp_state_t *s;

  s = *state;
  if (s == NULL) {
      printf("Already close TCP\r\n");
      return;
  }

  if (s->conn_pcb != NULL) {
    tcp_arg(s->conn_pcb, NULL);
    tcp_sent(s->conn_pcb, NULL);

    err = tcp_close(s->conn_pcb);
    if (err != ERR_OK) {
      /* don't want to wait for free memory here... */
      tcp_abort(s->conn_pcb);
    }
    s->conn_pcb = NULL;
    printf("Connection stopped!\r\n");
  }

  if (s->server_pcb != NULL) {
    tcp_arg(s->server_pcb, NULL);
    tcp_accept(s->server_pcb, NULL);

    err = tcp_close(s->server_pcb);
    if (err != ERR_OK) {
      /* don't want to wait for free memory here... */
      tcp_abort(s->server_pcb);
    }
    printf("\r\n TCP server stopped!\r\n");
  }

  s->interval = 0;
  s->msg_size = 0;

  if (s->p_send_buf != NULL) {
      free(s->p_send_buf);
      s->p_send_buf = NULL;
  }

  s->remote_port = 0;
  s->time_started = 0;
  s->state = SRV_NONE;

  /* Free tcp_state_t */
  mem_free(s);
  *state = NULL;
}

/**************************************************************************//**
 * @brief Implement TCP client to send message
 *****************************************************************************/
err_t tcp_client_send_msg(tcp_state_t *state) 
{
  err_t err;
  struct tcp_pcb *tpcb;

  tpcb = tcp_new_ip_type(IP_GET_TYPE(state->remote_addr));
  tcp_nagle_disable(tpcb);
  tpcb->flags |= TF_ACK_NOW;
  tpcb->flags |= TF_NODELAY;

  state->conn_pcb = tpcb;
  state->tcp_tmr_cb = tcp_send_tmr_cb; // Set timer cb  

  tcp_arg(tpcb, (void *)state);
  tcp_sent(tpcb, tcp_sent_cb);

  err = tcp_connect(tpcb, &state->remote_addr,
                    state->remote_port, tcp_client_connected);

  if (err != ERR_OK) {
      printf("Failed to connect to remote server!\r\n");
  }
  return err;
}

/**************************************************************************//**
 * @brief Timer callback for sending messages
 *****************************************************************************/
void tcp_send_tmr_cb (void  *p_tmr, void  *p_arg) {
  
  PP_UNUSED_PARAM(p_tmr);  
  err_t err;
  tcp_state_t *s = (tcp_state_t *)p_arg;

  s->time_started = sys_now();

  /// TODO: Improve the way to write with while loop
  err = tcp_write(s->conn_pcb,
                  (const void*) s->p_send_buf,
                  s->msg_size,
                  TCP_WRITE_FLAG_COPY);

  if (err == ERR_OK) {
      err = tcp_output(s->conn_pcb);
      if (err != ERR_OK) {
          printf("tcp_output error\r\n");          
      }
      s->time_started = sys_now();
  } else {
      printf("write_err = %d\r\n", err);
  }
}

/**************************************************************************//**
 * @brief Create & start a timer
 *****************************************************************************/
RTOS_ERR create_tcp_timer(tcp_state_t *state) 
{
  OS_TICK tmr_tick_interval;    //number of ticks per interval
                                //for the OSTimeTickRateHzGet
  OS_RATE_HZ tick_rate;         //OS tick rate
  RTOS_ERR err;

  tick_rate = OSTimeTickRateHzGet(&err);
  RTOS_ERR_CHECK(err, "Failed to got tick_rate");

  tmr_tick_interval = (state->interval * tick_rate) / (OSTmrUpdateCnt * 1000);

  OSTmrCreate (&state->tcp_tmr,
               "TCP timer",
               0,                     //initial delay
               tmr_tick_interval,     //
               OS_OPT_TMR_PERIODIC,   //OS_OPT
               state->tcp_tmr_cb,     //OS_TMR_CALLBACK_PTR
               (void *)state,         //callback arg
               &err);
  RTOS_ERR_CHECK(err, "Failed to create timer");

  OSTmrStart(&state->tcp_tmr, &err);
  RTOS_ERR_CHECK(err, "Failed to start timer");
  return err;
}

/**************************************************************************//**
 * Set station link status to up.
 *****************************************************************************/
sl_status_t set_sta_link_up(void)
{
  netifapi_netif_set_up(&sta_netif);
  netifapi_netif_set_link_up(&sta_netif);
  if (use_dhcp_client) {
    dhcpclient_set_link_state(1);
  }
  return SL_STATUS_OK;
}
/**************************************************************************//**
 * Set station link status to down.
 *****************************************************************************/
sl_status_t set_sta_link_down(void)
{
  if (use_dhcp_client) {
    dhcpclient_set_link_state(0);
  }
  netifapi_netif_set_link_down(&sta_netif);
  netifapi_netif_set_down(&sta_netif);
  return SL_STATUS_OK;
}
/**************************************************************************//**
 * Set AP link status to up.
 *****************************************************************************/
sl_status_t set_ap_link_up(void)
{
  netifapi_netif_set_up(&ap_netif);
  netifapi_netif_set_link_up(&ap_netif);
  if (use_dhcp_server) {
      dhcpserver_start();
  }
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Set AP link status to down.
 *****************************************************************************/
sl_status_t set_ap_link_down(void)
{
  if (use_dhcp_server) {
      dhcpserver_stop();
  }
  netifapi_netif_set_link_down(&ap_netif);
  netifapi_netif_set_down(&ap_netif);
  return SL_STATUS_OK;
}

#if LWIP_RAW  /*!< LWIP_RAW is configured in lwipopts.h */
/***************************************************************************//**
 * @brief
 *    This callback function handles received messages
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static u8_t ping_recv_fnp(void *arg,
                         struct raw_pcb *pcb,
                         struct pbuf *p,
                         const ip_addr_t *addr)
{
  (void) arg;
  (void) pcb;

  LWIP_ASSERT("p != NULL", p != NULL);

  struct icmp_echo_hdr *iecho_hdr_ptr = NULL;

  /** Filter out the received messages by length, only receive ICMP messages.
   * ICMP message size = sizeof(struct icmp_echo_hdr) + data_size.
   * Thus, tot_len >= PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr)
   * Additionally, remove/peel the IP header.
   * */
  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr)))
      && (pbuf_remove_header(p, PBUF_IP_HLEN) == 0)) {

      iecho_hdr_ptr = (struct icmp_echo_hdr *) p->payload; /*!< ICMP header */

      if ((iecho_hdr_ptr->id == PING_ID)
          && (iecho_hdr_ptr->seqno == lwip_htons(ping_seq_num))) {

          /* Update statistic variables */
          ping_nb_packet_received++;

          uint32_t duration = sys_now() - ping_time;
          ping_echo_total_time += duration;

          if (duration < ping_echo_min_time) {
              ping_echo_min_time = duration;
          }

          if (duration > ping_echo_max_time) {
              ping_echo_max_time = duration;
          }

          printf("Reply from %s: bytes=%d, time=%lums\r\n", ipaddr_ntoa(addr),
                                                            p->len,
                                                            duration);
          pbuf_free(p);

          /* Eat the packet (received ICMP) */
          return 1; /*!< Be careful with '1'. Because it would be deleted */
      }

      /* Packet not eaten, restore the original one */
      pbuf_add_header(p, PBUF_IP_HLEN);
  }

  /* Packet has not eaten */
  return 0;
}


/***************************************************************************//**
 * @brief
 *    This function sends ping ICMP request message
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static int ping_send(struct raw_pcb *ping_raw_pcb,
                     const ip_addr_t *dest_ip_addr,
                     uint16_t data_size)
{

  struct pbuf *pbf = NULL;  /*!< packet buffer pointer (layer 3) */
  struct icmp_echo_hdr *iecho;
  size_t ping_msg_size, i;
  int res = -1;

  /* ping message size */
  ping_msg_size = sizeof(struct icmp_echo_hdr) + data_size;

  /* Allocate a packet buffer memory with ping msg size and IPv4 layer header size */
  pbf = pbuf_alloc(PBUF_IP, ping_msg_size, PBUF_RAM);
  if (pbf != NULL) {
      iecho = (struct icmp_echo_hdr *) pbf->payload;

      ICMPH_TYPE_SET(iecho, ICMP_ECHO);
      ICMPH_CODE_SET(iecho, 0);
      iecho->chksum = 0;
      iecho->id = PING_ID;     /*!< MSB = LSB (0xAFAF) - no need to convert */
      iecho->seqno = lwip_htons(++ping_seq_num);

      /* Fill the additional data buffer with some data */
      for (i = 0; i < data_size; i++) {
          ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
      }

      iecho->chksum = inet_chksum(iecho, ping_msg_size);

      res = raw_sendto(ping_raw_pcb, pbf, dest_ip_addr);
      if (res == 0) {
          ping_time = sys_now();
          ping_nb_packet_sent++;
      }
      pbuf_free(pbf);
  }
  return res;
}

/**************************************************************************//**
 * @brief: Implementation of the ping command.
 *
 * @param[in]
 *         + req_num: Number of request messages (ICMP)
 *         + ip_str: Remote IP's address
 *
 * @param[out]  None
 *
 * @return
 *        SL_STATUS_OK if success
 *        SL_STATUS_FAIL if error
 *****************************************************************************/
sl_status_t ping_cmd(uint32_t req_num, char *ip_str)
{

  int res = -1;

  struct raw_pcb *ping_pcb = NULL; /*!< protocol control block */
  ip_addr_t ping_addr;

  /* Parsing IP address */
  res = ipaddr_aton(ip_str, &ping_addr);
  if (res == 0) {
      printf("Failed to convert string (%s) to IP\r\n", ip_str);
      return SL_STATUS_FAIL;
  }

  /* Allocate a new resource raw_pcb */
  ping_pcb = raw_new(IP_PROTO_ICMP);
  if (ping_pcb == NULL) {
      LOG_DEBUG("Failed to allocate resource for ping_pcb\r\n");
      return SL_STATUS_ALLOCATION_FAILED;
  }

  /* Configure the IP stack */
  raw_recv(ping_pcb, ping_recv_fnp, NULL);
  raw_bind(ping_pcb, IP_ADDR_ANY);

  printf("Pinging %s with %d bytes of data:\r\n",
           ip_str, PING_DEFAULT_DATA_SIZE);

  /* Reset internal variables */
  ping_seq_num = 0;
  ping_nb_packet_sent = 0;
  ping_nb_packet_received = 0;
  ping_echo_total_time = 0;
  ping_echo_min_time = 0xFFFFFFFF;
  ping_echo_max_time = 0;

  while(req_num-- > 0) {
      /* Send ping request to IP destination */
      res = ping_send(ping_pcb, &ping_addr, PING_DEFAULT_DATA_SIZE);
      if (res != 0) {
          printf("Send ping request to IP (%s) failed\r\n", ip_str);
          raw_remove(ping_pcb); /*!< release ping_pcb */
          return SL_STATUS_FAIL;
      }
      /* Wait for receiving ICMP responses from the destination IP */
      sys_msleep(PING_DEFAULT_INTERVAL_SEC * 1000);
  }

  if (res == 0) {
    /* Display statistics */
    printf("\r\nPing statistics for %s:\r\n", ip_str);
    printf("\tPackets: Sent = %u, Received = %u, Lost = %u (%u%% loss),\r\n",
           ping_nb_packet_sent,
           ping_nb_packet_received,
           ping_nb_packet_sent-ping_nb_packet_received,
           ((ping_nb_packet_sent-ping_nb_packet_received)*100)/ping_nb_packet_sent);
    printf("Approximate round trip times in milli-seconds:\r\n");
    printf("\tMinimum = %lums, Maximum = %lums, Average = %lums\r\n",
           ping_echo_min_time,
           ping_echo_max_time,
           ping_echo_total_time/ping_nb_packet_received);
  }
  raw_remove(ping_pcb); /*!< release ping_pcb */
  return SL_STATUS_OK;
}
#endif /* LWIP_RAW */

/***************************************************************************//**
 * @brief
 *    This common function invokes the registered set function
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static void lwip_iperf_results (void *arg,
                                enum lwiperf_report_type report_type,
                                const ip_addr_t* local_addr,
                                uint16_t local_port,
                                const ip_addr_t* remote_addr,
                                uint16_t remote_port,
                                uint32_t bytes_transferred,
                                uint32_t ms_duration,
                                uint32_t bandwidth_kbitpsec)
{
  (void)report_type;
  (void)local_addr;
  (void)local_port;
  (void)remote_addr;
  (void)remote_port;

  int mode = (int) arg;

  if (mode == IPERF_CLIENT_MODE) {
    last_client_bytes_transferred = bytes_transferred;
    last_client_ms_duration = ms_duration;
    last_client_bandwidth_kbitpsec = bandwidth_kbitpsec;

    printf("\r\nIperf Client Report:\r\n" );
    printf("Interval %d.%ds\r\n",
           (int)(ms_duration/1000),
           (int)(ms_duration%1000));
    printf("Bytes transferred %d.%dM\r\n",
           (int)(bytes_transferred/1024/1024),
           (int)((((bytes_transferred/1024)*1000)/1024)%1000));
    printf("Bandwidth %d.%d Mbps\r\n\r\n",
           (int)(bandwidth_kbitpsec/1024),
           (int)(((bandwidth_kbitpsec*1000)/1024)%1000));

    if (iperf_client_is_foreground_mode) {
      /* Give back the hand to the shell */
      wifi_cli_resume(&g_cli_sem, 0); /*!< "sem_event_type" is 0, a general event */
    }
  } else {
    /* Server stopped, display the last client report */
    printf("\r\nIperf Last Client Report:\r\n" );
    printf("Interval %d.%ds\r\n",
           (int)(last_client_ms_duration/1000),
           (int)(last_client_ms_duration%1000));
    printf("Bytes transferred %d.%dM\r\n",
           (int)(last_client_bytes_transferred/1024/1024),
           (int)((((last_client_bytes_transferred/1024)*1000)/1024)%1000));
    printf("Bandwidth %d.%d Mbps\r\n\r\n",
           (int)(last_client_bandwidth_kbitpsec/1024),
           (int)(((last_client_bandwidth_kbitpsec*1000)/1024)%1000));
  }
}

/***************************************************************************//**
 * @brief
 *    Start iperf as server mode.
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void iperf_server(void)
{
  if (iperf_server_session != NULL) {
    /* An iPerf server is already running, kill it first */
    printf("A server is running, stop it first\r\n");
  } else {
    /* Reset session values */
    last_client_bytes_transferred = 0;
    last_client_ms_duration = 0;
    last_client_bandwidth_kbitpsec = 0;

    LOCK_TCPIP_CORE();
    iperf_server_session = lwiperf_start_tcp_server_default(lwip_iperf_results,
                                                            (void *)IPERF_SERVER_MODE);
    UNLOCK_TCPIP_CORE();

    if (iperf_server_session != NULL) {
      printf("iPerf TCP server started\r\n");
    } else {
      printf("iPerf TCP server error\r\n");
    }
  }
}

/**************************************************************************//**
 * @brief: Start iperf as client mode.
 *
 * @param[in]
 *         + ip_str: IP address string of remote iperf server
 *         + duration: duration in seconds
 *         + remote_port: Port of remote iperf server
 *         + is_foreground_mode: enable/disable foreground mode
 *
 * @param[out] None
 *
 * @return     None
 *****************************************************************************/
void iperf_client(char *ip_str,
                  uint32_t duration,
                  uint32_t remote_port,
                  bool is_foreground_mode)
{
  int res;
  ip_addr_t srv_addr;
  RTOS_ERR_CODE err_code;

  /* parse the remote server IP address */
  res = ipaddr_aton(ip_str, &srv_addr);
  if (res == 0) {
      /* Parsing error */
      printf("Failed to parse the remote server IP address\r\n");
      return;
  }

  iperf_client_is_foreground_mode = is_foreground_mode;

  LOCK_TCPIP_CORE();
  iperf_client_session = lwiperf_start_tcp_client(&srv_addr,
                                                 remote_port,
                                                 LWIPERF_CLIENT,
                                                 (uint32_t)duration,
                                                 lwip_iperf_results,
                                                 (void *)IPERF_CLIENT_MODE);
  UNLOCK_TCPIP_CORE();

  if (iperf_client_session != NULL) {

      printf("iPerf TCP client started on server %s\r\n", ip_str);

      if (iperf_client_is_foreground_mode == true) {
         /*  Wait at least 1 second until the test is done */
          err_code = wifi_cli_wait(&g_cli_sem,
                                   0,
                                   ((uint32_t)duration + 1) * 1000);

          if (err_code == RTOS_ERR_TIMEOUT) {
              LOG_DEBUG("Wait timeout!\r\n");
          }

         /* Reset to the default client mode */
         iperf_client_is_foreground_mode = false;
      }
  } else {
      printf("start iPerf TCP client error\r\n");
  }

}

/***************************************************************************//**
 * @brief
 *    Stop iperf server mode
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void stop_iperf_server(void)
{
  if (iperf_server_session != NULL) {
      printf("Stop server\r\n");

      LOCK_TCPIP_CORE();
      lwiperf_abort(iperf_server_session);
      UNLOCK_TCPIP_CORE();

      iperf_server_session = NULL;
    }
}

/***************************************************************************//**
 * @brief
 *    Stop iperf client mode
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void stop_iperf_client(void)
{
  if (iperf_client_session != NULL) {
      printf("Stop client\r\n");

      LOCK_TCPIP_CORE();
      lwiperf_abort(iperf_client_session);
      UNLOCK_TCPIP_CORE();

      iperf_client_session = NULL;
    }
}

/***************************************************************************//**
 * @brief
 *    This function initializes Wi-Fi network interfaces
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static void netif_config(void)
{
  ip_addr_t sta_ipaddr, ap_ipaddr;
  ip_addr_t sta_netmask, ap_netmask;
  ip_addr_t sta_gw, ap_gw;

  if (use_dhcp_client) {
    ip_addr_set_zero_ip4(&sta_ipaddr);
    ip_addr_set_zero_ip4(&sta_netmask);
    ip_addr_set_zero_ip4(&sta_gw);
  } else {
    IP_ADDR4(&sta_ipaddr, \
             sta_ip_addr0, \
             sta_ip_addr1, \
             sta_ip_addr2, \
             sta_ip_addr3);

    IP_ADDR4(&sta_netmask, \
             sta_netmask_addr0, \
             sta_netmask_addr1, \
             sta_netmask_addr2, \
             sta_netmask_addr3);

    IP_ADDR4(&sta_gw, \
             sta_gw_addr0, \
             sta_gw_addr1, \
             sta_gw_addr2, \
             sta_gw_addr3);
  }

  /* Initialize the SoftAP information */
  IP_ADDR4(&ap_ipaddr, \
           ap_ip_addr0, \
           ap_ip_addr1, \
           ap_ip_addr2, \
           ap_ip_addr3);

  IP_ADDR4(&ap_netmask, \
           ap_netmask_addr0, \
           ap_netmask_addr1, \
           ap_netmask_addr2, \
           ap_netmask_addr3);

  IP_ADDR4(&ap_gw, \
           ap_gw_addr0, \
           ap_gw_addr1, \
           ap_gw_addr2, \
           ap_gw_addr3);

  /* Add Station interfaces */
  netif_add(&sta_netif,
            &sta_ipaddr,
            &sta_netmask,
            &sta_gw,
            NULL,
            &sta_ethernetif_init,
            &tcpip_input);

  /* Add SoftAP interfaces */
  netif_add(&ap_netif,
            &ap_ipaddr,
            &ap_netmask,
            &ap_gw,
            NULL,
            &ap_ethernetif_init,
            &tcpip_input);

  /* Register the default network interface. */
  netif_set_default(&sta_netif);
}

/***************************************************************************//**
 * @brief
 *    This task create lwIP TCP/IP stack; configure default network interface
 *    & start DHCP client if used
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void lwip_init_start_task(void *p_arg)
{
  (void)p_arg;
  RTOS_ERR err;

  /* Create tcip_ip stack thread */
  tcpip_init(NULL, NULL);

  /* Initialize Wifi network interfaces */
  netif_config();

  /* Start DHCP Client*/
  if (use_dhcp_client) {
      dhcpclient_start();
  }

  /* Delete Lwip init task */
  OSTaskDel(NULL, &err);
}

/***************************************************************************//**
 * @brief
 *    Start lwip tcp/ip stack & related tasks
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void lwip_init_start(void)
{
  RTOS_ERR err;

  /* Create lwip initialization task */
  OSTaskCreate(&wfx_cli_lwip_task_tcb,
               "lwip start task",
               lwip_init_start_task,
               DEF_NULL,
               WFX_CLI_LWIP_TASK_PRIO,
               &wfx_cli_lwip_task_stk[0],
               (WFX_CLI_LWIP_TASK_STK_SIZE/10u),
               WFX_CLI_LWIP_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);

  /* Check err code */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}

