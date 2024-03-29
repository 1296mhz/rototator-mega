#define LCD_2004A
//#define LCD_I2C_2004A
#define COMPORT_SPEED 19200
#define PIN_CCW 24  // Поворот против часовой стрелки
#define PIN_CW 22   // Поворот по часовой стрелки
#define PIN_UP 26   // Актуатор вверх
#define PIN_DOWN 28 // Актуатор вниз
#define STEP 1      // Шаг
// Датчики
#define AZSENSOR A0 // Номер пина для аналогового датчика азимута
#define ELSENSOR A2 // Номер пина для аналогового датчика элевации
// Кнопки
#define BTN_CW 23
#define BTN_CCW 25
#define BTN_UP 27
#define BTN_DOWN 29
#define BTN_APPLY_AZ 31
#define BTN_APPLY_EL 30
//Переделать с 19 на 20
#define BTN_OPERATE 20
#define BTN_MODE 21
int prevAz = 0;
int prevEl = 0;
bool prevOperate;
byte prevAzArrow = 0;
byte prevElArrow = 0;
byte prevMode = 0;
//петля усреднения
const int numReadings = 25;
int readIndexAz = 0;
int readIndexEl = 0;
int azAngle = 0;          // Угол азимута
int azOldSensorValue = 0; // Предыдущее значение с датчика азимута
int azTarget = 300;       // Цель для поворота
boolean azMove = false;   // Флаг включения/отключения вращения по азимуту
String strAzAngle;        // Текущее положение антенны
String strAzTarget;       // Цель для перемещения
int azCorrect = 0;        // Коррекция азимута нуля градусов
int oldsensorValue = 0;
int totalAz = 0;
int averageAz = 0; // усреднение азимута
int correctionAz = 10;
int elAngle = 0;
int elOldSensorValue = 0;
int elTarget = 0;
boolean overlap = false;
boolean noLimit = true;
boolean elMove = false;
String strOverlap = "no";
String strElAngle;
String strElTarget;
int elCorrect = 0;
unsigned long timing;

int totalEl = 0;
int averageEl = 0; // усреднение элевации
boolean operate = false;
byte mode = 0;

String PortRead;
int azPortTarget = 0;
int elPortTarget = 0;
const byte averageFactor = 30;
// String Serial1Name = "                ";
// 0 - clear, 1 - CW or UP, 2 - CCW or DOWN
byte azArrow = 0;
byte elArrow = 0;

char motd[] = "R8CDF ROTATOR 22R1  ";
byte heart[8] = {0b00000, 0b01010, 0b11111, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000};
byte heartOff[8] = {0b00000, 0b01010, 0b11111, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000};
byte upArrow[8] = {0b00000, 0b00000, 0b00100, 0b01010, 0b10001, 0b00000, 0b00000, 0b00000};
byte dwArrow[8] = {0b00000, 0b00000, 0b10001, 0b01010, 0b00100, 0b00000, 0b00000, 0b00000};

long previousMillisTimeSWOne = 0; //счетчик прошедшего времени для мигания изменяемых значений.
long previousMillisTimeSWTwo = 0; //счетчик прошедшего времени для мигания изменяемых значений.
long intervalTimeSWOne = 500;     //интервал мигания изменяемых значений.
long intervalTimeSWTwo = 100;     //интервал мигания изменяемых значений.
long iOperateView = 0;
String nameTarget;
#if defined(LCD_2004A)
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#endif

#if defined(LCD_I2C_2004A)
// Wiring: SDA pin is connected to A4 and SCL pin to A5.
// Connect to LCD via I2C, default address 0x27 (A0-A2 not jumpered)
#include <Wire.h>                                       // Library for I2C communication
#include <LiquidCrystal_I2C.h>                          // Library for LCD
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // address, chars, rows.
#endif

int avaregeAprox(int sensorValue)
{
  if (averageFactor > 0) // усреднение показаний для устранения "скачков"
  {
    oldsensorValue = sensorValue;
    return (oldsensorValue * (averageFactor - 1) + sensorValue) / averageFactor;
  }
}

uint8_t btn(int KEY)
{
  bool currentValue = digitalRead(KEY);
  bool prevValue;
  if (currentValue != prevValue)
  {
    delay(75);
    currentValue = digitalRead(KEY);
    return 0;
  }

  prevValue = currentValue;
  return 1;
};

