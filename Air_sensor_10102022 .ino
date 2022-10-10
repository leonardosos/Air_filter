#include "PMS.h"                  //https://github.com/fu-hsi/pms
#include "MHZ19.h"                // https://github.com/WifWaf/MH-Z19     
#include "DHT.h"                  // - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
#include <LiquidCrystal_I2C.h>    // https://randomnerdtutorials.com  
#include "ThingSpeak.h"           // https://thingspeak.com/
#include <WiFi.h>



//////////////////////////////////////////////         

const char* ssid     = "";   //wifi setting
const char* password = "";

WiFiClient  client;

unsigned long myChannelNumber = ;  //thingspeak.com setting 
const char * myWriteAPIKey = "";

//////////////////////////////////////////////              

PMS pms(Serial2);
PMS::DATA data;

String val1, val2, val3;

//////////////////////////////////////////////              

#define RX_PIN 26    // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 27    // Tx pin which the MHZ19 Rx pin is attached to

MHZ19 myMHZ19;       // Constructor for library

unsigned long getDataTimer = 0;

//////////////////////////////////////////////

#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 

DHT dht(DHTPIN, DHTTYPE);

//////////////////////////////////////////////

// set the LCD number of columns and rows
int lcdColumns = 20;
int lcdRows = 4;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

int botton_state = LOW;
const int SENSOR_PIN = 33;

//////////////////////////////////////////////

unsigned long minut_refresh = 4;
unsigned long millisecondi_refresh = minut_refresh * 1000 * 60;

#define wait millisecondi_refresh


unsigned long minut_refresh_lcd = 3;
unsigned long millisecond_refresh_lcd = minut_refresh_lcd * 1000 * 60;

#define wait_lcd millisecond_refresh_lcd

unsigned long start_time;
unsigned long lcd_time;

float list_value[7];
bool schermo_on = false;
bool first_read = false;
bool upload_ok = true;

void setup() {

  Serial.begin(9600);     //serial monitor

  Serial2.begin(9600);    //PMS sensors

  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);       // MH-Z19 serial start   
    
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  myMHZ19.begin(Serial1);                                // *Serial(Stream) refence must be passed to library begin(). 
  myMHZ19.autoCalibration();                             // Turn auto calibration ON (OFF autoCalibration(false))

  dht.begin();


  lcd.init();                                            // initialize LCD                    
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Connecting to Wi-fi:");
  lcd.setCursor(0, 1);
  lcd.print(ssid);  

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);                            // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected.");
  delay(1500);
  lcd.noBacklight();
  WiFi.mode(WIFI_STA);  


  pinMode(SENSOR_PIN, INPUT);
  pinMode(2, OUTPUT);
 
  start_time = millis();
  
}

void loop() {

  int botton_state = digitalRead(SENSOR_PIN);

  if(botton_state == HIGH){       //show lcd

    if (schermo_on == true){
      lcd.noBacklight();
      schermo_on = false;
    }
    else{
      lcd.backlight();
      lcd_time = millis();
      schermo_on = true;
      
      if(upload_ok == true){
        refresh_lcd(list_value);
      }
    }
    delay(250);
  }


  if ((millis() - lcd_time >= wait_lcd) && (schermo_on == true)){  //turn off lcd after lcd_time
    lcd.noBacklight();
    schermo_on = false;
  }


  if ((millis() - start_time >= wait) || (first_read == false)){   //reading sensor and send data

    Serial.println("Start reading sensors ... ");

    get_PMS(list_value);
    get_co2(list_value);
    get_temp(list_value);

    Serial.println();

    if(upload_ok == true){
    refresh_lcd(list_value);
    }

    send_data(list_value);

    start_time = millis();
    first_read = true;

  }
}




/////////////////////////////////////function

void get_PMS(float* list_value){

    if (pms.readUntil(data))
  {
    val1 = data.PM_AE_UG_1_0;    
    val2 = data.PM_AE_UG_2_5;
    val3 = data.PM_AE_UG_10_0;

    int val_pm1;                 
    int val_pm2;
    int val_pm10;

    val_pm1 = val1.toInt();      //trasform from string to int
    val_pm2 = val2.toInt();
    val_pm10 = val3.toInt();
    
    list_value[0] = val_pm1;
    list_value[1] = val_pm2;
    list_value[2] = val_pm10;
 
    Serial.print("PM1.0: ");                //print value on serial monitor
    Serial.print(list_value[0]);
    Serial.print(" (ug/m3)  ");

    Serial.print("PM2.5: ");
    Serial.print(list_value[1]);
    Serial.print(" (ug/m3)  ");

    Serial.print("PM10: ");
    Serial.print(list_value[2]);
    Serial.println(" (ug/m3)  ");

  }
}


