#define analogPin  A2     // подключен ИК-фоторезистор
#define Ref_Pin    A0     // источник опорного напряжения 5В для ИК-фоторезистора, к A0 от него 10кОм
#define IR_Pin     A4     // ИК-диод, вкл/выкл 5V

#include <FastDefFunc.h>   // библиотека для убыстрения функций: pinMode, digitalWrite, ...Read, analogWrite, ...Read

int val = 0;              // переменная для хранения считываемого значения

void setup(){
  Serial.begin(9600);              //  установка связи по serial
  Serial.flush();                  //  ждём, когда все старые данные пройдут
  
  pinModeFast(Ref_Pin, OUTPUT);       // назначение опорного пина на выхода 5В
  pinModeFast(IR_Pin, OUTPUT);        // назначение выходом 5В, чтоб ИК-диод светил

  pinMode(LED_BUILTIN, OUTPUT);       // используем для индикации на Arduino
}
 
void loop(){
  val = analogReadFast(analogPin);    // считываем значение с аналогового пина
  if(val >= 800)                      // если оно больше 800 
    digitalWriteFast(LED_BUILTIN, HIGH); // вкл светодиод
  else
     digitalWriteFast(LED_BUILTIN, LOW); // в противном случае держим выключенным
  Serial.println(val);             // выводим полученное значение (можно в плоттере посмотреть)
}
