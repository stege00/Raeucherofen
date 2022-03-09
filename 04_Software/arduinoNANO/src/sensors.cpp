#include "sensor.h"

// FiFo storage
const int ELEMENT_CNT_MAX = 20;                         // defines maximum count of datasets that are stored
int cnt_sensorData = 0;
sensorData data_latest;                                 // will store latest set of sensor data
sensorData data_saved[ELEMENT_CNT_MAX];                 // will store latest x sets of data

// Sensor variables
const int analog_flame_threshold = 800;                 // 0... 1023
int cnt_flame = 0;                                      // counts flame detections in given tries
const int analog_hall_threshold = 800;
int cnt_hall = 0;


// necessary protothreads
pt ptSensorFire;
pt ptSensorHall;
pt ptSensorTempHum;

// status of protothreads
int status_pt1 = 1;
int status_pt2 = 1;
int status_pt3 = 1;

void get_latest_data(struct pt* pt) {
  PT_BEGIN(pt);  

  getDateTime();

  status_pt1 = PT_SCHEDULE(get_sensor_fire(&ptSensorFire, 1));
  status_pt2 = PT_SCHEDULE(get_sensor_hall(&ptSensorHall));
  status_pt3 = PT_SCHEDULE(get_sensor_temperature_humidity(&ptSensorTempHum));
  // PT_SCHEDULE returns int: 0 if finished, nonzero if still running
  PT_WAIT_WHILE(status_pt1 | status_pt2 | status_pt3);

  if (cnt_sensorData >= ELEMENT_CNT_MAX) {
    // first in first out storage:
    for (int i=0; i < ELEMENT_CNT_MAX - 1; i++){
      data_saved[i]=data_saved[i+1];
    }
    data_saved[ELEMENT_CNT_MAX - 1] = data_latest;
  }else {
    data_saved[cnt_sensorData]=data_latest;
    cnt_sensorData += 1;
  }

  PT_END(pt);
}

void getDateTime() {
  /*
  Function to get current Date and Time via the WiFi module.
  */

  unsigned long epochTime = WiFi.getTime() + UTC_OFFSET;               // time since 1.1.1970 in sec.
  int currentYear = year(epochTime);
  int currentMonth = month(epochTime);
  int monthDay = day(epochTime);
  int currentHour = hour(epochTime);
  int currentMinute = minute(epochTime);
  int currentSecond = second(epochTime);
  
  (String(monthDay)+"."+String(currentMonth)+"."+String(currentYear)).toCharArray(data_latest.date_formatted, 11);
  (String(currentHour)+":"+String(currentMinute)+":"+String(currentSecond)).toCharArray(data_latest.time_formatted, 9); 
}

int get_sensor_temperature_humidity(struct pt* pt) {
  /*
  Function to get sensor data from the temperature/humidity sensor module.
  Requests sensor data via I2C (adress of HYT221 is 0x28).
  First two bytes are humidity the last two are temperature.
  */

  PT_BEGIN(pt);

  Wire.beginTransmission(i2cAdress_HumTemp);
  Wire.requestFrom(i2cAdress_HumTemp, 4);

  PT_YIELD_UNTIL(pt, (Wire.availible == 4));

  // Read the bytes if they are available
  int c1 = Wire.read();
  int c2 = Wire.read();
  int c3 = Wire.read();
  int c4 = Wire.read();
  
  Wire.endTransmission();

  // combine bytes
  int rawHumidity = b1 << 8 | b2;
  // first two bits are status/stall bits --> compound bitwise to get 14 bit measurement
  rawHumidity =  (rawHumidity &= 0x3FFF);
  rawHumidity = rawHumidity * 1000 >> 14;           // sensordata in permille

  // Mask away 2 least significant bits (14 bit measurement)
  b4 = (b4 >> 2);
  // combine bytes
  int rawTemperature = b3 << 6 | b4;
  rawTemperature = 1650 * rawTemperature >> 14 - 40    // sensordata factor 10 to big

  data_latest.humidity = rawHumidity;
  data_latest.temperature = rawTemperature;

  PT_END(pt);
}

int get_sensor_fire(struct pt* pt, int para) {
  /*
  Function to get sensor data from the fire detection sensor module.
  Reads repeatedly sensor data to circumvent bouncing of sensor, if more than 60% of the Sensorvalues show positive, give positive flamedetection back.
  */
  PT_BEGIN(pt);

  cnt_flame = 0;

  for (int i = 0; i < 10; i++)
  {
    if (para && (analogRead(pin_flame_analog) > analog_flame_threshold)) {
      cnt_flame ++;
    }
    else if (!para && (digitalRead(pin_flame_digital)))
    {
      cnt_flame ++;
    }
    PT_SLEEP(5);
  }

  if (cnt_flame > 6) 
  {
    data_latest.flame_detection = true;
  }
  else
  {
    data_latest.flame_detection = false;
  }

  PT_END(pt);
}

int get_sensor_hall(struct pt* pt) {
  /*
  Function to get sensor data from the hall sensor (door detection).
  Reads repeatedly sensor data to circumvent bouncing of sensor, if less than 50% of the Sensorvalues show positive, give open door detection back.
  */
	PT_BEGIN(pt);

  cnt_hall = 0;

  for (int i = 0; i < 10; i++)
  {
    if (analogRead(pin_hall_analog) > analog_hall_threshold) {
      cnt_hall ++;
    }
    PT_SLEEP(5);
  }

  if (cnt_hall < 5) 
  {
    data_latest.open_door = true;
  }
  else
  {
    data_latest.open_door = false;
  }

  PT_END(pt);
}
