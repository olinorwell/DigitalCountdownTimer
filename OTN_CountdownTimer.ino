// COUNTDOWN TIMER
// Written by Oli Norwell - August 2020
// www.olinorwell.com
// Tutorial: https://www.olinorwell.com/designing-and-building-a-digital-kitchen-timer
// MIT licence - do whatever you want with it - it would be nice to hear if you have improved it
//
// We split the code into modules
//
// Countdown - everything to do with an actual countdown
// Rotary Encoder - everything to do with the rot enc that sets our start time
// Sound - everything to do with the sound we output
// Display - everything to do with the 7 seg 4 digit display

//#define DEBUG		// if this is active then we output messages through serial

// Countdown state
#define COUNTDOWN_SETUP     0
#define COUNTDOWN_ACTIVE    1
#define COUNTDOWN_PAUSED    2
#define COUNTDOWN_END       3

int countdownState = COUNTDOWN_SETUP;		// begin with us ready to set a countdown time
int countdownMins = 0;				// current mins to go
int countdownSecs = 15;				// current secs to go
int countdownStartMins = 0;			// what did we start with? used when resetting
int countdownStartSecs = 15;

long lastCountdownChange = 0;			// how long since we adjusted the countdown

// Timer button
#define BUT_PIN           4			// where is our button connected (other end to ground)
long butLastChange = 0;				// when did we last register a state change?
int butLastState = LOW;				// what was the last state of the button?

// Rotary Encoder (https://en.wikipedia.org/wiki/Rotary_encoder)
#define ROT_UPDATE_MINS   0			// Mode to edit mins
#define ROT_UPDATE_SECS   1			// Mode to edit seconds

#define ROT_PIN_A       12			// Pin that pin A of rot enc is connected to
#define ROT_PIN_B       13			// Pin that pin B of rot enc is connected to
#define ROT_PIN_BUT     11			// Pin that button pin of rot enc is connect to (other to ground)

int rotPinAState = 0;				// State of pin A of rot enc
int rotPinBState = 0;				// State of pin B of rot enc
int rotPinALastState = 0;			// Previous state of pin A (we don't care for B)
int rotWhichUpdate = ROT_UPDATE_MINS;		// what is it currently updated? mins or secs?
int rotButState = 0;				// State of the rot enc button
long rotButLastChange = 0;			// when was the button state last changed?
int rotButLastState = HIGH;			// previous state of rot enc button

// Display (I am using a fairly large 4 digit 7 segment display with the TM1637 IC)
#include <TM1637.h>				// https://github.com/AKJ7/TM1637
TM1637 tm1637(2, 3);				// Initialise library pin 2 to clock, pin 3 to DIO

// RGB LED light - an LED with 4 pins, RGB and ground
#define LED_MODE_NOTHING        0		// what state our LED is in, which determines its colour
#define LED_MODE_SETUP          1
#define LED_MODE_COUNTDOWN      2
#define LED_MODE_PAUSED         3
#define LED_MODE_END            4

int ledMode = LED_MODE_NOTHING;			// store our current state
long ledModeLastChange = 0;			// used for blinking lights (not done yet)
int ledModeBlinking = 0;			// used for blinking lights (not done yet)
int ledPinR = 5;				// pin on Arduino connected to LED R pin
int ledPinB = 6;				// pin on Arduino connected to LED B pin
int ledPinG = 7;				// pin on Arduino connected to LED G pin

void updateLED();				// Function to update the state of the light
void setLEDMode(int mode);			// Function to set the state of the light

// Sound
#include <toneAC.h>         // ToneAC library - which allows us to get a louder tone than the Arduino library
				// https://github.com/teckel12/arduino-toneac

#define SOUND_MODE_SILENCE      0
#define SOUND_MODE_ALARM        1

#define SOUND_STATE_SILENCE     0
#define SOUND_STATE_TONE1       1
#define SOUND_STATE_SILENCE2    2
#define SOUND_STATE_TONE2       3

