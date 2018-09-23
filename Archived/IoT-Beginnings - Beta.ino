#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Wifi
#include <SoftwareSerial.h>
#define RX 2
#define TX 3
String accessPoint = "Police_Ops1";       
String password = "Ma11hew15";
int channel = 407580; 
String writeAPI = "M4TG5ULS2PYGO8LO";
String HOST = "api.thingspeak.com";
String PORT = "80";
SoftwareSerial esp8266(RX,TX); 

// Temperature & Humdity Sensor
#include <dht.h>
#define dataPin 4
dht DHT;

// Ultra-sonic Sensor
#include <SR04.h>
#define triggerPin 6
#define echoPin 5
SR04 sr04 = SR04(echoPin,triggerPin);
float a;
float tankDepth = 60;
float tankLenght = 175;
float tankWidth = 110;

// ph sensor
#define sensorPin A2            //pH meter Analog output to Arduino Analog Input 0
#define Offset -7.0            //deviation compensate
#define LED 13
#define samplingInterval 20
#define printInterval 800
#define ArrayLength  40    //times of collection
int pHArray[ArrayLength];   //Store the average value of the sensor feedback
int pHArrayIndex=0;   


// General program runtime variables
int countTrueCommand;
int countTimeCommand; 
boolean found = false; 

float tempSensor = 0;
float humiditySensor = 0;
float ultraSensor = 0; 
float pHSensor = -1;
float waterVolume = 0;

bool testing = false;
int testRun = 1;
int testCount = 0;

void setup()
{
  Serial.begin(9600);
  esp8266.begin(115200);
  sendCommand("AT",2,"OK");
  sendCommand("AT+CWMODE=1",2,"OK");
  sendCommand("AT+CWJAP=\""+ accessPoint +"\",\""+ password +"\"",5,"OK");

//  pinMode(LED,OUTPUT); 
//  Serial.println("pH meter experiment!");

  lcd.begin(16,2);
  lcd.print("Last pH ->  v0.1");
  lcd.setCursor(1,1);
  lcd.print(pHSensor);
  lcd.print(" pH   ");
}


void loop() 
{
    testCount++; 
//      Serial.print("Test Counter = " ); 
//      Serial.println(testCount); 
    
    // Send Temperature data to ThingSpeak
    Serial.println("< Getting Temp Data >" );
    tempSensor = getSensorData(1);
    
    // Send Ultrasound data to ThingSpeak
    Serial.println("< Getting Ultrasound Data >" );
    ultraSensor = getSensorData(2);
    waterVolume = (ultraSensor * tankLenght * tankWidth) / 1000; // Calc total water volume and convert to litres

    // Send pH data to ThingSpeak
    Serial.println("< Getting pH Data >" );
    pHSensor = getSensorData(3);

    sendData ();
    
    countTrueCommand++;
    
    if ((testCount == testRun) && (testing))
    {
        Serial.println("Test run COMPLETED");
        sendCommand("AT+CIPCLOSE=0",5,"OK");
        while(1) {}; // Infinite loop!   
    }

    lcd.setCursor(1,1);
    lcd.print(pHSensor);
    lcd.print(" pH   ");
    lcd.setCursor(12,1);
    lcd.print(testCount);

    Serial.println("Waiting 10mins between tests");
    delay (600000); // Send results every 10mins
    Serial.println("DONE waiting .... ");
}

void sendData() 
{
    sendCommand("AT+CIPMUX=1",5,"OK");
    sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,8,"OK");
    String getData = "GET /update?api_key="+ writeAPI
        +"&field1="+String(tempSensor)
        +"&field2="+String(ultraSensor)
        +"&field3="+String(pHSensor)
        +"&field4="+String(waterVolume);
    Serial.println(getData);
    sendCommand("AT+CIPSEND=0," +String(getData.length()+4),4,">");
    esp8266.println(getData);
    sendCommand("AT+CIPCLOSE=0",5,"OK");
}

