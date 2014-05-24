#include "mbed.h"
#include "shared_buf.h"
#include "mmap.h"
#include "pinout.h"
#include "locks.h"

unsigned int read_pow(unsigned int req_gain_ctrl, JTAG* pJtag) {
    static char send_buffer[BUF_SIZE];
    float sen, mem1, mem2, core;
    if (req_gain_ctrl != 0 && req_gain_ctrl != 1) {
        return 0; // rtn error
    }
    else {            
        pow_sensr_mutex.lock();
        gain_ctrl = req_gain_ctrl;
        wait(1);
        
        sen = meas_sen.read();
        mem1 = meas_mem1.read();
        mem2 = meas_mem2.read();
        core = meas_core.read();       
        
        pow_sensr_mutex.unlock();
        sprintf(send_buffer, "%f,%f,%f,%f", sen, mem1, mem2, core);
        unsigned int len = jtag_readbuffer(send_buffer, pJtag);
        return len;
    }    
}