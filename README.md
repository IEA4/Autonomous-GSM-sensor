## Автономный GSM-датчик

### Автономный датчик удалённого наблюдения с передачей данных по мобильной связи

### __Необходимые компоненты:___

### __Функционал:___

Посредством датчика устройство призводит проверку объекта слежения через заданный в коде интервал времени. Если обработчик прибора зафиксирует успех -- отправит сообщение владельцу в виде "заряд аккумулятора в мВ, 1". На прибор можно звонить, отвечает на номера из белого списка. На каждый такой вызов отвечает сообщением с зарядом аккумулятора. По истечении заряда отправляет оповещение о низком уровне.

#### ___Примечание:___

В папке bibl находятся используемые в проекте библиотеки, из которых все кроме FastDefFunc можно найти [у Алекса Гайвера](https://github.com/GyverLibs). FastDefFunc содержит несколько "убыстрённых" стандартных функций:  можно их и не использовать (что нежелательно), тогда нужно в имеющихся в коде "какая_то_стандартная_функцияFast" убрать суффикс "Fast".

Папка "Test_IR_detector" --- для теста работоспособности пары ИК-светодиод/ИК-фоторезистор.

Папка "Test_AT_command" --- для тестов работы связки Arduino+SIM800L на выполнение AT-команд.
Последние смотреть в руководстве SIM800_Series_AT_Command_Manual_V1.09

Папка "Autonom_GSM-detector" --- схема, порядок включения, пример приходящих сообщений, код автономного
датчика удалённого наблюдения за ловушкой