int up=2;
int down=3;
int left=4;
int right=5;
int stopped=6;
int recover=7;
boolean bt_stopped = LOW;
boolean bt_recover = LOW;
boolean bt_up = LOW;
boolean bt_down = LOW;
boolean bt_left = LOW;
boolean bt_right = LOW;
int times = 0;
int max_times = 5; //周期 = max_times * 20 ms
byte button_cmd[11]  = {0x0a,0x0a,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x0b,0x0b};//按键帧
void setup(void){
  pinMode(stopped,INPUT_PULLUP); 
  pinMode(recover,INPUT_PULLUP); 
  pinMode(up,INPUT_PULLUP); 
  pinMode(down,INPUT_PULLUP); 
  pinMode(left,INPUT_PULLUP); 
  pinMode(right,INPUT_PULLUP); 
  
  //Serial1.begin(115200);
  Serial.begin(9600);
 
}
void loop(void)
{
   bt_stopped = !digitalRead(stopped);
   bt_recover = !digitalRead(recover);
   bt_up = !digitalRead(up);
   bt_down = !digitalRead(down);
   bt_left = !digitalRead(left);
   bt_right = !digitalRead(right);
   
   if(bt_up||bt_down||bt_left||bt_right||bt_stopped||bt_recover)
   {
      button_cmd[3] = bt_up;
      button_cmd[4] = bt_down;
      button_cmd[5] = bt_left;
      button_cmd[6] = bt_right;
      button_cmd[7] = bt_stopped;
      button_cmd[8] = bt_recover;
      Serial.write(&button_cmd[0],11);
      delay(10);
    }
    if(times == max_times)
     {
        times = 0;
     }
    times+=1;
   delay(10);
}

