#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS.h>

#include "ArduinoLowPower.h"

#include "wiring_private.h" // pinPeripheral() function
// Serial2
#define PIN_SERIAL2_RX (9ul) // PA15
#define PIN_SERIAL2_TX (10ul) // PA14
#define PAD_SERIAL2_TX (UART_TX_PAD_2)
#define PAD_SERIAL2_RX (SERCOM_RX_PAD_3)
#define SERCOM_INSTANCE_SERIAL2 &sercom2
Uart Serial2(SERCOM_INSTANCE_SERIAL2, PIN_SERIAL2_RX, PIN_SERIAL2_TX, PAD_SERIAL2_RX, PAD_SERIAL2_TX);

char szz[32] = {};
int counter = 0;
int fix = 0;

TinyGPS gps;

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

void setup() {
  Serial.begin(9600);
  //while (!Serial);
  Serial2.begin(38400);
  // Assign pins 9 & 10 SERCOM functionality
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(9, PIO_SERCOM);

  Serial.println("GPS LoRa Sender");
  pinMode(RFM_TCX_ON,OUTPUT);
  pinMode(RFM_SWITCH,OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  LoRa.setPins(SS,RFM_RST,RFM_DIO0);
  
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  //LoRa.setTxPower(20);
  //LoRa.setSpreadingFactor(12);  

  //Restart GPS
  uint8_t GPSon[] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x37};
  sendUBX(GPSon, sizeof(GPSon)/sizeof(uint8_t));
}

void loop() {
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;

  gps.f_get_position(&flat, &flon, &age);

  print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
  print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
  
  Serial.print(" Sending packet: ");
  Serial.println(counter);
  digitalWrite(LED_BUILTIN,1);
  // send packet
  LoRa.beginPacket();
  digitalWrite(RFM_SWITCH,0);
  
  //LoRa.print("LAT: 21.089978 LON: -101.658615 hello Panda ");

  print_date(gps);

  LoRa.print("LAT: ");
  LoRa.print(flat,6);
  LoRa.print(" ");
  LoRa.print("LON: ");
  LoRa.print(flon,6);
  LoRa.print(" ");
  LoRa.print("hello ");
  LoRa.print(counter);
  LoRa.print(" ");
  LoRa.print(myName());
  
  LoRa.endPacket();

  counter++;
  //delay(500);
  digitalWrite(LED_BUILTIN,0);
  //smartdelay(5000);

  if(fix){
  //Set GPS to backup mode (sets it to never wake up on its own)
  uint8_t GPSoff[] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4D, 0x3B};
  sendUBX(GPSoff, sizeof(GPSoff)/sizeof(uint8_t));

  //delay(30000);
  Serial.println("Starting LowPower mode...");
  //delay(30000);
  LowPower.sleep(30000);
  LowPower.sleep(30000);
  LowPower.sleep(30000);
  LowPower.sleep(30000);
  LowPower.sleep(30000);
  LowPower.sleep(30000);
  LowPower.sleep(30000);
  LowPower.sleep(30000);

 
  //Restart GPS
  uint8_t GPSon[] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x37};
  sendUBX(GPSon, sizeof(GPSon)/sizeof(uint8_t));
  }
  
  smartdelay(10000);
}

void SERCOM2_Handler() {
  Serial2.IrqHandler();
}

// Send a byte array of UBX protocol to the GPS
void sendUBX(uint8_t *MSG, uint8_t len) {
  for(int i=0; i<len; i++) {
    Serial2.write(MSG[i]);
  }
  //Serial.println();
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial2.available())
      gps.encode(Serial2.read());
  } while (millis() - start < ms);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE){
    Serial.print("********** ******** ");
    fix = 0;
  }
  else
  {
    //char sz[32];
    sprintf(szz, "%02d/%02d/%02d %02d:%02d:%02d ",
        month, day, year, hour, minute, second);
    Serial.print(szz);
    fix = 1;
  }
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  smartdelay(0);
}

static void print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
    fix = 0;
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
    fix = 1;  
  }
  smartdelay(0);
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartdelay(0);
}

const char* myName() {
  return "Panda";
}

