/* Mesh Internal Communication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"
#include "driver/uart.h"

/*******************************************************
 *                Macros
 *******************************************************/
//#define MESH_P2P_TOS_OFF

/*******************************************************
 *                Constants
 *******************************************************/
#define RX_SIZE          (1500)
#define TX_SIZE          (1460)

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;

/*******************************************************
 *                Variable Definitions
 *******************************************************/
static const char *MESH_TAG = "mesh_main";
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
static const mesh_addr_t broadcast_group_id = {
  .addr = { 0x01, 0x00, 0x5E, 0x66, 0x66, 0x66 }
};
static uint8_t tx_buf[TX_SIZE] = { 0, };
static uint8_t rx_buf[RX_SIZE] = { 0, };
static bool is_running = true;
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;
const int uart_num = UART_NUM_0;
mesh_addr_t id = {0,};
uint8_t mac[6];

static void uart_event_task(void *pvParameters)
{
	uart_event_t event;
  esp_err_t err;
	size_t buffered_size;
	uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
	int i;
	mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
	int route_table_size = 0;

  mesh_data_t data;
  data.data = dtmp;
  data.size = sizeof(tx_buf);
  data.proto = MESH_PROTO_BIN;

  int pos = 0;
  uint8_t size = 0;

	bzero(dtmp, RD_BUF_SIZE);

	for(;;) {
		//Waiting for UART event.
		if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
			switch(event.type) {
				//Event of UART receving data
				/*We'd better handler data event fast, there would be much more data events than
					other types of events. If we take too much time on data event, the queue might
					be full.*/
				case UART_DATA:
					size = uart_read_bytes(uart_num, dtmp + pos, event.size, portMAX_DELAY);
					//uart_write_bytes(uart_num, (char*)(dtmp + pos), event.size);
          pos += size;

          for(i = 0; i < pos; i++) {
            if(!(dtmp[i] == '\n' || dtmp[i] == '\r' || dtmp[i] == '\0')) continue;

            //printf("\nSending to %d clients: %s<\n", route_table_size, dtmp);

            esp_mesh_get_routing_table((mesh_addr_t *) &route_table,
                                       CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &route_table_size);

            //esp_mesh_send(NULL, &data, MESH_DATA_P2P, NULL, 0);
            esp_mesh_send(&broadcast_group_id, &data, MESH_DATA_P2P | MESH_DATA_GROUP, NULL, 0);
            /*
            for (i = 0; i < route_table_size; i++) {
              err = esp_mesh_send(&route_table[i], &data, MESH_DATA_P2P, NULL, 0);
              ESP_ERROR_CHECK(err);
            }
            */

            bzero(dtmp, RD_BUF_SIZE);
            pos = 0;
            xQueueReset(uart0_queue);
          }

					break;

				case UART_FIFO_OVF:
				case UART_BUFFER_FULL:
				case UART_BREAK:
				case UART_PARITY_ERR:
				case UART_FRAME_ERR:
				case UART_PATTERN_DET:
				default:
					break;
			}
		}
	}
	free(dtmp);
	dtmp = NULL;
	vTaskDelete(NULL);
};

/*******************************************************
 *                Function Definitions
 *******************************************************/
