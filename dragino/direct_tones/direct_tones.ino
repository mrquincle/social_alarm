//Modified version of the SX127x_FSK_Modem sketch

// include the library
#include <RadioLib.h>

/*
   // Pin mapping for Dragino Board Version 1.3 (open J_DIO5), same for 1.4
   const lmic_pinmap lmic_pins = {
     .nss = 10,
     .rxtx = LMIC_UNUSED_PIN,
     .rst = 9,
     .dio = {2, 6, 7},  // Specify pin numbers for DIO0, 1, 2
                        // connected to D2, D6, D7
   };

   See also https://wiki.dragino.com/index.php?title=Lora_Shield
  
   I've indeed a Dragino board with v1.4. 
   It comes with an R9 resistor loaded by default, so D10 connects to CS.
   
   RESET - D9
   DIO0  - D2
   DIO5  - D8
   DIO2  - D7
   DIO1  - D6

   DIO0 is used for TX and DIO1 for RX in LoRA mode. DIO2 is used for RX in FSK mode. More details on
   https://www.thethingsnetwork.org/forum/t/lora-stack-and-dios/1675/3

   dio3, FSK nothing, LoRA CADDETECTED or CADDONE, channel activity detection
   dio4, FSK preamble detected
   dio5, FSK nothing, LoRA nothing
   dio0, FSK/LORA, tx done or rx done
   dio1, timeouts
   dio2, LORA, change channel

   The v1.4 uses the SX1276 (see the label/print on the chip, which is SX1276 1808 176956).
*/

// TODO: Check with the logic what is happening on those pins.
int pin_cs    = 10;
int pin_reset =  9;
int pin_dio0  =  2;
int pin_dio1  =  6;
int pin_dio2  =  7;
int pin_dio5  =  8;

// Global radio object
SX1276 radio = new Module(pin_cs, pin_dio0, pin_reset, pin_dio1);

// Global buffer
const int buflen = 20;
uint8_t* data = new uint8_t[buflen + 1];

int cnt = 0;

void directMode() {
  // activate direct mode transmitter
  int state = radio.transmitDirect();
  if (state != ERR_NONE) {
    Serial.println(F("Unable to start direct transmission mode, code "));
    Serial.println(state);
  } else {
    Serial.println(F("Set direct mode"));
  }
}


