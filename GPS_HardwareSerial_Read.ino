#define RXD2 16
#define TXD2 17

#define GPS_BAUD 9600

HardwareSerial gpsSerial(2);

void setup() {
  Serial.begin(115200);
  Serial.println("Serial 0 started");
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial 2 started");
  Serial.println("-----------------------------------------");
}

void loop() {
  while (gpsSerial.available() > 0) {
    char gpsData = gpsSerial.read();
    Serial.print(gpsData);
  }
}
