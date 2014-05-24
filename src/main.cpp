#include "mbed.h"
#include "rtos.h"
#include "jtag.h"
#include "pinout.h"
#include "main.h"
#include "mmap.h"
#include "panic.h"
#include "init_hw.h"
#include "basic_io.h"
#include "signal.h"
#include "locks.h"
#include "power.h"
#include "GPIOInterrupt.h"
#include "wireless_wifi.h"
#include "Ethernet.h"
#include "power_sensor.h"

JTAG *jtag;

void dispatch_loop(void const *args);
void data_send_loop(void const *args); 
void data_recv_loop(void const *args);
void read_pow_loop(void const *args);
void debug_loop(void const *args);

Thread *dispatch_thread_ptr;
Thread *data_send_thread_ptr;
Thread *data_recv_thread_ptr;
Thread *read_pow_thread_ptr;

unsigned int data_send_type;
unsigned int data_recv_type;
unsigned int wireless_recv_size;
unsigned int enet_recv_size; 
unsigned int req_gain_ctrl;

void req_intr_handler(void);

int main() {
    JTAG jt;
    jtag = &jt;
    init_hw();  
    
    // create working threads first
    Thread dispatch_thread(dispatch_loop, NULL);
    Thread data_send_thread(data_send_loop, NULL);
    Thread data_recv_thread(data_recv_loop, NULL);
    Thread debug_thread(debug_loop, NULL); 
    Thread read_pow_thread(read_pow_loop, NULL);
    
    dispatch_thread_ptr = &dispatch_thread;   
    data_send_thread_ptr = &data_send_thread;
    data_recv_thread_ptr = &data_recv_thread;
    read_pow_thread_ptr = &read_pow_thread;
    
    // enable GPIO interrupt
    enable_GPIO_intr(&req_intr_handler);
    
    jtag->reset();  
    jtag->leaveState();
    jtag->PowerupDAP();  
    // setup necessary internal clock source selection
    jtag->writeMemory(intclk_source, 2);
    jtag->writeMemory(extclk_source, 1);
    jtag->writeMemory(ext_div_by, 10);
    power_core(1);    
    
    // Begin to load program
    mbed_printf("Begining loading program.\r\n"); 

    if (jtag->loadProgram()) {  
        mbed_printf("Load Failed!\r\n");
    } else {   
        mbed_printf("Load Succeed!\r\n");     
    
        // Reset M3 chip
        CORERESETn = 0;  
        CORERESETn = 1; 
        
        char line[80]; 
        while (1) {  
            //printf("Type 'quit' to quit.\r\n"); 
            mbed_scanf("%s", line);
            if (strncmp(line, "quit", 80) == 0)
                break;
            else if (strncmp(line, "debug", 80) == 0);
                debug_thread.signal_set(SIG_DEBUG);  
        }
    }

    dispatch_thread.terminate();
    data_send_thread.terminate();
    data_recv_thread.terminate();
    read_pow_thread.terminate();
    debug_thread.terminate();

    jtag->reset();

    mbed_printf("Powering Down\r\n");
    power_down();
    mbed_printf("Done.\r\n");
    while (1) 
        ; 
}

void req_intr_handler(void){
    static bool init = true;
    if (init) {
        jtag->reset();
        jtag->leaveState();
        jtag->PowerupDAP();
        init = false;
    }
    dispatch_thread_ptr->signal_set(SIG_DISPATCH); 
    return; 
}

void dispatch_loop(void const *args) {
    while (1) {  
        Thread::signal_wait(SIG_DISPATCH);
        JTAG_mutex.lock();
        // check request type and wake up corresponding threads
        unsigned int type = jtag->readMemory(IO_TYPE);
        unsigned int val = jtag->readMemory(READBUF_BEGIN);
        jtag->writeMemory(IO_TYPE, EMPTY_REQ);
        JTAG_mutex.unlock();
        
        switch(type) {
        case PANIC_REQ:
            //fall through
        case TERM_PRINT_REQ:
            data_send_type = SEND_TYPE_TERM;
            data_send_thread_ptr->signal_set(SIG_DATA_SEND);
            break;
        case TERM_SCAN_REQ:  
            data_recv_type = RECV_TYPE_TERM;
            data_recv_thread_ptr->signal_set(SIG_DATA_RECV);
            break;
        case ENET_SEND_REQ:
            data_send_type = SEND_TYPE_ENET;
            data_send_thread_ptr->signal_set(SIG_DATA_SEND);
            break;
        case ENET_RECV_REQ:
            data_recv_type = RECV_TYPE_ENET;
            enet_recv_size = val;
            data_recv_thread_ptr->signal_set(SIG_DATA_RECV);
            break; 
        case WIRELESS_SEND_REQ: 
            data_send_type = SEND_TYPE_WIRELESS;
            data_send_thread_ptr->signal_set(SIG_DATA_SEND);            
            break;
        case WIRELESS_RECV_REQ:
            data_recv_type = RECV_TYPE_WIRELESS;
            wireless_recv_size = val;
            data_recv_thread_ptr->signal_set(SIG_DATA_RECV);
            break;
        case READ_POW_REQ:
            req_gain_ctrl = val;
            read_pow_thread_ptr->signal_set(SIG_POW_READ);
            break;
        default:
            mbed_printf("Unsupported request: %08x\r\n", type);
            continue;
        }
    }
}

