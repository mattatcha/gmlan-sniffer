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

int main() {
    pc.baud(115200);
    CANMessage rxmsg;
    int baudrate = 33300;
    
    // Put device in monitor mode - sniff only
    gmlan.monitor(true);
    gmlan.frequency(baudrate);
    
    clearAndHome();
    pc.printf("Starting packet capture at %i bps\r\n", baudrate);
    
    while(1) {
        if (gmlan.read(rxmsg)) {
            rx_led = 0;
            pc.printf("Message: ");
            for (unsigned int i = 0; i < rxmsg.len; i++)
                pc.printf("%02X ", rxmsg.data[i]);
            pc.printf("\r\n");
            rx_led = 0;
        }
        rx_led = 1;
    }
}
