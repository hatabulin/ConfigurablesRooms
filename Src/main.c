
/**
  ******************************************************************************
  * @file           : main.c
  * @author         : SergikLutsk, dragosha2000@gmx.net
  ******************************************************************************
  *
  * ConfigRoom device
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"
#include "usb_device.h"

/* USER CODE BEGIN Includes */
#include "net.h"
#include "enc28j60.h"
#include "dhcp_client.h"
#include "dnslkup.h"
#include "ip_arp_udp_tcp.h"
#include "dnslkup.h"
#include "websrv_help_functions.h"
#include "pgmspace.h"
#include "eeprom.h"
#include "string.h"
#include "hex_utils.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#ifdef DHCP
uint8_t rval=0;
static int8_t processing_state=0;
#endif
uint8_t ipaddr[4],gwip[4],dnsip[4],hisip[4],netmask[4],destip[4],mymac[6],destmac[6];
static uint8_t otherside_www_ip[4] = {10,10,0,100}; // will be filled by dnslkup

uint8_t flag[4];
#define flag_use_dhcp flag[0]
#define BUFFER_SIZE 1024
#define MAX_CNT 50
#define TRANS_NUM_GWMAC 1
#define WEBSERVER_VHOST "10.10.0.100"

uint16_t dat_p;
uint16_t p_len;
uint16_t cnt = MAX_CNT-1;
uint8_t lock_status_hw[2];
uint8_t lock_sens[2];
uint8_t sec=0;
uint8_t client_data_ready;
uint8_t net_buf[BUFFER_SIZE+1];
uint8_t gwmac[6];
uint8_t pingsrcip[4];
uint8_t dns_state=0;
uint8_t gw_arp_state=0;
uint8_t start_web_client=0;
uint8_t web_client_attempts=0;
uint8_t urlvarstr[21];// = {"/google.com"};
uint8_t web_client_sendok=0;
//uint8_t syn_ack_timeout;

// default IP data
const uint8_t IPADDR_[4] = {10,10,0,77};
const uint8_t MYMAC_[6] = {0x74,0x69,0x69,0x23,0x30,0x31};
const uint8_t  DESTADDR_[4] = {10,10,0,99};

const uint8_t FLAG_[4] = {0,0,0,0};
const char hoststr[] = {"10.10.0.1"}; //const char website[] PROGMEM = "api-conference-rooms.int-24.com";
const char http_header[] 	= {"HTTP/1.1 200 OK\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"};
const char error_header[] 	= {"HTTP/1.1 404 File not found\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"};
const char lock_on_html[] = {"HTTP/1.1 201 OK\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"
							"<head><h1>LOCK ON</br></br></head>"};
const char lock_off_html[] = {"HTTP/1.1 202 OK\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"
							"<head><h1>LOCK OFF</br></br></head>"};
const char dhcp_page_true[] = {"%s<p><h3><input name='use_dhcp' type='radio' value='0'> use STATIC</h3></p>"
        					   "<p><h3><input name='use_dhcp' type='radio' value='1' checked> use DHCP</h3></p>"
								"<p><input type='submit' value='SAVE'></p>"
								"</form>"
								"</html>"
								"\n\r\n\r"};
const char dhcp_page_false[] = {"%s<p><h3><input name='use_dhcp' type='radio' value='0' checked> use STATIC</h3></p>"
                				"<p><h3><input name='use_dhcp' type='radio' value='1'> use DHCP</h3></p>"
								"<p><input type='submit' value='SAVE'></p>"
								"</body>"
								"</form>"
                				"</html>"
                				"\n\r\n\r"};
