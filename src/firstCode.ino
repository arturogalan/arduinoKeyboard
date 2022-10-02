/*******************************************************************
passw0rd
1234
* RYAN UPDATE THIS (MAYBE LATER ITS DEC 24 AND I JUST GOT THE VIDEO DONE)
 * A multi-mode Macro keyboard with Arduino Pro Micro using row column matrix.
 * This is a basic sketch to understand key layout, basic hotkeys macros and some extras like:
 *    -RBG addressble LEDs compatible with Adafruit's neopixel library.  
 *    -i2c for 4x20 LCD to describe each function.  (you might have a different display...)
 *    -mouse movement + mouse clicks.
 *Some mouse movement (in pixels) pull their values from analogRead(pin 0) so make sure something is connected.
 *Don't forget to solder all the diode and encoder solder jumpers on the backside!!
 * (c) 2020 Ryan Bates

  Last time I touched this: Dec 13 2020
  web: www.retrobuiltgames.com
  youtube: https://www.youtube.com/c/ryanbatesrbg
  twitter: @retrobuiltgames
  Buy this PCB at [TINDIE LINK RYAN: WHENEVER YOU GET THAT DONE]

Ryan's Pro Tips:
=============== Keyboard Control================================================================================
Keyboard.write();   Sends a keystroke to a connected computer. 
                    This is similar to pressing and releasing a key on your keyboard.
                    Will send a shift command if applicable. Example: Keyboard.write('K') will 
                    automatically do SHIFT + k. 
                    Can also accept ASCII code like this:
                    //Keyboard.write(32); // This is space bar (in decimal)
                    Helpful list of ASCII + decimal keys http://www.asciitable.com/
                    

Keyboard.press();   Best for holding down a key with multi-key commands; like copy/ paste
                    This example is [ctrl] + [shift] + [e] 
                      //Keyboard.press(KEY_LEFT_CTRL);
                      //Keyboard.press(KEY_LEFT_SHIFT);
                      //Keyboard.press('e'); 
                      //delay(100); Keyboard.releaseAll();
                

Keyboard.print();   Sends a keystroke(s)
                    Keyboard.print("stop using aim bot"); // types this in as a char or int! (not a string)!


Keyboard.println(); Sends a keystroke followed by a newline (carriage return)
                     Very practical if you want to type a password and login in one button press!
                     
SOMETIMES, applications are coded to recognize Keyboard.press() and not Keyboard.write() and vice versa.
You might have to experiment. 

=============== Mouse Control================================================================================
Mouse.move(x, y, wheel);  Moves the mouse and or scroll wheel up/down left/right.
                          Range is -128 to +127. units are pixels 
                          -number = left or down
                          +number = right or up

Mouse.press(b);         Presses the mouse button (still need to call release). Replace "b" with:
                        MOUSE_LEFT   //Left Mouse button
                        MOUSE_RIGHT  //Right Mouse button
                        MOUSE_MIDDLE //Middle mouse button
                        MOUSE_ALL    //All three mouse buttons
                        
Mouse.release(b);       Releases the mouse button.

Mouse.click(b);         A quick press and release.

**********************************************************************************************************/

// --------------------------------------------------------------
// Standard Libraries
// --------------------------------------------------------------

#include "Keyboard.h"
// Library with a lot of the HID definitions and methods
// Can be useful to take a look at it see whats available
// https://github.com/arduino-libraries/Keyboard/blob/master/src/Keyboard.h
#include <Mouse.h> //there are some mouse move functions for encoder_Mode 2 and 3
#include <Keypad.h>
// This library is for interfacing with the 3x4 Matrix
// Can be installed from the library manager, search for "keypad"
// and install the one by Mark Stanley and Alexander Brevig
// https://playground.arduino.cc/Code/Keypad/

const byte ROWS = 3; //three rows
const byte COLS = 4; //four columns

char previousPressedKey;
boolean hasReleasedKey = false;  //use for button Hold mode. Only works with one button at a time for now...

#include <Encoder.h>   
//Library for simple interfacing with encoders (up to two)
//low performance ender response, pins do not have interrupts
Encoder RotaryEncoderA(14, 15); //the LEFT encoder (encoder A)
Encoder RotaryEncoderB(10, 16);  //the RIGHT encoder (encoder B)

// --------------------------------------------------------------
// Screen control
// --------------------------------------------------------------
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);



// Your LCD hardware address or type might be different... This LCD library might not work for your application
const int LCD_NB_ROWS = 4 ;           //for the 4x20 LCD display.begin(), but i think this is kinda redundant 
const int LCD_NB_COLUMNS = 20 ;
unsigned long previousMillis = 0;     // values to compare last time interval was checked (For LCD refreshing)
unsigned long currentMillis = millis(); // values to compare last time interval was checked (For LCD refreshing) and DemoTimer

