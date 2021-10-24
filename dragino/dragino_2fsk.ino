//Modified version of the SX127x_FSK_Modem sketch

// include the library
#include <RadioLib.h>

// SX1276 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3 // conflicts with 6 below, and where is 7?

// https://github.com/matthijskooijman/arduino-lmic/issues/59

/*
 * // Pin mapping for Dragino Board Version 1.3 (open J_DIO5)
 * const lmic_pinmap lmic_pins = {
 *   .nss = 10,
 *   .rxtx = LMIC_UNUSED_PIN,
 *   .rst = 9,
 *   .dio = {2, 6, 7},  // Specify pin numbers for DIO0, 1, 2
 *                      // connected to D2, D6, D7
 * };
 *
 * See also https://wiki.dragino.com/index.php?title=Lora_Shield
 *
 * - DIO 0 on Arduino D2
 * - DIO 1 on Arduino D6
 * - DIO 2 on Arduino D7
 * - DIO 5 on Arduino D8
 *
 * DIO0 is used for TX and DIO1 for RX in LoRA mode. DIO2 is used for RX in FSK mode.
 */

// TODO: Check with the logic what is happening on those pins.
int pin_nss   = 10;
int pin_reset =  9;
int pin_dio0  =  2;
int pin_dio1  =  6;
int pin_dio2  =  7;

// Global radio object
SX1276 radio = new Module(pin_nss, pin_dio0, pin_reset, pin_dio2);

// Global buffer
const int buflen = 20;
uint8_t* data = new uint8_t[buflen+1];

/*
 * See https://github.com/mrquincle/RadioLib/blob/master/src/modules/SX127x/SX1276.h for a cloned
 * repository with additional information on the use of this lib and the parameters.
 *
 * FSK modem initialization method.
 * Must be called at least once from Arduino sketch to initialize the module.
 *
 * Parameters
 *   - freq            Carrier frequency in MHz. Allowed values range from 137.0 MHz to 525.0 MHz.
 *   - br              Bit rate of the FSK transmission in kbps (kilobits per second).
 *                       Allowed values range from 1.2 to 300.0 kbps.
 *   - freqDev         Frequency deviation of the FSK transmission in kHz.
 *                       Allowed values range from 0.6 to 200.0 kHz.
 *                       Note that the allowed range changes based on bit rate setting, so that the condition
                         FreqDev + BitRate/2 <= 250 kHz is always met.
 *   - rxBw            Receiver bandwidth in kHz.
 *                       Allowed values are 2.6, 3.1, 3.9, 5.2, 6.3, 7.8, 10.4, 12.5, 15.6, 20.8, 25, 31.3, 41.7, 50,
 *                       62.5, 83.3, 100, 125, 166.7, 200 and 250 kHz.
 *   - power           Transmission output power in dBm. Allowed values range from 2 to 17 dBm.
 *   - preambleLength  Length of FSK preamble in bits.
 *   - enableOOK       Use OOK modulation instead of FSK.
 *
 * Note: Up to 525.0 MHz is incorrect, it can handle 868 MHz as well.
 */
