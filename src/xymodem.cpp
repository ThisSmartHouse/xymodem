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
#include "xymodem.h"

XYModem::XYModem(Stream &serial, XYModem::Protocol mode)
{
	this->debug = NULL;
	this->protocol = mode;
	this->stream = &serial;
	this->checksum_type = ChecksumType::NONE;

}

XYModem::XYModem(Stream &serial, XYModem::Protocol mode, SoftwareSerial &debug)
{
	this->debug = &debug;
	this->protocol = mode;
	this->stream = &serial;
	this->checksum_type = ChecksumType::NONE;

	switch(mode) {
		case Protocol::XMODEM:
			debug_println("Setting Protocol to XMODEM");
			break;
		case Protocol::YMODEM:
			debug_println("Setting Protocol to YMODEM");
			break;
		default:
			debug_println("Unknown Protocol!");
			break;
	}
}

void XYModem::hexDump (char *desc, void *addr, int len) {

    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    if(debug == NULL) {
    	return;
    }

    // Output description if given.
    if (desc != NULL)
        debug->printf ("%s:\r\n", desc);

    if (len == 0) {
        debug->printf("  ZERO LENGTH\r\n");
        return;
    }
    if (len < 0) {
        debug->printf("  NEGATIVE LENGTH: %i\r\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                debug->printf ("  %s\r\n", buff);

            // Output the offset.
            debug->printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        debug->printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        debug->printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    debug->printf ("  %s\r\n", buff);
}

uint8_t XYModem::receiveResponse()
{
	uint8_t inByte;
	uint16_t timeout = 5000;
	do {
		delay(100);
		timeout -= 100;
	} while((stream->available() <= 0) && (timeout > 0));

	if(timeout <= 0) {
		return 0;
	}

	stream->readBytes(&inByte, 1);

	return inByte;
}

void XYModem::flushReadBuffer()
{
	uint8_t inByte;

	while(stream->available() > 0) {
		stream->readBytes(&inByte, 1);
	}

}

XYModem::ChecksumType XYModem::determineChecksum()
{
	debug_println("Determing Checksum Type for Destination...");

	switch(receiveResponse()) {
		case CRC16:
			checksum_type = CRC_16;
			debug_println("Checksum Type is CRC-16");
			break;
		case NAK:
			checksum_type = OLD;
			debug_println("Checksum Type is Byte Count");
			break;
		case 0:
		default:
			debug_println("ERROR: Could not determine Checksum Type!");
			return NONE;
	}

	return checksum_type;
}

uint8_t XYModem::sendPayload(uint8_t payload[], size_t length, uint8_t packetType)
{
	uint8_t modemPacket[1029];
	uint16_t crcVal;
	uint16_t payloadLength = 0x0;
	size_t writtenLength;
	uint8_t retries = 0;
	uint8_t responseByte;

	do {

		flushReadBuffer();

		debug_print("Sending Packet #");
		debug_println(packetNumber);

		switch(packetType) {
			case SOH:
				payloadLength = 128;
				break;
			case STX:
				payloadLength = 1024;
				break;
			default:
				debug_println("Invalid Packet Type");
				return 0;
		}

		if(length > payloadLength) {
			debug_println("Specified length is larger than maximum payload length");
			return 0;
		}

		memset(&modemPacket[0], 0x0, sizeof(modemPacket));

		modemPacket[0] = packetType;
		modemPacket[1] = packetNumber;
		modemPacket[2] = ~packetNumber;

		memcpy(&modemPacket[3], payload, length);

		crcVal = crc.XModemCrc(&modemPacket[3], 0, payloadLength);

		modemPacket[payloadLength -2 + 5] = (uint8_t)(crcVal >> 8) & 0xFF;
		modemPacket[payloadLength -1 + 5] = (uint8_t)(crcVal & 0xFF);

		this->hexDump(NULL, (void *)&modemPacket[0], payloadLength + 5);

		writtenLength = stream->write(modemPacket, payloadLength + 5);


		if(writtenLength != (payloadLength + 5)) {
			debug_print("Error Writing Packet! Wrote ");
			debug_print(writtenLength);
			debug_print(" instead of ");
			debug_print(sizeof(modemPacket));
			debug_println(" bytes(s)");
			return 0;
		}

		responseByte = receiveResponse();

		switch(responseByte) {
			case ACK:
				debug_println("Packet Acknowledged!");
				packetNumber++;
				break;
			case NAK:
				debug_println("Packet Not Acknowledged, Retrying...");
				retries++;
				break;
			case CAN:
				responseByte = receiveResponse();

				if(responseByte == CAN) {
					debug_println("Target Cancelled Transmission.");
					return CAN;
				}
				break;
			case CRC16:
				debug_println("Got another CRC16 announcement, Retrying...");
				retries++;
				break;
			default:
				debug_println("Unknown Response Byte Received");
				this->hexDump(NULL, &responseByte, 1);
				retries++;
				break;
		}

	} while((responseByte != ACK) && (retries <= 10));

	return responseByte;
}

bool XYModem::endFileTransmission()
{
	uint8_t eot = EOT;
	uint8_t responseByte;
	uint8_t retries = 0;
	uint8_t payload = 0x0;

	debug_println("Ending Transmission");

	do {
		stream->write(&eot, 1);
		responseByte = receiveResponse();
		retries++;
	} while((responseByte != ACK) && (retries <= 10));

	if(responseByte != ACK) {
		debug_println("Failed to receive an ACK for our EOT");
		return false;
	}

	debug_println("Received ACK for EOT");

	if(protocol == Protocol::YMODEM) {
		// Eat up the next checksum announcment because YModem expects another file to transfer
		determineChecksum();

		// Send a NULL filename
		packetNumber = 0;
		if(sendPayload(&payload, sizeof(payload), SOH) != ACK) {
			debug_println("Failed to close YMODEM Session");
			return false;
		}

	}



	return true;
}

bool XYModem::transmit(File & file)
{
	String filename, fileSize;
	uint8_t payload[1024];
	packetNumber = 0;

	if(determineChecksum() == NONE) {
		return false;
	}

	if(protocol == Protocol::YMODEM) {

		packetNumber = 0;

		filename = file.name();
		filename = filename.substring(filename.lastIndexOf('/') + 1);
		fileSize = file.size();

		memcpy(&payload[0], filename.c_str(), filename.length());
		memcpy(&payload[filename.length() + 1], fileSize.c_str(), fileSize.length());

		if(sendPayload(&payload[0], filename.length() + fileSize.length() + 2, STX) != ACK) {
			debug_println("Failed to receive ACK for Packet..");
			return false;
		}

	} else {
		packetNumber = 1;
	}

	file.seek(0, SeekMode::SeekSet);

	while(file.position() < file.size()) {

		uint16_t readSize = (file.size() - file.position() >= sizeof(payload)) ? sizeof(payload) : (file.size() - file.position());

		memset(&payload[0], 0x0, sizeof(payload));

		debug_print("Reading ");
		debug_print(readSize);
		debug_println(" byte(s) from source file");

		file.read(&payload[0], readSize);

		if(sendPayload(&payload[0], readSize, STX) != ACK) {
			debug_println("Failed to recieve ACK for Packet...");
			return false;
		}

	}

	return endFileTransmission();
}
