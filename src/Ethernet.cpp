/*
 * Wired Ethernet Connection.
 *
 * Created by Ruining He
 */
#include "Ethernet.h"
#include "EthernetInterface.h"
#include "basic_io.h"
#include "mmap.h"
#include "shared_buf.h"

const int SERVER_PORT = 1500;

const char *const LOCAL_ADDRESS = "192.168.1.64";
const char *const REMOTE_ADDRESS = "132.239.95.211";


const char *const MASK = "255.255.255.0";
const char *const GATEWAY = "192.168.1.1";

static bool ethernet_open = false;
static EthernetInterface eth;
static Ethernet_TCPSocketConnection socket;

// success : 0
// failed  : !=0
int init_ethernet() {
    if (ethernet_open == false) {
        int status = eth.init(LOCAL_ADDRESS, MASK, GATEWAY);
        if (status != 0){
            printf("Ethernet Initialization Failed: Failed to init. \r\n");
            return status;
        }
        status = eth.connect();
        if (status != 0){
            printf("Ethernet Initialization Failed: Failed to connect. \r\n");
            return status;
        }
        printf("Ethernet Initialization: local address is: %s\r\n", LOCAL_ADDRESS);
        int counter = 8;
        while (counter -- >0) {
            status = socket.connect(REMOTE_ADDRESS, SERVER_PORT);
            if (status == 0){
                break;
            }
        }
        if (status != 0){
            printf("Ethernet Initialization Failed: Failed to connect to server (%s:%d)\r\n", REMOTE_ADDRESS, SERVER_PORT);
            return status;
        }
        socket.set_blocking(false, 2000);
        ethernet_open = true;
        printf("Ethernet Initialization Succeed: Connected to server (%s:%d)\r\n", REMOTE_ADDRESS, SERVER_PORT);
    }
    return 0;
}

void close_ethernet() {
    if (ethernet_open) {
        eth.disconnect();
    }
}

int enet_send(JTAG *pJtag) {
    // buffer for writing to ethernet
    static char inet_txbuffer[BUF_SIZE];
    int size;
    
    // check if ethernet is already initialized
    if (ethernet_open == false) {
        return 0;
    }
    // copy data in ram buffer to inet_txbuffer for sending
    jtag_writebuffer(pJtag, inet_txbuffer);    
    size = socket.send_all(inet_txbuffer, sizeof(inet_txbuffer));        
    return size;
}

int enet_recv(JTAG *pJtag, unsigned int size) {
    // buffer for reading from ethernet
    static char inet_rxbuffer[BUF_SIZE];
    
    // check if ethernet is already initialized
    if (ethernet_open == false) {
        return 0;
    }
    int ret = socket.receive_all(inet_rxbuffer, BUF_SIZE);
printf("recved: %s\r\n", inet_rxbuffer);
    if (ret <= 0){
        return 0;
    }
    // copy content in inet_rxbuffer to ram buffer
    inet_rxbuffer[ret] = '\0';
    ret = jtag_readbuffer(inet_rxbuffer, pJtag);
    return ret;
}