// Wifi
#include <SoftwareSerial.h>
#define RX 2
#define TX 3
String accessPoint = "ASUS_Guest";       
String password = "Aqua1234";
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
int tankDepth = 60;
int tankLength = 175;
int tankWidth = 110;

// ph sensor
#define sensorPin A2            //pH meter Analog output to Arduino Analog Input 0
#define Offset 0.15            //deviation compensate
#define LED 13

// Sound Sensor
int  soundPin  =  A0;     // select the input  pin for  the potentiometer 

// Sensor Array
#define samplingInterval 50
#define ArrayLength  150    //times of collection
int sensorArray[ArrayLength];   //Store the average value of the sensor feedback
int sensorArrayIndex=0;   

// General program runtime variables
int countTrueCommand;
int countTimeCommand; 
boolean found = false; 

float tempSensor = 0;
float humiditySensor = 0;
float ultraSensor = 0; 
float pHSensor = -1;
float waterVolume = 0;
float soundSensor = 0;
double cycleTime;
long delayTime = 1200000;
double looptime;

bool testing = false;
int testRun = 3;
int testCount = 0;

void setup()
{
  Serial.begin(9600);
  esp8266.begin(115200);
  sendCommand("AT",2,"OK");
  sendCommand("AT+CWMODE=1",2,"OK");
  sendCommand("AT+CWJAP=\""+ accessPoint +"\",\""+ password +"\"",5,"OK");
}

void loop() 
{
    looptime = millis();

    testCount++;

    // Send Temperature data to ThingSpeak
    Serial.println("< Getting Temp Data >");
    getSensorData(1);

    // Send Ultrasound data to ThingSpeak
    Serial.println("< Getting Ultrasound Data >");
    getSensorData(2);
    waterVolume = (ultraSensor * tankLength * tankWidth) / 1000; // Calc total water volume and convert to litres

    // Send pH data to ThingSpeak
    Serial.println("< Getting pH Data >");
    getSensorData(3);

    // Send Sound data to ThingSpeak
    Serial.println("< Getting Sound Data >");
    getSensorData(4);

    if (testCount == 1)
    {
      cycleTime = delayTime + ((millis() - looptime) / 60000);
      Serial.print("1st cycleTime ");
      Serial.println(cycleTime, 2);
    }

    sendData ();  
    countTrueCommand++;

    if ((testCount == testRun) && (testing))
    {
      Serial.print("Test Counter = ");
      Serial.println(testCount);
      Serial.println("Test run COMPLETED");
      sendCommand("AT+CIPCLOSE=0", 5, "OK");
      while (1)
      {
      }; // Infinite loop!
    }

    Serial.print("Waiting 20mins between tests mins...");
    Serial.println(delayTime);
    delay (delayTime); // Send results every 20mins
    cycleTime = ((millis() - looptime)/60000);  
    Serial.print("cycleTime ");
    Serial.print(cycleTime,2);
    Serial.println(" mins ... DONE waiting .... ");
}

void sendData() 
{
    sendCommand("AT+CIPMUX=1",5,"OK");
    sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,8,"OK");
    String getData = "GET /update?api_key="+ writeAPI
        +"&field1="+String(tempSensor)
        +"&field2="+String(ultraSensor)
        +"&field3="+String(pHSensor)
        +"&field4="+String(waterVolume)
        +"&field5="+String(soundSensor)
        +"&field6="+String(cycleTime);
    Serial.println(getData);
    sendCommand("AT+CIPSEND=0," +String(getData.length()+4),4,">");
    esp8266.println(getData);
    sendCommand("AT+CIPCLOSE=0",5,"OK");
}

void getSensorData(int Sensor)
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

//      return (tempSensor);
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

//      return (ultraSensor);
    }
    else
    if (Sensor==3)
    {     
      Serial.println("  <Sampling pH values>");
      bool sampling=true;
      sensorArrayIndex=0;
      static float voltage;   
        
      while (sensorArrayIndex < ArrayLength)
      {
            sensorArray[sensorArrayIndex++]=analogRead(sensorPin);
            delay (samplingInterval);
//            Serial.println("<<<< SAMPLING in progress >>>>" );
      }        
         
      voltage = averagearray(sensorArray, ArrayLength)*5.0/1024;
      pHSensor = 3.5*voltage+Offset;
      Serial.print("    Voltage:");
      Serial.print(voltage,2);
      Serial.print("    pH value: ");
      Serial.println(pHSensor,2);
      Serial.println("  <DONE sampling pH values!>" );
          
//      return (pHSensor);
    }
    else
    if (Sensor==4)
    {     
      Serial.println("  <Sampling sound>");
      bool sampling=true;
      sensorArrayIndex=0;
      static float voltage;  
     
      while (sensorArrayIndex < ArrayLength)
      {
            sensorArray[sensorArrayIndex++]=analogRead(soundPin);
            delay (samplingInterval);
//            Serial.println("<<<< SAMPLING sound in progress >>>>" );
      }               
      soundSensor = averagearray(sensorArray, ArrayLength);
      
      Serial.print("    sound value: ");
      Serial.println(soundSensor,2);
      Serial.println("  <DONE sampling SOUNDS values!>" );
          
//      return (soundSensor);
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
 
    if(esp8266.find(readReplay)) //ok
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

 float averagearray(int* arr, int number)
 {
  int i;
  int max,min;
  float avg;
  long amount=0;
 
  if(number<=0)
  {
    Serial.println("Error number for the array to average!/n");
    return 0;
  }
  
  if(arr[0]<arr[1])
  {
      min=arr[0];
      max=arr[1];
  }
    else
  {
      min=arr[1];
      max=arr[0];
  }
    
  for(i=2;i<number;i++)
  {
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
    
    
    avg = (float)amount/(number-2);
  }
  
  return avg;
}
