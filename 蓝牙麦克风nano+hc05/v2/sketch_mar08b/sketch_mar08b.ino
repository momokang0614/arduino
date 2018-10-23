int bt_speech=4;
int bt_mode=3;
boolean bt_speech_laststate = HIGH;
boolean bt_speech_currentstate = HIGH;
boolean bt_mode_laststate = HIGH;
boolean bt_mode_currentstate = HIGH;
byte shuchu[8]={0x0a,0x0a,0x03,0x60,0x00,0x00,0x0b,0x0b};
void setup() {
  pinMode(bt_speech,INPUT); 
  pinMode(bt_mode,INPUT); 
  // put your setup code here, to run once:
  Serial.begin(115200);
}

void loop() {
   bt_speech_currentstate = digitalRead(bt_speech);
   bt_mode_currentstate = digitalRead(bt_mode);
   if(bt_speech_laststate!=bt_speech_currentstate || bt_mode_laststate!=bt_mode_currentstate)
   {
    delay(20);
    if(bt_speech_laststate!=bt_speech_currentstate || bt_mode_laststate!=bt_mode_currentstate)
      {
        if(bt_speech_laststate!=bt_speech_currentstate)
        {
          if(bt_speech_currentstate == LOW){shuchu[4] = 1;}
          else{shuchu[4] = 2;}
        }else{shuchu[4] = 0;}
        if(bt_mode_laststate!=bt_mode_currentstate)
        {
          if(bt_mode_currentstate == LOW){shuchu[5] = 1;}
          else{shuchu[5] = 2;}
          }else{shuchu[5] = 0;}
        
        
      
        Serial.write (&shuchu[0],8);
        bt_speech_laststate = bt_speech_currentstate;
        bt_mode_laststate = bt_mode_currentstate;
      }
   
    }
   
   delay(10);
  //byte shuchu[5]={0x0a,0x0a,byte(128),0x0b,0x0b};
  //Serial.write (&shuchu[0],5);
  //delay(500);
}
