// Michael Klements
// Skittles Colour Sorter
// 11 January 2019
// www.the-diy-life.com

#include <Wire.h>               //Include the required libraries
#include "Adafruit_TCS34725.h"
#include <Servo.h>

#define SORTER_LIMIT        180
#define SORTER_SPACING      (SORTER_LIMIT / 5)
#define SORTER_POS_YELLOW   (0 + 0)
#define SORTER_POS_ORANGE   ((SORTER_SPACING*1) + 0)
#define SORTER_POS_RED      ((SORTER_SPACING*2) + 0)
#define SORTER_POS_GREEN    ((SORTER_SPACING*3) + 0)
#define SORTER_POS_PURPLE   ((SORTER_SPACING*4) + 0)
#define SORTER_POS_BLUE     ((SORTER_SPACING*5) + 0)

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);   //Setup the colour sensor through Adafruit library
Servo selector;
Servo sorter;

int pinLED = 4;      //Assign pins for the colour picker LED, push button and RGB LED

int colourRedSP = 85;                           //Setpoint for Red to determine Yellow, Orange, Red from Green & Purple
int colourGreenSPGP = 97;                       //Setpoint for Green to determine Green from Purple
int colourGreenSPY = 91;                        //Setpoint for Green to determine Yellow from Orange and Red
int colourGreenSPO = 81;                        //Setpoint for Green to determine Orange from Red

int selectorPosition[3] = {26,80,130};    //Selector positions for Drop Area, Sensor and Hopper
int selectorSpeed = 15;                   //Speed to move selector

// int sorterPosition[5] = {8,42,82,120,157};   //Sorter colour positions for Yellow, Orange, Red, Green and Purple
int sorterPosition[6] = {SORTER_POS_YELLOW, SORTER_POS_ORANGE, SORTER_POS_RED, SORTER_POS_GREEN, SORTER_POS_PURPLE, SORTER_POS_BLUE};
int sorterSpeed = 5;                         //Speed to move sorter
//int sorterPos = sorterPosition[2];           //Stores current sorter position
int sorterPos = 0;
int sorterIndex = 0;
int sorterValue = sorterPosition[sorterIndex];

void setup()
{
  Serial.begin(115200);
  Serial.println("\nSkittles Colour Sorter");
  
  pinMode (pinLED, OUTPUT);     //Assign output pins
  selector.attach(5);
  sorter.attach(9);
  tcs.begin();                //Connect to the colour sensor
  digitalWrite(pinLED, LOW);  //Turn off the sensor's white LED
  selector.write(selectorPosition[0]);  //Set selector to drop position to start
  delay(500);
  sorter.write(sorterPos);            //Set sorter to middle position to start
  delay(500);
}

void loop()
{
  sorterIndex = random(0, 6);
  sorterValue = sorterPosition[sorterIndex];
  Serial.print("Selector to ");
  Serial.print(sorterIndex);
  Serial.print(", ");
  Serial.print(sorterValue);
  Serial.println();
  //moveSorter(sorterPosition[sorterIndex]);
  sorter.write(sorterValue);
  delay(1000);
  return;
  
  moveSelector(0,2);                    //Move selector from drop position to hopper
  delay(200);
  moveSelector(2,1);                    //Move skittle from hopper to sensor
  delay(200);
  float red, green, blue;
  digitalWrite(pinLED, HIGH);           //Turn the sensor LED on for identification
  delay(50);
  tcs.setInterrupt(false);              //Start measurement
  delay(60);                            //Takes 50ms to read
  tcs.getRGB(&red, &green, &blue);      //Get the required RGB values
  tcs.setInterrupt(true);
  delay(100);

  Serial.print("red = ");
  Serial.print(red, 2);
  Serial.print(", green = ");
  Serial.print(green, 2);
  Serial.print(", blue = ");
  Serial.print(blue, 2);
  Serial.println();
  
  digitalWrite(pinLED, LOW);            //Turn off the sensor LED
  moveSorter (chooseTube(red,green));   //Move sorter to desired colour position
  moveSelector (1,0);                   //Drop skittle
}

int chooseTube (int red, int green)
{
  int tempPosition;
  if (red >= colourRedSP)                      //If red is high
  {
    if (green >= colourGreenSPY)               //If green is high
    {
      tempPosition = sorterPosition[0];        //Colour is yellow
    }
    else if (green <= colourGreenSPO)          //If green is low
    {
      tempPosition = sorterPosition[1];        //Colour is orange
    }
    else                                       //Else green must be medium
    {
      tempPosition = sorterPosition[2];        //Colour is red
    }
  }
  else                                         //Red must be medium or low
  {
    if (green >= colourGreenSPGP)              //If green is high
    {
      tempPosition = sorterPosition[3];        //Colour is green
    }
    else                                       //Else green must be medium or low
    {
      tempPosition = sorterPosition[4];        //Colour is purple
    }
  }
  return tempPosition;
}

void moveSelector (int pos, int target)
{
  if (pos < target)                            //Determine movement direction
  {
    for (int i=selectorPosition[pos]; i<=selectorPosition[target] ; i++) 
    {
      selector.write(i);      //Move servo in 1 degree increments with a delay between each
      delay(selectorSpeed);
    }
  }
  else
  {
    for (int i=selectorPosition[pos]; i>=selectorPosition[target] ; i--) 
    {
      selector.write(i);     //Move servo in 1 degree increments with a delay between each
      delay(selectorSpeed);
    }
  }
}

void moveSorter (int target)
{
  if (sorterPos < target)      //Determine movement direction
  {
    for (int i=sorterPos; i<=target ; i++) //Move servo in 1 degree increments with a delay between each
    {
      sorter.write(i);
      delay(sorterSpeed);
    }
  }
  else if (sorterPos > target)
  {
    for (int i=sorterPos; i>=target ; i--) //Move servo in 1 degree increments with a delay between each
    {
      sorter.write(i);
      delay(sorterSpeed);
    }
  }
  sorterPos = target;           //Update current sorter position
}

void calibrateSorter ()
{
  for (int i=0; i<=4; i++)
  {
    moveSorter (sorterPosition[i]);
    delay(2000);
  }
}

void calibrateSelector ()
{
    selector.write(selectorPosition[2]);
    delay(2000);
    moveSelector (2,1);
    delay(2000);
    moveSelector (1,0);
    delay(2000);
}
