#include <ArduinoJson.h>
//Startup must ask raspberry for a ID!!! used to restart the loop

//uid q:  {"message":"who am i"}
//uid a:  {"uid":2,"nomtemperature":512,"power":0} //Nominal temperature still required to controll the fan
//uid a:  {"uid":2,"nomtemperature":512,"nomtemperatyre":512,"power":0}

//To reenable the power:  {"power":0,"uid":2} / {"power":0,"uid":"all"}
//To disable the power: {"power":1,"uid":2} / {"power":1,"uid":"all"}
//To update nominal values: {"uid":2,"nomtemperature":300} / {"uid":"all","nomoxygen":300} Only temperature updates

//The server does the conversion from 0-1024 to a value

int temperatureValue = 0;
int oxygenValue = 0;
int nominalTemperatureValue = 0;
int nominalOxygenValue = 0;

int powerState = LOW;
int uid = 0;

int temperatureInterval = 1000;
long temperaturePreviousMillis = 0;
int temperatureLedState = LOW;

int oxygenInterval = 1000;
long oxygenPreviousMillis = 0;
int oxygenLedState = LOW;

int sendInterval = 1000;
long sendPreviousMillis = 0;

int p4 = false, p5 = false, p6 = false, p7 = false, p8 = false, p9 = false, p10 = false, p11 = false, p12 = false, p13 = false;

void setup() {
  pinMode(2, INPUT);  //Panic button
  pinMode(3, OUTPUT); //Fan
  pinMode(A5, OUTPUT); //Power Led

  //Oxygen Leds
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT); //led is broken
  pinMode(8, OUTPUT);
  //Temperature Leds
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);


  Serial.begin(115200);
  Serial.setTimeout(5000);
  while (uid == 0)
  {
    //Ask server for a ID
    DynamicJsonDocument doc(100);
    doc["message"] = "who am i";
    serializeJson(doc, Serial);
    Serial.println();

    String json;
    json = Serial.readString();
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      //Serial.print(F("deserializeJson() failed: "));
      //Serial.println(error.c_str());
    }
    else
    {
      uid = doc["uid"];
      if (doc["nomtemperature"]  )
      {
        int temp = doc["nomtemperature"];
        nominalTemperatureValue = (128*temp+12800)/25;
      }
      /*if (doc["nomoxygen"]  )
        {
        nominalOxygenValue = doc["nomoxygen"];
        }*/
      if (doc["power"]  )
      {
        powerState = doc["power"];
      }
      else
      {
        powerState = 0;
      }
    }
  }

  if (powerState == 0)
  {
    digitalWrite(A5, HIGH);
  }
  Serial.setTimeout(5);
}