int soundMode = SOUND_MODE_SILENCE;      // By default we sit in silence
int soundState = SOUND_STATE_SILENCE;	 // By default silence
int soundFreq = 2100;			 // our starting frequency
int soundDir = 1;			 // If frequency is getting higher or lower
long soundLastUpdate = 0;		 // when did we last update the sound frequency

// Function to setup the LED output pins
void setupLED()
{
  pinMode(ledPinR, OUTPUT);  
  pinMode(ledPinB, OUTPUT);  
  pinMode(ledPinG, OUTPUT);  
  
  return;
}

// function to set the LED mode
void setLEDMode(int mode)
{
  ledMode = mode;

  return;
}

// Update the LED - depending on the mode decide what our LED pins should output to the component
void updateLED()
{

  // depending on the LED mode we create a different colour on our RGB LED component
  
  if(ledMode == LED_MODE_NOTHING)
  {
    digitalWrite(ledPinR, LOW);
    digitalWrite(ledPinB, LOW);
    digitalWrite(ledPinG, LOW);
  }
  else if(ledMode == LED_MODE_SETUP)
  {
    digitalWrite(ledPinR, HIGH);
    digitalWrite(ledPinB, LOW);
    digitalWrite(ledPinG, LOW);
  }
  else if(ledMode == LED_MODE_COUNTDOWN)
  {
    digitalWrite(ledPinR, LOW);
    digitalWrite(ledPinB, HIGH);
    digitalWrite(ledPinG, LOW);
  }
  else if(ledMode == LED_MODE_PAUSED)
  {
    digitalWrite(ledPinR, HIGH);
    digitalWrite(ledPinB, HIGH);
    digitalWrite(ledPinG, LOW);
  }
  else if(ledMode == LED_MODE_END)
  {
    digitalWrite(ledPinR, LOW);
    digitalWrite(ledPinB, LOW);
    digitalWrite(ledPinG, HIGH);
  }

  return;
}

// Function to setup our button
void setupBut()
{
  digitalWrite(BUT_PIN, HIGH);
  pinMode(BUT_PIN, INPUT_PULLUP); // Input with a pull-up resistor (so high when not connected to ground physically by the button)

  return;
}

// Function to deal with our main button being pressed
void butPressed()
{
  // This adjusts the state of our device, which uses a finite state machine with 4 different possible states

  if(countdownState == COUNTDOWN_SETUP)
  {
    // If we are in the setup mode, then set our mode to active so the countdown can begin
    countdownStartMins = countdownMins;
    countdownStartSecs = countdownSecs;
    
    countdownState = COUNTDOWN_ACTIVE;
    setLEDMode(LED_MODE_COUNTDOWN);	// change what the LED state is
  }
  else if(countdownState == COUNTDOWN_ACTIVE)
  {
  // If we are currently in the active countdown mode - move us to the paused state
    countdownState = COUNTDOWN_PAUSED;
    setLEDMode(LED_MODE_PAUSED);	// change what the LED state is
  }
  else if(countdownState == COUNTDOWN_PAUSED)
  {
  // If we are currently in the paused countdown mode - move us back to the active state
    countdownState = COUNTDOWN_ACTIVE;
    setLEDMode(LED_MODE_COUNTDOWN);	// change what the LED state is
  }
  else if(countdownState == COUNTDOWN_END)
  {
  // If our countdown is already over, then move us back to the setup stage
    setLEDMode(LED_MODE_SETUP);

    killSound();			// stop our alarm sound
    
    countdownState = COUNTDOWN_SETUP;	// reset values ready for the setup state again
    countdownMins = countdownStartMins;
    countdownSecs = countdownStartSecs;
  }

  return;
}

