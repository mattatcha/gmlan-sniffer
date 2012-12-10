#include "mbed.h"
#include <vector>
#include <algorithm>

DigitalOut rx_led(LED1);
DigitalOut filter_led(LED4);
Serial pc(USBTX, USBRX);
CAN gmlan(p30, p29);
Timer uptime;
bool filterPackets = false;
vector<int> filteredPackets;
extern "C" void mbed_reset();

namespace mbed {
    class CANHeader {
        // Example header packet for Steering Wheel Switches:
        // Hexadecimal:    0x10     0x0D     0x00     0x60
        // Binary:       00010000 00001101 00000000 01100000
        // Priority:        ---
        // Arbitration:        -- -------- ---
        // Sending ECU:                       ----- --------

        private:
            int priorityID, arbitrationID, senderID;
        
        public:
            int priority(void) { return priorityID; }
            void priority(int _priority) { priorityID = _priority; }
            int arbitration(void) { return arbitrationID; }
            void arbitration(int _arbitration) { arbitrationID = _arbitration; }
            int sender(void) { return senderID; }
            void sender(int _sender) { senderID = _sender; }
            
            void decode(int _header) {
                priorityID = (_header >> 26) & 0x7;
                arbitrationID = (_header >> 13) & 0x1FFF;
                senderID = (_header >> 0)  & 0x1FFF;
            }
            int encode(void) {
                long int buffer = 0;
                buffer = (buffer << 3) | 0x0; // 3 bit padding
                buffer = (buffer << 3) | priorityID;
                buffer = (buffer << 13) | arbitrationID;
                buffer = (buffer << 13) | senderID;
                return buffer;
            }
    };
}

void processMessage()
{
    // Turn on rx_led to indicate we are receiving data
    rx_led = 1;
    
    // Create a CANMessage object and read the buffer into it
    CANMessage rxmsg;
    gmlan.read(rxmsg);
    
    // Check to see if our header is in the filtered packet list or we are filtering, if so we skip over
    bool packetFound = false;
    if (find(filteredPackets.begin(), filteredPackets.end(), rxmsg.id) != filteredPackets.end())
        packetFound = true;
        
    // If we are filtering packets and this one isn't found, add it to the list
    if (filterPackets == true && packetFound == false)
    {
        filteredPackets.push_back(rxmsg.id);
        pc.printf("Added 0x%08X to filter at position %u\r\n", rxmsg.id, filteredPackets.size());
    }
    else if (filterPackets == false && packetFound == false) {
        CANHeader rxHeader;
        rxHeader.decode(rxmsg.id);
        
        // Print these results to the serial terminal using printf to format
        // Ref: http://www.cplusplus.com/reference/cstdio/printf/
        pc.printf("[%10.2f] ", uptime.read());            // Seconds (to 2 decimal places) since start
        pc.printf("[0x%1X] ", rxHeader.priority());       // Packet priority (0-7, 0 being highest)
        pc.printf("[0x%4X] ", rxHeader.arbitration());    // Arbitration ID (0x0 to 0x1FFF)
        pc.printf("[0x%4X] ", rxHeader.sender());         // ECU ID (0x0 to 0x1FFF)
        pc.printf("[0x%08X] ", rxmsg.id);                 // Full header for legacy reasons (4 bytes)
        pc.printf("[%d] ", rxmsg.len);                    // Length of message (0-8 bytes typically)
        
        // Process actual message here, only run this if we have a message
        if (rxmsg.len > 0)
        {
            pc.printf("[%02X", rxmsg.data[0]);  // Print first byte
            for (unsigned int i = 1; i < rxmsg.len; i++)
            pc.printf(" %02X", rxmsg.data[i]);  // Print additional byte(s) with a preceeding space
            pc.printf("]");                     // Print closing bracket
        }
        pc.printf("\r\n");                      // Print carriage return and newline
    }
        
    // Turn off our rx_led as we have finished reading data
    rx_led = 0;
}

void clearAndHome()
{
    pc.printf("%c", 27);    // ESC
    pc.printf("[2J");       // clear screen
    pc.printf("%c", 27);    // ESC
    pc.printf("[H");        // cursor to home
}

int main() {
    // Set serial baud rate to 115kbit
    pc.baud(115200);
    
    // Set CANBUS to 33.3kbit and put into monitor mode (does not ACK packets, aka stealth mode)
    int baudrate = 33333;
    gmlan.frequency(baudrate);
    gmlan.monitor(false);
    
    // Clear serial terminal
    clearAndHome();
    pc.printf("Keys: F = start/stop filter capture, D = display filtered headers, C = clear filter\r\n");
    pc.printf("Starting packet capture at %i bps\r\n\r\n", baudrate);
    
    // Start capturing packets
    uptime.start();
    gmlan.attach(&processMessage);
    
    while(1) {
        // Poll serial for keypresses for certain tasks
        if (pc.readable())
        {
            switch (pc.getc())
            {
                case 'f':
                case 'F':
                {
                    // Toggle filter
                    filterPackets = !filterPackets;
                    filter_led = (int)filterPackets;
                    break;
                }
                case 'c':
                case 'C':
                {
                    // Clear filter
                    filteredPackets.clear();
                    pc.printf("Packet filter cleared\r\n");
                    break;
                }
                case 'd':
                case 'D':
                {
                    // Show filter
                    clearAndHome();
                    pc.printf("[%u] entries in filter:\r\n", filteredPackets.size());
                    for (int i=0; i < filteredPackets.size(); i++)
                        pc.printf("%u: 0x%08X ", i+1, filteredPackets[i]);
                    pc.printf("\r\n");
                    break;
                }
                case 'r':
                case 'R':
                {
                    // Restart mbed
                    pc.printf("Restarting mbed...\r\n");
                    mbed_reset();
                    break;
                }
                default:
                    pc.printf("Unknown keypress!\r\n");
                    break;
            }
        }       
        // Sleep for 20ms repeatedly, as all messages are handled by an interrupt, this prevents
        // keeping the mbed at full load
        wait(0.02);
    }
}