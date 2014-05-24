#ifndef SIGNAL_H_
#define SIGNAL_H_

#define EMPTY_REQ         (16U)
#define TERM_PRINT_REQ    (0U)
#define TERM_SCAN_REQ     (1U)
#define ENET_SEND_REQ     (2U)
#define ENET_RECV_REQ     (3U)
#define WIRELESS_SEND_REQ (4U)
#define WIRELESS_RECV_REQ (5U)
#define READ_POW_REQ      (6U)
#define PANIC_REQ         (15U)

#define EMPTY_ACK   (16U)
#define PRINT_ACK   (0U)
#define SCAN_ACK    (1U)
#define ENET_SEND_ACK (2U)
#define ENET_RECV_ACK (3U)
#define WIRELESS_SEND_ACK (4U)
#define WIRELESS_RECV_ACK (5U)
#define READ_POW_ACK      (6U)

// core send a print or scan request to mbed
#define SIG_DISPATCH     (0x1)
#define SIG_DATA_SEND    (0x2)
#define SIG_DATA_RECV    (0x3)
#define SIG_POW_READ     (0x4)
// user type debug, wake up a thread to print content of ram buffer for debug
#define SIG_DEBUG (0x20)

#define SEND_TYPE_TERM      (0x1)
#define SEND_TYPE_ENET      (0x2)
#define SEND_TYPE_WIRELESS  (0x3)

#define RECV_TYPE_TERM      (0x4)
#define RECV_TYPE_ENET      (0x5)
#define RECV_TYPE_WIRELESS  (0x6)



#endif