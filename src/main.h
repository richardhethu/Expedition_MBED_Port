#ifndef MAIN_H
#define MAIN_H


extern "C" void mbed_reset();
/*
extern "C" void HardFault_Handler() 
{   
    mbed_reset(); 
}*/

extern "C" void HardFault_Handler() {
    pc.printf("In Hard Fault Handler\r\n");
     static char msg[80];
   sprintf(msg, "SCB->HFSR = 0x%08x\n", SCB->HFSR);
   pc.printf(msg);
   if ((SCB->HFSR & (1 << 30)) != 0) {
       pc.printf("Forced Hard Fault\n");
       sprintf(msg, "SCB->CFSR = 0x%08x\n", SCB->CFSR );
       pc.printf(msg);
   }
   while(1);
}
extern "C" void NMI_Handler() {
    pc.printf("NMI Fault!\n");
    //NVIC_SystemReset();
}
extern "C" void MemManage_Handler() {
    pc.printf("MemManage Fault!\n");
    //NVIC_SystemReset();
}
extern "C" void BusFault_Handler() {
    pc.printf("BusFault Fault!\n");
    //NVIC_SystemReset();
}
extern "C" void UsageFault_Handler() {
    pc.printf("UsageFault Fault!\n");
    //NVIC_SystemReset();
}


class Watchdog {
public:
// Load timeout value in watchdog timer and enable
    void kick(float s) {
        LPC_WDT->WDCLKSEL = 0x1;                // Set CLK src to PCLK  
        uint32_t clk = SystemCoreClock / 16;    // WD has a fixed /4 prescaler, PCLK default is /4
        LPC_WDT->WDTC = s * (float)clk;
        LPC_WDT->WDMOD = 0x3;                   // Enabled and Reset
        kick();
    }
// "kick" or "feed" the dog - reset the watchdog timer
// by writing this required bit pattern
    void kick() {
        LPC_WDT->WDFEED = 0xAA;
        LPC_WDT->WDFEED = 0x55;
    }
};
Watchdog wdt;

#endif