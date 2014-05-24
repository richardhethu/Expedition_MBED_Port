/*
 * Wired Ethernet Connection.
 *
 * Created by Ruining He
 */

#ifndef ETHERNET_H_
#define ETHERNET_H_

#include "jtag.h"


extern bool ethernet_open;

extern int init_ethernet();
extern int enet_send(JTAG *pJtag);
extern int enet_recv(JTAG *pJtag, unsigned int size);

#endif