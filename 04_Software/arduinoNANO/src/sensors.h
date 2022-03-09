#include <Wire.h>
#include <TimeLib.h>
#include <protothreads.h>

#define UTC_OFFSET (3600)

void get_latest_data(struct pt* pt);
void getDateTime();
int get_sensor_temperature_humidity(struct pt* pt);
int get_sensor_fire(struct pt* pt, int para);
int get_sensor_hall(struct pt* pt);

const int analog_flame_threshold = 800;                 // 0... 1023
int cnt_flame = 0;                                      // counts flame detections in given tries
const int analog_hall_threshold = 800;
int cnt_hall = 0;