int azSensor()
{

  azAngle = avaregeAprox(analogRead(AZSENSOR));
  azAngle = int(azAngle / 1024.0 * 360) + correctionAz;

  if (azAngle >= 0 && azAngle <= 360)
  {
    strOverlap = "no";
    overlap = false;
  }

  if (azAngle == 3)
  {
    overlap = true;
    azAngle = 1;
  }

  if (azAngle < 3)
  {
    strOverlap = "overlap <";
    azAngle = 1;
  }

  if (azAngle >= 360)
  {
    overlap = true;
    strOverlap = "overlap >";
    azAngle = 360;
  }

  return azAngle;
}

int elSensor()
{
  elAngle = avaregeAprox(stabilitySensor(analogRead(ELSENSOR)));
  // elAngle = int(elAngle / 1024.0 * 277);
  elAngle = 180 - int(elAngle / 1024.0 * 180);
  // if (elAngle < 0)
  // {
  //   elAngle = 0;
  // }

  // if (elAngle >= 89 && elAngle <= 91)
  // {
  //   elAngle = 90;
  // }

  // if (elAngle >= 110 && elAngle <= 360)
  // {
  //   elAngle = 0;
  // }

  return elAngle;
}

void getSensors()
{
  azSensor();
  elSensor();
}

void getKeysMain()
{
  if (btn(BTN_CW) == 0)
  {
    delay(1);
    if (azTarget + STEP <= 360)
      azTarget += STEP;
  }

  if (btn(BTN_CCW) == 0)
  {
    delay(1);
    if (azTarget - STEP >= 1)
      azTarget -= STEP;
  }

  if (btn(BTN_UP) == 0)
  {
    delay(10);
    if (elTarget + STEP <= 360)
      elTarget += STEP;
  }

  if (btn(BTN_DOWN) == 0)
  {
    delay(10);
    if (elTarget - STEP >= 0)
      elTarget -= STEP;
  }

  if (btn(BTN_APPLY_AZ) == 0)
  {
    delay(50);
    azMove = true;
  }

  if (btn(BTN_APPLY_EL) == 0)
  {
    delay(50);
    elMove = true;
  }
}

void getKeysOperate()
{

  /*
      0 MANUAL
      1 PC
      2 MANUAL BUTTON
  */
  if (btn(BTN_MODE) == 0)
  {
    if (mode == 2)
    {
      mode = 0;
      azTarget = 300;
      elTarget = 0;
    }
    else if (mode == 0)
    {
      mode = 1;
    }
    else if (mode == 1)
    {
      mode = 2;
      azTarget = 300;
      elTarget = 0;
    }
  }

  if (btn(BTN_OPERATE) == 0)
  {
    delay(100);
    if (operate)
    {
      operate = false;
      azArrow = 0;
      elArrow = 0;
      iOperateView = 0;
      if (azMove)
      {
        azMove = false;
      }

      if (elMove)
      {
        elMove = false;
      }

      digitalWrite(PIN_CW, LOW);
      digitalWrite(PIN_CCW, LOW);
      digitalWrite(PIN_UP, LOW);
      digitalWrite(PIN_DOWN, LOW);
    }
    else
    {
      operate = true;
      lcd.setCursor(17, 3);
      lcd.print(char(1));
    }
  }
}

void buttonManual(int az, int el)
{

  if (btn(BTN_CW) == 0 && az < 450)
  {
    digitalWrite(PIN_CW, HIGH);
    azArrow = 1;
  }
  else
  {
    digitalWrite(PIN_CW, LOW);
    azArrow = 0;
  }

  if (btn(BTN_CCW) == 0 && az >= 1)
  {
    digitalWrite(PIN_CCW, HIGH);
    azArrow = 2;
  }
  else
  {
    digitalWrite(PIN_CCW, LOW);
    // azArrow = 0;
  }

  if (btn(BTN_DOWN) == 0 && el <= 360)
  {
    digitalWrite(PIN_UP, HIGH);
    elArrow = 2;
  }
  else
  {
    digitalWrite(PIN_UP, LOW);
    elArrow = 0;
  }

  if (btn(BTN_UP) == 0 && el >= 0)
  {
    if (el <= 90)
    {
      digitalWrite(PIN_DOWN, HIGH);
      elArrow = 1;
    }
  }
  else
  {
    digitalWrite(PIN_DOWN, LOW);
    //  elArrow = 0;
  }
}

