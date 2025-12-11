#include <Arduino.h>
#include <Stamp.h>

#define RXD2 16
#define TXD2 17
#define GPS_BAUD 115200
#define NUM_OF_ROLLS 1

HardwareSerial gpsSerial(2);

const uint32_t SECONDS_TO_ADD = 619315200 * NUM_OF_ROLLS;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Serial 0 started");
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial 2 started");
  Serial.println("Waiting for GPS data...");
  Serial.println("-----------------------------------------");
}

void loop() {
  while (gpsSerial.available() > 0) {
    String sentence = gpsSerial.readStringUntil('\n');
    if (sentence.startsWith("$GPRMC") || sentence.startsWith("$GNRMC")){
      String fixed = fix(sentence);
      Serial.print(fixed);
    }
    else{
        Serial.print(sentence + "\n");
    }
  }
  if (Serial.available()){
    String cmd = Serial.readString();
    if(!cmd.endsWith(("\r"))) cmd+="\r\n";
    gpsSerial.print(cmd);
  }
}

String fix(String nmea){
  int commaCount = 0;
  int datePos = -1;

  for (int i = 0; i < nmea.length() - 6; ++i){
    if(nmea[i]==',') ++commaCount;
    if(commaCount == 9 && nmea.substring(i+1, i+7).length()==6 &&
    isDigit(nmea[i+1]) && isDigit(nmea[i+2]) && isDigit(nmea[i+3]) &&
    isDigit(nmea[i+4]) && isDigit(nmea[i+5]) && isDigit(nmea[i+6])){
      datePos = i + 1;
      break;
    }
  }
  if (datePos == -1) return nmea + "\n";
  String oldDate = nmea.substring(datePos, datePos + 6);
  uint8_t day = (uint8_t)oldDate.substring(0,2).toInt();
  uint8_t month = (uint8_t)oldDate.substring(2,4).toInt();
  uint16_t year = (uint16_t)oldDate.substring(4,6).toInt();
  
  uint32_t s = StampUtils::dateToUnix(day, month, 2000 + year,0,0,0,0);
  s += SECONDS_TO_ADD;
  Datime d = Datime(s);
  day = d.day;
  month = d.month;
  year = (d.year) % 100;

  char newDate[7];
  sprintf(newDate, "%02d%02d%02d", day, month, year);

  String result = nmea;
  result = result.substring(0, datePos) + String(newDate) + result.substring(datePos + 6);
  result = updateChecksum(result);

  return result + "\r\n";
}

String updateChecksum(String sentence){
  int starPos = sentence.indexOf('*');
  if(starPos!=-1) sentence = sentence.substring(0, starPos);
  uint8_t checksum = 0;
  for(int i = 1; i < sentence.length();++i){
    checksum ^= sentence[i];
  }
  char chk[4];
  sprintf(chk, "*%02X", checksum);
  return sentence + chk;
}