// Function to update our main button state
void updateBut()
{
  // This is my simple debouncing code, that will ensure 50ms have passed before
  // allowing further state changes
  if(millis() - butLastChange > 50)
  {
    if(digitalRead(BUT_PIN) == LOW)
    {
      if(butLastState == HIGH)
      {
        butLastChange = millis();
        butLastState = LOW;
        
        butPressed();		// run the function for a button press
      }
    }
    else 
    {
      butLastState = HIGH;
    }
  }

  return;
}

// Function to setup the rotary encoder
void setupRotEnc()
{
  pinMode(ROT_PIN_A, INPUT_PULLUP);		// Input with an internal pull-up resistor
  pinMode(ROT_PIN_B, INPUT_PULLUP);		// Input with an internal pull-up resistor
  pinMode(ROT_PIN_BUT, INPUT_PULLUP); 		// Button with an internal pull-up resistor
  // this means that when not doing anything the value will be pulled to high

  rotPinALastState = digitalRead(ROT_PIN_A);	// see what the current value is and set to first old value

 return;
}

// Function to deal with the rot enc button being pressed
// the user can push in the rotary encoder dial to press the button
void rotButPressed()
{
// all we do is change what was being edited, from mins to secs and back
  if(rotWhichUpdate == ROT_UPDATE_MINS)
  {
      rotWhichUpdate = ROT_UPDATE_SECS;  
  }
  else 
  {
      rotWhichUpdate = ROT_UPDATE_MINS;
  }

// Note: I plan to add a 'reset' option if the button has been held down
// for say over 3 seconds, this isn't yet implemented

  return;
}

// Function to deal with the rot enc decreasing our value
void rotUpdateSettingDown()
{
// Depending on what we are currently updating, make the change
// but only if mins or secs are larger than 0
  if(rotWhichUpdate == ROT_UPDATE_MINS)
  {
    if(countdownMins > 0) countdownMins--; 
  }
  if(rotWhichUpdate == ROT_UPDATE_SECS)
  {
    if(countdownSecs > 0) countdownSecs--; 
  }

  return;
}

// Function to deal with the rot enc increasing our value
void rotUpdateSettingUp()
{
// Depending on what we are currently updating, make the change
// but only if the mins are less than 99 or seconds less than 59
  if(rotWhichUpdate == ROT_UPDATE_MINS)
  {
    if(countdownMins < 99) countdownMins++; 
  }
  if(rotWhichUpdate == ROT_UPDATE_SECS)
  {
    if(countdownSecs < 59) countdownSecs++; 
  }

  return;
}

// Update our rotary encoder
void updateRotEnc()
{
   rotPinAState = digitalRead(ROT_PIN_A);		// Grab the state of pin A
   rotPinBState = digitalRead(ROT_PIN_B);		// Grab the state of pin B

   // If pin A has changed state - then depending on whether it is now the same as B or not we can see what way it was turned
   if(rotPinAState != rotPinALastState)
   {
       if(rotPinBState != rotPinAState)
       {
          rotUpdateSettingDown();
       }
       else 
       {
         rotUpdateSettingUp();
       }
   }

   // Record the state as the new 'old state'
   rotPinALastState = rotPinAState;

  // Rot button - with simple de-bouncing code - ensuring we wait 50ms before accepting a change of state
  if(millis() - rotButLastChange > 50)
  {
    if(digitalRead(ROT_PIN_BUT) == LOW)
    {
      if(rotButLastState == HIGH)
      {
        rotButLastChange = millis();
        rotButLastState = LOW;
        rotButPressed();
      }
    }
    else 
    {
      rotButLastState = HIGH;
    }
  }
   
   return;
}

// Setup our display using the 3rd party library we have chosen to use
void setupDisplay()
{
    tm1637.init();			// Init the display
    tm1637.setBrightness(5);		// Set a medium brightness
    
    return;
}