int getSensorData(int Sensor)
{
    if (Sensor==1)
    {
      float readData = DHT.read22(dataPin); // Reads the data from the sensor
      tempSensor = DHT.temperature;
      humiditySensor = DHT.humidity;
      Serial.print("Temp Sensor = " ); 
      Serial.print(tempSensor); 
      Serial.println(" degC");
      delay(500);

      return (tempSensor);
    }
    else
    if (Sensor==2)
    {
      ultraSensor = sr04.Distance();
      Serial.print("ultra Range Sensor = " ); 
      Serial.print(ultraSensor,2);
      Serial.print(" cm :: ");
      ultraSensor = tankDepth - ultraSensor; // Offset for depth of 40cm tank
      Serial.print(ultraSensor,2);
      Serial.println(" deep");
      delay(500);

      return (ultraSensor);
    }
    else
    if (Sensor==3)
    {     
      Serial.println("<Sampling pH values>");
      bool sampling=true;
      pHArrayIndex=0;
      static unsigned long samplingTime = millis();
      static unsigned long printTime = millis();
      static float voltage;   

      Serial.print("Sampling Time ... ");
      Serial.println(samplingTime);
      Serial.print("Print Time ... ");
      Serial.println(printTime);
        
      while (pHArrayIndex < ArrayLength)
      {
            pHArray[pHArrayIndex++]=analogRead(sensorPin);
            delay (samplingInterval);
            Serial.println("<<<< SAMPLING in progress >>>>" );
      }        
         
      voltage = averagearray(pHArray, ArrayLength)*5.0/1024;
      pHSensor = 3.5*voltage+Offset;
      Serial.print("Voltage:");
      Serial.print(voltage,2);
      Serial.print("    pH value: ");
      Serial.println(pHSensor,2);
      digitalWrite(LED,digitalRead(LED)^1);
      printTime=millis();
      Serial.println("<DONE sampling pH values!>" );
          
//          if(millis()-samplingTime > samplingInterval)
//          {
//            pHArray[pHArrayIndex++]=analogRead(sensorPin);
//            if(pHArrayIndex==ArrayLength)pHArrayIndex=0;
//        
//           voltage = averagearray(pHArray, ArrayLength)*5.0/1024;
//            pHSensor = 3.5*voltage+Offset;
//        
//            samplingTime=millis();
//            Serial.println("<<<< SAMPLING in progress >>>>" );
//          }
//  
//          if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
//          {
//            Serial.print("Voltage:");
//            Serial.print(voltage,2);
//            Serial.print("    pH value: ");
//            Serial.println(pHSensor,2);
//            digitalWrite(LED,digitalRead(LED)^1);
//            printTime=millis();
//            Serial.println("<DONE sampling pH values!>" );
//            sampling=false;
//          } 

      return (pHSensor);
    }
}


void sendCommand(String command, int maxTime, char readReplay[]) 
{
  Serial.print("... ");
  Serial.print(countTrueCommand);
  Serial.print(". AT command => ");
  Serial.print(command);
  Serial.print(" ");

  String responseText;
  
  while(countTimeCommand < (maxTime*1))
  {
    esp8266.println(command);

// Debugging entries to see AT responses
//    responseText = esp8266.readString();
//    Serial.print("RESPONSE => ");
//    Serial.println(responseText);
  
    if(esp8266.find(readReplay))//ok
    {
      found = true;
      break;
    }  
    countTimeCommand++;
  }
  
  if(found == true)
  {
    Serial.println("OYI");
    countTrueCommand++;
    countTimeCommand = 0;
  }
  
  if(found == false)
  {
    Serial.println("Fail");
    countTrueCommand = 0;
    countTimeCommand = 0;
  }
  
  found = false;
 }

 double averagearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
 
  if(number<=0){
    Serial.println("Error number for the array to average!/n");
    return 0;
  }
  
  if(number<5)
  {   //less than 5, calculated directly statistics
    for(i=0;i<number;i++)
    {
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }
  else
  {
    if(arr[0]<arr[1])
    {
      min = arr[0];max=arr[1];
    }
    else
    {
      min=arr[1];max=arr[0];
    }
    
    for(i=2;i<number;i++){
      if(arr[i]<min)
      {
        amount+=min;        //arr<min
        min=arr[i];
      }
      else
      {
        if(arr[i]>max)
        {
          amount+=max;    //arr>max
          max=arr[i];
        }
        else
        {
          amount+=arr[i]; //min<=arr<=max
        }
      } //else if
    } //for
    
    avg = (double)amount/(number-2);
  }//if
  
  return avg;
}