/*
   See https://github.com/mrquincle/RadioLib/blob/master/src/modules/SX127x/SX1276.h for a cloned
   repository with additional information on the use of this lib and the parameters.

   FSK modem initialization method.
   Must be called at least once from Arduino sketch to initialize the module.

   Parameters
     - freq            Carrier frequency in MHz. Allowed values range from 137.0 MHz to 525.0 MHz.
     - br              Bit rate of the FSK transmission in kbps (kilobits per second).
                         Allowed values range from 1.2 to 300.0 kbps.
     - freqDev         Frequency deviation of the FSK transmission in kHz.
                         Allowed values range from 0.6 to 200.0 kHz.
                         Note that the allowed range changes based on bit rate setting, so that the condition
                         FreqDev + BitRate/2 <= 250 kHz is always met.
     - rxBw            Receiver bandwidth in kHz.
                         Allowed values are 2.6, 3.1, 3.9, 5.2, 6.3, 7.8, 10.4, 12.5, 15.6, 20.8, 25, 31.3, 41.7, 50,
                         62.5, 83.3, 100, 125, 166.7, 200 and 250 kHz.
     - power           Transmission output power in dBm. Allowed values range from 2 to 17 dBm.
     - preambleLength  Length of FSK preamble in bits.
     - enableOOK       Use OOK modulation instead of FSK.

   Note: Up to 525.0 MHz is incorrect, it can handle 868 MHz as well.
*/
void setup() {
  Serial.begin(115200);
  Serial.println("Arduino Reset");

  // There's a crystal on board which should give: 13.58125*2*2*2*2*2*2 = 869.2 MHz
  // It looks like we have to receive either 869.240 or 869.288 from the spectrum analysis.
  // If we choose 869.265 this is 25kHz down, 23kHz up.
  //float freq = 869.265f;
  //float freqDev = 25.0;
  float freq = 869.265f;
  float freqDev = 25.0;

  // The baudrate I'm least certain about, it looks like:
  //   in baudline:
  //      8.5 symbols in 1ms, that would be a baudrate of 8500
  //   in gnuradio:
  //      24x4 bits + 2 bits = 98 bits within ~50 ms, that would be a bitrate of 98*20=1960, times 2?
  //float br = 1.96; // 3.92;
  float br = 1.2;

  // Or baudrate is half of this (2 bits per symbol)
  // br = 1.2; // 3.92;

  // Receiver bandwidth at 50 kHz seems pretty safe.
  float rxBw = 50.0;

  // Transmission level at 10 dBm should be enough.
  float power = 10.0;

  // There is no preamble (or just "default" 0xFF etc.)
  // In setPreambleLength modem should be SX127X_FSK_OOK for this to be accepted.
  // This sets SX127X_REG_PREAMBLE_MSB_FSK and SX127X_REG_PREAMBLE_LSB_FSK to 0.
  // In datasheets/sx1276_77_78_79.pdf table 41, page 91, RegPreambleMsb and RegPreambleLsb.
  // See below on the final choice, which is 24 (3 bytes).

  // With preamble below 8 I don't see anything on the spectrum.
  uint16_t preambleLength = 8;

  // We are using 2-FSK, not OOK.
  // If we enable OOK, we get ERR_INVALID_MODULATION.
  bool enableOOK = false;

  // This then calls SX127x::beginFSK(SX1278_CHIP_VERSION, ...)
  //   https://github.com/jgromes/RadioLib/blob/master/src/modules/SX127x/SX1276.cpp#L35 calls
  //   https://github.com/jgromes/RadioLib/blob/master/src/modules/SX127x/SX127x.cpp#L55
  // Note that it by default sets a syncWord[] = {0x12, 0xAD};.
  Serial.print("Frequency set to ");
  Serial.println(freq);
  Serial.print("Frequency deviation set to ");
  Serial.print(freqDev);
  Serial.println(" kHz");
  Serial.print("Baudrate set to ");
  Serial.println(br);

  Serial.print(F("[SX1276] Initializing... "));
  int state = radio.beginFSK(freq, br, freqDev, rxBw, power, preambleLength, enableOOK);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // The SX127X have two data operation modes (4.2.9.2). In continuous mode each bit transmitted or received is
  // accessed in real-time on the DIO2/DATA pin. The packet mode operates on payload bytes to/from the FIFO.
  // This is build with preamble, sync word, and optional CRC.

  // no addresses
  state = radio.disableAddressFiltering();
  if (state != ERR_NONE) {
    Serial.print(F("Unable to disable address filtering, code "));
    Serial.println(state);
    while (true);
  } else {
    Serial.println("Disable address filtering");
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
  // Ah, there's SX127X_PREAMBLE_POLARITY_55.

  uint8_t syncWord[] = {0xCC, 0xBB};
  uint8_t syncWordLen = 2;
  state = radio.setSyncWord(syncWord, syncWordLen);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to set sync word, code "));
    Serial.println(state);
    while (true);
  } else {
    Serial.print(F("Set sync word to "));
    for (int i = 0; i < syncWordLen; i++) {
      Serial.print(syncWord[i]);
      Serial.print(" ");
    }
    Serial.println();
  }

  state = radio.setCRC(false);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to set CRC to false, code "));
    Serial.println(state);
    while (true);
  } else {
    Serial.println("Set CRC to false");
  }

  // No encoding
  state = radio.setEncoding(RADIOLIB_ENCODING_NRZ);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to set encoding to manchester, code "));
    Serial.println(state);
    while (true);
  } else {
    Serial.println("Enable Manchester encoding");
  }

  // Set direct mode
  directMode();
}

/*
 * Immediately generate sine waves. Make sure you connect DIO2 with pin 4.
 */
void transmit_tones() {
  Serial.print("Msg");
  Serial.println(cnt++);
  tone(4, 500);
  delay(1000);
  tone(4, 5000);
  delay(1000);
  tone(4, 10000);
  delay(1000);
  tone(4, 15000);
  delay(1000);
  tone(4, 25000);
  delay(1000);
  tone(4, 50000);
  delay(1000);
}

void loop() {
  transmit_tones();

  delay(1000);
}