void esp_mesh_p2p_tx_main(void *arg)
{
    int i;
    esp_err_t err;
    int send_count = 0;
    mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
    int route_table_size = 0;
    mesh_data_t data;
    data.data = tx_buf;
    data.size = sizeof(tx_buf);
    data.proto = MESH_PROTO_BIN;
#ifdef MESH_P2P_TOS_OFF
    data.tos = MESH_TOS_DEF;
#endif /* MESH_P2P_TOS_OFF */

    is_running = true;
    while (is_running) {
        /* non-root do nothing but print */
        if (!esp_mesh_is_root()) {
          /*
            ESP_LOGI(MESH_TAG, "layer:%d, rtableSize:%d, %s", mesh_layer,
                     esp_mesh_get_routing_table_size(),
                     (is_mesh_connected && esp_mesh_is_root()) ? "ROOT" : is_mesh_connected ? "NODE" : "DISCONNECT");
                     */
            vTaskDelay(10 * 1000 / portTICK_RATE_MS);
            continue;
        }

        esp_mesh_get_routing_table((mesh_addr_t *) &route_table,
                                   CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &route_table_size);
        if (send_count && !(send_count % 100)) {
            //ESP_LOGI(MESH_TAG, "size:%d/%d,send_count:%d", route_table_size,
                     //esp_mesh_get_routing_table_size(), send_count);
        }

        /*
        for (i = 0; i < route_table_size; i++) {
            err = esp_mesh_send(&route_table[i], &data, MESH_DATA_P2P, NULL, 0);
            if (err) {
                ESP_LOGE(MESH_TAG,
                         "[ROOT-2-UNICAST:%d][L:%d]parent:"MACSTR" to "MACSTR", heap:%d[err:0x%x, proto:%d, tos:%d]",
                         send_count, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                         MAC2STR(route_table[i].addr), esp_get_free_heap_size(),
                         err, data.proto, data.tos);
            } else if (!(send_count % 100)) {
                //ESP_LOGW(MESH_TAG,
                         //"[ROOT-2-UNICAST:%d][L:%d][rtableSize:%d]parent:"MACSTR" to "MACSTR", heap:%d[err:0x%x, proto:%d, tos:%d]",
                         //send_count, mesh_layer,
                         //esp_mesh_get_routing_table_size(),
                         //MAC2STR(mesh_parent_addr.addr),
                         //MAC2STR(route_table[i].addr), esp_get_free_heap_size(),
                         //err, data.proto, data.tos);
            }
        }
        */
        /* if route_table_size is less than 10, add delay to avoid watchdog in this task. */
        if (route_table_size < 10) {
            vTaskDelay(1 * 1000 / portTICK_RATE_MS);
        }
    }
    vTaskDelete(NULL);
}

void esp_mesh_p2p_rx_main(void *arg)
{
    int recv_count = 0;
    esp_err_t err;
    mesh_addr_t from;
    int send_count = 0;
    mesh_data_t data;
    int flag = 0;
    data.data = rx_buf;
    data.size = RX_SIZE;

    is_running = true;
    printf("Starting RX\n");

    while (is_running) {
        data.size = RX_SIZE;
        err = esp_mesh_recv(&from, &data, 1000, &flag, NULL, 0);

        if(memcmp(from.addr, mac, 6) == 0) continue;

        if (err != ESP_OK || !data.size) {
            //ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }

        printf("%s\n", rx_buf);
    }

    printf("RX barfed\n");

    vTaskDelete(NULL);
}

esp_err_t esp_mesh_comm_p2p_start(void)
{
    static bool is_comm_p2p_started = false;
    if (!is_comm_p2p_started) {
        is_comm_p2p_started = true;
        xTaskCreate(esp_mesh_p2p_tx_main, "MPTX", 3072, NULL, 5, NULL);
        xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, NULL);
    }
    return ESP_OK;
}

