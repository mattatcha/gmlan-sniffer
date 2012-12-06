#include "mbed.h"

DigitalOut rx_led(LED1);
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
    // Get system time and turn on rx_led to indicate we are receiving data
    time_t seconds = time(NULL);
    rx_led = 1;
    
    // Create a CANMessage object and read the buffer into it
    CANMessage rxmsg;
    gmlan.read(rxmsg);
    
    // Output to the USB serial device formatted data, walking through and formatting each packet
    // http://www.cplusplus.com/reference/cstdio/printf/ is a useful resource to describe printf
    pc.printf("[%10d]\t[%d]\t[0x%08X]\t", seconds, rxmsg.len, rxmsg.id);
    for (unsigned int i = 0; i < rxmsg.len; i++)
        pc.printf("%02X ", rxmsg.data[i]);
    pc.printf("\r\n");
    
    // Turn off our rx_led as we have finished reading data
    rx_led = 0;
}

int main() {
    // Reset uptime timer and set serial baud rate to 115kbit
    set_time(0);
    pc.baud(115200);
    
    // Set CANBUS to 33.3kbit and put into monitor mode (does not ACK packets, aka stealth mode)
    int baudrate = 33333;
    gmlan.frequency(baudrate);
    gmlan.monitor(true);
    
    // Clear serial terminal and start capturing packets
    clearAndHome();
    pc.printf("Starting packet capture at %i bps\r\n", baudrate);
    gmlan.attach(&processMessage);
    
    while(1) {
        // Sleep for 20ms repeatedly, all messages are handled by an interrupt, this prevents keeping the mbed at full load
        wait(0.02);
    }
}