void get_co2(float* list_value){

  int CO2; 

  /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
  if below background CO2 levels or above range (useful to validate sensor). You can use the 
  usual documented command with getCO2(false) */

  CO2 = myMHZ19.getCO2();                             // Request CO2 (as ppm)
  int8_t Temp;
  Temp = myMHZ19.getTemperature();                    // Request Temperature (as Celsius)

  list_value[3] = CO2;
  list_value[4] = Temp;   


  Serial.print("CO2: ");                        //print value on serial monitor
  Serial.print(list_value[3]);                                
  Serial.print(" (ppm)");    

  Serial.print(" (con temp sensore di: ");                  
  Serial.print( (int) list_value[4]); 
  Serial.println(" C)");

                     
}



void get_temp(float* list_value){

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) ) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  list_value[5] = h;
  list_value[6] = t;  
  list_value[7] = hic;

  Serial.print("Humidity: ");                 //print value on serial monitor
  Serial.print(list_value[5]);
  Serial.print("%  Temperature: ");
  Serial.print(list_value[6]);
  Serial.print(" C ");
  Serial.print(" (con: ");    
  Serial.print(list_value[7]);
  Serial.println(" C percepiti)");
}



void refresh_lcd(float* list_value){          //refresh data on lcd

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("pm1");
  lcd.setCursor(0, 1);
  lcd.print("->");  
  lcd.setCursor(3, 1);
  lcd.print((int) list_value[0]);

  lcd.setCursor(6, 0);
  lcd.print("pm2.5");
  lcd.setCursor(6, 1);
  lcd.print("->");  
  lcd.setCursor(9, 1);
  lcd.print((int) list_value[1]);

  lcd.setCursor(14, 0);
  lcd.print("pm10");
  lcd.setCursor(14, 1);
  lcd.print("->");  
  lcd.setCursor(17, 1);
  lcd.print((int) list_value[2]);

  lcd.setCursor(0, 2);
  lcd.print("CO2");
  lcd.setCursor(4, 2);
  if(list_value[3] > 10){
  lcd.print((int) list_value[3]);
  }
  else {
  lcd.print("heating");
  }

  lcd.setCursor(11, 2);
  lcd.print("(Ts");
  lcd.setCursor(15, 2);
  lcd.print((int) list_value[4]);
  lcd.setCursor(17, 2);
  lcd.print(")");

  lcd.setCursor(0, 3);
  lcd.print("Hum");
  lcd.setCursor(4, 3);
  lcd.print(list_value[5]);

  lcd.setCursor(10, 3);
  lcd.print("Temp");
  lcd.setCursor(15, 3);
  lcd.print(list_value[6]);
}

void send_data(float* list_value){                //send data to thingspeak for grafh

  if(WiFi.status() != WL_CONNECTED){             // Reconnect to WiFi if it down
    Serial.print("Attempting to connect");

    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Connecting to Wi-fi:");
    lcd.setCursor(0, 1);
    lcd.print(ssid);  

    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, password); 
      delay(500);
      Serial.println(" . ");   
    } 
    Serial.println("\nConnected.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi connected.");
    delay(1500);
    lcd.noBacklight();
  }

  // set the fields with the values
  ThingSpeak.setField(1, (int) list_value[0]);
  ThingSpeak.setField(2, (int) list_value[1]);
  ThingSpeak.setField(3, (int) list_value[2]);
  if(list_value[3] > 10){
  ThingSpeak.setField(4, (int) list_value[3]);}
  ThingSpeak.setField(5, (int) list_value[4]);
  ThingSpeak.setField(6, list_value[5]);
  ThingSpeak.setField(7, list_value[6]);
  ThingSpeak.setField(8, list_value[7]);

  // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if(x == 200){
    Serial.println("Channel update successful.");
    Serial.println("");
    upload_ok = true;

/*
    digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)   --> conferm Channel update successful on esp32 board led <--
    delay(500);             // wait for a second
    digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
    delay(500);             // wait for a second
    digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(500);             // wait for a second
    digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
    delay(500);             // wait for a second
    digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(500);             // wait for a second
    digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
*/
  }
  else{
    Serial.print("Problem updating channel. HTTP error code ");
    Serial.println(String(x));

    lcd.clear();
    lcd.backlight();
    lcd_time = millis();
    schermo_on = true;
    upload_ok = false; 

    lcd.setCursor(0, 1);
    lcd.print("Problem ");
    lcd.setCursor(0, 2);
    lcd.print(" with updating");  
    lcd.setCursor(0, 3);
    lcd.print("  channel."); 
    
  }
}






