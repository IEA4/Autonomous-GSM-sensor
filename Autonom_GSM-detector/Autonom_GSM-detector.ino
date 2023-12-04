#define Charge_min 3570          // напряжение аккумулятора в мВ, ниже которого осущ-ся отправка смс по взведенному периоду пробуждения независимо от срабатывания датчика
#define critical_Charge 3525     // напряжение, ниже которого отключается внешнее прерывание, модуль усыпляется на несколько дней, пока напряжение не станет ниже допустимого

#define time_wake 85140000        // период в мс, по которому будет просыпаться Ардуино и SIM800L  //если поставить ровно сутки (86 400 000) -- на 33мин позже пробуждение
#define time_det 20000            // длительность проверки детектором
#define time_proc 15000           // длительность обработки причины пробуждения

#define Level_IR  850     // порог шума сигнала c  ИК-фотодиода, выше которого считать о наличии обнаружения

#define analogPin  A2     // подключен ИК-фотодиод
#define Ref_Pin    A0     // источник опорного напряжения 5В для ИК-фотодиода, к A0 от него 9.1кОм
#define IR_Pin     A4     // ИК-светодиод, вкл/выкл 5V

#define RING_Pin   2      // внешнее прерывание для Arduino из SIM800L
#define DTR_Pin    5      // DTR пин SIM800L для усыпления/пробуждения модуля

#include <FastDefFunc.h>              // библиотека для убыстрения функций: pinMode, digitalWrite, ...Read, analogWrite, ...Read

#include <GyverPower.h>               // библиотека режимов сна https://alexgyver.ru/gyverpower/

#include <SoftwareSerial.h>                 // библиотека программной реализации обмена по UART-протоколу
SoftwareSerial SIM800(7,9);                 // 7 - пин D7 Arduino связан с пином TX SIM800L, 9 - пин D9 Arduino связан RX пином SIM800L

const String whiteListPhones = "+79xxxxxxxxx, +79yyyyyyyyy";     // Белый список номеров
String innerPhone = whiteListPhones.substring(0,12);       // Для хранения определенного номера (в процессе звонка). Первый номер в белом списке по умолчанию

bool flag_det = 1;                      // флажок пробуждение по прерыванию
bool flag_true_call;                    //   ...  звонка номера из белого списка
volatile bool flag_att;                 //   ...  пробуждения по внешнему прерыванию

void setup() {
  Serial.begin(9600);                   // скорость обмена данными монитора порта с компьютером
  SIM800.begin(9600);                   // ............................    SIM800 с компьютером
  Serial.flush();                       // ждём, когда все старые данные пройдут

  delay(20000);                         // физ.задержка, чтоб в случае подачи питания сразу и ардуино выключть его, а также при случайных перезагрузках успеть найти сеть

  sendATCommand(F("AT"), true);                       // проверка готовности модуля к работе и автонастройка скорости
  sendATCommand(F("AT+CLIP=1"), true);                // автоопределитель номера во время входящего звонка
  sendATCommand(F("ATE0"), true);                     // выключаем ECHO Mode
  sendATCommand(F("AT+CNETLIGHT=0"), true);           // вырубаем светодиод-индикатор на SIM800L
  sendATCommand(F("AT+CMGF=1"), true);                // текстовый формат сообщений
  sendATCommand(F("AT+CSCLK=1"),true);                // спящий режим для SIM800L

  sendSMS(innerPhone, "Qual_con: " + quality_con() + ". Voltage now: " + charge_parsing()+ "; critical: "+ (String)critical_Charge);            //на номер по умолчанию отсылаем смс с качеством связи и текущим напряжением

  pinModeFast(Ref_Pin, OUTPUT);                   // назначение опорного пина на выхода 5В
  pinModeFast(IR_Pin, OUTPUT);                    // назначение выходом 5В, чтоб ИК-диод светил

  pinModeFast(RING_Pin, INPUT_PULLUP);            // к примеру, кнопка подключена к GND и D, без резистора // на RING_Pin пине предустановлено HIGH

  //power.setSystemPrescaler(PRESCALER_2);          // [испол., если выбрано 8МГц] частоту процессора устанавливаем на 8МГц (по умолчанию PRESCALER_1)/* не работает */
  //power.calibrate(power.getMaxTimeout());         // калибровка тайм-аутов watchdog для sleepDelay  /* не работает */
  power.hardwareDisable(PWR_SPI);                 // выключение указанной периферии
  power.setSleepMode(POWERDOWN_SLEEP);            // установка режима сна

  pinModeFast(DTR_Pin, OUTPUT);                   // к DTR пину GSM модуля
  digitalWriteFast(DTR_Pin,HIGH);                 // выключаем SIM800L (чтоб потом пробуждать каждый раз при заходе в бесконечный цикл while(;;))

  pinModeFast(LED_BUILTIN,OUTPUT);               // для индикации на Arduino
  digitalWriteFast(LED_BUILTIN,HIGH);            // включаем
  delay(5000);                                   //       ...на 5 секунд
  digitalWriteFast(LED_BUILTIN,LOW);             //                      ... и выключаем
}

