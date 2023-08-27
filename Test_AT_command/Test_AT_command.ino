/*
AT+CBC -- заряд аккума
AT+CSQ -- качество связи (от 10 -- хорошо)
*/
#include<SoftwareSerial.h>                                     // библиотека програмной реализации обмена по UART-протоколу

SoftwareSerial SIM800(7,9);                                    // 7 - RX Arduino (TX SIM800L), 9 - TX Arduino (RX SIM800L)

String _response="";                                           // переменная для хранения ответа SIM800L
String whiteListPhones = "+7xxxxxxxxxx, +7yyyyyyyyyy";         // Белый список телефонов

void setup() {
  Serial.begin(9600);                                          // скорость обмена данными монитора порта с компьютером
  SIM800.begin(9600);                                          // ............................    SIM800 с компьютером
  Serial.println(F("Start!"));                                 // if сериал запустился //F-означает, используемый ответ будет хранится в Flash, а не оперативке

  delay(300);                                                  // физическая задержка

  sendATCommand("AT", true);                       // проверка готовности модуля к работе и автонастройка скорости (ответом должен быть "ОК"),
//  sendATCommand("AT+CLIP=1", true);                // автоопределитель номера во время входящего звонка
  //sendATCommand("ATE0", true);                     // выключаем ECHO Mode
  //sendATCommand("AT+CSCLK=1",true);              // спящий режим для SIM800L
//  sendATCommand("AT+CMGF=1", true);                // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)!
}

void loop() {
    if(SIM800.available()){                                     // Если модем, что-то отправил...
      _response = waitResponse();                                // Получаем ответ от модема для анализа
      _response.trim();                                          // Убираем лишние пробелы в начале и конце
      Serial.println(_response);                                 // Если нужно выводим в монитор порта
      if(_response.startsWith("RING")) {                         // Есть входящий вызов
        int phoneindex = _response.indexOf("+CLIP: \"");               // Есть ли информация об определении номера, если да, то phoneindex>-1
        String innerPhone = "";                                                // Переменная для хранения определенного номера
        if(phoneindex >= 0) {                                                        // Если информация была найдена
          phoneindex += 8;                                                                     // Парсим строку и ...
          innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex));   // ...получаем номер
          Serial.println("Number: " + innerPhone);                                             // Выводим номер в монитор порта
        }
        if(innerPhone.length() >= 7 && whiteListPhones.indexOf(innerPhone)>=0) {            // Длина номера должен быть > 6 цифр, и номер в белом списке
          sendATCommand("ATA", true);                                          // поднятие трубки после 3 гудков

          //Serial.println(F("Трубка поднята"));
        }
        else {
          sendATCommand("ATH", true);                               // в противном случае отклоняем вызов
        }
      }

      if (_response.startsWith("+CMGS:")) {       // Пришло сообщение об отправке SMS
        int index = _response.lastIndexOf("\r\n");// Находим последний перенос строки, перед статусом
        String result = _response.substring(index + 2, _response.length()); // Получаем статус
        result.trim();                            // Убираем пробельные символы в начале/конце
        if (result == "OK") Serial.println ("Message was sent. OK");                   // Если результат ОК - все нормально
        else  Serial.println ("Message was not sent. Error");                          // Если нет, нужно повторить отправку
      }

       if (_response.startsWith("+CBC:")) {       // если получен заряд аккума
        unsigned int chardge;
        chardge = _response.substring(11).toInt();
        Serial.println(chardge);
      }

      if(_response.startsWith("NO CARRIER")){
         // Serial.println(F("Связь прервана"));
      }
    }

    if (Serial.available()) {                                     // Если что-то отправлено через порт взаимодействия
        SIM800.write(Serial.read());                              // Ожидаем команды по Serial и отправляем полученную команду модему
    }
}

String sendATCommand(String cmd, bool waiting){                      // функция отправки команды SIM800
  String _resp="";                                                   //переменная для хранения результата
  //Serial.println(cmd);                                               //дублируем команду в монитор порта
  SIM800.println(cmd);
  if(waiting){
    _resp = waitResponse();
  Serial.println(_resp);
  }
  return _resp;
}

void sendSMS(String phone, String message){
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
}

String waitResponse(){                                               //функция ожидания ответа и получения обратного результата
  String _resp = "";                                                 //переменная для хранения результата
  long _timeout = millis() + 10000;                                  //переменная для ожидания 10с
  while (!SIM800.available()&& millis() < _timeout){}                //ждём получение ответа в течение 10с
  if(SIM800.available())    _resp = SIM800.readString();
  else     Serial.println(F("Timeout > 10s"));
  return _resp;
}