int check_State = 0;                  // state to check trigger the demo interrupt
int updateLCD_flag = 0;               // LCD updater, this flag is used to only update the screen once between mode changes
                                      // and once every 3 second while in a mode. Saves cycles / resources

#include <Adafruit_NeoPixel.h>  //inclusion of Adafruit's NeoPixel (RBG addressable LED) library
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN            A2 // Which pin on the Arduino is connected to the NeoPixels?
#define NUMPIXELS      13 // How many NeoPixels are attached to the Arduino? 13 total, but they are address from 0,1,2,...12.

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int colorUpdate = 0;   //setting a flag to only update colors once when the mode is switched. 
const int b = 3;       // Brightness control variable. Used to divide the RBG vales set for the RGB LEDs. full range is 0-255. 255 is super bright
                       // In fact 255 is obnoxiously bright, so this use this variable to reduce the value. It also reduces the current draw on the USB


char keys[ROWS][COLS] = {
  {'1', '2', '3', '4'},  //  the keyboard hardware is  a 3x4 grid... 
  {'5', '6', '7', '8'},
  {'9', '0', 'A', 'B'},  // these values need  to be single char, so...
};
// The library will return the character inside this array when the appropriate
// button is pressed then look for that case statement. This is the key assignment lookup table.
// Layout(key/button order) looks like this
//     |----------------------------|
//     |                  [2/3]*    |     *TRS breakout connection. Keys 5 and 6 are duplicated at the TRS jack
//     |      [ 1] [ 2] [ 3] [ 4]   |     * Encoder A location = key[1]      
//     |      [ 5] [ 6] [ 7] [ 8]   |     * Encoder B location = Key[4]
//     |      [ 9] [10] [11] [12]   |      NOTE: The mode button is not row/column key, it's directly wired to A0!!
//     |----------------------------|




// Variables that will change:
int modePushCounter = 0;       // counter for the number of button presses
int buttonState = 0;           // current state of the button
int lastButtonState = 0;       // previous state of the button
int mouseMove;                 // variable that holds how many pixels to move the mouse cursor
String password;               // string for rando password generator, its a global variable because i might do something fancy with it?
int demoLoops = 0;             // counts how many demo loops have occured since active (Used in auto-attract mode)
long int demoTime = 30000;     // intitalize the demo time as 30second by default 

long positionEncoderA  = -999; //encoderA LEFT position variable
long positionEncoderB  = -999; //encoderB RIGHT position variable

const int ModeButton = A0;     // the pin that the Modebutton is attached to
const int pot = A1;           // pot for adjusting attract mode demoTime or mouseMouse pixel value
//const int Mode1= A2;
//const int Mode2= A3; //Mode status LEDs

byte rowPins[ROWS] = {4, 5, A3 };    //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9 };  //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup() {
  display.begin(LCD_NB_COLUMNS , LCD_NB_ROWS);
  randomSeed(analogRead(0));           // is this a bad idea? it's called before assigning the button as pull up... the switch is open at rest..
  pinMode(ModeButton, INPUT_PULLUP);  // initialize the button pin as a input:  
  Serial.begin(9600); // initialize serial communication:
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
   // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Custom Keyboard");
  display.print("connected!");
  display.println("(c) Arturo Galan");
  display.setCursor(0,0);
  display.display(); // actually display all of the above
  Keyboard.begin();
  pixels.begin(); // This initializes the NeoPixel library.
}