void serialManual()
{

  if (Serial1.available())
  {
    int inByte = Serial1.read();
    Serial.write(inByte);
    if (inByte == 111)
    {
      String ser = String("AZ:" + String(azAngle) + "#EL:" + String(elAngle) + "#OP:" + String(operate) + "#AZARROW:" + String(azArrow) + "#ELARROW:" + String(elArrow) + "#MODE:" + String(mode));
      Serial1.println(ser);
    }
  }
}

String IntAzToString(int someIntVolue)
{
  if (someIntVolue < 0)
  {
    return "" + String(someIntVolue);
  }
  if (someIntVolue < 10)
  {
    return "  " + String(someIntVolue);
  }

  if (someIntVolue < 100)
  {
    return " " + String(someIntVolue);
  }

  if (someIntVolue >= 100)
  {
    return String(someIntVolue);
  }
}

String IntElToString(int someIntVolue)
{
  if (someIntVolue < 0)
  {
    return "" + String(someIntVolue);
  }
  if (someIntVolue < 10)
  {
    return "  " + String(someIntVolue);
  }

  if (someIntVolue < 100)
  {
    return " " + String(someIntVolue);
  }

  if (someIntVolue >= 100)
  {
    return String(someIntVolue);
  }
}

void cw(boolean azMoveFlag)
{
  if (azMoveFlag)
  {
    digitalWrite(PIN_CCW, LOW);
    azArrow = 1;
    digitalWrite(PIN_CW, HIGH);
  }
  else
  {
    azArrow = 0;
    digitalWrite(PIN_CW, LOW);
  }
}

void ccw(boolean azMoveFlag)
{
  if (azMoveFlag)
  {
    digitalWrite(PIN_CW, LOW);
    azArrow = 2;
    digitalWrite(PIN_CCW, HIGH);
  }
  else
  {
    azArrow = 0;
    digitalWrite(PIN_CCW, LOW);
  }
}

void up(boolean elMoveFlag)
{
  if (elMoveFlag)
  {
    digitalWrite(PIN_DOWN, LOW);
    elArrow = 1;
    digitalWrite(PIN_UP, HIGH);
  }
  else
  {
    digitalWrite(PIN_UP, LOW);
  }
}
void down(boolean elMoveFlag)
{
  if (elMoveFlag)
  {
    digitalWrite(PIN_UP, LOW);
    elArrow = 2;
    digitalWrite(PIN_DOWN, HIGH);
  }
  else
  {
    digitalWrite(PIN_UP, LOW);
  }
}

int stabilitySensor(int SENSOR)
{

  int currentValue = SENSOR;
  int prevValue;
  if (currentValue != prevValue)
  {

    if (millis() - timing > 50000)
    { // Вместо 10000 подставьте нужное вам значение паузы
      timing = millis();
      currentValue = SENSOR;
      return currentValue;
    }
  }

  prevValue = currentValue;
  // return prevValue;
}

void applyKeys()
{
  if (operate)
  {
    if (btn(BTN_APPLY_AZ) == 0)
    {
      delay(50);
      azMove = true;
    }

    if (btn(BTN_APPLY_EL) == 0)
    {
      delay(50);
      elMove = true;
    }
  }
}

// Views display func
void operateView()
{
  if (operate)
  {

    for (int i = 0; i <= 2; i++)
    {
      if (millis() - previousMillisTimeSWOne >= intervalTimeSWOne)
      {
        iOperateView = !iOperateView;
        previousMillisTimeSWOne = previousMillisTimeSWOne + intervalTimeSWOne;
      }

      if (millis() - previousMillisTimeSWTwo >= intervalTimeSWTwo)
      {
        iOperateView = !iOperateView;
        previousMillisTimeSWTwo = previousMillisTimeSWTwo + intervalTimeSWTwo;
      }

      if (azMove || elMove)
      {
        if (iOperateView == 1)
        {
          lcd.setCursor(17, 3);
          lcd.print(char(1));
        }
        if (iOperateView == 0)
        {
          lcd.setCursor(17, 3);
          lcd.print(" ");
        }
      }
      else
      {
        lcd.setCursor(17, 3);
        lcd.print(char(1));
      }
    }
  }
  else
  {
    lcd.setCursor(17, 3);
    lcd.print(" ");
    lcd.setCursor(18, 3);
    lcd.print(" ");
    lcd.setCursor(19, 3);
    lcd.print(" ");
  }
}

