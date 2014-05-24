/* Wireless wifi Connection.
 *
 * Created by Ruining He
 */

#ifndef WIRELESS_WIFI_H_
#define WIRELESS_WIFI_H_

#include "jtag.h"

extern int init_wifi();
extern int wireless_send(JTAG *pJtag);
extern int wireless_send22();
extern int wireless_recv(JTAG *pJtag, unsigned int size);

#endif