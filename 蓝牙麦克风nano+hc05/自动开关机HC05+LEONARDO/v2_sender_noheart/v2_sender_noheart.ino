int bt_speech=4;//按住说话按键 D4
int bt_mode=5;//模式切换按钮D5
int power = 3;//开关控制 D3
int charge = 18;//充电事件读取 A0

boolean bt_speech_laststate = HIGH;
boolean bt_speech_currentstate = HIGH;
boolean bt_mode_laststate = HIGH;
boolean bt_mode_currentstate = HIGH;

int bt_charge_currentvalue = 0;
int bt_charge_lastvalue = 0;
int charge_break_confirm = 0;


byte MicID = 0x01;
int sender_id = 0;

byte button_cmd[16]  = {0x0a,0x0a,0x03,0x60,sender_id,0x00,0x0b,0x0b,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//
byte heart_cmd[16]   = {0x0a,0x0a,0x03,0x61,sender_id,0x00,0x00,0x0b,0x0b,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//
int count = 0;
byte i = 0;

void setup(void){
  pinMode(charge,INPUT); 
  pinMode(power,OUTPUT); 
  pinMode(bt_speech,INPUT_PULLUP); 
  pinMode(bt_mode,INPUT_PULLUP); 
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop(void)
{
  
   bt_speech_currentstate = digitalRead(bt_speech);
   bt_mode_currentstate = digitalRead(bt_mode);
   bt_charge_currentvalue = analogRead(charge);
  
   if(bt_charge_currentvalue > 600 )
   {
     charge_break_confirm = 0;
    }
   else if(bt_charge_currentvalue < 400)
   {
     if(charge_break_confirm < 20)
     {
      charge_break_confirm += 1;
      if(charge_break_confirm == 20)
      {
        Serial.println("disconnect");
        digitalWrite(power,HIGH);
        delay(50);
        digitalWrite(power,LOW);
        
        }
      }
    }
    bt_charge_lastvalue = bt_charge_currentvalue;
   
   
   if(bt_speech_currentstate == LOW || bt_mode_currentstate == LOW)
   {
    if(bt_speech_currentstate == LOW)//
    {
      
      button_cmd[5] = bt_speech_currentstate + 1;
      if(count == 0)
      {
        //radio.write(&button_cmd,16);
        Serial1.write(&button_cmd[0],16);
        
      }
    }
    else if(bt_mode_currentstate == LOW && bt_mode_laststate == HIGH)//
    {
      button_cmd[5] = bt_mode_currentstate + 3;
      //radio.write(&button_cmd,16);
      Serial1.write(&button_cmd[0],16);
    }
    
    
    }
    bt_mode_laststate = bt_mode_currentstate;
    count+=1;
    if(count==10) count =0;
   delay(10);
}


