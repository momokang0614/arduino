#define PT_USE_TIMER
#define PT_USE_SEM
#include "pt.h"
static struct pt thread1,thread2;
int mode_state = 0;
int bt_speech=4;
int bt_mode=3;
int state_led = 13;
boolean bt_speech_laststate = HIGH;
boolean bt_speech_currentstate = HIGH;
boolean bt_mode_laststate = HIGH;
boolean bt_mode_currentstate = HIGH;
byte shuchu[8]={0x0a,0x0a,0x03,0x60,0x00,0x00,0x0b,0x0b};
void setup() {
  PT_INIT(&thread1);
  PT_INIT(&thread2);
  pinMode(bt_speech,INPUT); 
  pinMode(bt_mode,INPUT); 
  pinMode(state_led,OUTPUT); 
  // put your setup code here, to run once:
  Serial.begin(115200);
}

static int thread1_entry(struct pt *pt)
{
  PT_BEGIN(pt);
  while(1)
  {
    bt_speech_currentstate = digitalRead(bt_speech);
    if(bt_speech_laststate!=bt_speech_currentstate)
    {
      delay(20);
      if(bt_speech_laststate!=bt_speech_currentstate)
      {
        if(bt_speech_currentstate==LOW)
        {
          //Serial.print("down mode:");Serial.println(mode_state);
          shuchu[5] = 1;
        }
        else
        {
          //Serial.print("up mode:");Serial.println(mode_state);
          shuchu[5] = 2;
          }
        bt_speech_laststate = bt_speech_currentstate;
         Serial.write (&shuchu[0],8);
        }
      }
    PT_TIMER_DELAY(pt,10);
    }
  PT_END(pt);
}

static int thread2_entry(struct pt *pt)
{
  PT_BEGIN(pt);
  while(1)
  {
    bt_mode_currentstate = digitalRead(bt_mode);
    if(bt_mode_laststate!=bt_mode_currentstate)
    {
      delay(20);
      if(bt_mode_laststate!=bt_mode_currentstate)
      {
        if(bt_mode_currentstate==LOW)
        {
          //Serial.println("mode down");
          mode_state+=1;
          if(mode_state == 4) mode_state = 0;
          shuchu[4] = mode_state;
          delay(400);
          for(int i = 0; i<mode_state+1;i++)
            {
              digitalWrite(state_led,HIGH);delay(200);
              digitalWrite(state_led,LOW);delay(200);
              //Serial.write (&shuchu[0],8);
            }
          }
        
        bt_mode_laststate = bt_mode_currentstate;
          
        }
      }
    PT_TIMER_DELAY(pt,10);
    }
  PT_END(pt);
}
void loop() {

   thread1_entry(&thread1);
   thread2_entry(&thread2);
}