void loop() {
char key = keypad.getKey();
mouseMove = (analogRead(pot)); //reading the analog input, pot = pin A1
mouseMove = map(mouseMove, 0,1023, 1,124); //remap the analog pot values fron 1 to 124
checkModeButton();

  switch (modePushCounter) {                  // switch between keyboard configurations:
    case 0:                                   //Application Alpha or MODE 0. Random Password Generator (for fun; be smart with passwords)
    encoderA_Mode0();                         //custom function for encoder A
    encoderB_Mode0();                         //custom function for encoder A
    LCD_update_0();                           //Mode 0 text for LCD,
    setColorsMode0();                         //indicate what mode is loaded by changing the key colors
     
       if (key) {
    //Serial.println(key);
    switch (key) {
      case '1':                                             //open notepad to take notes of these great passwords
       Keyboard.press(KEY_LEFT_GUI); 
        Keyboard.press('r'); delay(150); 
        Keyboard.release(KEY_LEFT_GUI); 
        Keyboard.release('r'); 
        delay(150);                                        //give your system time to catch up with these android-speed keyboard presses
        Keyboard.println("notepad");  break;               //don't forget the break statement!
      case '2': RandoPasswordGenerator(); break;           // generate a not-so-sophisticated password 
      
      case '3': Keyboard.press(KEY_LEFT_CTRL);   
                Keyboard.print('z');                       //undo
                break;
      case '4': display.setCursor(0,0); display.print("Rando PW Generator  ");
                display.setCursor(0,1); display.print("                    ");
                display.setCursor(0,2); display.print("Your password:      ");
                display.setCursor(0,3); display.print("                    "); //overwrites the LCD row
                display.setCursor(0,3); 
                break; 
      case '5': Keyboard.press(KEY_LEFT_CTRL);   
                Keyboard.print('a');                        //select all    
                break;
      case '6': Keyboard.press(KEY_LEFT_CTRL); 
                Keyboard.print('c');                         //copy
                break;
                
      case '7': Keyboard.press(KEY_LEFT_CTRL);  
                Keyboard.print('v');                         // paste
                break;
                
      case '8': Keyboard.press(KEY_DELETE); 
                 break;
      case '9': Keyboard.println("passw0rd"); break;
      case '0': Keyboard.println("1234"); break;
      case 'A': Keyboard.println("mypassword"); break;
      case 'B': Keyboard.println("1111"); break;

    }
    delay(100); Keyboard.releaseAll(); // this releases the buttons 
  }
      break;
      
    case 1:    //  Application Delta or MODE 2 ========================================================================
               //this mode mimics a password manager. Types ['login ID'] + [tab] + ['password'] + [enter]
               //This is not a really secure way manage your passwords, but it's an idea...
      encoderA_Mode2();                         //move mouse
      encoderB_Mode2();                         //move mouse
      setColorsMode1();
      LCD_update_1();                           //Mode 1 text for LCD
   if (key) {
    //Serial.println(key);
    switch (key) {
      case '1': Keyboard.print("admin_1");      Keyboard.write(9); //tab key
                Keyboard.println("password_1");  
                pixels.setPixelColor(1, pixels.Color(0,150,0)); // change the color to green when pressed, wait 100ms so the change can be observed
                break;
      case '2': Keyboard.print("admin_2");      Keyboard.write(9); //tab key
                Keyboard.println("password_2");  
                pixels.setPixelColor(2, pixels.Color(0,150,0)); 
                break;    
      case '3': Keyboard.print("admin_3");      Keyboard.write(9); //tab key
                Keyboard.println("password_3"); 
                pixels.setPixelColor(3, pixels.Color(0,150,0)); 
                break;
      case '4': Keyboard.print("admin_4");      Keyboard.write(9); //tab key
                Keyboard.println("password_4"); 
                pixels.setPixelColor(4, pixels.Color(0,150,0)); 
                break;   
      case '5': Keyboard.print("admin_5");      Keyboard.write(9); //tab key
                Keyboard.println("password_5"); 
                pixels.setPixelColor(5, pixels.Color(0,150,0)); 
                break;    
      case '6': Keyboard.print("admin_6");      Keyboard.write(9); //tab key
                Keyboard.println("password_6"); 
                pixels.setPixelColor(6, pixels.Color(0,150,0)); 
                break;    
      case '7': Keyboard.print("admin_7");      Keyboard.write(9); //tab key
                Keyboard.println("password_7"); 
                pixels.setPixelColor(7, pixels.Color(0,150,0)); 
                 break;   
      case '8': Keyboard.print("admin_8");      Keyboard.write(9); //tab key
                Keyboard.println("password_8"); 
                pixels.setPixelColor(8, pixels.Color(0,150,0)); 
                break;    
      case '9': Keyboard.print("admin_9");      Keyboard.write(9); //tab key
                Keyboard.println("password_9");
                pixels.setPixelColor(9, pixels.Color(0,150,0)); 
                break;     
      case '0': Keyboard.print("admin_10");      Keyboard.write(9); //tab key
                Keyboard.println("password_10");                
                pixels.setPixelColor(10, pixels.Color(0,150,0));
                break; 
      case 'A': Keyboard.print("admin_11");      Keyboard.write(9); //tab key
                Keyboard.println("password_11"); 
                pixels.setPixelColor(11, pixels.Color(0,150,0)); 
                break;
      case 'B': Keyboard.print("admin_12");      Keyboard.write(9); //tab key
                Keyboard.println("password_12");
                pixels.setPixelColor(12, pixels.Color(0,150,0)); 
                break;

      
    }
    pixels.show();                                      //update the color after the button press
    delay(100); Keyboard.releaseAll();                   // this releases the buttons
    //delay(100);                                        //delay a bit to hold the color (optional)
    colorUpdate = 0;                                    //call the color update to change the color back to Mode settings   
  }
      break;
 //====================================================================================================================     
    case 2:    // Application Delta (some basic Arduino IDE Shortcuts and HotKeys
    encoderA_Mode2();                                  //moves mouse (sensitivity) based on the analog pot setting
    encoderB_Mode2();                                  // 
    LCD_update_2();                                    // Mode 2 text for LCD
    setColorsMode2();                                  // set color layout for this mode
    if (key) {
     switch (key) {
      case '1': 
        Keyboard.press(KEY_LEFT_CTRL);    Keyboard.press('g');
        Keyboard.release(KEY_LEFT_CTRL);  Keyboard.release('g'); delay(250);
        for (int tab=0; tab<8; tab++){           //press tab 8 times to select files in the browser window
           Keyboard.press(KEY_TAB); Keyboard.release(KEY_TAB); delay(100);}
        break;
        
      case '2'://macro that opens genesis emulator
        Keyboard.press(KEY_LEFT_GUI);
        Keyboard.press('r');
        Keyboard.release(KEY_LEFT_GUI);
        Keyboard.release('r');
        delay(50); //give your system time to catch up with these android-speed keyboard presses
        Keyboard.println("F:\\Roms and Emulators\\Sega Genesis\\Fusion.exe"); delay(50); 
        // you need two "\\" to call one "\" when using them in a string
        break;
  
      case '3': Keyboard.press(KEY_RETURN); Keyboard.release(KEY_RETURN);  //hard reset for K Fusion 
        break;
  
      case '4': Keyboard.press(KEY_ESC); Keyboard.release(KEY_ESC);   
        break;
      
      case '5': Keyboard.press(KEY_F5);  Keyboard.release(KEY_F5);   
        break;
  
      case '6': Keyboard.press(KEY_LEFT_CTRL);    Keyboard.press('g');
        Keyboard.release(KEY_LEFT_CTRL);  Keyboard.release('g'); delay(250);
        Keyboard.println("Ms. Pac-Man (USA, Europe).gen");            
        break;
  
      case '7': Keyboard.press(KEY_LEFT_CTRL);    Keyboard.press('g');
        Keyboard.release(KEY_LEFT_CTRL);  Keyboard.release('g'); delay(250);
        Keyboard.println("Devilish - The Next Possession (USA).gen"); 
        break;
  
      case '8': Keyboard.press(KEY_F8); Keyboard.release(KEY_F8);     
        break;
  
     }
     delay(50); Keyboard.releaseAll(); // this releases the buttons
     }
          break;
      
    case 3:                //Auto Attract (Demo) Mode for a Raspberry Pi running RetroPie
     //LCD_update_3();                                     //Mode 3 text for display. not used, The AutoAttract mode has its own LCD code
     setColorsMode3();
     //encoderA_Mode3();                                  //  skip encoders to get better responce from keys
     //encoderB_Mode3();                                  //  
     getDemoTime();
     key_sequencerRPi();
      if (key) {
        switch (key) {
        case '1':   demoTime = analogRead(pot); //Hold key to read analog pot for manual set of demoTimer
                    demoTime = map(demoTime, 0, 1023, 30, 300); //30 to 300ms
                    demoTime = demoTime * 1000; //30 to 300 sec
                    break;
                    
        case '2': demoTime = demoTime + 30000;  //add xx seconds (like adding a credits to non-credit game). Arduino doesnt 
                                                // always catch this, some other subroutines are slowing things down
                  break;                         
        case '3': Keyboard.press('5'); break;
        case '4': Keyboard.press('6'); break;
        case '5': previousMillis = currentMillis;     break; // works, most of the time
        case '6': Keyboard.press(KEY_UP_ARROW); break;
        case '7': Keyboard.press('1'); break;
        case '8': Keyboard.press('2'); break;
        case '9': Keyboard.press(KEY_LEFT_ARROW);  break;
        case '0': Keyboard.press(KEY_DOWN_ARROW);  break;
        case 'A': Keyboard.press(KEY_RIGHT_ARROW);  break;
        case 'B': Keyboard.press(KEY_TAB);  break;
          
          break;                                
               }
       Keyboard.releaseAll(); // this releases the buttons
               }
     break;
  }
  delay(1);  // delay in between reads for stability

}
//---------------------Sub Routine Section--------------------------------------------------------------------------
void setColorsMode0(){
  if (colorUpdate == 0){                                     // have the neopixels been updated?
      pixels.setPixelColor(0, pixels.Color(  80,  0,200));    
      pixels.setPixelColor(1, pixels.Color(  0,  0,100));    
      pixels.setPixelColor(2, pixels.Color(220,  0,  0));
      pixels.setPixelColor(3, pixels.Color(220,  0,200));
      pixels.setPixelColor(4, pixels.Color(220,  0,200));
      pixels.setPixelColor(5, pixels.Color( 80,102,  0));
      pixels.setPixelColor(6, pixels.Color(  0,200,  0));
      pixels.setPixelColor(7, pixels.Color(  0,200,  0));
      pixels.setPixelColor(8, pixels.Color(140,  0,  0));
      pixels.setPixelColor(9, pixels.Color(  0, 10, 20));
      pixels.setPixelColor(10,pixels.Color(  0, 10, 40));
      pixels.setPixelColor(11, pixels.Color(  0, 10,60));
      pixels.setPixelColor(12, pixels.Color(  0, 10,80));
      pixels.show();
      colorUpdate=1;              }                           // neoPixels have been updated. 
                                                              // Set the flag to 1; so they are not updated until a Mode change
}

