int bt_speech=4;//按住说话键  D4
int bt_mode=3;//模式切换键 D3
boolean bt_speech_laststate = HIGH;
boolean bt_speech_currentstate = HIGH;
boolean bt_mode_laststate = HIGH;
boolean bt_mode_currentstate = HIGH;
byte MicID = 0x01;
byte button_cmd[8]={0x0a,0x0a,0x03,0x60,MicID,0x00,0x0b,0x0b};//
byte heart_cmd[9]={0x0a,0x0a,0x03,0x61,MicID,0x00,0x00,0x0b,0x0b};//麦克风ID 0x01
int count = 0;
long readVcc() {//电源电压获取函数，使用内部电压基准NANO为1.1V
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; 
  result = 1.016 * result + 966.997;
  return result;
}
void setup() {
  pinMode(bt_speech,INPUT); //按键需要使用上拉电阻，若改为 pinMode(bt_speech,INPUT_PULLUP); 则无需上拉
  pinMode(bt_mode,INPUT); //
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
        
        if(bt_speech_laststate!=bt_speech_currentstate)//说话键事件
        {
          button_cmd[5] = bt_speech_currentstate + 1;
        }
        else//模式键事件
        {
          button_cmd[5] = bt_mode_currentstate + 3;
        }
        
        Serial.write (&button_cmd[0],8);
        bt_speech_laststate = bt_speech_currentstate;
        bt_mode_laststate = bt_mode_currentstate;
      }
    }
    if(++count == 100) //心跳信息发送，每隔约1s发送一次
    {
      count = 0;
      short vin = readVcc();
      heart_cmd[5] = vin>>8;
      heart_cmd[6] = vin;
      Serial.write (&heart_cmd[0],9);
      }
   delay(10);
}
