#include "mbed.h"

DigitalOut rx_led(LED1);
DigitalOut tx_led(LED4);
Serial pc(USBTX, USBRX);
CAN gmlan(p30, p29);

void clearAndHome()
{
    pc.printf("%c", 27);    // ESC
    pc.printf("[2J");       // clear screen
    pc.printf("%c", 27);    // ESC
    pc.printf("[H");        // cursor to home
}

void processMessage()
{
    time_t seconds = time(NULL);
    rx_led = 1;
    CANMessage rxmsg;
    gmlan.read(rxmsg);
    pc.printf("[%10d]\t[%d]\t[0x%08X]\t", seconds, rxmsg.len, rxmsg.id);
    for (unsigned int i = 0; i < rxmsg.len; i++)
        pc.printf("%02X ", rxmsg.data[i]);
    pc.printf("\r\n");
    rx_led = 0;
}

int main() {
    set_time(0);
    pc.baud(115200);
    int baudrate = 33333;
//    CANMessage rxmsg;
    
    // Put device in monitor mode - sniff only
    gmlan.monitor(true);
    gmlan.frequency(baudrate);
    
    clearAndHome();
    pc.printf("Starting packet capture at %i bps\r\n", baudrate);
    
    gmlan.attach(&processMessage);
    
    while(1) {
        wait(0.02);
    }
//        if (gmlan.read(rxmsg)) {
//            rx_led = 0;
//            pc.printf("Message: ");
//            for (unsigned int i = 0; i < rxmsg.len; i++)
//                pc.printf("[%d] 0x%X %02X ", time(NULL), rxmsg.id, rxmsg.data[i]);
//            pc.printf("\r\n");
//            rx_led = 0;
//        }
//        rx_led = 1;
//    }
}
