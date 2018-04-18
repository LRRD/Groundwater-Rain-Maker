//Mason Parrone 6/20/17 - Changed aVal and bVal after calculating linear equation following short volumetric test with Steve Gough.
//Mason Parrone 3/28/18 - Rewritten for Groundwater - Issues with error values due to math structure of old groundwater code. RPS would round, Ml/s calculated from RPS.
//Mason Parrone 3/30/18 - PulseIn High instead of low because low took too long to report readings at low flows, changed from linear to polynomial to increase accuracy
//Introduced last Q to help filter out crap values, current sens reading compares to last Q if > 15 it filters it out. Got rid of all the crap values :D
//Increased encoder_count to +10 for CW direction of knob less than 30. This gets keeps the user from having to turn the knob 15 times to get water to come out. 
#include <LiquidCrystal.h>

LiquidCrystal lcd (9, 8, 4, 5, 6, 7);  // LCD 4-bit interface.
static double set_PWM = 0; //Sets starting PWM speed to 0.
static int encoder_count = 0;
#define paddlewheelPin 2 //Pin connected to signal wire of paddlewheel sensor
int FlowRate = 0; //Stores flow rate
float aVal = -.0126;    ////
float bVal = 1.6749;    ////a, b, and c values from: ax^2 + bx + c        //Values found for Texas Tech Em3 Groundwater System
float cVal = .9671;    ////
float freQuency = 0;
int released;
float mlspersec = 0;
int lastQ = 0;
#define range 60 //Maximum value in microseconds at slowest spinning paddlewheel speed

void readPaddlewheel ()
{
  //function reads just one pulse :
  freQuency = (abs(pulseIn(paddlewheelPin, HIGH)));
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //Calculate flow and discard crap values
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if ((freQuency > 0) && (freQuency < range)) //Non-crap frequency value received
  {
    FlowRate = (range - (freQuency)); //Only display the good values (magFlowRate) not all freQuency values
    if ((abs(FlowRate - lastQ)) < 15)
    {
      lastQ = (FlowRate); //Store this good value as last Q
      mlspersec = ((aVal * (FlowRate * FlowRate)) + (bVal * FlowRate) + (cVal)); //Still gotta dope it with A, B, and C
    }
    else //Within range but crap value
    {
      FlowRate = (lastQ); //Display last good value if bad value received
    }
  }
  else
  {
    FlowRate = (lastQ); //Display last good value if bad value received
  }
}

////////////////////////////////////////////////////////////////////////////
// Encoder handling. ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Taken From Alix controller code.
// The encoder count value.
// The sequence of gray codes in the increasing and decreasing directions.
static uint8_t cw_gray_codes[4] = { 2, 0, 3, 1 };
static uint8_t ccw_gray_codes[4] = { 1, 3, 0, 2 };
// The gray code we last read from the encoder.
static uint8_t previous_gray_code = 0;
// Reset the encoder to zero.
static void reset_encoder()
{
  encoder_count = 0;
}

// Look for encoder rotation, updating encoder_count as necessary.
static void check_encoder()
{
  // Get the Gray-code state of the encoder.
  // A line is "low" if <= 512; "high" > 512 or above.
  int gray_code = ((analogRead(3) > 512) << 1) | (analogRead(4) > 512);

  // If the gray code has changed, adjust the intermediate delta.
  if (gray_code != previous_gray_code) {

    // adding 4 units for each click for Grayhill 62A11 encoder, removed "half ticks"
    if ((gray_code == cw_gray_codes[previous_gray_code]) && (encoder_count < 30))
    {
      encoder_count = encoder_count + 10;
    }
    else if ((gray_code == cw_gray_codes[previous_gray_code]) && (encoder_count >= 30))
    {
      encoder_count = encoder_count + 2;
    }
    else if (gray_code == ccw_gray_codes[previous_gray_code])
    {
      encoder_count = encoder_count - 2;
    }//all half tick code here removed for Grayhill 62A
    previous_gray_code = gray_code;
  }
}

////////////////////////////////////////////////////////////////////////////
// Switch handling. ////////////////////////////////////////////////////////

static unsigned int switch_was_down = 0;
static unsigned int switch_released()
{
  // The switch is depressed if its sense line is low (<512).
  int switch_is_down = (analogRead(19) < 512);

  // The action takes place when the switch is released.
  released = (switch_was_down && !switch_is_down);

  // Remember the state of the switch.
  switch_was_down = switch_is_down;

  // Was the switch just released?
  return released;
}

///////////////////////////////////////////////////////////////////////////////////
///// Motor Speed Control  ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////


void set_motor_PWM() {
  if (encoder_count >= 255) {
    encoder_count = 255;
  }

  if (encoder_count <= 0) {
    encoder_count = 0;
  }

  for (set_PWM; set_PWM < encoder_count; set_PWM ++)
  {
    analogWrite(3, set_PWM);  //digital pin 3, sloppy programming, should be named; gough
  }
  //this is pin to MOSFET gate
  //be sure to leave off pulldown resistor on board used this way; R2 I think

  for (set_PWM; set_PWM > encoder_count; set_PWM --) 
  {
    analogWrite(3, set_PWM);
  }

}

/// Screen Refresh.  /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

long elapsed_ms; //used to refresh the lcd Screen
static long last_display_update_ms = 0; //used to refresh the lcd Screen

void refresh_lcd()
{
  elapsed_ms = millis() - last_display_update_ms;

  if ((last_display_update_ms == 0) || (elapsed_ms >= 500))
  {
    readPaddlewheel();  //works great in this timed loop
    lcd.clear ();
    lcd.print (mlspersec);
    lcd.print (" ml/s");
    lcd.setCursor (0, 1);
    lcd.print ("Hz = ");
    lcd.print(FlowRate);
    last_display_update_ms = millis();
  }
}

//////////////////////////////////////////////////////////////////////////////////
// Setup / Main. /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void setup()
{
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Groundwater");
  lcd.setCursor(0, 1);
  lcd.print("Controller");
  delay(2500);
  pinMode(3, OUTPUT); // init pin 3 as output.
  digitalWrite(19, HIGH); //set switch pullup for Grayhill optical encoder
  reset_encoder();
}

void loop()
{
  int switch_event;
  check_encoder();
  switch_event = switch_released();  //Run routine that checks switch
  refresh_lcd(); //Check sensor for new flow value and update the display.


  if (analogRead(19) < 512) //Encoder Pressed
  {
    reset_encoder();
    set_motor_PWM();  // should turn motor off
    lcd.clear();
    lcd.print("Flow Reset");
    FlowRate = 0;
    lastQ = 0;
    mlspersec = 0;
    delay (200);
  }

  if (encoder_count != set_PWM) {
    set_motor_PWM();
  }

}