void setColorsMode1(){
  if (colorUpdate == 0){                                     // have the neopixels been updated?
      pixels.setPixelColor(0, pixels.Color( 80,  0,200));    //gradient mix
      pixels.setPixelColor(1, pixels.Color( 10,  0,200));    //gradient mix
      pixels.setPixelColor(2, pixels.Color( 20,  0,200));
      pixels.setPixelColor(3, pixels.Color( 40,  0,200));
      pixels.setPixelColor(4, pixels.Color( 60,  0,200));
      pixels.setPixelColor(5, pixels.Color( 80,  0,200));
      pixels.setPixelColor(6, pixels.Color(100,  0,200));
      pixels.setPixelColor(7, pixels.Color(120,  0,200));
      pixels.setPixelColor(8, pixels.Color(140,  0,200));
      pixels.setPixelColor(9, pixels.Color(160,  0,200));
      pixels.setPixelColor(10,pixels.Color(180,  0,200));
      pixels.setPixelColor(11, pixels.Color(200,  0,200));
      pixels.setPixelColor(12, pixels.Color(220,  0,200));
      pixels.show();
      colorUpdate=1;              }                           // neoPixels have been updated. 
                                                              // Set the flag to 1; so they are not updated until a Mode change
}

void setColorsMode2(){
  if (colorUpdate == 0){                                      // have the neopixels been updated?
      pixels.setPixelColor(0, pixels.Color( 51,102,  0));
      pixels.setPixelColor(1, pixels.Color(  0,  0,150)); 
      pixels.setPixelColor(2, pixels.Color(  0,150, 0));
      pixels.setPixelColor(3, pixels.Color(150, 0,  0));
      pixels.setPixelColor(4, pixels.Color(220,  0,200));
      pixels.setPixelColor(5, pixels.Color(150,  0,150));
      pixels.setPixelColor(6, pixels.Color(  0, 80,150));
      pixels.setPixelColor(7, pixels.Color(  0, 80,150));
      pixels.setPixelColor(8, pixels.Color( 80,102,  0));
      pixels.setPixelColor(9, pixels.Color(  0, 0,  0));
      pixels.setPixelColor(10,pixels.Color(  0, 0,  0));
      pixels.setPixelColor(11, pixels.Color( 0, 0,  0));
      pixels.setPixelColor(12, pixels.Color( 0, 0,  0));
      pixels.show(); colorUpdate=1;                  }      // neoPixels have been updated. 
                                                            // Set the flag to 1; so they are not updated until a Mode change
}

