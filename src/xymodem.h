/*
  +----------------------------------------------------------------------+
  | XYModem                                                              |
  +----------------------------------------------------------------------+
  | Copyright (c) 2018 John Coggeshall                                   |
  +----------------------------------------------------------------------+
  | Licensed under the Apache License, Version 2.0 (the "License");      |
  | you may not use this file except in compliance with the License. You |
  | may obtain a copy of the License at:                                 |
  |                                                                      |
  | http://www.apache.org/licenses/LICENSE-2.0                           |
  |                                                                      |
  | Unless required by applicable law or agreed to in writing, software  |
  | distributed under the License is distributed on an "AS IS" BASIS,    |
  | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      |
  | implied. See the License for the specific language governing         |
  | permissions and limitations under the License.                       |
  +----------------------------------------------------------------------+
  | Authors: John Coggeshall <john@thissmarthouse.com>                   |
  +----------------------------------------------------------------------+
*/
#ifndef YMODEM_H_INC
#define YMODEM_H_INC

#include "Arduino.h"
#include <SoftwareSerial.h>
#include <FS.h>
#include "Crc16.h"

#define debug_print(data) \
		do { if(this->debug != NULL) this->debug->print(data); this->debug->flush(); } while(0);

#define debug_print_hex(data) \
		do { if(this->debug != NULL) this->debug->print("0x"); this->debug->print(data, HEX); this->debug->print(" "); this->debug->flush();} while(0);

#define debug_println(data) \
		do { if(this->debug != NULL) this->debug->println(data); this->debug->flush(); } while(0);

#define SOH 			0x01
#define STX 			0x02
#define EOT 			0x04
#define ACK 			0x06
#define NAK 			0x15
#define CAN  			0x18
#define CRC16 			0x43

#define ABORT			0x41
#define ABORT_ALT 		0x61

class XYModem
{
	public:

		enum Protocol {
			XMODEM,
			YMODEM,
			UNKNOWN
		};

		enum ChecksumType {
			OLD,
			CRC_16,
			NONE
		};

		XYModem(Stream &serial, XYModem::Protocol mode);
		XYModem(Stream &serial, XYModem::Protocol mode, SoftwareSerial &debug);

		bool transmit(File &file);

	private:

		SoftwareSerial *debug = NULL;
		XYModem::Protocol protocol;
		Stream *stream;
		XYModem::ChecksumType checksum_type;

		Crc16 crc;
		uint8_t packetNumber = 0x00;

		void hexDump (char *desc, void *addr, int len);
		void flushReadBuffer();
		uint8_t sendPayload(uint8_t payload[], size_t length, uint8_t packetType);
		uint8_t receiveResponse();
		bool endFileTransmission();
		XYModem::ChecksumType determineChecksum();
};

#endif