void data_send_loop(void const *args) {
    unsigned int rtn;
    while (1) {
        Thread::signal_wait(SIG_DATA_SEND);
        switch (data_send_type) {
            case SEND_TYPE_TERM :
                rtn = term_printf(jtag);
                JTAG_mutex.lock();
                jtag->writeMemory(ACK_RETURN_VAL, rtn);
                jtag->writeMemory(ACK_TYPE, PRINT_ACK);
                // generate acknowledge signal
                ack_intr = 1;
                ack_intr = 0;
                JTAG_mutex.unlock();  
                break;
                
            case SEND_TYPE_ENET :
                rtn = enet_send(jtag);
                JTAG_mutex.lock();
                jtag->writeMemory(ACK_RETURN_VAL, rtn);
                jtag->writeMemory(ACK_TYPE, ENET_SEND_ACK);
                // generate acknowledge signal
                ack_intr = 1;
                ack_intr = 0;
                JTAG_mutex.unlock(); 
                break;
                
            case SEND_TYPE_WIRELESS :
                rtn = wireless_send(jtag); 
                JTAG_mutex.lock();
                jtag->writeMemory(ACK_RETURN_VAL, rtn);
                jtag->writeMemory(ACK_TYPE, WIRELESS_SEND_ACK);            
                // generate acknowledge signal
                ack_intr = 1; 
                ack_intr = 0;  
                JTAG_mutex.unlock(); 
                break;
                
            default:
                panic("Unsupported data send request\r\n");
        }  
    }
}

void data_recv_loop(void const *args) {
    unsigned int rtn;
    while (1) {
        Thread::signal_wait(SIG_DATA_RECV);
        switch (data_recv_type) {
            case RECV_TYPE_TERM :
                rtn = term_scanf(jtag);
                JTAG_mutex.lock();
                jtag->writeMemory(ACK_RETURN_VAL, rtn);
                jtag->writeMemory(ACK_TYPE, SCAN_ACK);
                // generate acknowledge signal
                ack_intr = 1;
                ack_intr = 0;
                JTAG_mutex.unlock();
                break;
                
            case RECV_TYPE_ENET :
                rtn = enet_recv(jtag, enet_recv_size);
                JTAG_mutex.lock();
                jtag->writeMemory(ACK_RETURN_VAL, rtn);
                jtag->writeMemory(ACK_TYPE, ENET_RECV_ACK);
                // generate acknowledge signal
                ack_intr = 1;
                ack_intr = 0;
                JTAG_mutex.unlock();
                break;
                
            case RECV_TYPE_WIRELESS :
                rtn = wireless_recv(jtag, wireless_recv_size);        
                JTAG_mutex.lock();
                jtag->writeMemory(ACK_RETURN_VAL, rtn);        
                jtag->writeMemory(ACK_TYPE, WIRELESS_RECV_ACK);
                
                // generate acknowledge signal
                ack_intr = 1;
                ack_intr = 0;
                JTAG_mutex.unlock();
                break;
                
            default:
                panic("Unsupported data recv request\r\n");
        }  
    }
}

void read_pow_loop(void const *args) {
    unsigned int rtn;
    while (1) {
        Thread::signal_wait(SIG_POW_READ);
        rtn = read_pow(req_gain_ctrl, jtag);       
        JTAG_mutex.lock();
        jtag->writeMemory(ACK_RETURN_VAL, rtn);     
        jtag->writeMemory(ACK_TYPE, READ_POW_ACK);
        
        // generate acknowledge signal
        ack_intr = 1;
        ack_intr = 0;
        JTAG_mutex.unlock();
    }
}

void debug_loop(void const *args) {
    static bool init = true;
    while (1) {
        Thread::signal_wait(SIG_DEBUG);
        if (init) {
            JTAG_mutex.lock();
            jtag->reset();
            jtag->leaveState();
            jtag->PowerupDAP();
            init = false;
            JTAG_mutex.unlock();
        }
        debug_print(jtag);
    }
}
