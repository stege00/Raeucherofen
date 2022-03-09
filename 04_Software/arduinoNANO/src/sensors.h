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
extern const int ELEMENT_CNT_MAX;
extern int cnt_sensorData;
extern sensorData data_latest;
extern sensorData data_saved[];

// Sensor variables
extern const int analog_flame_threshold;
extern int cnt_flame;
extern const int analog_hall_threshold;
extern int cnt_hall;
