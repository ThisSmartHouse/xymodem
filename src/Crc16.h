#ifndef CRC16_H
#define CRC16_H

#include "Arduino.h"

class Crc16 {
   private:
        //Crc parameters
        uint16_t _msbMask;
        uint16_t _mask;
        uint16_t _xorIn;
        uint16_t _xorOut;
        uint16_t _polynomial;
        uint8_t _reflectIn;
        uint8_t _reflectOut;
        //Crc value
		uint16_t _crc;
		uint8_t reflect(uint8_t data, uint8_t bits = 32);

   public:
        inline Crc16()
        {
            //Default to XModem parameters
            _reflectIn = false;
            _reflectOut = false;
            _polynomial = 0x1021;
            _xorIn = 0x0000;
            _xorOut = 0x0000;
            _msbMask = 0x8000;
            _mask = 0xFFFF;
            _crc = _xorIn;
        }
        inline Crc16(uint8_t reflectIn, uint8_t reflectOut, uint16_t polynomial, uint16_t xorIn, uint16_t xorOut, uint16_t msbMask, uint16_t mask)
        {
            _reflectIn = reflectIn;
            _reflectOut = reflectOut;
            _polynomial = polynomial;
            _xorIn = xorIn;
            _xorOut = xorOut;
            _msbMask = msbMask;
            _mask = mask;
            _crc = _xorIn;
        }
        void clearCrc();
        void updateCrc(uint8_t data);
        uint16_t getCrc();
        unsigned int fastCrc(uint8_t data[], uint8_t start, uint16_t length, uint8_t reflectIn, uint8_t reflectOut, uint16_t polynomial, uint16_t xorIn, uint16_t xorOut, uint16_t msbMask, uint16_t mask);
        inline unsigned int XModemCrc(uint8_t data[], uint8_t start, uint16_t length)
		{
            //  XModem parameters: poly=0x1021 init=0x0000 refin=false refout=false xorout=0x0000
            return fastCrc(data, start, length, false, false, 0x1021, 0x0000, 0x0000, 0x8000, 0xffff);
		}
};

#endif
