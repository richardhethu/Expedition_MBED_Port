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
 *
 * @section DESCRIPTION
 *
 * Wifly RN131-C, wifi module
 *
 * Datasheet:
 *
 * http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Wireless/WiFi/WiFly-RN-UM.pdf
 */

#ifndef WIFLY_H
#define WIFLY_H

#include "mbed.h"
#include "CBuffer.h"

#define DEFAULT_WAIT_RESP_TIMEOUT 500

enum Security {
    NONE = 0,
    WEP_128 = 1,
    WPA = 2,
    MIX = 3,
    WPA2 = 4,
    Adhoc = 6
};

class Wifly
{

public:
    /*
    * Constructor
    *
    * @param tx mbed pin to use for tx line of Serial interface
    * @param rx mbed pin to use for rx line of Serial interface
    * @param reset reset pin of the wifi module ()
    * @param tcp_status connection status pin of the wifi module (GPIO 6)
    * @param ssid ssid of the network
    * @param phrase WEP or WPA key
    * @param sec Security type (NONE, WEP_128 or WPA)
    */
    Wifly(  PinName tx, PinName rx, PinName reset, PinName tcp_status, const char * ssid, const char * phrase, Security sec);

    /*
    * Send a string to the wifi module by serial port. This function desactivates the user interrupt handler when a character is received to analyze the response from the wifi module.
    * Useful to send a command to the module and wait a response.
    *
    *
    * @param str string to be sent
    * @param len string length
    * @param ACK string which must be acknowledge by the wifi module. If ACK == NULL, no string has to be acknoledged. (default: "NO")
    * @param res this field will contain the response from the wifi module, result of a command sent. This field is available only if ACK = "NO" AND res != NULL (default: NULL)
    *
    * @return true if ACK has been found in the response from the wifi module. False otherwise or if there is no response in 5s.
    */
    int send(const char * str, int len, const char * ACK = NULL, char * res = NULL, int timeout = DEFAULT_WAIT_RESP_TIMEOUT);

    /*
    * Connect the wifi module to the ssid contained in the constructor.
    *
    * @return true if connected, false otherwise
    */
    bool join();

    /*
    * Close a connection with the access point
    *
    * @ returns true if successful
    */
    bool leave();

    /*
    * Read a string if available
    *
    *@param str pointer where will be stored the string read
    */
    bool read(char * str);

    /*
    * Reset the wifi module
    */
    void reset();

    /*
    * Check if characters are available
    *
    * @return number of available characters
    */
    int readable();

    /*
    * Check if characters are available
    *
    * @return number of available characters
    */
    int writeable();

    /*
    * Check if a tcp link is active
    *
    * @returns true if successful
    */
    bool is_connected();

    /*
    * Read a character
    *
    * @return the character read
    */
    char getc();

    /*
    * Flush the buffer
    */
    void flush();

    /*
    * Write a character
    *
    * @param the character which will be written
    */
    int putc(char c);


    /*
    * To enter in command mode (we can configure the module)
    *
    * @return true if successful, false otherwise
    */
    bool cmdMode();

    /*
    * To exit the command mode
    *
    * @return true if successful, false otherwise
    */
    bool exit();

    /*
    * Close a tcp connection
    *
    * @ returns true if successful
    */
    bool close();

    /*
    * Send a command to the wify module. Check if the module is in command mode. If not enter in command mode
    *
    * @param str string to be sent
    * @param ACK string which must be acknowledge by the wifi module. If ACK == NULL, no string has to be acknoledged. (default: "NO")
    * @param res this field will contain the response from the wifi module, result of a command sent. This field is available only if ACK = "NO" AND res != NULL (default: NULL)
    *
    * @returns true if successful
    */
    bool sendCommand(const char * cmd, const char * ack = NULL, char * res = NULL, int timeout = DEFAULT_WAIT_RESP_TIMEOUT);

    bool dnsLookup(const char * host, char * ip);

    static Wifly * getInstance() {
        return inst;
    };

protected:
    Serial wifi;
    DigitalOut reset_pin;
    DigitalIn tcp_status;
    bool wpa;
    bool dhcp;
    char phrase[30];
    char ssid[30];
    const char * ip;
    const char * netmask;
    const char * gateway;
    int channel;
    CircBuffer<char> buf_wifly;
    Security security;
    char * getStringSecurity();

    bool cmd_mode;

    static Wifly * inst;

    void attach_rx(bool null);
    void handler_rx(void);
};

#endif