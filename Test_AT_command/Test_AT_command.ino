/*
AT+CBC -- заряд аккума
AT+CSQ -- качество связи (от 10 -- хорошо)
*/
#include <SoftwareSerial.h>
SoftwareSerial SIM800(7,9);        // 7 - пин D7 Arduino связан с пином TX SIM800L, 9 - пин D9 Arduino связан RX пином SIM800L
void setup() {
  Serial.begin(9600);               // Скорость обмена данными с компьютером
  Serial.println("Start!");
  SIM800.begin(9600);               // Скорость обмена данными с модемом
  SIM800.println("AT");             // Ответом должен быть "ОК"
}

void loop() {
  if (SIM800.available())           // Ожидаем прихода данных (ответа) от модема...
    Serial.write(SIM800.read());    // ...и выводим их в Serial
  if (Serial.available())           // Ожидаем команды по Serial...
    SIM800.write(Serial.read());    // ...и отправляем полученную команду модему
}
