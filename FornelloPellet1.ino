//pin
#define PIN_FIRE_SENSOR (2)
#define PIN_BUTTON_ON_OFF (3)
#define PIN_LED_SPEGNIMENTO (10)
#define PIN_LED_ACCENSIONE (11)
#define PIN_LED_ERR (12)
#define PIN_LED_ON_OFF (9)
#define PIN_RES (7)
#define PIN_MOTOR_LOAD (6)
#define PIN_MOTOR_FUME_FAN (5)
#define PIN_MOTOR_AIR_FAN (4)

//modes
#define M_OFF (0)
#define M_ST (1)
#define M_NO (2)
#define M_SH (3)

//Serial
#define ENABLE_SERIAL (true) //is serial activated
#define BOARD_RATE (9600)


//times
const int SECOND = 1000;

//Startup times
const unsigned long TST_ACCENSIONE = 500UL * SECOND;
const unsigned long TST_LOAD_ON = 2UL * SECOND;
const unsigned long TST_LOAD_OFF = 6UL * SECOND;

//Runnig times
const unsigned long TR_LOAD_ON = 4UL * SECOND;
const unsigned long TR_LOAD_OFF = 5UL * SECOND;

//Shutdown times
const unsigned long T_SH = 1200UL * SECOND;

//button intterupt
const int DEBOUNCE_TIME = 2000;

//alter Overflow
const unsigned long MAX_TIME = 4294967295 - (SECOND * 5000UL);
const unsigned long MIN_TIME = 5000UL * SECOND;



unsigned long nxLOn;  //next load tick
unsigned long nxLOff; //next Shutdown tick
bool load;            //is motor load activated

unsigned long endMode; //startup/shutdown mode end tick

int mode = 0; //current mode

bool buttonOnOff = false; //button logic state
bool buttonUpDown = false; //is the button up or down
bool buttonLastState = false;    //last button reading value
unsigned long buttonLastChangeTime = 0; //tick of the last button rading change value

unsigned long now; //current cicle tick


void setup(){
#if ENABLE_SERIAL
  Serial.begin(BOARD_RATE);
  Serial.println("Start Serial:");
  Serial.println("Stufa a pellet di Luca Salvadori. v. 0.3");
#endif
  
  pinMode(PIN_LED_ACCENSIONE, OUTPUT);
  pinMode(PIN_LED_ERR, OUTPUT);
  pinMode(PIN_LED_ON_OFF, OUTPUT);
  pinMode(PIN_RES, OUTPUT);
  pinMode(PIN_MOTOR_LOAD, OUTPUT);
  pinMode(PIN_MOTOR_AIR_FAN, OUTPUT);
  pinMode(PIN_MOTOR_FUME_FAN, OUTPUT);
  pinMode(PIN_BUTTON_ON_OFF, INPUT_PULLUP);
  pinMode(PIN_FIRE_SENSOR, INPUT_PULLUP);

  resOff();
  loadOff();
  fumeFanOff();
  airFanOff();

  if(digitalRead(PIN_FIRE_SENSOR) == LOW){
    delay(100);
    if(digitalRead(PIN_FIRE_SENSOR) == LOW){//if it sense that there is fire
      digitalWrite(PIN_LED_ERR, HIGH);
      mode = M_NO;
      beginShutdown();
    }
  }


   
}