const char home_page[] = {"HTTP/1.1 200 OK\r\nServer: nginx\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"
						"<form>"
						"<head><h1>ConferenceRoom v2.0 build:140419</br><h2>(c) MINT innovations, 2019</br></br></head>"
						"<body>"
						"<h3>IP:</h3>"
						"<p><input type='text' name='ip0' id='ip1' value='%03u' size=2"
						"<p><input type='text' name='ip1' id='ip2' value='%03u' size=2"
						"<p><input type='text' name='ip2' id='ip3' value='%03u' size=2"
						"<p><input type='text' name='ip3' id='ip4' value='%03u' size=2</p>"
						"<h3>MAC:</h3>"
						"<p><input type='text' name='mac0' id='mac1' value='%02X' size=2"
						"<p><input type='text' name='mac1' id='mac2' value='%02X' size=2"
						"<p><input type='text' name='mac2' id='mac3' value='%02X' size=2"
						"<p><input type='text' name='mac3' id='mac4' value='%02X' size=2"
						"<p><input type='text' name='mac4' id='mac5' value='%02X' size=2"
						"<p><input type='text' name='mac5' id='mac6' value='%02X' size=2</p>"
						"<h3>DEST IP:</h3>"
						"<p><input type='text' name='dip0' id='dip1' value='%03u' size=2"
						"<p><input type='text' name='dip1' id='dip2' value='%03u' size=2"
						"<p><input type='text' name='dip2' id='dip3' value='%03u' size=2"
						"<p><input type='text' name='dip3' id='dip4' value='%03u' size=2</p>"
						"<h3>lock_sens[]={%01u,%01u},lock_status_hw[]={%01u,%01u}</h3>"};
const char Write_page[] = {"HTTP/1.1 202 OK\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"
							"<head><h1>New values stored in MCU memory.</h1>\r\n"
							"<h1>Hardware reset...</h1>\r\n\r\n</br></br></head>"};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM1_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void browserresult_callback(uint16_t webstatuscode,uint16_t datapos __attribute__((unused)), uint16_t len __attribute__((unused)));
uint8_t findKeyVal (char* instr, char *strbuf, uint8_t maxlen, char *key);
void ReadConstantsFromEEP();
void WriteDefaultsToEEP(void);

void HardwareError (float error_num, char infinite) {
	if (infinite)
	while (1) {
		int i = 0;
		while (i++ < ((int)((float)(error_num) / 1) + 1) ) {
			GPIOC->BSRR = LED_Pin;
			HAL_Delay(1000 / error_num);
			GPIOC->BSRR = LED_Pin << 16;
			HAL_Delay(1000 / error_num);
		}
	};

	GPIOC->BSRR = LED_Pin;
	HAL_Delay(1000);
	GPIOC->BSRR = LED_Pin << 16;
	HAL_Delay(100);

	int i=0;
	while (i++ < error_num) {
		GPIOC->BSRR = LED_Pin;
		HAL_Delay(300);
		GPIOC->BSRR = LED_Pin << 16;
		HAL_Delay(200);
	}

	HAL_Delay(500);
	NVIC_SystemReset();

	return;
}

static inline void BlinkLed(int times, int delay) {
	int i = 0;

	while (i++ < times) {
		GPIOC->BSRR = LED_Pin;
		HAL_Delay(delay);
		GPIOC->BSRR = LED_Pin << 16;
		HAL_Delay(delay);
	}

	return;
}

void PrintIP(void) {
	char str[100];
	snprintf(str,sizeof(str),"my ip: %03u.%03u.%03u.%03u\n\r",ipaddr[0],ipaddr[1],ipaddr[2],ipaddr[3]);
	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
	snprintf(str,sizeof(str),"dest ip: %03u.%03u.%03u.%03u\n\r",destip[0],destip[1],destip[2],destip[3]);
	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
	snprintf(str,sizeof(str),"gw ip: %03u.%03u.%03u.%03u\n\r",gwip[0],gwip[1],gwip[2],gwip[3]);
	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
	snprintf(str,sizeof(str),"netmask: %03u.%03u.%03u.%03u\n\r",netmask[0],netmask[1],netmask[2],netmask[3]);
	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
}