void loop() {
  DynamicJsonDocument doc(150);
  if (powerState == LOW)
  {
    powerState = digitalRead(2);
    if (powerState == HIGH)
    {
      //Should reduce false positives
      for (int i = 0; i < 10; i++)
      {
        powerState += digitalRead(2);
        delay(1);
      }
      if (powerState > 9)
      {
        powerState = HIGH;
        digitalWrite(A5, LOW);
      }
      else
      {
        powerState = LOW;
      }
    }

    temperatureValue = analogRead(A0);
    oxygenValue = analogRead(A1);



    if (temperatureValue >= (nominalTemperatureValue * 1.1))
    {
      analogWrite(3, 255);
    }
    else if (temperatureValue < (nominalTemperatureValue * 0.9))
    {
      analogWrite(3, 0);
    }

    { //Sends to serial only every 1/s
      long currentMillis = millis();
      sendInterval = 1000;
      if (currentMillis - sendPreviousMillis >= sendInterval || powerState != 0)
      {
        sendPreviousMillis  = currentMillis;

        //Saves the data into a json object
        doc["uid"] = uid;
        doc["temperature"] = temperatureValue;
        doc["oxygen"] = oxygenValue;
        doc["power"] = powerState;

        //Sends the json obj to serial
        serializeJson(doc, Serial);
        Serial.println();
      }
    }

    //Checks if a new message has come
    if (Serial.available() > 0)
    {
      String json;
      //Reads the message
      json = Serial.readString();

      DeserializationError error = deserializeJson(doc, json);
      // Test if parsing succeeds.
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
      }
      else
      {
        //Checks if the message is ment for this collector
        if (doc["uid"] == uid || doc["uid"] == "all" || doc["uid"] == 0)
        {
          if ((doc.containsKey("13") || doc.containsKey("12") || doc.containsKey("11") || doc.containsKey("10") || doc.containsKey("9")) &&
              (doc.containsKey("8") || doc.containsKey("7") || doc.containsKey("6") || doc.containsKey("5") || doc.containsKey("4")))
          {
            p4 = doc["4"];
            p5 = doc["5"];
            p6 = doc["6"];
            p7 = doc["7"];
            p8 = doc["8"];
            p9 = doc["9"];
            p10 = doc["10"];
            p11 = doc["11"];
            p12 = doc["12"];
            p13 = doc["13"];
          }

          //Updates values from message
          if (doc.containsKey("nomtemperature"))
          {
            int temp = doc["nomtemperature"];
            nominalTemperatureValue = (128*temp+12800)/25;
          }
          /*if (doc.containsKey("nomoxygen"))
            {
            nominalOxygenValue = doc["nomoxygen"];
            }*/
          if (doc.containsKey("power"))
          {
            powerState = doc["power"];
          }
        }

      }
    }

    //Temperature leds above nominal
    if (p13 == 1)
    {
      digitalWrite(9, LOW);
      digitalWrite(10, LOW);
      digitalWrite(11, LOW);
      digitalWrite(12, LOW);
      digitalWrite(13, HIGH);
    }
    else if (p13 == 2)
    {
      temperatureInterval = 250;
      long currentMillis = millis();
      if (currentMillis - temperaturePreviousMillis >= temperatureInterval)
      {
        temperaturePreviousMillis  = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (temperatureLedState == LOW)
        {
          digitalWrite(9, LOW);
          digitalWrite(10, LOW);
          digitalWrite(11, LOW);
          digitalWrite(12, LOW);
          digitalWrite(13, HIGH);
          temperatureLedState = HIGH;
        }
        else
        {
          digitalWrite(13, LOW);
          temperatureLedState = LOW;
        }
      }
    }
    else if (p12 == 1)
    {
      digitalWrite(9, LOW);
      digitalWrite(10, LOW);
      digitalWrite(11, LOW);
      digitalWrite(12, HIGH);
      digitalWrite(13, LOW);
    }
    else if (p12 == 2)
    {
      temperatureInterval = 500;
      long currentMillis = millis();
      if (currentMillis - temperaturePreviousMillis >= temperatureInterval)
      {
        temperaturePreviousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (temperatureLedState == LOW)
        {
          digitalWrite(9, LOW);
          digitalWrite(10, LOW);
          digitalWrite(11, LOW);
          digitalWrite(12, HIGH);
          digitalWrite(13, LOW);
          temperatureLedState = HIGH;
        }
        else
        {
          digitalWrite(12, LOW);
          temperatureLedState = LOW;
        }
      }
    }
    //Temperature leds below   nominal
    else if (p9 == 1)
    {
      digitalWrite(9, HIGH);
      digitalWrite(10, LOW);
      digitalWrite(11, LOW);
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);
    }
    else if (p9 == 2)
    {
      temperatureInterval = 250;
      long currentMillis = millis();
      if (currentMillis - temperaturePreviousMillis >= temperatureInterval)
      {
        temperaturePreviousMillis  = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (temperatureLedState == LOW)
        {
          digitalWrite(9, HIGH);
          digitalWrite(10, LOW);
          digitalWrite(11, LOW);
          digitalWrite(12, LOW);
          digitalWrite(13, LOW);
          temperatureLedState = HIGH;
        }
        else
        {
          digitalWrite(9, LOW);
          temperatureLedState = LOW;
        }
      }
    }
    else if (p10 == 1)
    {
      digitalWrite(9, LOW);
      digitalWrite(10, HIGH);
      digitalWrite(11, LOW);
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);
    }
    else if (p10 == 2)
    {
      temperatureInterval = 500;
      long currentMillis = millis();
      if (currentMillis - temperaturePreviousMillis >= temperatureInterval)
      {
        temperaturePreviousMillis  = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (temperatureLedState == LOW)
        {
          digitalWrite(9, LOW);
          digitalWrite(10, HIGH);
          digitalWrite(11, LOW);
          digitalWrite(12, LOW);
          digitalWrite(13, LOW);
          temperatureLedState = HIGH;
        }
        else
        {
          digitalWrite(10, LOW);
          temperatureLedState = LOW;
        }
      }
    }
    else //if (p11 == 1)
    {
      digitalWrite(9, LOW);
      digitalWrite(10, LOW);
      digitalWrite(11, HIGH);
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);
    }

    //Oxygen leds above nominal
    if (p8 == 1)
    {
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, HIGH);
    }
    else if (p8 == 2)
    {
      oxygenInterval = 250;
      long currentMillis = millis();
      if (currentMillis - oxygenPreviousMillis >= oxygenInterval)
      {
        oxygenPreviousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (oxygenLedState == LOW)
        {
          digitalWrite(4, LOW);
          digitalWrite(5, LOW);
          digitalWrite(6, LOW);
          digitalWrite(7, LOW);
          digitalWrite(8, HIGH);
          oxygenLedState = HIGH;
        }
        else
        {
          digitalWrite(8, LOW);
          oxygenLedState = LOW;
        }
      }
    }
    else if (p7 == 1)
    {
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
      digitalWrite(7, HIGH);
      digitalWrite(8, LOW);
    }
    else if (p7 == 2)
    {
      oxygenInterval = 500;
      long currentMillis = millis();
      if (currentMillis - oxygenPreviousMillis >= oxygenInterval)
      {
        oxygenPreviousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (oxygenLedState == LOW)
        {
          digitalWrite(4, LOW);
          digitalWrite(5, LOW);
          digitalWrite(6, LOW);
          digitalWrite(7, HIGH);
          digitalWrite(8, LOW);
          oxygenLedState = HIGH;
        }
        else
        {
          digitalWrite(7, LOW);
          oxygenLedState = LOW;
        }
      }
    }
    //Oxygen leds below nominal
    else if (p4 == 1)
    {
      digitalWrite(4, HIGH);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);
    }
    else if (p4 == 2)
    {
      oxygenInterval = 250;
      long currentMillis = millis();
      if (currentMillis - oxygenPreviousMillis >= oxygenInterval)
      {
        oxygenPreviousMillis  = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (oxygenLedState == LOW)
        {
          digitalWrite(4, HIGH);
          digitalWrite(5, LOW);
          digitalWrite(6, LOW);
          digitalWrite(7, LOW);
          digitalWrite(8, LOW);
          oxygenLedState = HIGH;
        }
        else
        {
          digitalWrite(4, LOW);
          oxygenLedState = LOW;
        }
      }
    }
    else if (p5 == 1)
    {
      digitalWrite(4, LOW);
      digitalWrite(5, HIGH);
      digitalWrite(6, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);
    }
    else if (p5 == 2)
    {
      oxygenInterval = 500;
      long currentMillis = millis();
      if (currentMillis - oxygenPreviousMillis >= oxygenInterval)
      {
        oxygenPreviousMillis  = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (oxygenLedState == LOW)
        {
          digitalWrite(4, LOW);
          digitalWrite(5, HIGH);
          digitalWrite(6, LOW);
          digitalWrite(7, LOW);
          digitalWrite(8, LOW);
          oxygenLedState = HIGH;
        }
        else
        {
          digitalWrite(5, LOW);
          oxygenLedState = LOW;
        }
      }
    }
    else //if (p6 == 1)
    {
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, HIGH);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);
    }

  }
  else
  {
    //Checks if a new message has come
    if (Serial.available() > 0)
    {
      String json;
      //Reads the message
      json = Serial.readString();

      //Parses the json string
      DeserializationError error = deserializeJson(doc, json);
      //Test if parsing succeeds.
      if (error)
      {
        /*Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());*/
      }
      //Checks if the message contained reenable
      if (doc["power"] == 0 && (doc["uid"] == uid || doc["uid"] == "all" || doc["uid"] == 0))
      {
        powerState = LOW;
        digitalWrite(A5, HIGH);
      }
      //Can also be received
      if (doc.containsKey("nomtemperature"))
      {
        int temp = doc["nomtemperature"];
        nominalTemperatureValue = (128*temp+12800)/25;
      }
      /*if (doc.containsKey("nomoxygen"))
        {
        nominalOxygenValue = doc["nomoxygen"];
        }*/
    }
    else if (doc["power"] >= 1)
    {
      digitalWrite(A5, LOW);
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);
      digitalWrite(9, LOW);
      digitalWrite(10, LOW);
      digitalWrite(11, LOW);
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);

      { //Sends to serial only every 5/s
        long currentMillis = millis();
        sendInterval = 5000;
        if (currentMillis - sendPreviousMillis >= sendInterval)
        {
          DynamicJsonDocument powerDoc(50);
          sendPreviousMillis  = currentMillis;

          //Saves the data into a json object
          powerDoc["uid"] = uid;
          powerDoc["power"] = powerState;

          //Sends the json obj to serial
          serializeJson(powerDoc, Serial);
          Serial.println();
        }
      }
    }
    delay(200);
  }
  //delay(1000);
  /*Serial.print("nominalT: ");
    Serial.println(nominalTemperatureValue);
    Serial.print("nominalO: ");
    Serial.println(nominalOxygenValue);*/
}
