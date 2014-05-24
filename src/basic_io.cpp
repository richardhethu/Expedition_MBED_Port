/*
 * Implementation of a family of print and scan functions
 *
 * Written by Zimin Wang
 * Revised by Ruining He
 */
 
#include <stdarg.h>
#include <stdio.h>
#include "mbed.h"
#include "mmap.h"
#include "signal.h"
#include "pinout.h"
#include "basic_io.h"
#include "locks.h"
#include "shared_buf.h"

// buffer for read from or write to terminal
static char term_txbuffer[BUF_SIZE];
static char term_rxbuffer[BUF_SIZE];

// Send buffer to stdout.
static void term_sendbuffer(const char *buffer, size_t size);

// Read stdin and write a maximum of size bytes data to buffer.
static void term_readbuffer(char *buffer, size_t size);

/** MBED printf() **/
int mbed_printf(const char *format, ...) {
    int size;
    va_list args;
    
    va_start(args, format); 
    size = mbed_vprintf(format, args);
    va_end(args);
       
    return size; 
}

int mbed_vprintf(const char *format, va_list args) {
    int size;
    
    // tx_line is in critical section. We need a mutex to protect it.
    term_write_mutex.lock();     
    size = vsnprintf(term_txbuffer, BUF_SIZE, format, args);
    term_sendbuffer(term_txbuffer, BUF_SIZE);
    term_write_mutex.unlock();
    
    return size;
}

/** MBED scanf() **/
int mbed_scanf(const char *format, ...) {
    int size;
    va_list args;
    
    va_start(args, format);
    size = mbed_vscanf(format, args);
    va_end(args);
    
    return size;
}

int mbed_vscanf(const char *format, va_list args) {
    int size;
    
    // term_rxbuffer is in critical section, we need a mutex to protect it.
    term_read_mutex.lock();
    term_readbuffer(term_rxbuffer, BUF_SIZE);
    size = vsscanf(term_rxbuffer, format, args);
    term_read_mutex.unlock();
    return size;   
}


/** Term printf() service **/
int term_printf(JTAG *pJtag) {
    int len;
    
    // acquire mutex to protect term_txbuffer
    term_write_mutex.lock();
    
    // read data from ram buffer
    len = jtag_writebuffer(pJtag, term_txbuffer);  
    term_sendbuffer(term_txbuffer, BUF_SIZE);
    
    // release locks
    term_write_mutex.unlock();
    return len;
}

/** Term scanf() service **/
int term_scanf(JTAG *pJtag) {
    int len;
    // acquire locks to protect term_rxbuffer
    term_read_mutex.lock();
    
    // read from terminal and write to ram buffer
    term_readbuffer(term_rxbuffer, BUF_SIZE);
    len = jtag_readbuffer(term_rxbuffer, pJtag);
   
    // release locks
    term_read_mutex.unlock();
    return len;
}

void debug_print(JTAG *pJtag) {
    // print the content of ram buffer
    unsigned int value;
    
    for (unsigned int i = WRITEBUF_BEGIN; i < WRITEBUF_END; i+=4) {
        value = pJtag->readMemory(i);
        mbed_printf("%08x\r\n", value);
    }
    mbed_printf("\r\n");
    for (unsigned int i = READBUF_BEGIN; i < READBUF_END; i+=4) {
        value = pJtag->readMemory(i);
        mbed_printf("%08x\r\n", value);
    }
}

static void term_sendbuffer(const char *buffer, size_t size) {
    int i;
    
    i = 0;
    while (i < size) {
        if (buffer[i] != '\0' && pc.writeable()) {
            pc.putc(buffer[i++]);
        } else if (buffer[i] == '\0') {
            break;
        }
    }
    return;
}

static void term_readbuffer(char *buffer, size_t size) {
    int i;
    char temp;

    i = 0;
    while (i < size-1) { // save for null character
        if (pc.readable()) {
            temp = pc.getc();
            if (temp == '\r')
                break;
            else
                buffer[i++] = temp;
        }
    }
    buffer[i] = '\0';
    return;
}

