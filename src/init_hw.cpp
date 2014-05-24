#include "mbed.h"
#include "power.h"
#include "mmap.h"
#include "wireless_wifi.h"
#include "Ethernet.h"
#include "panic.h"

extern JTAG* jtag;

void init_hw(void) {
   
    // Power and Reset Hardware
    float core_volt = 1;  
    power_down();  
    power_up(core_volt); // Power Up Chip
    printf("Powered up!\r\n");

    PORESETn = 0;
    CORERESETn = 0; 
    wait_us(100);
    PORESETn = 1;
    CORERESETn = 1;
    //set_pll_frequency (200, jtag); // default internal frequency is 80MHz   
    
    // We check if JTAG works well by checking idcode returned by issuing a test instruction and read result idcode back.
    jtag->reset();  
    jtag->leaveState(); 
    int idcode = jtag->readID();  
    if(idcode != 0x4ba00477) {
        panic("ERROR: IDCode %X\r\n", idcode);  
    }
    
    printf("==== Wifi initialization =================== \r\n");
    scan_data_in = 1; // need this?
    scan_phi = 1;
    //init_wifi();

    printf("\r\n\r\n==== Ethernet initialization =================== \r\n");
    init_ethernet();
}