void setColorsMode3(){
  if (colorUpdate == 0){                                     // have the neopixels been updated?
      pixels.setPixelColor(0, pixels.Color(100,  0,  0));   // red
      pixels.setPixelColor(1, pixels.Color(  0,  0, 30));   // green
      pixels.setPixelColor(2, pixels.Color(  0,  0,110));   // off 
      pixels.setPixelColor(3, pixels.Color( 80,102,  0));   // yellow
      pixels.setPixelColor(4, pixels.Color(220,  0,200));   // magenta
      pixels.setPixelColor(5, pixels.Color(150,  0, 10));   // red
      pixels.setPixelColor(6, pixels.Color(  0,100,100));   // green
      pixels.setPixelColor(7, pixels.Color( 80,102,  0));   // yellow
      pixels.setPixelColor(8, pixels.Color(220,  0,200));   // magenta
      pixels.setPixelColor(9, pixels.Color(  0,100, 100));   // green
      pixels.setPixelColor(10, pixels.Color(  0,100,100));   // green
      pixels.setPixelColor(11, pixels.Color(  0,100,100));   // green
      pixels.setPixelColor(12, pixels.Color(  0,100,  0));   // blue
      pixels.show(); colorUpdate=1;                 }       // neoPixels have been updated. 
                                                            // Set the flag to 1; so they are not updated until a Mode change
}

void checkModeButton(){
  buttonState = digitalRead(ModeButton);
  if (buttonState != lastButtonState) { // compare the buttonState to its previous state
    if (buttonState == LOW) { // if the state has changed, increment the counter
      // if the current state is LOW then the button cycled:
      modePushCounter++;
      //Serial.println("pressed");
      //Serial.print("number of button pushes: ");
      //Serial.println(modePushCounter);
      updateLCD_flag = 0;   // forces a screen refresh 
      colorUpdate = 0;      // set the color change flag ONLY when we know the mode button has been pressed. 
                            // Saves processor resources from updating the neoPixel colors all the time
    } 
    delay(50); // Delay a little bit to avoid bouncing
  }
  lastButtonState = buttonState;  // save the current state as the last state, for next time through the loop
   if (modePushCounter >3){       //reset the counter after 4 presses CHANGE THIS FOR MORE MODES
      modePushCounter = 0;}
}