void LockStatusRead() {
	lock_status_hw[0] = HAL_GPIO_ReadPin(SENSE1_GPIO_Port,SENSE1_Pin);
	lock_status_hw[1] = HAL_GPIO_ReadPin(SENSE2_GPIO_Port,SENSE2_Pin);
	lock_sens[0] = HAL_GPIO_ReadPin(SENSE1_GPIO_Port,SENSE1_Pin);
	lock_sens[1] = HAL_GPIO_ReadPin(SENSE1_GPIO_Port,SENSE1_Pin);
}

void EEpromCheckAndRead(void) {
	char str[30];
	uint32_t data_;

	EE_Read(EEP_CHECK_ADDR,&data_);
	if (data_!= CHECK_DATA) {
		snprintf(str,strlen(str),"EEPROM format and write defaults...\n\r");
		HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
		EE_Format();
		WriteDefaultsToEEP();
		EE_Write(EEP_CHECK_ADDR,CHECK_DATA);
	 } else {
		 ReadConstantsFromEEP();
	 }
}

void WriteDefaultsToEEP(void) {
	char str[30];
	union {
		uint32_t data32;
		unsigned char data4x8[4];
	} tmp;
	// MAC defaults sets...
	//
	tmp.data4x8[0] = MYMAC_[0];
	tmp.data4x8[1] = MYMAC_[1];
	tmp.data4x8[2] = MYMAC_[2];
	tmp.data4x8[3] = MYMAC_[3];
	EE_Write(EE_MYMAC1,tmp.data32 );
	tmp.data4x8[0] = MYMAC_[4];
	tmp.data4x8[1] = MYMAC_[5];
	tmp.data4x8[2] = 0;
	tmp.data4x8[3] = 0;
	EE_Write(EE_MYMAC2,tmp.data32);

	// IPs defaults sets...
	//
	EE_Write(EE_IPADDR,(uint32_t)(IPADDR_[0] | IPADDR_[1]<<8 | IPADDR_[2]<<16 | IPADDR_[3]<<24) );
	EE_Write(EE_DESTADDR,(uint32_t)(DESTADDR_[0] | DESTADDR_[1]<<8 | DESTADDR_[2]<<16 | DESTADDR_[3]<<24) );

	tmp.data4x8[0] = FLAG_[0];
	tmp.data4x8[1] = FLAG_[1];
	tmp.data4x8[2] = FLAG_[2];
	tmp.data4x8[3] = FLAG_[3];
	EE_Write(EE_FLAGSADDR,tmp.data32);

	snprintf(str,sizeof(str),"EEPROM write defaults OK.\n\r");
	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);

	ReadConstantsFromEEP();
}
void ReadConstantsFromEEP(void) {
	char str[30];
	uint32_t data_;
	union {
		uint32_t data32;
		unsigned char data4x8[4];
	} tmp;

	EE_Read(EE_IPADDR,&tmp.data32);
	ipaddr[0] = tmp.data4x8[0];
	ipaddr[1] = tmp.data4x8[1];
	ipaddr[2] = tmp.data4x8[2];
	ipaddr[3] = tmp.data4x8[3];

	EE_Read(EE_MYMAC1,&tmp.data32);
	mymac[0] = tmp.data4x8[0];
	mymac[1] = tmp.data4x8[1];
	mymac[2] = tmp.data4x8[2];
	mymac[3] = tmp.data4x8[3];
	EE_Read(EE_MYMAC2,&tmp.data32);
	mymac[4] = tmp.data4x8[0];
	mymac[5] = tmp.data4x8[1];

	EE_Read(EE_DESTADDR,&tmp.data32);
	gwip[0] = otherside_www_ip[0] = destip[0] = tmp.data4x8[0];
	gwip[1] = otherside_www_ip[1] = destip[1] = tmp.data4x8[1];
	gwip[2] = otherside_www_ip[2] = destip[2] = tmp.data4x8[2];
	gwip[3] = otherside_www_ip[3] = destip[3] = tmp.data4x8[3];

	EE_Read(EE_FLAGSADDR,&data_);
	flag_use_dhcp = data_ & 0xFF;
	flag[1] = data_>>8 & 0xFF;
	flag[2] = data_>>16 & 0xFF;
	flag[3] = data_>>24 & 0xFF;

	snprintf(str,sizeof(str),"FLAGS read OK.\n\r");
	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
}

