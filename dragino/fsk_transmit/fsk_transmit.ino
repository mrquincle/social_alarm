//Modified version of the SX127x_FSK_Modem sketch

// include the library
#include <RadioLib.h>

/*
   // Pin mapping for Dragino Board Version 1.3 (open J_DIO5), same for 1.4 (which I happen to have)
   const lmic_pinmap lmic_pins = {
     .nss = 10,
     .rxtx = LMIC_UNUSED_PIN,
     .rst = 9,
     .dio = {2, 6, 7},  // Specify pin numbers for DIO0, 1, 2
                        // connected to D2, D6, D7
   };

   See also https://wiki.dragino.com/index.php?title=Lora_Shield
  
   It comes with an R9 resistor loaded by default, so D10 connects to CS.
   
   RESET - D9
   DIO0  - D2
   DIO5  - D8
   DIO2  - D7
   DIO1  - D6

   Doublecheck with https://github.com/dragino/Lora/blob/master/Lora%20Shield/hardware/v1.4/Lora%20Shield%20v1.4.sch.pdf.

   DIO0 - D2       - 2
   DIO1 - D6/PD6   - 6
   DIO2 - D7/PD7   - 7
   DIO5 - D8/PD8   - 8

   DIO0 is used for TX and DIO1 for RX in LoRA mode. DIO2 is used for RX in FSK mode. More details on
   https://www.thethingsnetwork.org/forum/t/lora-stack-and-dios/1675/3

   Table 30 describes packet mode:
   For TX:
     DIO0: 00, packet sent
           01,
           10,
           11, temp change / low bat
     DIO1: FIFO
     DIO3: 01, TX ready

   Do we need to hook up DIO3 to see TX ready? Probably not, and we can just FIFO.

   Doublecheck, the v1.4 uses the SX1276 (see the label/print on the chip, which is SX1276 1808 176956).
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
int cnt = 0;
const int outbuflen = 63;
char outbuf[outbuflen];

void setup() {
  Serial.begin(115200);
  Serial.println("Arduino Reset");

  radio.beginFSK(868.0, 1.96, 25.0, 50, 10, 24, true);

  // Make sure it's zeroed out
  for (int i = 0; i < outbuflen; ++i) {
    outbuf[i] = 0;
  }
}

/**
   Transmit data and see if we can see it when analyzing the spectrum.

   It transmits... But it sets ERR_TX_TIMEOUT (after 5 seconds).

   What it transmits is described in 4.2.13.3 Tx Processing of the datasheet.

   "The  transmission  of  packet  data  is  initiated  by  the  Packet  Handler  only  if  the  chip  is  in  Tx  mode  and  the  transmission  
condition defined by TxStartCondition is fulfilled. If transmission condition is not fulfilled then the packet handler transmits a 
preamble sequence until the condition is met. This happens only if the preamble length /= 0, otherwise it transmits a zero or 
one until the condition is met to transmit the packet data."

  This is exactly what I see happening... There's a preamble, but apparently the TxStartCondition is not fulfilled.
*/
void transmit_fsk() {
  sprintf(outbuf, "Msgno=%d", cnt++);
  Serial.print("Msg");
  Serial.println(cnt++);
  int state = radio.transmit(outbuf);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to transmit data, code "));
    Serial.println(state);
    return;
  }
  delay(2000);
}

void loop() {
  transmit_fsk();
  delay(1000);
}

