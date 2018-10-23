int bt_speech=3;
int bt_mode=4;
boolean bt_speech_laststate = HIGH;
boolean bt_speech_currentstate = HIGH;
boolean bt_mode_laststate = HIGH;
boolean bt_mode_currentstate = HIGH;
byte shuchu[6]={0x0a,0x0a,0x00,0x00,0x0b,0x0b};
void setup() {
  pinMode(bt_speech,INPUT_PULLUP); 
  pinMode(bt_mode,INPUT_PULLUP); 
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop() {
   bt_speech_currentstate = digitalRead(bt_speech);
   bt_mode_currentstate = digitalRead(bt_mode);
   if(bt_speech_laststate!=bt_speech_currentstate || bt_mode_laststate!=bt_mode_currentstate)
   {
    delay(20);
    if(bt_speech_laststate!=bt_speech_currentstate || bt_mode_laststate!=bt_mode_currentstate)
      {
        shuchu[2] = bt_speech_currentstate;
        shuchu[3] = bt_mode_currentstate;
        Serial.write (&shuchu[0],6);
        Serial1.write (&shuchu[0],6);
        bt_speech_laststate = bt_speech_currentstate;
        bt_mode_laststate = bt_mode_currentstate;
      }
   
    }
  
   delay(10);
}
