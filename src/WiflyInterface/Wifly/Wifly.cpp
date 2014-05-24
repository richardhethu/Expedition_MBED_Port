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

#include "mbed.h"
#include "Wifly.h"
#include <string>
#include <algorithm>

//Debug is disabled by default
#if 0
#define DBG(x, ...) std::printf("[Wifly : DBG]"x"\r\n", ##__VA_ARGS__);
#define WARN(x, ...) std::printf("[Wifly : WARN]"x"\r\n", ##__VA_ARGS__);
#define ERR(x, ...) std::printf("[Wifly : ERR]"x"\r\n", ##__VA_ARGS__);
#else
#define DBG(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#endif

#define INFO(x, ...) printf("[Wifly : INFO]"x"\r\n", ##__VA_ARGS__);

#define MAX_TRY_JOIN 3

Wifly * Wifly::inst;

Wifly::Wifly(   PinName tx, PinName rx, PinName _reset, PinName tcp_status, const char * ssid, const char * phrase, Security sec):
    wifi(tx, rx), reset_pin(_reset), tcp_status(tcp_status), buf_wifly(512)
{
    this->wpa = wpa;
    security = sec;

    // change all ' ' in '$' in the ssid and the passphrase
    strcpy(this->ssid, ssid);
    for (int i = 0; i < strlen(ssid); i++) {
        if (this->ssid[i] == ' ')
            this->ssid[i] = '$';
    }
    strcpy(this->phrase, phrase);
    for (int i = 0; i < strlen(phrase); i++) {
        if (this->phrase[i] == ' ')
            this->phrase[i] = '$';
    }

    inst = this;
    attach_rx(false);
    cmd_mode = false;
}

bool Wifly::join()
{
    char cmd[100];

    for (int i= 0; i < MAX_TRY_JOIN; i++) {
        if (!sendCommand("set comm time 20\r", "AOK"))
            continue;

        if (!sendCommand("set comm size 128\r", "AOK"))
            continue;

        if (!sendCommand("set sys iofunc 0x40\r", "AOK"))
            continue;

        // no string sent to the tcp client
        if (!sendCommand("set comm remote 0\r", "AOK"))
            continue;

        // tcp protocol
        if (!sendCommand("set ip proto 2\r", "AOK"))
            continue;

        // tcp retry
        if (!sendCommand("set ip flags 0x7\r", "AOK"))
            continue;

        //no echo
        if (!sendCommand("set u m 1\r", "AOK"))
            continue;

        // no auto join
        if (!sendCommand("set w j 0\r", "AOK"))
            continue;

        //dhcp
        sprintf(cmd, "set i d %d\r", (dhcp) ? 1 : 0);
        if (!sendCommand(cmd, "AOK"))
            continue;

        // ssid
        sprintf(cmd, "set w s %s\r", ssid);
        if (!sendCommand(cmd, "AOK"))
            continue;

        //auth
        //sprintf(cmd, "set w a 3\r");//
        sprintf(cmd, "set w a %d\r", security);
        //printf("set w a %d\r", security);
        if (!sendCommand(cmd, "AOK"))
            continue;

        // if no dhcp, set ip, netmask and gateway
        if (!dhcp) {
            DBG("not dhcp\r");

            sprintf(cmd, "set i a %s\r\n", ip);
            if (!sendCommand(cmd, "AOK"))
                continue;

            sprintf(cmd, "set i n %s\r", netmask);
            if (!sendCommand(cmd, "AOK"))
                continue;

            sprintf(cmd, "set i g %s\r", gateway);
            if (!sendCommand(cmd, "AOK"))
                continue;
        }

        //key step
        if (security != NONE || security != Adhoc ) {
            if (security == WPA)
                sprintf(cmd, "set w p %s\r", phrase);
            else if (security == MIX)
                sprintf(cmd, "set w p %s\r", phrase);
            else if (security == WPA2)
                sprintf(cmd, "set w p %s\r", phrase);
            else if (security == WEP_128)
                sprintf(cmd, "set w k %s\r", phrase);

            if (!sendCommand(cmd, "AOK"))
                continue;
        }

        // save
        if (!sendCommand("save\r", "Stor"))
            return false;

        //join the network
        sprintf(cmd, "join\r");
        if (!sendCommand(cmd, "Associated", NULL, 3000))
            continue;

        if (dhcp) {
            if (!sendCommand("", "DHCP=ON", NULL, 3000))
                continue;
        }

        exit();

        INFO("\r\nssid: %s\r\nphrase: %s\r\nsecurity: %s\r\n\r\n", this->ssid, this->phrase, getStringSecurity());
        return true;
    }
    return false;
}

char * Wifly::getStringSecurity()
{
    switch(security) {
        case NONE:
            return "NONE";
        case WEP_128:
            return "WEP_128";
        case WPA:
            return "WPA";
        case WPA2:
            return "WPA2";
        case MIX:
            return "Mixed WPA1 & WPA2-PSK";
        case Adhoc:
            return "Adhoc, Join any Adhoc network";
    }
    return "UNKNOWN";
}