void modeView()
{
  if (mode == 1)
  {
    lcd.setCursor(0, 3);
    lcd.print("PC           ");
    lcd.setCursor(0, 2);
    // lcd.print("SN: ");
    // lcd.setCursor(4, 2);
    // lcd.setCursor(0, 2);
    // lcd.print(Serial1Name);
  }
  else if (mode == 0)
  {
    lcd.setCursor(0, 3);
    lcd.print("MANUAL       ");
  }
  else if (mode == 2)
  {
    lcd.setCursor(0, 3);
    lcd.print("BUTTON MANUAL");
  }
}

void azArrowView()
{
  if (azArrow == 0)
  {
    lcd.setCursor(18, 3);
    lcd.print(" ");
  }
  else if (azArrow == 1)
  {
    lcd.setCursor(18, 3);
    lcd.print(">");
  }
  else if (azArrow == 2)
  {
    lcd.setCursor(18, 3);
    lcd.print("<");
  }
}

void elArrowView()
{

  if (elArrow == 0)
  {
    lcd.setCursor(19, 3);
    lcd.print(" ");
  }
  else if (elArrow == 1)
  {
    lcd.setCursor(19, 3);
    lcd.print(char(2));
    // lcd.print("U");
  }
  else if (elArrow == 2)
  {
    lcd.setCursor(19, 3);
    lcd.print(char(3));
  }
}
void SerialSend()
{
  if (prevAz != azAngle || prevEl != elAngle || prevAzArrow != azArrow || prevElArrow != elArrow || prevMode != mode || prevOperate != operate)
  {
    String ser = String("AZ:" + String(azAngle) + "#EL:" + String(elAngle) + "#OP:" + String(operate) + "#AZARROW:" + String(azArrow) + "#ELARROW:" + String(elArrow) + "#MODE:" + String(mode));
    Serial1.println(ser);
    prevEl = elAngle;
    prevAz = azAngle;
    prevAzArrow = azArrow;
    prevElArrow = elArrow;
    prevMode = mode;
    prevOperate = operate;
  }
}

