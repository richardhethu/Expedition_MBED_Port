/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "TCPSocketConnection.h"
#include <algorithm>

TCPSocketConnection::TCPSocketConnection() {}

int TCPSocketConnection::connect(const char* host, const int port)
{
    //open the connection
    char cmd[30];
    char rcv[40];

    // if we cannot enter in cmd mode, maybe we have a tcp connection opened.
    // in this case we need to wait at least 0.25s to enter in cmd mode
    if (!wifi->cmdMode()) {
        wait(0.25);
        if (!wifi->cmdMode()) {
            wifi->send("\r", 1);
            wait(0.1);
            wifi->exit();
            return -1;
        }
    }
    if (wifi->dnsLookup(host, rcv)) {
        sprintf(cmd, "set ip host %s\r", rcv);
        if (!wifi->sendCommand(cmd, "AOK"))
            return -1;
    } else {
        return -1;
    }
    sprintf(cmd, "set ip remote %d\r", port);
    if (!wifi->sendCommand(cmd, "AOK"))
        return -1;

    if (wifi->sendCommand("open\r", NULL, rcv)) {
        if (strstr(rcv, "OPEN") == NULL) {
            if (strstr(rcv, "Connected") != NULL) {
                if (!wifi->sendCommand("close\r", "CLOS"))
                    return -1;
                if (!wifi->sendCommand(cmd, "OPEN"))
                    return -1;
            } else {
                return -1;
            }
        }
    } else {
        return -1;
    }
    wifi->flush();
    return 0;
}

bool TCPSocketConnection::is_connected(void)
{
    return wifi->is_connected();
}

int TCPSocketConnection::send(char* data, int length)
{
    Timer tmr;

    if (!_blocking) {
        tmr.start();
        while (tmr.read_ms() < _timeout) {
            if (wifi->writeable())
                break;
        }
        if (tmr.read_ms() >= _timeout) {
            return -1;
        }
    }
    return wifi->send(data, length);
}

// -1 if unsuccessful, else number of bytes written
int TCPSocketConnection::send_all(char* data, int length)
{
    Timer tmr;
    int idx = 0;
    tmr.start();
//error("before while\r\n");
    while ((tmr.read_ms() < _timeout) || _blocking) {
    
        idx += wifi->send(data, length);

        if (idx == length){//error("idx == length\r\n");
            return idx;}
    }//error("return\r\n");
    return (idx == 0) ? -1 : idx;
}

// -1 if unsuccessful, else number of bytes received
int TCPSocketConnection::receive(char* data, int length)
{
    Timer tmr;
    int time = -1;
    
    
    if (!_blocking) {
        tmr.start();
        while (time < _timeout + 20) {
            if (wifi->readable()) {
                break;
            }
            time = tmr.read_ms();
        }
        if (time >= _timeout + 20) {
            return -1;
        }
    }


    while(!wifi->readable());
    int nb_available = wifi->readable();
    for (int i = 0; i < min(nb_available, length); i++) {
        data[i] = wifi->getc();
    }

    return min(nb_available, length);
}


// -1 if unsuccessful, else number of bytes received
int TCPSocketConnection::receive_all(char* data, int length)
{
    Timer tmr;
    int idx = 0;
    int time = -1;

    tmr.start();
    
    while (time < _timeout || _blocking) {

        int nb_available = wifi->readable();
        for (int i = 0; i < min(nb_available, length); i++) {
            data[idx++] = wifi->getc();
        }

        if (idx == length)
            break;

        time = tmr.read_ms();
    }

    return (idx == 0) ? -1 : idx;
}