void mesh_event_handler(mesh_event_t event)
{
    static uint8_t last_layer = 0;
    ESP_LOGD(MESH_TAG, "esp_event_handler:%d", event.id);

    switch (event.id) {
    case MESH_EVENT_STARTED:
        esp_mesh_get_id(&id);
        printf("<MESH_EVENT_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_STOPPED:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_CHILD_CONNECTED:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 //event.info.child_connected.aid,
                 //MAC2STR(event.info.child_connected.mac));
        break;
    case MESH_EVENT_CHILD_DISCONNECTED:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 //event.info.child_disconnected.aid,
                 //MAC2STR(event.info.child_disconnected.mac));
        break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
        printf("<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
        //ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 //event.info.routing_table.rt_size_change,
                 //event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_PARENT_CONNECTED:
        esp_mesh_get_id(&id);
        mesh_layer = event.info.connected.self_layer;
        memcpy(&mesh_parent_addr.addr, event.info.connected.connected.bssid, 6);
            printf("<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        last_layer = mesh_layer;
        is_mesh_connected = true;
        if (esp_mesh_is_root()) {
            tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        }

        ESP_ERROR_CHECK(esp_mesh_set_group_id(&broadcast_group_id, 1));

        esp_mesh_comm_p2p_start();
        break;
    case MESH_EVENT_PARENT_DISCONNECTED:
        //ESP_LOGI(MESH_TAG,
                 //"<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 //event.info.disconnected.reason);
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_LAYER_CHANGE:
        mesh_layer = event.info.layer_change.new_layer;
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 //last_layer, mesh_layer,
                 //esp_mesh_is_root() ? "<ROOT>" :
                 //(mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
        break;
    case MESH_EVENT_ROOT_ADDRESS:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 //MAC2STR(event.info.root_addr.addr));
        break;
    case MESH_EVENT_ROOT_GOT_IP:
        /* root starts to connect to server */
        //ESP_LOGI(MESH_TAG,
                 //"<MESH_EVENT_ROOT_GOT_IP>sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                 //IP2STR(&event.info.got_ip.ip_info.ip),
                 //IP2STR(&event.info.got_ip.ip_info.netmask),
                 //IP2STR(&event.info.got_ip.ip_info.gw));
        break;
    case MESH_EVENT_ROOT_LOST_IP:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_LOST_IP>");
        break;
    case MESH_EVENT_VOTE_STARTED:
        //ESP_LOGI(MESH_TAG,
                 //"<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 //event.info.vote_started.attempts,
                 //event.info.vote_started.reason,
                 //MAC2STR(event.info.vote_started.rc_addr.addr));
        break;
    case MESH_EVENT_VOTE_STOPPED:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    case MESH_EVENT_ROOT_SWITCH_REQ:
        //ESP_LOGI(MESH_TAG,
                 //"<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 //event.info.switch_req.reason,
                 //MAC2STR( event.info.switch_req.rc_addr.addr));
        break;
    case MESH_EVENT_ROOT_SWITCH_ACK:
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
        break;
    case MESH_EVENT_TODS_STATE:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d",
                 //event.info.toDS_state);
        break;
    case MESH_EVENT_ROOT_FIXED:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 //event.info.root_fixed.is_fixed ? "fixed" : "not fixed");
        break;
    case MESH_EVENT_ROOT_ASKED_YIELD:
        //ESP_LOGI(MESH_TAG,
                 //"<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 //MAC2STR(event.info.root_conflict.addr),
                 //event.info.root_conflict.rssi,
                 //event.info.root_conflict.capacity);
        break;
    case MESH_EVENT_CHANNEL_SWITCH:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>");
        break;
    case MESH_EVENT_SCAN_DONE:
        //ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 //event.info.scan_done.number);
        break;
    default:
        //ESP_LOGI(MESH_TAG, "unknown id:%d", event.id);
        break;
    }
}

int no_log(const char *tag, va_list args) {
  return 0;
}

void app_main(void)
{
    esp_log_set_vprintf(no_log);

    uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
    };

    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    uart_driver_install(uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);

    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);

    printf("\n\nSuperCon 2018\n");

    ESP_ERROR_CHECK(nvs_flash_init());
    /*  tcpip initialization */
    tcpip_adapter_init();
    /* for mesh
     * stop DHCP server on softAP interface by default
     * stop DHCP client on station interface by default
     * */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));

    esp_efuse_mac_get_default(&mac);

#if 0
    /* static ip settings */
    tcpip_adapter_ip_info_t sta_ip;
    sta_ip.ip.addr = ipaddr_addr("192.168.1.102");
    sta_ip.gw.addr = ipaddr_addr("192.168.1.1");
    sta_ip.netmask.addr = ipaddr_addr("255.255.255.0");
    tcpip_adapter_set_ip_info(WIFI_IF_STA, &sta_ip);
#endif
    /*  wifi initialization */
    ESP_ERROR_CHECK(esp_event_loop_init(NULL, NULL));
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());
    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
#ifdef MESH_FIX_ROOT
    ESP_ERROR_CHECK(esp_mesh_fix_root(1));
#endif
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    /* mesh event callback */
    cfg.event_cb = &mesh_event_handler;
    /* router */
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
    memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,
        strlen(CONFIG_MESH_ROUTER_PASSWD));
    /* mesh softAP */
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
        strlen(CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
    //ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s\n",  esp_get_free_heap_size(),
    //esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed");
}
