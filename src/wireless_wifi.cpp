#include "mbed.h"
#include "shared_buf.h"
#include "WiflyInterface.h"
#include "mmap.h"

#define MAX_TRY ( 3 )

static const char* SSID = "NETGEAR_3891E";
static const char* PWD = "keeponfighting";
static const char* ECHO_SERVER_ADDRESS = "192.168.1.10";
static const int ECHO_SERVER_PORT = 7;

/* wifly object where:
    *     - p28 and p27 are for the serial communication
    *     - p30 is for the reset pin
    *     - p29 is for the connection status
    *     - "mbed" is the ssid of the network
    *     - "password" is the password
    *     - WPA2 is the security
    */
static WiflyInterface wifly(p28, p27, p30, p29, SSID, PWD, WPA2);

static TCPSocketConnection socket;
static bool wireless_open = false;

void init_wifi()
{
    int tries = 0;
    wifly.init(); // use DHCP
    while (tries < MAX_TRY && !wifly.connect()) {
        tries ++;
        printf("Connecting to wifi...\r\n"); // join the network
    }
    if (tries == MAX_TRY) {
        wireless_open = false;
        printf("Wifi initialization failed: Can't connect to wifi (%s).\r\n", SSID); // join the network
        return;    
    }
    printf("Connected to Wifi. IP Address is %s\n\r", wifly.getIPAddress());
    
    tries = 0;
    while (tries < MAX_TRY && socket.connect(ECHO_SERVER_ADDRESS, ECHO_SERVER_PORT) < 0) {
        tries ++;
        printf("Wifly unable to connect to (%s) on port (%d)\r\n", ECHO_SERVER_ADDRESS, ECHO_SERVER_PORT);        
        wait(1);
    }
    if (tries == MAX_TRY) {
        wireless_open = false;
        printf("Wifi initialization failed: Failed to connect to server (%s:%s).\r\n", ECHO_SERVER_ADDRESS, ECHO_SERVER_PORT);
        return;    
    }
    socket.set_blocking(false, 2000);    
    wireless_open = true;
    printf("Wifi initialization succeed: Wifi connected to server (%s:%s) \r\n",ECHO_SERVER_ADDRESS, ECHO_SERVER_PORT);
}

int wireless_send(JTAG *pJtag) {
    // buffer for writing to wifi
    static char wireless_txbuffer[BUF_SIZE];
    int size; 
    
    // check if wifi is already initialized
    if (wireless_open == false) {
        return 0;
    }
    
    // copy data in ram buffer to wireless_txbuffer for sending
    jtag_writebuffer(pJtag, wireless_txbuffer);
    size = socket.send_all(wireless_txbuffer, strlen(wireless_txbuffer));
    return size;
}

int wireless_recv(JTAG *pJtag, unsigned int size) {
    // buffer for reading from wifi
    static char wireless_rxbuffer[BUF_SIZE];
    
    // check if wifi is already initialized
    if (wireless_open == false) {
        return 0;
    }
    int ret = socket.receive_all(wireless_rxbuffer, size);
    if (ret <= 0){    
        return 0;
    }
    // copy content in wireless_rxbuffer to ram buffer
    wireless_rxbuffer[ret] = '\0';
    ret = jtag_readbuffer(wireless_rxbuffer, pJtag);

    return ret;
}