void loop() {
  for(;;){
    digitalWriteFast(DTR_Pin,LOW);               // будим SIM800L
    delay(200);                                  // физическая задержка, чтоб модуль включился

    if (flag_att == 1){                          // если пробуждение было по внешнему прерыванию
      Process();                                 // обрабатываем причину этого пробуждения
    }

    String charge = charge_parsing();                // измеряем заряд аккумулятора

    if(flag_det == 1){                        // звонил номер из белого списка или пришло время детектирования
      bool det = detected();                  // проверка ИК-датчиком
      if(det){                                // при действительном обнаружении
        sendSMS(innerPhone, charge +  ", 1");      // отправляем смс: [заряд аккум,  1]. Последнее -- знак того, что детектор срабатал
      }
      else if(det == 0 && (flag_true_call == 1 || charge.toInt() <= Charge_min)){   // при отсут. обнаружения в случае низ. напряжения или при звонке номера из белого списка
        if(charge.toInt() > critical_Charge){
          sendSMS(innerPhone, charge);           // отправляем смс с зарядом, если это значение выше критического
        }
        else{
          sendSMS(innerPhone, charge + ". Low charge. Sleep forever" );         // смс с зарядом + "уведомление об отключении"
        }
      }
    }

    flag_true_call = 0;                         // кладём флажок звонка номера из белого списка
    flag_det = 1;                               // разрешаем детектирование по пробуждению
    flag_att = 0;                               // при пробуждении по таймеру запрещаем обработку причины внешнего аппаратного прерывания

    digitalWriteFast(DTR_Pin, HIGH);              // усыпляем SIM800L
    delay(200);                                   // физическая задержка, чтоб всё удачно выключилось

    if(charge.toInt() > critical_Charge){           // если напряжение источника питания выше критического ...
      attachInterrupt(0, wakeup, FALLING);          // включение внешнего прерывания  по спаду
      delay(200);
      power.sleepDelay(time_wake);                  // сон на (заданный период) в миллисекундах (до 52 суток)
    }
    else if(charge.toInt() <= critical_Charge){               // если напряжение критически низкое, то внешнее прерывание отключено
      power.sleep(SLEEP_FOREVER);                             // сон до полного истечения заряда, пока не заменят акум
    }
  }
}

// функция детектирования
bool detected(){
  digitalWriteFast(IR_Pin, HIGH);                  // включаем ИК-диод
  digitalWriteFast(Ref_Pin, HIGH);                 // подаём опорное питание на ИК-фоторезистор

  byte i = 0;                                  // счётчик подсчёта кол-ва среагирований
  bool detect = 0;                             // обнаружено да/нет
  unsigned long t_one, t_det;                  // время первого обнаружения и время начала текущего обнаружения
  unsigned long t_act = millis();              // время начала детектирования

  do {
    if (analogReadFast(analogPin) >= Level_IR){       // если сигнал с ИК-фоторезистора выше порога шума
      if (millis() - t_det > 500){                    // реагируем только на одно срабатывание за 500мс (чтоб дважды и более не реагировать на одно и то же событие)
        t_det = millis();
        ++i;
        if(i == 1){
          t_one = millis();                        // отсчёт первого срабатывания
        }
        if(i >= 2 && (millis() - t_one) <= 10000){   // если было не менее 2 срабатываний в течении 10 секунд, то
          detect = 1;                                // обнаружено
          break;                                   // сразу выходим из цикла, чтобы оповестить об успехе
        }
      }
    }
  }
  while (millis() - t_act < time_det);         // даём time_det мс для обнаружения

  digitalWriteFast(IR_Pin, LOW);                        // выключаем...
  digitalWriteFast(Ref_Pin, LOW);

  return detect;
}

