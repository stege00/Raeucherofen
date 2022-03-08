#include "menu.h"

int check_buttons(struct pt* pt) {
  /*
  function to check input from buttons and debounce if needed
  */

  PT_BEGIN(pt);

  if(digitalRead(buttons.DOWN))
  {
    PT_SLEEP(pt, 5);
    if(digitalRead(buttons.DOWN))
    {
		  if(state_screen!=state_min)
		  {
		    next_state-=1;
		  }
      PT_YIELD_UNTIL(pt, !digitalRead(buttons.DOWN));
    }
  }
  if(digitalRead(buttons.UP))
  {
    PT_SLEEP(pt, 5);
    if(digitalRead(buttons.UP))
    {
		  if(state_screen!=state_max)
		  {
		    next_state+=1;
		  }
      PT_YIELD_UNTIL(pt, !digitalRead(buttons.UP));
    }
  }

  PT_END(pt);
}

void update_lcd() {
  last_state=state_screen;
  state_screen=next_state;
  //state_machine

  switch (state_screen)
  {
      case 0:							  //all sensors
      {
        lcd.print("Case 0 ");
        break;
      }
      case 1:							//data temp
      {
        lcd.print("Temp: ");
        lcd.print((float)data_latest.temperature);
        break;
      }
      case 2:							//data humi
      {
		    lcd.print("Feucht: ");
        lcd.print((float)data_latest.humidity);
        break;
      }
      case 3:							//data fire
      {
        lcd.print("Feuer: ");
        if(data_latest.flame_detection)
			  {
				  lcd.print("Ja");
			  }
		    else
			  {
				  lcd.print("Nein");
			  }
		    // lcd.setCursor(0,1);
		    // lcd.print(get_sensor_fire(1));
        break;
      }
      case 4:							//data hall
      {
        lcd.print("Tuer ");
        if(data_latest.open_door)
			  {
				  lcd.print("auf");
			  }
		    else
			  {
				  lcd.print("zu");
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
}


