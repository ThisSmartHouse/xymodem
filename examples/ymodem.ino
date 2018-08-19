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

#include "Arduino.h"
#include <SoftwareSerial.h>
#include <FS.h>
#include <xymodem.h>

/*
 To use this example, set up your computer to receieve a file
 via YMODEM. For example, if you are on a Linux-like environment
 you can use the 'rz' command from the terminal like so:

 rz --timeout=1000 --ymodem -b 1> /dev/cu.SLAB_USBtoUART 0< /dev/cu.SLAB_USBtoUART

*/ 

/* For debugging during transmission, you can setup a separate
   software serial connection via D0, D1 (NodeMCU) if you want */
SoftwareSerial swSer(D0, D1, false, 256);
XYModem *protocol;

void setup()
{
	/* Make sure this matches your baud rate for rz */
	Serial.begin(9600);
	swSer.begin(115200);

	while(!Serial) {
		/* waiting.. */
	}

	protocol = new XYModem(Serial, XYModem::YMODEM, swSer);

	swSer.println("Booting");

	/* Let's create a file to transmit real fast and store in SPIFFS... */
	SPIFFS.begin();
	SPIFFS.format();

	swSer.println("SPIFFS Formatted");

	File f = SPIFFS.open("/testing.txt", "w");

	if(!f) {
		swSer.println("Failed to open testing file");
	}

	swSer.println("Creating Dummy Testing File");

	for(int i = 0; i < 1000; i++) {
		f.println(i);
	}

	f.close();
}

void loop()
{
	/* Open our file, and transmit! */
	File f = SPIFFS.open("/testing.txt", "r");

	if(!f) {
		swSer.println("Failed to open testing file");
		return;
	}

	swSer.println("Opened testing file for reading");

	if(!protocol->transmit(f)) {
		swSer.println("Transmission Failed.");
		while(true) {
			yield();
		}
	}

	swSer.println("Transmission Successful!");
	while(true) {
		yield();
	}

}

