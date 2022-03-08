#include <Wire.h>
#include <TimeLib.h>
#include <protothreads.h>

void get_latest_data(struct pt* pt);
void getDateTime();
int get_sensor_temperature_humidity(struct pt* pt);
int get_sensor_fire(struct pt* pt, int para);
int get_sensor_hall(struct pt* pt);