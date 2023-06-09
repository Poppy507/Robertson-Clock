#include <DCF77.h>
#include <TimeLib.h>

#define DCF_Pin 2
#define Pend_Pin 6
#define Dial_Pin 7

time_t prevDisplay = 0;          // when the digital clock was displayed
time_t time;
DCF77 DCF = DCF77(DCF_Pin,0);

//defining variables for clock
unsigned long PendCount = 0;
int Pend_State = 0;
int Prev_Pend_State = 0;
int TurnDial = 30;
int HalfMinCount = 0;
signed long TimeDiff = 0;
int DelayState = 0;
int DialTime = 0;
long CatchUp = 0;
bool HaltPend = false;

const long interval = 200; //ms
int driveState = LOW;
int Mode = 0;
unsigned long prevMillis = 0;

void setup() {
  pinMode(DCF_Pin, INPUT);
  pinMode(Pend_Pin, INPUT);
  pinMode(Dial_Pin, OUTPUT);
  DialTime = PendCount/30;

  //Obtained from SyncProvider.pde by Thijs Elenbaas
  Serial.begin(9600);
  DCF.Start();
  setSyncInterval(30);
  setSyncProvider(DCF.getTime);

  //setSyncProvider(DCF.getTime);

  Serial.println("Waiting for DCF77 time ... ");
  Serial.println("It will take at least 2 minutes until a first update can be processed.");
  while (timeStatus()== timeNotSet) { 
     // wait until the time is set by the sync provider     
     Serial.print(".");
     delay(2000);
  }
}

//Code lines 52-219 written by Youngjae Lee
void loop() {
  
    //use hour(),minute(),seconds() to determine total number of secs
  unsigned long GPSTimeSec = hour()* (unsigned long)3600 + minute()*60 + second();
  if(GPSTimeSec > 43199) {
    GPSTimeSec = GPSTimeSec - 43200; //convert to 12hour system
  }

  Pend_State = digitalRead(Pend_Pin);

  if (Prev_Pend_State != Pend_State){
    if (Pend_State == HIGH) { //drive the dial once every 30 pendulum swings (maybe put it in the else section of the if below)
      digitalClockDisplay(); //time check diagnostic
      Serial.print(" sec:");
      Serial.print(GPSTimeSec);//check if the GPS signal conversion is working
      HalfMinCount ++;
      if (HaltPend == false) { //check if clock is waiting for time to catch up i.e. pause the dial if HaltPend = true
        if (PendCount < 43200) { //only displays in 12 hour system
          PendCount ++;
        } else {
          PendCount = 0; //reset dial time to 0 when next day
        }
      }
    
      Serial.print(" pend:");
      Serial.print(PendCount);
      
      Serial.print(" HalfMinCount:");
      Serial.println(HalfMinCount);
      
      TimeDiff = GPSTimeSec - PendCount; //Positive means dial is behind, Negative means dial is ahead
      Serial.println(" --CHECK-- ");
      
      Serial.print(" Discrepancy:");
      Serial.println(TimeDiff);

      if (TimeDiff < 30 && TimeDiff > -30) { //difference is within 30s
        DelayState = 1;
        Serial.print(" within 30s ");
        HaltPend = false;
        TurnDial = 30 - TimeDiff;
        Serial.print(" TurnDial:");
        Serial.println(TurnDial);

        if (HalfMinCount == TurnDial) { //turn dial every 30s normally +-offsets
            HaltPend = false;
            driveDial();
            DialTime ++;
            PendCount = (DialTime * 30) - 1;
            TurnDial = 30;
            HalfMinCount = 0;
          }
        }  
        
      else if (TimeDiff > 0) { //dial is behind
        Serial.println(" dial is behind" );
        if(TimeDiff < 21600) {//21600 for 6 hour max
          Mode = 1;
          DelayState = 2;
          int CatchUp1 = TimeDiff/30;
          int CatchUp2 = CatchUp1/30;
          CatchUp = CatchUp1 + CatchUp2;
          Serial.println(CatchUp);
        }
        else {
          Mode = 2;
          DelayState = 3;
          HaltPend = true;
          CatchUp = (43200 - TimeDiff);
          Serial.print(" New Time Diff:");
          Serial.println(CatchUp);
        }
        int i=0;
        while (i <= CatchUp) {
           Pend_State = digitalRead(Pend_Pin);
           if (Prev_Pend_State != Pend_State){
            if (Pend_State == HIGH) {
              if (Mode == 1) {
                Serial.print(" DIAL ");
                Serial.println(i);
                driveDial();
                DialTime ++;
                PendCount = PendCount + 30;
              }
              else if (Mode == 2) {
                Serial.print(" WAIT ");
                Serial.println(i);
              }
            i ++;
            }
          Prev_Pend_State = Pend_State;
          }
        }
      }

      else if (TimeDiff < 0) { //dial time is ahead /////something wrong with TimeDiff/CatchUp thingy
        Serial.println(" dial is ahead" );
        int m = 0;
        if (TimeDiff > -21600) {
          Mode == 1;
          DelayState = 2;
          int CatchUp1 = TimeDiff/30;
          int CatchUp2 = CatchUp1/30;
          CatchUp = CatchUp1 + CatchUp2;
          Serial.println(CatchUp);
        }
        else {
          Mode = 2;
          DelayState = 3;
          HaltPend = true;
          CatchUp = (43200 + TimeDiff) * -1;
          Serial.print(" New Time Diff:");
          Serial.println(CatchUp);          
        }
        while (CatchUp <= m) {
          Pend_State = digitalRead(Pend_Pin);
           if (Prev_Pend_State != Pend_State){
            if (Pend_State == HIGH) {
              if (Mode == 2) {
                DelayState = 2;
                Serial.print(" DIAL ");
                Serial.println(m);
                driveDial();
                DialTime ++;
                PendCount = PendCount + 30;
              }
              else if (Mode == 1) {
                DelayState = 3;
                Serial.print(" WAIT ");
                Serial.println(m);
              
              }
              m--;
            }
            Prev_Pend_State = Pend_State;
          }
        }          
      }

      if(DelayState != 1) {
        if(HalfMinCount == 30) {
          HalfMinCount = 0;
        }
      }
      Serial.print(" DialTime: ");
      Serial.println(DialTime);
    }
    Prev_Pend_State = Pend_State;
  }
}

void driveDial(){
  //drive the dial
  int ledState = LOW;
  unsigned long prevMillis = 0;
  unsigned long curMillis = millis();
  const long interval = 200;
  
  if (curMillis - prevMillis >= interval) {
    // save the last time you blinked the LED
    prevMillis = curMillis;
    Serial.print(" -Dial pin HIGH- ");
    digitalWrite(Dial_Pin, HIGH);
    delay(200);
    digitalWrite(Dial_Pin, LOW);
  }
}

//Obtained from SyncProvider.pde by Thijs Elenbaas
void digitalClockDisplay(){
  // digital clock display of the time
  Serial.println("");
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
