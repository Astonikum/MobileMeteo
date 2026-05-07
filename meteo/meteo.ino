#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>
#include <RobotClass_LiquidCrystal.h>
#include <MQ135.h>

#define analogPin A0

#define But_1 10
#define But_2 11
#define But_3 9

#define GRAPH_POINTS 16
#define CO2_CALIBRATION_TIME 54000000UL

MQ135 gasSensor = MQ135(analogPin);

RobotClass_LiquidCrystal LCD(
  8,7,6,5,4,3,
  CP_UTF8
);

DHT dht(2, DHT11);

int mode = 1;

unsigned long startMillis = 0;
unsigned long updateMillis = 0;
unsigned long lastGraphUpdate = 0;

int t = 0;
int h = 0;
int co2 = 0;

int tempHistory[GRAPH_POINTS];
int humHistory[GRAPH_POINTS];
int co2History[GRAPH_POINTS];

byte degreeSymbol[8] = {
  0b11000,
  0b11000,
  0b00000,
  0b00111,
  0b01000,
  0b01000,
  0b00111,
  0b00000
};

byte percentSymbol[8] = {
  0b11000,
  0b11001,
  0b00010,
  0b00100,
  0b01000,
  0b10011,
  0b00011
};

byte graphChars[6][8] = {
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
  },
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111
  },
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111
  },
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111,
    0b11111
  },
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111,
    0b11111,
    0b11111
  },
  {
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
  }
};

void pushHistory(int* arr, int value) {

  for (int i = 0; i < GRAPH_POINTS - 1; i++) {
    arr[i] = arr[i + 1];
  }

  arr[GRAPH_POINTS - 1] = value;
}

void drawGraph(int* values, int minVal, int maxVal) {

  for (int i = 9; i < GRAPH_POINTS; i++) {

    int level = map(
      constrain(values[i], minVal, maxVal),
      minVal,
      maxVal,
      0,
      5
    );

    LCD.setCursor(i, 1);

    LCD.write((byte)(level + 2));
  }
}

void setup() {

  Serial.begin(9600);

  dht.begin();

  LCD.begin(16, 2);

  pinMode(But_1, INPUT_PULLUP);
  pinMode(But_2, INPUT_PULLUP);
  pinMode(But_3, INPUT_PULLUP);

  LCD.createChar(0, degreeSymbol);
  LCD.createChar(1, percentSymbol);

  for (int i = 0; i < 6; i++) {
    LCD.createChar(i + 2, graphChars[i]);
  }

  startMillis = millis();
  updateMillis = millis();

  t = round(dht.readTemperature());
  h = round(dht.readHumidity());
  co2 = round(gasSensor.getPPM());

  for (int i = 0; i < GRAPH_POINTS; i++) {
    tempHistory[i] = 22;
    humHistory[i] = 45;
    co2History[i] = 700;
  }

  LCD.clear();
  LCD.setCursor(0, 0);
}

void loop() {

  bool b1 = digitalRead(But_1);
  bool b2 = digitalRead(But_2);
  bool b3 = digitalRead(But_3);

  Serial.print(" B1=");
  Serial.print(b1);

  Serial.print(" B2=");
  Serial.print(b2);

  Serial.print(" B3=");
  Serial.print(b3);

  Serial.print(" MODE=");
  Serial.println(mode);

  // Кнопка нажата = LOW (0)

  if (!b3) {
    mode = 1;
  }

  if (!b2) {
    mode = 2;
  }

  if (!b1) {
    mode = 3;
  }

  if (millis() - updateMillis > 1000) {

    updateMillis = millis();
    Serial.print("upd");

    t = round(dht.readTemperature());
    h = round(dht.readHumidity());
    co2 = round(gasSensor.getPPM());

  }

  if (millis() - lastGraphUpdate > 4000) {

    lastGraphUpdate = millis();

    pushHistory(tempHistory, t);
    pushHistory(humHistory, h);
    pushHistory(co2History, co2);
  }

  LCD.clear();

  if (mode == 1) {

    LCD.setCursor(0, 0);
    LCD.print("Температура");

    LCD.setCursor(13, 0);
    LCD.print(String(t));

    LCD.setCursor(15, 0);
    LCD.write((byte)0);

    bool normal =
      (t >= 18 && t <= 25);

    if (normal) {

      LCD.setCursor(0, 1);
      LCD.print("В норме");
      drawGraph(tempHistory, 15, 30);

    } else {

      LCD.setCursor(0, 1);

      if (t < 20) {
        LCD.print("Холодно");
      } else {
        LCD.print("Жарко, проветрите");
      }
    }
  }

  else if (mode == 2) {

    LCD.setCursor(0, 0);
    LCD.print("Влажность");

    LCD.setCursor(13, 0);
    LCD.print(String(h));

    LCD.setCursor(15, 0);
    LCD.write((byte)1);

    bool normal =
      (h >= 40 && h <= 60);

    if (normal) {

      LCD.setCursor(0, 1);
      LCD.print("В норме");
      drawGraph(humHistory, 20, 80);

    } else {

      LCD.setCursor(0, 1);

      if (h < 40) {
        LCD.print("Сухо, увлажните");
      } else {
        LCD.print("Слишком влажно");
      }
    }
  }

  else if (mode == 3) {
    unsigned long passed =
      millis() - startMillis;

    if (passed < CO2_CALIBRATION_TIME) {
      int co2 = 0;

      unsigned long remain =
        (CO2_CALIBRATION_TIME - passed)
        / 3600000UL;

      LCD.setCursor(0, 1);

      LCD.print("Калибровка ");
      LCD.print(String(remain + 1));
      LCD.print("ч");

    } else {

      bool normal =
        (co2 <= 1000);

      if (normal) {

        LCD.setCursor(0, 1);
        LCD.print("В норме");
        drawGraph(co2History, 400, 2000);

      } else {

        LCD.setCursor(0, 1);

        if (co2 <= 1400) {
          LCD.print("Проветривайте!");
        } else {
          LCD.print("Проветр. сильно!");
        }
      }
    }
    LCD.setCursor(0, 0);
    LCD.print("CO2");

    LCD.setCursor(10, 0);
    LCD.print(String(co2));

    LCD.setCursor(13, 0);
    LCD.print("ppm");
  }

  delay(100);
}