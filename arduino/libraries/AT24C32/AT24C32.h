#ifndef _AT24C32_H
#define _AT24C32_H

/**
 * Device is based on a 0101 (0x50) address and three address
 * lines, allowing up to eight devices on the one bus. (0x50 to 0x57).
 * The 32k version is 32k bits (4k bytes) organized in 8 bit bytes.
 * "pages" of 32 bytes can also be addressed in a single operation.
 */

const uint16_t WIRE_ERROR = 0xffff;

class AT24C32 {

  private:
    uint8_t device_address;


  public:
    AT24C32(int device = 0);
    void setAddress(uint16_t address);
    uint8_t readByte(uint16_t address);
    uint8_t readBytes(uint16_t address, void * data, uint8_t count);
    uint8_t readNextByte(void);

    uint8_t writeByte(uint16_t address, uint8_t data);
    uint16_t writeBytes(uint16_t address, void * data, uint8_t count);
};
#endif // _AT24C32_H