void setup()
{
  Serial1.begin(COMPORT_SPEED);
  Serial1.println(String("MOTD:" + String(motd)));
  Serial.begin(9600);
  Serial.println(String("MOTD:" + String(motd)));
  pinMode(PIN_CCW, OUTPUT);
  pinMode(PIN_CW, OUTPUT);
  pinMode(PIN_UP, OUTPUT);
  pinMode(PIN_DOWN, OUTPUT);
  pinMode(AZSENSOR, INPUT);
  pinMode(ELSENSOR, INPUT);

  pinMode(BTN_OPERATE, INPUT_PULLUP);
  pinMode(BTN_APPLY_AZ, INPUT_PULLUP);
  pinMode(BTN_APPLY_EL, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_CW, INPUT_PULLUP);
  pinMode(BTN_CCW, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  lcd.begin(20, 4);
#if defined(LCD_I2C_2004A)
  lcd.backlight();
#endif

  lcd.setCursor(0, 0);
  lcd.createChar(1, heart);
  lcd.createChar(2, upArrow);
  lcd.createChar(3, dwArrow);

  // lcd.print("R8CDF ROTATOR 2020 ");
  // for (int i = 0; i <= 19; i++)
  // {
  //   lcd.setCursor(i, 1);
  //   lcd.print("_");
  //   delay(150);
  //   lcd.setCursor(i, 1);
  //   lcd.print(motd[i]);
  //   delay(30);
  // }

  delay(100);
  // for (int i = 0; i <= 19; i++)
  // {
  //   lcd.setCursor(i, 1);
  //   lcd.print(" ");
  //   delay(1);
  // }
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("AZT: ");
  lcd.setCursor(0, 1);
  lcd.print("AZA: ");
  lcd.setCursor(9, 0);
  lcd.print("ELT: ");
  lcd.setCursor(9, 1);
  lcd.print("ELA: ");
  getSensors();
  operateView();
  modeView();
  azArrowView();
  elArrowView();
  SerialSend();
}

void loop()
{
  getSensors();
  SerialSend();
  getKeysOperate();
  operateView();
  modeView();
  azArrowView();
  elArrowView();

  if (operate)
  {
    if (mode == 1)
    {
      if (Serial1.available())
      {
        PortRead = Serial1.readString();
        //        int buffer_len = PortRead.length() + 1;
        //        char port_char_array[buffer_len];
        char az[7], el[7], sn[12];

        sscanf(PortRead.c_str(), "%s %s %s", &az, &el, &sn);
        azTarget = int(round(atof(az)));
        elTarget = int(round(atof(el)));
        nameTarget = String(sn);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("NAME: ");
        lcd.setCursor(6, 2);
        lcd.print(nameTarget.substring(0, 14));

        if (azTarget >= 0 && azTarget <= 360)
        {
          azMove = true;
        }

        if ((elTarget >= 0) && (elTarget <= 99))
        {
          elMove = true;
        }
        Serial1.flush();
      }
    }

    if (mode == 0)
    {
      applyKeys();
      getKeysMain();
    }

    if (mode == 2)
    {
      buttonManual(azAngle, elAngle);

      if (Serial1.available())
      {
        int inByte = Serial1.read();
        Serial.write(inByte);
        lcd.setCursor(0, 2);
        lcd.print(inByte);

        if (inByte == 62 && azAngle < 359)
        {
          digitalWrite(PIN_CW, HIGH);
          azArrow = 1;
        }
        else
        {
          digitalWrite(PIN_CW, LOW);
          azArrow = 0;
        }

        if (inByte == 60 && azAngle > 1)
        {
          digitalWrite(PIN_CCW, HIGH);
          azArrow = 2;
        }
        else
        {
          digitalWrite(PIN_CCW, LOW);
          // azArrow = 0;
        }

        if (inByte == 117 && elAngle <= 99)
        {
          digitalWrite(PIN_UP, HIGH);
          elArrow = 1;
        }
        else
        {
          digitalWrite(PIN_UP, LOW);
          elArrow = 0;
        }

        if (inByte == 100 && elAngle >= 0)
        {
          digitalWrite(PIN_DOWN, HIGH);
          elArrow = 2;
        }
        else
        {
          digitalWrite(PIN_DOWN, LOW);
          //  elArrow = 0;
        }
      }
    }

    if (azMove)
    {
      if (azTarget == azAngle)
      {
        azArrow = 0;
        digitalWrite(PIN_CW, LOW);
        digitalWrite(PIN_CCW, LOW);
        azMove = false;
      }
      else if (azTarget - azAngle >= 1)
      {
        cw(azMove);
      }
      else if (azAngle - azTarget >= 1)
      {
        ccw(azMove);
      }
    }

    if (elMove)
    {
      if (elTarget == elAngle)
      {
        elArrow = 0;
        digitalWrite(PIN_UP, LOW);
        digitalWrite(PIN_DOWN, LOW);
        elMove = false;
      }
      if (elTarget - elAngle >= 1)
      {

        down(elMove);
      }

      if (elAngle - elTarget >= 1)
      {
        up(elMove);
      }
    }
  }

  // Отображение азимута текущей цели антенны
  lcd.setCursor(5, 0);
  lcd.print(strAzTarget);
  // Отображение азимута текущего положения антенны
  lcd.setCursor(5, 1);
  lcd.print(strAzAngle);
  // Отображение данных с датчика азимута
  strAzAngle = IntAzToString(azAngle);
  // Отображение цели азимута
  strAzTarget = IntAzToString(azTarget);

  // Отображение елевации цели антенны
  lcd.setCursor(13, 0);
  lcd.print(strElTarget);
  // Отображение элевации
  lcd.setCursor(13, 1);
  lcd.print(strElAngle);
  // Отображение данных с датчика элевации
  strElAngle = IntElToString(elAngle);
  // Отображение цели элевации
  strElTarget = IntElToString(elTarget);
}
