//include
	#include <LiquidCrystal.h>

//LCD
	const int RS = 8, EN = 9, D4 = 4, D5 = 5, D6 = 6, D7 = 7;
	LiquidCrystal lcd(RS,EN,D4,D5,D6,D7);

//Muenüvaribalen
	int state_screen=1;
	int next_state=0;
	int last_state=0;
	int state_min=0;
	int state_max=5;
//Taster
	const int UP = 21, DOWN = 20, OK = 18;

//Sensoren




void setup() {
//debug
	pinMode(14, OUTPUT);

//LCD
	lcd.begin(16,2);    //set 16 columns and 2 rows of 16x2 LCD
  
  
//Taster
	pinMode(UP, INPUT);
	pinMode(DOWN, INPUT);
	pinMode(OK, INPUT);

}


double get_sensor_temp()
{
	return 100;
}

double get_sensor_humi()
{
	return 200;
}

bool get_sensor_fire()
{
	return true;
}

bool get_sensor_hall()
{
	return false;
}


void update_state()
{
  last_state=state_screen;
  state_screen=next_state;
}





void loop() {
  lcd.clear();
 //DOWN debounce
  if(digitalRead(DOWN))
  {
    delay(5);
    if(digitalRead(DOWN))
    {
		if(state_screen!=state_min)
		{
		  next_state-=1;
		}
    while(digitalRead(DOWN)){}
    }
  }
  

//UP debounce
  if(digitalRead(UP))
  {
    delay(5);
    if(digitalRead(UP))
    {
		if(state_screen!=state_max)
		{
		next_state+=1;
		}
    while(digitalRead(UP)){}
    }
  }
 /*
//OK debounce
   if(digitalRead(OK))
  {
    delay(5);
    if(digitalRead(OK))
    {
	
    while(digitalRead(OK)){}
    }
  }*/

//update state
  update_state();

//state_machine

  switch (state_screen)
  {
      case 0:							//all sensors
      {
        lcd.print("Case 0 ");
        break;
      }
       case 1:							//data temp
      {
        lcd.print("Temp: ");
        lcd.print((float)get_sensor_temp());
        break;
      }
       case 2:							//data humi
      {
        lcd.print("Feucht: ");
        lcd.print((float)get_sensor_humi());
        break;
      }
       case 3:							//data fire
      {
        lcd.print("Feuer: ");
        if(get_sensor_fire())
			{
				lcd.print("Ja");
			}
		else
			{
				lcd.print("Nein");
			}
        break;
      }
       case 4:							//data hall
      {
        lcd.print("Tuer ");
        if(get_sensor_hall())
			{
				lcd.print("zu");
			}
		else
			{
				lcd.print("auf");
			}
        break;
      }
       case 5:
      {
        lcd.print("Case 5 ");
        break;
      }
      default:
      {
        lcd.print("error ");
      }
  }
  
  
  lcd.setCursor(0,1);
  lcd.print((float)state_screen);
  delay(80);
}