void setup() {
	Serial.begin(9600);
	Serial.println("Arduino Reset");

	// There's a crystal on board which should give: 13.58125*2*2*2*2*2*2 = 869.2 MHz
	// It looks like we have to receive either 869.240 or 869.288 from the spectrum analysis.
	// If we choose 869.265 this is 25kHz down, 23kHz up.
	float freq = 869.265;
	float freqDev = 25;

	// The baudrate I'm least certain about, it looks like:
	//   in baudline:
	//      8.5 symbols in 1ms, that would be a baudrate of 8500
	//   in gnuradio:
	//      24x4 bits + 2 bits = 98 bits within ~50 ms, that would be a bitrate of 98*20=1960, times 2?
	float br = 1.96; // 3.92;

	// Receiver bandwidth at 50 kHz seems pretty safe.
	float rxBw = 50;

	// Transmission level at 10 dBm should be enough.
	int8_t power = 10;

	// There is no preamble (or just "default" 0xFF etc.)
	// In setPreambleLength modem should be SX127X_FSK_OOK for this to be accepted.
	// This sets SX127X_REG_PREAMBLE_MSB_FSK and SX127X_REG_PREAMBLE_LSB_FSK to 0.
	// In datasheets/sx1276_77_78_79.pdf table 41, page 91, RegPreambleMsb and RegPreambleLsb.
	// See below on the final choice, which is 24 (3 bytes).
	uint16_t preambleLength = 24;

	// We are using 2-FSK, not OOK.
	bool enableOOK = false;

	// This then calls SX127x::beginFSK(SX1278_CHIP_VERSION, ...)
	//   https://github.com/jgromes/RadioLib/blob/master/src/modules/SX127x/SX1276.cpp#L35 calls
	//   https://github.com/jgromes/RadioLib/blob/master/src/modules/SX127x/SX127x.cpp#L55
	// Note that it by default sets a syncWord[] = {0x12, 0xAD};.
	Serial.print(F("[SX1276] Initializing... "));
	int state = radio.beginFSK(freq, br, freqDev, rxBw, power, preambleLength, enableOOK);
	if (state == ERR_NONE) {
		Serial.println(F("success!"));
	} else {
		Serial.print(F("failed, code "));
		Serial.println(state);
		while(true);
	}

	// The SX127X have two data operation modes (4.2.9.2). In continuous mode each bit transmitted or received is
	// accessed in real-time on the DIO2/DATA pin. The packet mode operates on payload bytes to/from the FIFO.
	// This is build with preamble, sync word, and optional CRC.

	// no addresses
	state = radio.disableAddressFiltering();
	if (state != ERR_NONE) {
		Serial.print(F("Unable to disable address filtering, code "));
		Serial.println(state);
		while(true);
	}

	// It's difficult to see if it's using packets and even if it doesn't, it seems to be the case that also in
	// direct mode this lib writer expects a sync word to be there.
	// See: https://github.com/jgromes/RadioLib/discussions/337
	// Correction, that dicussion lead the writer to decide to allow the user to e.g. select on RSSI level rather than
	// message contents (preambles / sync words) by allowing no sync word: `setDirectSyncWord(0, 0);`.
	// Suppose we have the message: 0xFE76D1 or 0x01892E (not sure on hi/lo bits).
	// It is Manchester encoded, then 0xF is 10101010b or in two bytes raw 1010b 1010b, that is 0xAA.
	//                    or inverted 0x0 is 01010101b or 0x55.
	// That what is preceding it is the opposite, I guess for around 24 bits it is alternating bits.
	// This means probably that the preamble length should be 24 (in bits).
	// Also, 0x55 seems to be normal for the preamble, so that means the sync word is 0xAA.
	uint8_t syncWord[] = {0xAA};
	state = radio.setSyncWord(syncWord, 1);
	if (state != ERR_NONE) {
		Serial.print(F("Unable to set sync word, code "));
		Serial.println(state);
		while(true);
	}

	state = radio.setCRC(false);
	if (state != ERR_NONE) {
		Serial.print(F("Unable to set CRC to false, code "));
		Serial.println(state);
		while(true);
	}

	// Pretty sure it's Manchester, can be disabled by setting RADIOLIB_ENCODING_NRZ instead
	state = radio.setEncoding(RADIOLIB_ENCODING_MANCHESTER);
	if (state != ERR_NONE) {
		Serial.print(F("Unable to set encoding to manchester, code "));
		Serial.println(state);
		while(true);
	}
}

/**
 * This uses some information from @hansgaensbauer placed on https://github.com/jgromes/RadioLib/issues/174 about
 * using the SX1276RF1KAS and an Arduino UNO to receive FSK OOK packets (almost verbatim).
 */
void receive_fsk_method0() {
	int state = radio.startReceive(buflen, SX127X_RX);
	if (state != ERR_NONE) {
		Serial.print(F("Unable to start receiving data, code "));
		Serial.println(state);
		return;
	}

	while(!digitalRead(2));
	state = radio.readData(data, buflen);
	if (state != ERR_NONE) {
		Serial.print(F("Unable to read data, code "));
		Serial.println(state);
		return;
	}

	for(int i = 0; i<buflen; i++){
		Serial.print(data[i],HEX);
		Serial.print(" ");
	}
	Serial.println();
}

// see https://forum.arduino.cc/t/receiving-an-rf-868-mhz-signal-with-the-dragino-lora-bee/692261
// and https://platformio.org/lib/show/2873/LoRaLib
void receive_fsk_method1() {
	String str;
	int state = radio.receive(str);

	if (state == ERR_NONE) {
		// packet was successfully received
		Serial.println(F("success!"));

		// print the data of the packet
		Serial.print(F("[SX1276] Data:\t\t\t"));
		Serial.println(str);

		// print the RSSI (Received Signal Strength Indicator)
		// of the last received packet
		Serial.print(F("[SX1276] RSSI:\t\t\t"));
		Serial.print(radio.getRSSI());
		Serial.println(F(" dBm"));

		// print the SNR (Signal-to-Noise Ratio)
		// of the last received packet
		Serial.print(F("[SX1276] SNR:\t\t\t"));
		Serial.print(radio.getSNR());
		Serial.println(F(" dB"));

		// print frequency error
		// of the last received packet
		Serial.print(F("[SX1276] Frequency error:\t"));
		Serial.print(radio.getFrequencyError());
		Serial.println(F(" Hz"));

	} else if (state == ERR_RX_TIMEOUT) { // -6
		// timeout occurred while waiting for a packet
		Serial.println(F("timeout!"));

	} else if (state == ERR_CRC_MISMATCH) {
		// packet was received, but is malformed
		Serial.println(F("CRC error!"));

	} else {
		// some other error occurred
		Serial.print(F("failed, code "));
		Serial.println(state);
	}
}

/**
 * Transmit data and see if we can see it when analyzing the spectrum.
 */
void transmit_fsk() {

}

void loop() {
	//receive_fsk_method0();

	receive_fsk_method1();

	transmit_fsk();

	delay(100);
}