/* Listing:
 *  
 13:57:39.589 -> 
13:57:39.589 -> Arduino Reset
13:57:39.589 -> Chip select 10
13:57:39.589 -> IRQ 2
13:57:39.589 -> Reset pin 7
13:57:39.589 -> R 1 29⸮
13:57:41.213 -> Arduino Reset
13:57:41.213 -> Chip select 10
13:57:41.213 -> IRQ 2
13:57:41.213 -> Reset pin 9
13:57:41.213 -> RX 6
13:57:41.213 -> TX 255
13:57:41.213 -> R 42  12  
13:57:41.213 -> M SX127x
13:57:41.213 -> R 1 9 
13:57:41.213 -> R 1 9 
13:57:41.213 -> W 1 9 
13:57:41.213 -> R 1 9 
13:57:41.246 -> R 1 9 
13:57:41.246 -> R 1 9 
13:57:41.246 -> R 1 9 
13:57:41.246 -> W 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> W 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 2 1A  
13:57:41.246 -> W 2 3F  
13:57:41.246 -> R 2 3F  
13:57:41.246 -> R 3 B 
13:57:41.246 -> W 3 C6  
13:57:41.246 -> R 3 C6  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> W 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 4 0 
13:57:41.246 -> W 4 1 
13:57:41.246 -> R 4 1 
13:57:41.246 -> R 5 52  
13:57:41.246 -> W 5 99  
13:57:41.246 -> R 5 99  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> W 1 29  
13:57:41.246 -> R 1 29  
13:57:41.246 -> R 13  B 
13:57:41.246 -> W 13  B 
13:57:41.246 -> R 13  B 
13:57:41.246 -> R 1 29  
13:57:41.246 -> R D 8 
13:57:41.246 -> W D F 
13:57:41.246 -> R D F 
13:57:41.246 -> R 1 29  
13:57:41.246 -> R D F 
13:57:41.246 -> W D 1F  
13:57:41.246 -> R D 1F  
13:57:41.246 -> R 1 29  
13:57:41.280 -> R 1 29  
13:57:41.280 -> R 1 29  
13:57:41.280 -> W 1 29  
13:57:41.280 -> R 1 29  
13:57:41.280 -> R 12  15  
13:57:41.280 -> W 12  B 
13:57:41.280 -> R 12  B 
13:57:41.280 -> R 1 29  
13:57:41.280 -> R 1 29  
13:57:41.280 -> W 1 29  
13:57:41.280 -> R 1 29  
13:57:41.280 -> R B 2B  
13:57:41.280 -> W B 23  
13:57:41.280 -> R B 23  
13:57:41.280 -> R 1 29  
13:57:41.280 -> R 1 29  
13:57:41.280 -> W 1 29  
13:57:41.280 -> R 1 29  
13:57:41.280 -> R 1 29  
13:57:41.280 -> R 25  0 
13:57:41.280 -> W 25  0 
13:57:41.280 -> R 25  0 
13:57:41.280 -> R 26  3 
13:57:41.280 -> W 26  3 
13:57:41.280 -> R 26  3 
13:57:41.280 -> R 1 29  
13:57:41.280 -> R 27  93  
13:57:41.280 -> W 27  93  
13:57:41.280 -> R 27  93  
13:57:41.280 -> R 27  93  
13:57:41.280 -> W 27  91  
13:57:41.280 -> R 27  91  
13:57:41.280 -> W 28  12  AD  
13:57:41.280 -> R 1 29  
13:57:41.280 -> R 30  90  
13:57:41.280 -> W 30  90  
13:57:41.280 -> R 30  90  
13:57:41.280 -> R 33  0 
13:57:41.280 -> W 33  0 
13:57:41.280 -> R 33  0 
13:57:41.280 -> R 34  0 
13:57:41.313 -> W 34  0 
13:57:41.313 -> R 34  0 
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 1 29  
13:57:41.313 -> W 1 29  
13:57:41.313 -> R 1 29  
13:57:41.313 -> R E 2 
13:57:41.313 -> W E 2 
13:57:41.313 -> R E 2 
13:57:41.313 -> R E 2 
13:57:41.313 -> W E 2 
13:57:41.313 -> R E 2 
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 30  90  
13:57:41.313 -> W 30  90  
13:57:41.313 -> R 30  90  
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 30  90  
13:57:41.313 -> W 30  90  
13:57:41.313 -> R 30  90  
13:57:41.313 -> R 32  40  
13:57:41.313 -> W 32  40  
13:57:41.313 -> R 32  40  
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 1 29  
13:57:41.313 -> W 1 29  
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 6 6C  
13:57:41.313 -> W 6 D9  
13:57:41.313 -> R 6 D9  
13:57:41.313 -> R 7 80  
13:57:41.313 -> W 7 0 
13:57:41.313 -> R 7 0 
13:57:41.313 -> R 8 0 
13:57:41.313 -> W 8 0 
13:57:41.313 -> R 8 0 
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 1 29  
13:57:41.313 -> W 1 29  
13:57:41.313 -> R 1 29  
13:57:41.313 -> R 9 4F  
13:57:41.346 -> W 9 CF  
13:57:41.346 -> R 9 CF  
13:57:41.346 -> R 9 CF  
13:57:41.346 -> W 9 F8  
13:57:41.346 -> R 9 F8  
13:57:41.346 -> R 4D  84  
13:57:41.346 -> W 4D  84  
13:57:41.346 -> R 4D  84  
13:57:41.346 -> R 1 29  
13:57:41.346 -> Msg1
13:57:41.346 -> R 1 29  
13:57:41.346 -> R 1 29  
13:57:41.346 -> W 1 29  
13:57:41.346 -> R 1 29  
13:57:41.346 -> R 1 29  
13:57:41.346 -> Send message of length 7
13:57:41.346 -> R 1 29  
13:57:41.346 -> R 1 29  
13:57:41.346 -> W 1 29  
13:57:41.346 -> R 1 29  
13:57:41.346 -> R 1 29  
13:57:41.346 -> R 40  0 
13:57:41.346 -> W 40  0 
13:57:41.346 -> R 40  0 
13:57:41.346 -> R 1 29  
13:57:41.346 -> W 3E  FF  
13:57:41.346 -> W 3F  FF  
13:57:41.346 -> W 0 7 
13:57:41.346 -> R 30  90  
13:57:41.346 -> W 0 4D  73  67  6E  6F  3D  30  
13:57:41.346 -> R 1 29  
13:57:41.346 -> R 1 29  
13:57:41.346 -> W 1 2B  
13:57:41.346 -> R 1 2B  
13:57:46.518 -> R 1 2B  
13:57:46.518 -> W 3E  FF  
13:57:46.518 -> W 3F  FF  
13:57:46.518 -> R 1 2B  
13:57:46.518 -> R 1 2B  
13:57:46.518 -> W 1 29  
13:57:46.518 -> R 1 2B  
13:57:46.518 -> R 1 2B  
13:57:46.518 -> R 1 29  
13:57:46.518 -> Unable to transmit data, code -5

 */