bool Wifly::dnsLookup(const char * host, char * ip)
{
    string h = host;
    char cmd[30], rcv[100];
    int l = 0;
    char * point;
    int nb_digits = 0;

    // no dns needed
    int pos = h.find(".");
    if (pos != string::npos) {
        string sub = h.substr(0, h.find("."));
        nb_digits = atoi(sub.c_str());
    }
    //printf("substrL %s\r\n", sub.c_str());
    if (count(h.begin(), h.end(), '.') == 3 && nb_digits > 0) {
        strcpy(ip, host);
    }
    // dns needed
    else {
        nb_digits = 0;
        sprintf(cmd, "lookup %s\r", host);
        if (!sendCommand(cmd, NULL, rcv))
            return false;

        // look for the ip address
        char * begin = strstr(rcv, "=") + 1;
        for (int i = 0; i < 3; i++) {
            point = strstr(begin + l, ".");
            DBG("str: %s", begin + l);
            l += point - (begin + l) + 1;
        }
        DBG("str: %s", begin + l);
        while(*(begin + l + nb_digits) >= '0' && *(begin + l + nb_digits) <= '9') {
            DBG("digit: %c", *(begin + l + nb_digits));
            nb_digits++;
        }
        memcpy(ip, begin, l + nb_digits);
        ip[l+nb_digits] = 0;
        DBG("ip from dns: %s", ip);
    }
    return true;
}


void Wifly::flush()
{
    buf_wifly.flush();
}

bool Wifly::sendCommand(const char * cmd, const char * ack, char * res, int timeout)
{
    if (!cmd_mode) {
        cmdMode();
    }
    if (send(cmd, strlen(cmd), ack, res, timeout) == -1) {
        ERR("sendCommand: cannot %s\r\n", cmd);
        exit();
        return false;
    }
    return true;
}

bool Wifly::cmdMode()
{
    if (send("$$$", 3, "CMD") == -1) {
        ERR("cannot enter in cmd mode\r\n");
        return false;
    }
    cmd_mode = true;
    return true;
}

bool Wifly::leave()
{
    if (!sendCommand("leave\r", "DeAuth"))
        return false;
    exit();
    return true;

}

bool Wifly::is_connected()
{
    return (tcp_status.read() ==  1) ? true : false;
}


void Wifly::reset()
{
    reset_pin = 0;
    wait(0.2);
    reset_pin = 1;
    wait(0.2);
}

bool Wifly::close()
{
    wait(0.25);
    if (!cmdMode())
        return false;
    if (send("close\r", 6, "CLOS") == -1)
        return false;
    exit();
    return true;
}


int Wifly::putc(char c)
{
    while (!wifi.writeable());
    return wifi.putc(c);
}


bool Wifly::read(char * str)
{
    int length = buf_wifly.available();
    if (length == 0)
        return false;
    for (int i = 0; i < length; i++)
        buf_wifly.dequeue(&str[i]);
    str[length] = 0;
    return true;
}


bool Wifly::exit()
{
    flush();
    if (!cmd_mode)
        return true;
    if (send("exit\r", 5, "EXIT") == -1)
        return false;
    cmd_mode = false;
    flush();
    return true;
}


int Wifly::readable()
{
    return buf_wifly.available();
}

int Wifly::writeable()
{
    return wifi.writeable();
}

char Wifly::getc()
{
    char c;
    while (!buf_wifly.available());
    buf_wifly.dequeue(&c);
    return c;
}

void Wifly::handler_rx(void)
{
    //read characters
    while (wifi.readable()){
        char c = LPC_UART2->RBR;
        buf_wifly.queue(c);
    }
}

void Wifly::attach_rx(bool callback)
{
    if (!callback)
        wifi.attach(NULL);
    else
        wifi.attach(this, &Wifly::handler_rx);
}


int Wifly::send(const char * str, int len, const char * ACK, char * res, int timeout)
{
    char read;
    size_t found = string::npos;
    string checking;
    Timer tmr;
    int result = 0;

    DBG("will send: %s\r\n",str);

    attach_rx(false);

    //We flush the buffer
    while (wifi.readable())
        wifi.getc();

    if (!ACK || !strcmp(ACK, "NO")) {
        for (int i = 0; i < len; i++)
            result = (putc(str[i]) == str[i]) ? result + 1 : result;
    } else {
        //We flush the buffer
        while (wifi.readable())
            wifi.getc();

        tmr.start();
        for (int i = 0; i < len; i++)
            result = (putc(str[i]) == str[i]) ? result + 1 : result;

        while (1) {
            if (tmr.read_ms() > timeout) {
                //We flush the buffer
                while (wifi.readable())
                    wifi.getc();

                DBG("check: %s\r\n", checking.c_str());

                attach_rx(true);
                return -1;
            } else if (wifi.readable()) {
                read = wifi.getc();
                if ( read != '\r' && read != '\n') {
                    checking += read;
                    found = checking.find(ACK);
                    if (found != string::npos) {
                        wait(0.01);

                        //We flush the buffer
                        while (wifi.readable())
                            wifi.getc();

                        break;
                    }
                }
            }
        }
        DBG("check: %s\r\n", checking.c_str());

        attach_rx(true);
        return result;
    }

    //the user wants the result from the command (ACK == NULL, res != NULL)
    if ( res != NULL) {
        int i = 0;
        Timer timeout;
        timeout.start();
        tmr.reset();
        while (1) {
            if (timeout.read() > 2) {
                if (i == 0) {
                    res = NULL;
                    break;
                }
                res[i] = '\0';
                DBG("user str 1: %s\r\n", res);

                break;
            } else {
                if (tmr.read_ms() > 300) {
                    res[i] = '\0';
                    DBG("user str: %s\r\n", res);

                    break;
                }
                if (wifi.readable()) {
                    tmr.start();
                    read = wifi.getc();

                    // we drop \r and \n
                    if ( read != '\r' && read != '\n') {
                        res[i++] = read;
                    }
                }
            }
        }
        DBG("user str: %s\r\n", res);
    }

    //We flush the buffer
    while (wifi.readable())
        wifi.getc();

    attach_rx(true);
    DBG("result: %d\r\n", result)
    return result;
}