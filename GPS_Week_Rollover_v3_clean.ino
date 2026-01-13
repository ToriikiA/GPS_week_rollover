#include <Stamp.h>

#define RXD2 18         // Пин приема с gps модуля
#define TXD2 17         // Пин отправки на gps модуль
#define RXD1 25         // Пин приема с внешнего устройства
#define TXD1 26         // Пин передачи на внешнее устройство
#define OUT_BAUD 115200 // Скорость передачи данных на внешнем устройстве
#define GPS_BAUD 115200 // Скорость передачи данных на gps модуле
#define NUM_OF_ROLLS 1  // Количество сдвигов (для моего модуля GPS n-1, т.к. один уже учтен в прошивке)

HardwareSerial gpsSerial(2);
HardwareSerial outSerial(1);

const uint32_t SECONDS_TO_ADD = 619315200 * NUM_OF_ROLLS;

void setup() {
  // Костыль
  // Настройка скорости передачи gps модуля (батарейка, питающая память была вынута, настройки каждый раз ставятся по умолчанию)
  delay(1000);
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(500);
  gpsSerial.print("$PMTK251,115200*1F\r\n");
  delay(500);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  outSerial.begin(OUT_BAUD, SERIAL_8N1, RXD1, TXD1);
}

void loop() {
  while (gpsSerial.available() > 0) {

    String sentence = gpsSerial.readStringUntil('\n');
    if (sentence.indexOf("RMC") != -1) {
      String fixed = fix_RMC(sentence);
      outSerial.print(fixed);
    } else if (sentence.indexOf("ZDA") != -1) {
      String fixed = fix_ZDA(sentence);
      outSerial.print(fixed);
    } else {
      outSerial.print(sentence + "\n");
    }
  }
}

String fix_RMC(String nmea) {
  int commaCount = 0;
  int datePos = -1;

  for (int i = 0; i < nmea.length() - 6; ++i) {
    if (nmea[i] == ',') ++commaCount;
    if (commaCount == 9 && isDigit(nmea[i + 1]) && isDigit(nmea[i + 2]) && isDigit(nmea[i + 3]) && isDigit(nmea[i + 4]) && isDigit(nmea[i + 5]) && isDigit(nmea[i + 6])) {
      datePos = i + 1;
      break;
    }
  }
  if (datePos == -1) return nmea + "\n";
  String oldDate = nmea.substring(datePos, datePos + 6);
  uint8_t day = (uint8_t)oldDate.substring(0, 2).toInt();
  uint8_t month = (uint8_t)oldDate.substring(2, 4).toInt();
  uint16_t year = (uint16_t)oldDate.substring(4, 6).toInt();

  uint32_t s = StampUtils::dateToUnix(day, month, 2000 + year, 0, 0, 0, 0);
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

String fix_ZDA(String nmea) {
  int commaCount = 0;
  int dayPos = -1;
  int monthPos = -1;
  int yearPos = -1;
  uint8_t day;
  uint8_t month;
  uint16_t year;
  for (int i = 0; i < nmea.length(); ++i) {
    if (nmea[i] == ',') ++commaCount;
    if (commaCount == 2 && isDigit(nmea[i + 1]) && isDigit(nmea[i + 2])) {
      day = (uint8_t)nmea.substring(i + 1, i + 3).toInt();
      dayPos = i + 1;
    }
    if (commaCount == 3 && isDigit(nmea[i + 1]) && isDigit(nmea[i + 2])) {
      month = (uint8_t)nmea.substring(i + 1, i + 3).toInt();
      monthPos = i + 1;
    }
    if (commaCount == 4 && isDigit(nmea[i + 1]) && isDigit(nmea[i + 2]) && isDigit(nmea[i + 3]) && isDigit(nmea[i + 4])) {
      year = (uint16_t)nmea.substring(i + 1, i + 5).toInt();
      yearPos = i + 1;
    }
  }
  if (dayPos == -1 || monthPos == -1 || yearPos == -1) return nmea + "\n";

  uint32_t s = StampUtils::dateToUnix(day, month, year, 0, 0, 0, 0);
  s += SECONDS_TO_ADD;
  Datime d = Datime(s);
  day = d.day;
  month = d.month;
  year = d.year;

  char newDate[11];
  sprintf(newDate, "%02d,%02d,%04d", day, month, year);
  String result = nmea;
  result = result.substring(0, dayPos) + newDate + result.substring(yearPos + 4);
  result = updateChecksum(result);
  return result + "\r\n";
}

String updateChecksum(String sentence) {
  int starPos = sentence.indexOf('*');
  if (starPos != -1) sentence = sentence.substring(0, starPos);
  else return sentence;
  uint8_t checksum = 0;
  for (int i = 1; i < sentence.length(); ++i) {
    checksum ^= sentence[i];
  }
  char chk[4];
  sprintf(chk, "*%02X", checksum);
  return sentence + chk;
}