// Update our display - the 4 digit value we want to put on the display is the mins * 100 + secs
void updateDisplay()
{

  int val = countdownMins * 100;
  val += countdownSecs;

	// e.g. 3 mins 40 secs.... 3 * 100 = 300 + 40 = 0340 for the display.
	// 15 mins 8 secs = 15 * 100 = 1500 + 8 = 1508 for the display

    tm1637.display(val);	// Using the library we are using we can simply send it a value to display

    // note: I haven't set the : in the centre of the display - this was an oversight but could be easily added
    
    return;
}

// Stop making sound
void killSound()
{
  toneAC();				// Set sound to nothing
  soundMode = SOUND_MODE_SILENCE;   	// Set our sound mode to silence
  
  return;
}

// Update our sound module
void updateSound()
{
  if(soundMode == SOUND_MODE_SILENCE)
  {
      // The sound of silence....
  }
  else if(soundMode == SOUND_MODE_ALARM) // We are in alarm mode, tweak our noise
  {
  	// Note how this is a finite state machine and isn't blocking. We change the tone every so often
  	// but the rest of our program continues while we are making noise - useful if we added features such
  	// as a network connection which we'd need to give time to.
  
    if(millis() - soundLastUpdate > 100)	// Every 100ms we adjust the tone
    {
      soundLastUpdate = millis();		// Mark when we are making this change
      
        // Change state depending on current sound state
        if(soundState == SOUND_STATE_SILENCE) // We were silent - so this is us starting to make noise
        {
            if(soundDir == 1) soundFreq += 20;
            else soundFreq -= 20;
            if(soundDir == true && soundFreq > 2300) { soundDir = false; }
            if(soundDir == false && soundFreq < 1900) { soundDir = true; }
            
            // move to playing tone1
            toneAC(soundFreq);
            soundState = SOUND_STATE_TONE1;
        }
        else if(soundState == SOUND_STATE_TONE1) // We were making tone1, now be silent a bit
        {
            toneAC();
            soundState = SOUND_STATE_SILENCE2;	// Go to the second silent mode
        }
        else if(soundState == SOUND_STATE_SILENCE2) // We were having a silent moment, make noise again
        {
            // move to playing tone1
            toneAC(2100);
            soundState = SOUND_STATE_TONE2;
        }
        else if(soundState == SOUND_STATE_TONE2) // Return to the silence mode
        {
            toneAC();         
            soundState = SOUND_STATE_SILENCE;
        }
    }
    
    
  }

  return;
}

// Update our countdown
void updateCountdown()
{

  // If a second has passed then we update the countdown
  if(millis() - lastCountdownChange >= 1000)
  {
    lastCountdownChange = millis();		// Record when we made this change

    countdownSecs--;				// Reduce seconds by 1

    if(countdownSecs < 0) 			// If seconds are now -1, then we have to reduce mins
    {
      countdownSecs = 59;			// Set seconds to 59, which is what -1 really means to us
      countdownMins--;				// Reduce minutes by 1

      if(countdownMins < 0) 			// If minutes is now -1, then we are done
      {
        countdownState = COUNTDOWN_END;		// Set countdown state to the countdown having ended
        soundMode = SOUND_MODE_ALARM;		// Set the sound state to make a lot of noise
        countdownSecs = 0;			// Set our countdown to 0 mins, 0 secs (as mins was -1)
        countdownMins = 0;
      }
    }
  }

  return;
}

// Our setup function - ran once on power up
void setup() 
{

// Enable serial debugging if we desire it
#ifdef DEBUG
  Serial.begin(9600);
#endif
  
  // Setup our various modules
  setupBut();
  setupRotEnc();
  setupDisplay();
  setLEDMode(LED_MODE_SETUP);

  return;
}

// Our loop function - will run forever unless we cut the power or run out of it
void loop() 
{
  // Update the countdown, but only if we are actively counting down
  if(countdownState == COUNTDOWN_ACTIVE) updateCountdown();

  // Update our various modules
  updateRotEnc();
  updateDisplay();
  updateSound();
  updateBut();
  updateLED();
    
return;
}

// Go luck if you decide to build this device! Let me know how you get on