uint16_t http200ok(void) {

	return(fill_tcp_data(net_buf,0,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));
}

uint16_t HomePage(void) {
  	char temp_str[TEMP_STR_MAX];

  	snprintf(temp_str, sizeof(temp_str),home_page,	ipaddr[0],ipaddr[1],ipaddr[2],ipaddr[3],mymac[0],mymac[1],mymac[2],mymac[3],mymac[4],mymac[5],
													destip[0],destip[1],destip[2],destip[3], lock_sens[0],lock_sens[1],lock_status_hw[0],lock_status_hw[1] );
	if (flag_use_dhcp == 1) snprintf (temp_str,sizeof(temp_str), dhcp_page_true,temp_str);
	else snprintf (temp_str,sizeof(temp_str), dhcp_page_false,temp_str);
	return(fill_tcp_data_p(net_buf,0, temp_str));
}

// we were ping-ed by somebody, store the ip of the ping sender
// and trigger an upload to http://tuxgraphics.org/cgi-bin/upld
// This is just for testing and demonstration purpose
void ping_callback(uint8_t *ip) {
	uint8_t i=0;

	if (start_web_client==0) { // trigger only first time in case we get many ping in a row:
		start_web_client=1;
        while(i<4) { // save IP from where the ping came:
        	pingsrcip[i]=ip[i];
            i++;
        }
    }
}

void browserresult_callback(uint16_t webstatuscode,uint16_t datapos __attribute__((unused)), uint16_t len __attribute__((unused))) {
	char str[100];

	if (webstatuscode==200) {
		web_client_sendok++;
		snprintf(str,sizeof(str),"200:OK\r\n");
		HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
    } else {
		web_client_sendok++;
		snprintf(str,sizeof(str),"Server response 404\r\n");
		HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
    }
}

void arpresolver_result_callback(uint8_t *ip ,uint8_t transaction_number,uint8_t *mac) {
    uint8_t i=0;

    if (transaction_number==TRANS_NUM_GWMAC)
    	while(i<6) {
    		gwmac[i]=mac[i];i++;
    	} // copy mac address over:
}