void encoderA(){
  long newPos = RotaryEncoderA.read()/4; //When the encoder lands on a valley, this is an increment of 4.
  
  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(positionEncoderA);
    Keyboard.press(KEY_LEFT_ARROW);
    Keyboard.release(KEY_LEFT_ARROW);                      }

  if (newPos != positionEncoderA && newPos < positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(positionEncoderA);
    Keyboard.press(KEY_RIGHT_ARROW);
    Keyboard.release(KEY_RIGHT_ARROW);                      }
}

void encoderB(){
  long newPos = RotaryEncoderB.read()/4; //When the encoder lands on a valley, this is an increment of 4.
  if (newPos != positionEncoderB && newPos > positionEncoderB) {
    positionEncoderB = newPos;
    //Serial.println(positionEncoderB);
    Keyboard.press(KEY_UP_ARROW);
    Keyboard.release(KEY_UP_ARROW);                      }

  if (newPos != positionEncoderB && newPos < positionEncoderB) {
    positionEncoderB = newPos;
    //Serial.println(positionEncoderB);
    Keyboard.press(KEY_DOWN_ARROW);
    Keyboard.release(KEY_DOWN_ARROW);                      }
}

//=============== encoder definitions/assignments ===========================================
//this section allows a unique encoder function for each mode (profile). Four total in this case or modes 0 through 3.

//=============Encoder A & B Function ====== Set 0 =========================================================
void encoderA_Mode0(){
  long newPos = RotaryEncoderA.read()/4; //When the encoder lands on a valley, this is an increment of 4.
                                          // your encoder might be different (divide by 2) i dunno. 
  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    Keyboard.press(KEY_RIGHT_ARROW);
    Keyboard.release(KEY_RIGHT_ARROW);                      }

  if (newPos != positionEncoderA && newPos < positionEncoderA) {
    positionEncoderA = newPos;
    Keyboard.press(KEY_LEFT_ARROW);
    Keyboard.release(KEY_LEFT_ARROW);                      }
}

void encoderB_Mode0(){
  long newPos = RotaryEncoderB.read()/4; 
  if (newPos != positionEncoderB && newPos > positionEncoderB) {
    positionEncoderB = newPos;
     Keyboard.press(KEY_DOWN_ARROW);
    Keyboard.release(KEY_DOWN_ARROW);                      }

  if (newPos != positionEncoderB && newPos < positionEncoderB) {
    positionEncoderB = newPos;

    Keyboard.press(KEY_UP_ARROW);
    Keyboard.release(KEY_UP_ARROW);                         }
}
//=============Encoder A & B Function ====== Set 1 =========================================================
void encoderA_Mode1(){
  long newPos = RotaryEncoderA.read()/2; 
  if (newPos != positionEncoderA && newPos < positionEncoderA) {
    positionEncoderA = newPos;
     //tab increase
    Keyboard.write(9); //tab key
     }

  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    //tab decrease
    Keyboard.press(KEY_LEFT_SHIFT); 
    Keyboard.write(9); //tab key
    Keyboard.release(KEY_LEFT_SHIFT);                      }

}

void encoderB_Mode1(){
  long newPos = RotaryEncoderB.read()/2; 
  if (newPos != positionEncoderB && newPos < positionEncoderB) {
    positionEncoderB = newPos;
      //Font decrease | Arduino IDE
    Keyboard.press(KEY_LEFT_CTRL); 
    Keyboard.press('-');
    Keyboard.release('-');  Keyboard.release(KEY_LEFT_CTRL);         }

  if (newPos != positionEncoderB && newPos > positionEncoderB) {
    positionEncoderB = newPos;
      //Font increase | Arduino IDE
    Keyboard.press(KEY_LEFT_CTRL); 
    Keyboard.press('=');
    Keyboard.release('=');   Keyboard.release(KEY_LEFT_CTRL);       }
}

//=============Encoder A & B Function ====== Set 2 =========================================================
void encoderA_Mode2(){ //testing some encoder wheel play control for arcade games; centede, tempest...

  long newPos = RotaryEncoderA.read()/2; 
  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(mouseMove);
     Mouse.move(-mouseMove,0,0); //moves mouse right... Mouse.move(x, y, wheel) range is -128 to +127
                        }

  if (newPos != positionEncoderA && newPos < positionEncoderA) {
    positionEncoderA = newPos;
    Mouse.move(mouseMove,0,0); //moves mouse left... Mouse.move(x, y, wheel) range is -128 to +127
                         }
}