void loop(){
  now = millis();

  //input
  bool buttonReading = digitalRead(PIN_BUTTON_ON_OFF);

  if(buttonReading != buttonLastState){//is the reading change
    buttonLastChangeTime = millis();
  }

  if(isNowPast(buttonLastChangeTime + DEBOUNCE_TIME , millis())){ //is the value not change in a while
    if(buttonReading != buttonUpDown){//is the new value different from the last
      buttonUpDown = buttonReading;

      if(!buttonUpDown){
        buttonOnOff = !buttonOnOff;
        #if ENABLE_SERIAL
            Serial.print(now / SECOND);
            Serial.print(" : button state change -> ");
            Serial.println(buttonOnOff);  
        #endif

        if(buttonOnOff){
          beginStartMode();
        }else{
          beginShutdown();
        }
      }  
    }
  }
  buttonLastState = buttonReading;

  //programm
  switch (mode){
    case M_OFF:{ } break;

    case M_ST:{ //Startup
      if (isNowPast(endMode, now)){ 
        //begin mode run
#if ENABLE_SERIAL
        Serial.print(now / SECOND);
        Serial.println(" : BEGIN MOD RUN");
#endif
        resOff();
        digitalWrite(PIN_LED_ACCENSIONE, LOW);
        mode = M_NO;
      }

      if (load){
        if (isNowPast(nxLOff, now)){
          loadOff();
          nxLOn = now + TST_LOAD_OFF;
        }
      }else{
        if (isNowPast(nxLOn, now)){
          loadOn();
          nxLOff = now + TST_LOAD_ON;
        }
      }
    } break;

    case M_NO: { //mode run
      if (load){
        if (isNowPast(nxLOff, now))
        {
          loadOff();
          nxLOn = now + TR_LOAD_OFF;
        }
      }else{
        if (isNowPast(nxLOn, now)){
          loadOn();
          nxLOff = now + TR_LOAD_ON;
        }
      }
    } break;

    case M_SH:{
      //begin mode OFF
      if (isNowPast(endMode, now)){ 
        mode = M_OFF;
#if ENABLE_SERIAL
        Serial.print(now / SECOND);
        Serial.println(" : END SHUTDOWN");
#endif
        fumeFanOff();
        airFanOff();
        digitalWrite(PIN_LED_SPEGNIMENTO, LOW);
        digitalWrite(PIN_LED_ON_OFF, LOW);
      }
    } break;

  }
}

void beginStartMode(){
  if (mode != M_ST || mode != M_NO){
#if ENABLE_SERIAL
    Serial.print(now / SECOND);
    Serial.println(" : BEGIN START MODE");       
#endif
    digitalWrite(PIN_LED_ON_OFF, HIGH);
    digitalWrite(PIN_LED_ACCENSIONE, HIGH);
    digitalWrite(PIN_LED_SPEGNIMENTO, LOW);
    mode = M_ST;
    endMode = now + TST_ACCENSIONE;
    fumeFanOn();
    airFanOn();
    resOn();
    loadOn();
    nxLOff = now + TST_LOAD_ON;
  }
}

void beginShutdown(){
  if (mode != M_OFF || mode != M_SH){
#if ENABLE_SERIAL
    Serial.print(now / SECOND);
    Serial.println(" : BEGIN SHUTDOWN");
#endif
    mode = M_SH;
    endMode = now + T_SH;
    loadOff();
    resOff();
    airFanOn();
    fumeFanOn();
    digitalWrite(PIN_LED_SPEGNIMENTO, HIGH);
    digitalWrite(PIN_LED_ACCENSIONE, LOW);
  }
}


void loadOn(){
  digitalWrite(PIN_MOTOR_LOAD, LOW);
  load = true;
#if ENABLE_SERIAL
  Serial.print(now / SECOND);
  Serial.println(" : loadOn");
#endif
}

void loadOff(){
  digitalWrite(PIN_MOTOR_LOAD, HIGH);
  load = false;
#if ENABLE_SERIAL
  Serial.print(now / SECOND);
  Serial.println(" : loadOff");
#endif
}

void resOn(){
  digitalWrite(PIN_RES, LOW);
#if ENABLE_SERIAL
  Serial.print(now / SECOND);
  Serial.println(" : resOn");
#endif
}

void resOff(){
  digitalWrite(PIN_RES, HIGH);
#if ENABLE_SERIAL
  Serial.print(now / SECOND);
  Serial.println(" : resOff");
#endif
}

void fumeFanOn(){
  digitalWrite(PIN_MOTOR_FUME_FAN, HIGH);
#if ENABLE_SERIAL
  Serial.print(now / SECOND);
  Serial.println(" : fumeFanOn");
#endif
}

void fumeFanOff(){
  digitalWrite(PIN_MOTOR_FUME_FAN, LOW);
#if ENABLE_SERIAL
  Serial.print(now / SECOND);
  Serial.println(" : fumeFanOff");
#endif
}

void airFanOn(){
  digitalWrite(PIN_MOTOR_AIR_FAN, HIGH);
#if ENABLE_SERIAL
  Serial.print(now / SECOND);
  Serial.println(" : airFanOn");
#endif
}

void airFanOff(){
  digitalWrite(PIN_MOTOR_AIR_FAN, LOW);
#if ENABLE_SERIAL
  Serial.print(now / SECOND);
  Serial.println(" : airFanOff");
#endif
}

bool isNowPast(unsigned long eventTime, unsigned long now){
  if ((eventTime > MAX_TIME && now < MIN_TIME) || (eventTime < MIN_TIME && now > MAX_TIME)){
    return now < eventTime;
  }else{
    return now >= eventTime;
  }
}
