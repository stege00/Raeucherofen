#include <LiquidCrystal.h>
#include <protothreads.h>

int check_buttons(struct pt* pt);
void update_lcd();

// menu variables
int state_screen = 1, next_state = 0, last_state = 0;
const int state_min = 0, state_max = 5;