void net_poll() {
	char str[100];
	char keystr[10];
	uint8_t data;

	// 1-st task
	p_len=enc28j60PacketReceive(BUFFER_SIZE,net_buf);
	dat_p = packetloop_arp_icmp_tcp(net_buf,p_len);
	if (dat_p!=0) {
		snprintf(str,28,"/LOCK_OFF&%02X-%02X-%02X-%02X-%02X-%02X",mymac[0],mymac[1],mymac[2],mymac[3],mymac[4],mymac[5]);
	    char* index = strstr((char *)&(net_buf[dat_p]),str);

	    if (index>0) {
			lock_status_hw[0] = 0;
			HAL_GPIO_WritePin(RELAY_GPIO_Port,RELAY_Pin,RESET);
			snprintf(str,sizeof(str),"'/lock_off' received\n\r");
			HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
			dat_p = fill_tcp_data_p(net_buf,0, lock_off_html);
			www_server_reply(net_buf,dat_p); // send web page data
		}

		snprintf(str,29,"/LOCK_ON&%02X-%02X-%02X-%02X-%02X-%02X",mymac[0],mymac[1],mymac[2],mymac[3],mymac[4],mymac[5]);
		index = strstr((char *)&(net_buf[dat_p]),str);

		if (index >0) {
			lock_status_hw[0] = 1;
			HAL_GPIO_WritePin(RELAY_GPIO_Port,RELAY_Pin,SET);
			snprintf(str,sizeof(str),"'/lock_on' received\n\r");
			HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
			dat_p = fill_tcp_data_p(net_buf,0, lock_on_html);
			www_server_reply(net_buf,dat_p); // send web page data
		}
// check for receiving new values for change static ip, dest ip addreess
		if (strncmp((char *) & (net_buf[dat_p]),"GET /?",6)==0) {
			dat_p += 6;

			for (uint8_t x=0;x<4;x++) {
	    		snprintf(keystr,sizeof(keystr),"ip%X=",x);// KEY_IP[x];
	    		index = strstr((char *)&(net_buf[dat_p]),keystr);
	    		if (index>0) {
	    			dat_p+=4;
	    			data  =  dec2int_byte(&net_buf[dat_p] );
	    			ipaddr[x] = data;
	    			dat_p+=4;
	    		}
	    	}

			for (uint8_t x=0;x<6;x++) {
	    		snprintf(keystr,sizeof(keystr),"mac%X=",x);//
	    		index = strstr((char *)&(net_buf[dat_p]),keystr);
	    		if (index>0) {
	    			dat_p+=5;
	    			data  =  hex2int_byte((char *)&net_buf[dat_p] );
	    			mymac[x] = data;
	    			dat_p+=3;
	    		}
	    	}

			for (uint8_t x=0;x<4;x++) {
	    		snprintf(keystr,sizeof(keystr),"dip%X=",x);//
	    		index = strstr((char *)&(net_buf[dat_p]),keystr);
	    		if (index>0) {
	    			dat_p+=5;
	    			data  =  dec2int_byte(&net_buf[dat_p] );
	    			destip[x] = data;
	    			dat_p+=4;
	    		}
	    	}

    		snprintf(keystr,sizeof(keystr),"use_dhcp=");//
    		index = strstr((char *)&(net_buf[dat_p]),keystr);

    		if (index>0) flag_use_dhcp = net_buf[dat_p+9] - 0x30;

    		EE_Write(EE_IPADDR,(uint32_t)(ipaddr[0] | ipaddr[1]<<8 | ipaddr[2]<<16 | ipaddr[3]<<24) );
	    	EE_Write(EE_DESTADDR,(uint32_t)(destip[0] | destip[1]<<8 | destip[2]<<16 | destip[3]<<24) );
	    	union {
	    		uint32_t data32;
	    		unsigned char data4x8[4];
	    	} tmp;

	    	// MAC defaults sets...
	    	//
	    	tmp.data4x8[0] = mymac[0];
	    	tmp.data4x8[1] = mymac[1];
	    	tmp.data4x8[2] = mymac[2];
	    	tmp.data4x8[3] = mymac[3];
	    	EE_Write(EE_MYMAC1,tmp.data32 );

	    	tmp.data4x8[0] = mymac[4];
	    	tmp.data4x8[1] = mymac[5];
	    	tmp.data4x8[2] = 0;
	    	tmp.data4x8[3] = 0;
	    	EE_Write(EE_MYMAC2,tmp.data32);

	    	tmp.data4x8[0] = flag_use_dhcp;
	    	tmp.data4x8[1] = FLAG_[1];
	    	tmp.data4x8[2] = FLAG_[2];
	    	tmp.data4x8[3] = FLAG_[3];
	    	EE_Write(EE_FLAGSADDR,tmp.data32);

	    	snprintf(str,sizeof(str),"new ip address: %03u.%03u.%03u.%03u\n\r",ipaddr[0],ipaddr[1],ipaddr[2],ipaddr[3]);
	    	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
	    	snprintf(str,sizeof(str),"new destip address: %03u.%03u.%03u.%03u\n\r",destip[0],destip[1],destip[2],destip[3]);
	    	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
	    	snprintf(str,sizeof(str),"new mac address: %02X:%02X:%02X:%02X:%02X:%02X\n\r",mymac[0],mymac[1],mymac[2],mymac[3],mymac[4],mymac[5]);
	    	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);

	    	dat_p = fill_tcp_data(net_buf,0,Write_page);
			www_server_reply(net_buf,dat_p); // send web page data

			snprintf(str,sizeof(str),"softreset hardware...\n\r");

			HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
			NVIC_SystemReset();
		} else {
			dat_p = HomePage();
			www_server_reply(net_buf,dat_p); // send web page data
			snprintf(str,sizeof(str),"GET received\r\n");
			HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
		}
		dat_p = 0;
	}

	// second task - tcp client loop
	uint8_t i = enc28j60linkup();

	if (i >0) {
		if(p_len==0) {
			// we are idle here trigger arp and dns stuff here
			if (gw_arp_state==0) { // find the mac address of the gateway (e.g your dsl router).
                get_mac_with_arp(gwip,TRANS_NUM_GWMAC,&arpresolver_result_callback);
                gw_arp_state=1;
            }
            if (get_mac_with_arp_wait()==0 && gw_arp_state==1) gw_arp_state=2; // done we have the mac address of the GW
            if (dns_state==0 && gw_arp_state==2)
            	if (enc28j60linkup()) { // only for dnslkup_request we have to check if the link is up.
            		sec=0;
                    dns_state=1;
                    dnslkup_request(net_buf,WEBSERVER_VHOST,gwmac);
                }
            if (dns_state==1 && dnslkup_haveanswer()) {
            	dns_state=2;
            	dnslkup_get_ip(otherside_www_ip);
            }
            if (dns_state!=2)
            	if (sec > 60) { // retry every minute if dns-lookup failed:
                	dns_state=0;
                }
                // don't try to use web client before
                // we have a result of dns-lookup
                //continue;
            //----------
            if (start_web_client==1) {
            	sec=0;
                start_web_client=2;
                web_client_attempts++;
                mk_net_str(str,pingsrcip,4,'.',10);
                //urlencode(str,urlvarstr);
                //parse_ip(otherside_www_ip,WEBSERVER_VHOST);
                client_browse_url(PSTR("/"),(char*)urlvarstr,PSTR(WEBSERVER_VHOST),&browserresult_callback,otherside_www_ip,gwmac);
                snprintf(str,sizeof(str),"exec client_browse_url() function. Server response:\n\r");
                HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
            }
            if (sec>60 && start_web_client==2)	start_web_client=0; // reset after a delay to prevent permanent bouncing
        }
        if(dat_p==0) udp_client_check_for_dns_answer(net_buf,p_len);
	} else {
		snprintf(str,sizeof(str),"enc28j60 link is down :(\r\nResetting device...\n\r");
		HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
		NVIC_SystemReset();
	}
}
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  char str[100];
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_USB_DEVICE_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port,BUTTON_Pin)==0) WriteDefaultsToEEP();

  EEpromCheckAndRead();

  snprintf(str,sizeof(str),"EEPROM read OK.\n\r");
  HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);

  enc28j60_set_spi(&hspi1);

  enc28j60Init(mymac);
  enc28j60clkout(2);

  if (enc28j60getrev()>0) { // check revision of the enc28j60 module
	  snprintf(str,sizeof(str),"enc28j60 hardware init ok.\n\renc28j60 module revision: %02X\n\r",enc28j60getrev());
	  HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
      enc28j60PhyWrite(PHLCON,0x476);
      snprintf(str,sizeof(str),"MAC: %02X:%02X:%02X:%02X:%02X:%02X\n\r",mymac[0],mymac[1],mymac[2],mymac[3],mymac[4],mymac[5]);
      HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
      if (flag_use_dhcp == 1) {
    	  rval=0;
    	  HAL_TIM_Base_Start(&htim1);
    	  HAL_TIM_Base_Start_IT(&htim1);
    	  init_mac(mymac);
    	  snprintf(str,sizeof(str),"- device use dhcp server, please wait for answer...\n\r");
    	  HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
    	  while(rval==0) {
    		  p_len=enc28j60PacketReceive(BUFFER_SIZE, net_buf);
    		  net_buf[BUFFER_SIZE]='\0';
    		  rval=packetloop_dhcp_initial_ip_assignment(net_buf,p_len,mymac[5]);
    	  }
    	  dhcp_get_my_ip(ipaddr,netmask,gwip); // we have an IP:
    	  client_ifconfig(ipaddr,netmask);
    	  if (gwip[0]==0) {
    		  snprintf(str,sizeof(str),"\n\r(FAILED)gateway is not returned from server :(\n\r");
    		  HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
    		  // we must have a gateway returned from the dhcp server
    		  // otherwise this code will not work
    		  while(1); // stop here
    	  } else {
        	  snprintf(str,sizeof(str),"dhcp server give:\n\r");
        	  HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
    		  PrintIP();
    	  }
    	  // we have a gateway.
    	  // find the mac address of the gateway (e.g your dsl router).
    	  get_mac_with_arp(gwip,TRANS_NUM_GWMAC,&arpresolver_result_callback);
    	  while(get_mac_with_arp_wait()) {
    		  // to process the ARP reply we must call the packetloop
    		  p_len=enc28j60PacketReceive(BUFFER_SIZE, net_buf);
    		  packetloop_arp_icmp_tcp(net_buf,p_len);
    	  }
    	  if (string_is_ipv4(WEBSERVER_VHOST)) {
    		  // the the webserver_vhost is not a domain name but alreay
    		  // an IP address written as sting
    		  parse_ip(otherside_www_ip,WEBSERVER_VHOST);
    		  processing_state=2; // no need to do any dns look-up
    	  }
      } else {
    	  snprintf(str,sizeof(str),"- device use static ip\n\r");
    	  HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
    	  PrintIP();
      }
      init_udp_or_www_server(mymac,ipaddr);
	  www_server_port(80);
  } else {
	  snprintf(str,sizeof(str),"(FAILED)enc28j60 hardware init false:(\n\rSoft reset hardware...\n\r");
	  HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
	  NVIC_SystemReset();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
	  cnt++;
	  if (cnt>MAX_CNT) {
		  start_web_client = 1; // start browseUrl tcp...
		  cnt = 0;
	  }
	  net_poll();
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{

  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 48000;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 39999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ETHERNET_CS_GPIO_Port, ETHERNET_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ETHERNET_CS_Pin */
  GPIO_InitStruct.Pin = ETHERNET_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(ETHERNET_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BUTTON_Pin SENSE2_Pin SENSE1_Pin */
  GPIO_InitStruct.Pin = BUTTON_Pin|SENSE2_Pin|SENSE1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RELAY_Pin */
  GPIO_InitStruct.Pin = RELAY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RELAY_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	  if(GPIO_Pin== GPIO_PIN_2) {
		  client_data_ready = 1;//    net_poll();
	  } else __NOP();
}

/*
uint8_t StringToHex(String ss)
{
  char *str; //  = new char[sizeof(ss)+1];
  strtol(ss.c_str(), &str, 16);
}
*/
uint8_t findKeyVal (char *instr, char *strbuf, uint8_t maxlen, char *key) {
	uint8_t found=0;
	uint8_t i=0;
	const char *kp;

	kp=key;
	while(*instr &&  *instr!=' ' && *instr!='\n' && found==0) {
		if (*instr == *kp) {
			kp++;
			if (*kp == '\0') {
				instr++;
				kp=key;
				if (*instr == '=') found=1;
			}
		} else kp=key;
		instr++;
	}

	if (found==1) {
	    // copy the value to a buffer and terminate it with '\0'
		while(*instr &&  *instr!=' ' && *instr!='\n' && *instr!='&' && i<maxlen-1) {
			*strbuf=*instr;
			i++;
			instr++;
			strbuf++;
		}
		*strbuf='\0';
	}
	return(i);
}

uint16_t getIntArg(char* data, char* key, int value) {
    char temp[10];

    value =-1;
    if (findKeyVal(data + 7, temp, sizeof temp, key) > 0) value = atoi(temp);
    return value;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