// ... обработки причины пробуждения
void Process(){
  String response;                                           // переменная для хранения ответа SIM800L
  unsigned long timer_act = millis();                       // время начала обработки причины пробуждения
  bool flag_hang_up;                                      // флажок звонка номера из белого списка

  while((millis() - timer_act) <= time_proc){                  // отрабатываем причину пробуждения по прерыванию
    if(SIM800.available()) {                                    // Если модем, что-то отправил...
      response = waitResponse();                                // Получаем ответ от модема для анализа
      response.trim();                                          // Убираем лишние пробелы в начале и конце
      if(response.startsWith(F("RING"))) {                         // Если  есть входящий вызов
        unsigned int phoneindex = response.indexOf(F("+CLIP: \""));               // Есть ли информация об определении номера, если да, то phoneindex>-1
        if(phoneindex >= 0) {                                                        // Если информация была найдена
          phoneindex += 8;                                                                     // Парсим строку и ...
          innerPhone = response.substring(phoneindex, response.indexOf(F("\""), phoneindex));   // ...получаем номер
        }
        if(whiteListPhones.indexOf(innerPhone)>=0){             // если номер в белом списке
          sendATCommand(F("ATA"), true);                        // поднимаем трубку
          flag_hang_up = 1;                                     // поднимаем флажок о поднятой трубке
          flag_true_call = 1;                                   // отмечаем звонок номера из белого списка
        }
        else {
          sendATCommand(F("ATH"), true);                    // в противном случае отклоняем вызов
          flag_det = 0;                                     // и не будем детектировать
          innerPhone = whiteListPhones.substring(0,12);     // возвращаем в актуальный номер -- номер по умолчанию  (Если что, на него и будет отправлено сообщение)
          break;                                            // выходим из цикла
        }
      }
      else if (response.startsWith(F("+CMTI:"))){            // пробуждение было из-за прихода смс
        flag_det = 0;
        break;
      }
      else if(response.startsWith(F("NO CARRIER"))){  // Если звонящий завершит звонок
        flag_hang_up = 0;
        break;                                             // завершаем цикл
      }
    }
  }
  if (flag_hang_up == 1){                // если из цикла выход был при входящем  звонке из белого списка
    sendATCommand(F("ATH"), true);       // кладём трубку
  }
}

// ФУНКЦИЯ ОТПРАВКИ СМС
void sendSMS(String &phone, String message){
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
  sendATCommand(F("AT+CMGDA=\"DEL ALL\""), true);               // Удалить все сообщения, чтобы не забивали память модуля
}

// ... ИЗМЕРЕНИЯ ЗАРЯДА АККУМУЛЯТОРА
String charge_parsing(){
  String response;
  response = sendATCommand(F("AT+CBC"), true);
  response.trim();                                    // Убираем лишние пробелы в начале и конце
  if (response.startsWith(F("+CBC:"))){               // если получен ответ о заряде аккума
    byte b_lb = response.indexOf("\r\n");             // находим первый перенос строки
    byte b_com = response.lastIndexOf(",");           // находим последнюю запятую
    return response.substring(b_com + 1, b_lb);     // извлекаем подстроку от последней запятой до первого переноса строки (+CBC: 0,53,3831 )
  }
}

// ... ИЗМЕРЕНИЯ КАЧЕСТВА СВЯЗИ
String quality_con(){
  String response;
  response = sendATCommand(F("AT+CSQ"), true);
  response.trim();                                    // Убираем лишние пробелы в начале и конце
  if (response.startsWith(F("+CSQ:"))){               // если получен ответ о заряде аккума
    byte b_sp = response.indexOf(" ");                // находим первый пробел
    byte b_com = response.lastIndexOf(",");           // находим последнюю запятую
    return response.substring(b_sp + 1, b_com);   // извлекаем подстроку от первого пробела до последней запятой (+CSQ: 14,0)
  }
}

//... ОБРАБОТКА AT-КОМАНДЫ
String sendATCommand(String cmd, bool waiting){         // функция отправки команды SIM800
  String resp;                                          // переменная для хранения результата
  SIM800.println(cmd);
  if(waiting){
    resp = waitResponse();
  }
  return resp;
}

// ... ОЖИДАНИЯ ОТВЕТА ОТ SIM800L
String waitResponse(){                                        // функция ожидания ответа и получения обратного результата
  String resp;                                                // переменная для хранения результата
  unsigned long _timeout = millis() + 10000;                  // переменная для ожидания 10с
  while (!SIM800.available()&& millis() < _timeout){}         // ждём получение ответа в течение 10с
  if(SIM800.available())    resp = SIM800.readString();
  return resp;
}

// ... обработка прерывания
void wakeup(){                 // функция прерывания
    detachInterrupt(0);        // выключаем внешнее прерывание на D2
    flag_att = 1;
    power.wakeUp();            // помогает выйти из sleepDelay прерыванием (вызывать в будящем прерывании)
}