void encoderB_Mode2(){
  long newPos = RotaryEncoderB.read()/2; //When the encoder lands on a valley, this is an increment of 2.
  if (newPos != positionEncoderB && newPos < positionEncoderB) {
    positionEncoderB = newPos;
    Mouse.move(0,-mouseMove,0);                                                           }

  if (newPos != positionEncoderB && newPos > positionEncoderB) {
    positionEncoderB = newPos;
    Mouse.move(0,mouseMove,0);            
                                                              }
}

//=============Encoder A & B Function ====== Set 3 =========================================================
void encoderA_Mode3(){
  long newPos = RotaryEncoderA.read()/2; 
  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    Mouse.press(MOUSE_LEFT); //holds down the mouse left click
    Mouse.move(0,4,0); //moves mouse down... Mouse.move(x, y, wheel) range is -128 to +127
    Mouse.release(MOUSE_LEFT); //releases mouse left click
                                                               }

  if (newPos != positionEncoderA && newPos < positionEncoderA) { 
    positionEncoderA = newPos;
    Mouse.press(MOUSE_LEFT); //holds down the mouse left click
    Mouse.move(0,-4,0); //moves mouse up... Mouse.move(x, y, wheel) range is -128 to +127
    Mouse.release(MOUSE_LEFT); //releases mouse left click                   
                                                              }
}

void encoderB_Mode3(){
  long newPos = RotaryEncoderB.read()/2;
  if (newPos != positionEncoderB && newPos > positionEncoderB) {
    positionEncoderB = newPos;
    Mouse.press(MOUSE_LEFT); //holds down the mouse left click
    Mouse.move(-4,0,0); //moves mouse left... Mouse.move(x, y, wheel) range is -128 to +127  
    Mouse.release(MOUSE_LEFT); //releases mouse left click
                                                               }

  if (newPos != positionEncoderB && newPos < positionEncoderB) {
    positionEncoderB = newPos;
    Mouse.press(MOUSE_LEFT); //holds down the mouse left click
    Mouse.move(4,0,0); //moves mouse right... Mouse.move(x, y, wheel) range is -128 to +127
    Mouse.release(MOUSE_LEFT); //releases mouse left click                   
                                                              }
}
//========================     LCD Update Routines    ===================================
void LCD_update_0() { //This method is less heavy on tying up the arduino cycles to update the LCD; instead 
                      //this updates the LCD every 3 seconds. Putting the display.write commands
                      //in the key function loops breaks the 'feel' and responsiveness of the keys. 
unsigned long currentMillis = millis();
if (currentMillis - previousMillis >= 3000) {                         // if the elasped time greater than 3 seconds
    previousMillis = currentMillis;                                   // save the last time you checked the interval
    switch (updateLCD_flag) {
    case 0:    
      display.clearDisplay();   
      display.setCursor(0, 0); display.print("Rando PW Generator  ");
      display.setCursor(0, 1); display.print("                    ");
      display.setCursor(0, 2); display.print("Your password:      ");
      display.setCursor(0, 3);
      updateLCD_flag = 1;                                             // stops the LCD from updating every x seconds.
                                                                      // will not run again until the Mode key is pressed.
      break;
      }}}


    
void LCD_update_1() {
unsigned long currentMillis = millis();
if (currentMillis - previousMillis >= 3000) {                         // if the elasped time greater than 3 seconds
    previousMillis = currentMillis;                                   // save the last time you checked the interval
    switch (updateLCD_flag) {
    case 0:    
      display.clearDisplay();
      display.setCursor(0, 0); display.print("1: App 1 Login");
      display.setCursor(0, 1); display.print("2: App 2 Login");
      display.setCursor(0, 2); display.print("3: App 3 Login");
      display.setCursor(0, 3); display.print("4: App 4 Login");
      updateLCD_flag = 1;
      break;
    case 1:    
    display.clearDisplay();
      display.setCursor(0, 0); display.print("5: App 5 Login");
      display.setCursor(0, 1); display.print("6: App 6 Login");
      display.setCursor(0, 2); display.print("7: App 7 Login");
      display.setCursor(0, 3); display.print("8: App 8 Login");
      updateLCD_flag = 2;
      break;
    case 2:    
    display.clearDisplay();
      display.setCursor(0, 0); display.print(" 9: App  9 Login");
      display.setCursor(0, 1); display.print("10: App 10 Login");
      display.setCursor(0, 2); display.print("11: App 11 Login");
      display.setCursor(0, 3); display.print("12: App 12 Login");
      updateLCD_flag = 0;
      break;
      }}}
    
