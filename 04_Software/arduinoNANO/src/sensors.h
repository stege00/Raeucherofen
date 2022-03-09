#include <Wire.h>
#include <TimeLib.h>
#include <protothreads.h>

#define UTC_OFFSET (3600)

void get_latest_data(struct pt* pt);
void getDateTime();
int get_sensor_temperature_humidity(struct pt* pt);
int get_sensor_fire(struct pt* pt, int para);
int get_sensor_hall(struct pt* pt);

// definition of storage struct
  typedef struct sensorData
  {
    int humidity=0;
    int temperature=0;
    bool flame_detection=false;
    bool open_door=false;
    char date_formatted[11];
    char time_formatted[9];
  }sensorData;

// FiFo storage
const int ELEMENT_CNT_MAX = 20;                   // defines maximum count of datasets that are stored
int cnt_sensorData = 0;
sensorData data_latest;                           // will store latest set of sensor data
sensorData data_saved[ELEMENT_CNT_MAX];           // will store latest x sets of data

// Sensor variables
const int analog_flame_threshold = 800;                 // 0... 1023
int cnt_flame = 0;                                      // counts flame detections in given tries
const int analog_hall_threshold = 800;
int cnt_hall = 0;