void LCD_update_2() { 
unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 3000) {                       //if the elasped time greater than 3 seconds
    previousMillis = currentMillis;                                   // save the last time you checked the interval
     switch (updateLCD_flag) {
    case 0:    
      display.clearDisplay();
      display.setCursor(0, 0); display.print("1: SelectROM");
      display.setCursor(0, 1); display.print("2: K Fusion");
      display.setCursor(0, 2); display.print("3: Launch game");
      display.setCursor(0, 3); display.print("4: Escape");
      updateLCD_flag = 1;
      break;
    case 1:    
    display.clearDisplay();
      display.setCursor(0, 0); display.print("5: Save State");
      display.setCursor(0, 1); display.print("6: Launch Ms. Pacman");
      display.setCursor(0, 2); display.print("7: Launch Devilish");
      display.setCursor(0, 3); display.print("8: Load State");
      updateLCD_flag = 0;
      break;
    case 2:    //buttons 9-12 not used in this mode
    display.clearDisplay();
      display.setCursor(0, 0); display.print(" 9: -na-");
      display.setCursor(0, 1); display.print("10: -na-");
      display.setCursor(0, 2); display.print("11: -na-");
      display.setCursor(0, 3); display.print("12: -na-");
      updateLCD_flag = 0;
      break; 
      }}}

//void LCD_update_3() {                                                 // not used in the auto-attract mode
//unsigned long currentMillis = millis();
//  if (currentMillis - previousMillis >= 3000) {                       //if the elasped time greater than 3 seconds
//    previousMillis = currentMillis; 
//     switch (updateLCD_flag) {
//    case 0:    
//      display.clear();
//      
//      updateLCD_flag = 1;
//      break;
//    case 1:    
//     display.clear();
//      
//      updateLCD_flag = 2;
//      break;
//    case 2:    
//     display.clear();
//      
//      updateLCD_flag = 0;
//      break; 
//      }}}

void RandoPasswordGenerator(){
}

void getDemoTime() {

  display.setCursor(0, 1);  display.print("Demo Time: "); //display.print(demoTime / 1000); display.print("s "); //time in seconds
  display.setCursor(12, 1);
  if ((demoTime / 1000) <=600) {
    display.print(" "); //overwrite number space when missing leading zero when time is under 10:00 minutes
    //these LCDs are dumb when it comes to printing leading zeros in our number system
  }
  long minRemainder;
  minRemainder = (demoTime / 1000) % 60 ;
  display.print(round(demoTime / 60000)); display.print(":");
  if (minRemainder <= 9 && minRemainder >0 ) {
    display.print("0"); //add leading zero for remainder seconds under 10
  }
   //add leading zero for re
  display.print(round(minRemainder));
  if ((demoTime / 1000)==600) {
    display.print("0"); }

}

void key_sequencerRPi() {
  unsigned long currentMillis = millis();
  display.setCursor(0, 0); display.print("--Auto Demo Active--"); 
  display.setCursor(0, 2); display.print("Loop counter:      "); display.setCursor(14, 2);display.print(demoLoops);
  display.setCursor(0, 3); display.print("Next game in: ");
  if ((previousMillis / 1000) + (demoTime / 1000) - (currentMillis / 1000) <= 99) {
    display.print("0"); //leading zero
  }
  if ((previousMillis / 1000) + (demoTime / 1000) - (currentMillis / 1000) <= 9) {
    display.print("0"); //leading zero
  }
    display.print( (previousMillis / 1000) + (demoTime / 1000) - (currentMillis / 1000) );

  //================= a note about this cycle, this follows the example sketch "Blink without Delay"
  if (currentMillis - previousMillis >= demoTime) {                     //if the elasped time is a multiple of the demoTime...
    previousMillis = currentMillis;                                     // save the last time you checked the interval
    //... trigger the DEMO SEQUENCE
    if (check_State == 0) {                                             // do the update only once per interval
      display.clearDisplay();
      display.setCursor(0, 3); display.print("exiting game");
      Keyboard.press(KEY_ESC); delay(50); Keyboard.release(KEY_ESC); delay(3000);         // ESC quits game
      display.setCursor(0, 3); display.print("select game ");
      Keyboard.press(KEY_DOWN_ARROW); Keyboard.release(KEY_DOWN_ARROW); delay(1000);       // down_arrow selects next game in list
      Keyboard.write('a'); delay(100);                                                     // a launches game
      demoLoops = demoLoops + 1;
      display.setCursor(0, 2); display.print("Loop counter: "); display.print(demoLoops);
      check_State = 1;
     
         }

    else {
      check_State = 0;
      display.clearDisplay();
      display.setCursor(0, 3); display.print("exiting game");
      Keyboard.press(KEY_ESC); delay(50); Keyboard.release(KEY_ESC); delay(3000);        // ESC quits game
      display.setCursor(0, 3); display.print("select game ");
      Keyboard.press(KEY_DOWN_ARROW); Keyboard.release(KEY_DOWN_ARROW); delay(100);      // down_arrow selects next game in list
      Keyboard.write('a'); delay(10);                                                    // a launches game
      demoLoops = demoLoops + 1;
      display.setCursor(0, 2); display.print("Loop counter: "); display.print(demoLoops);
        }}}