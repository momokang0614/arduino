//blue led + :D2
//power switch black : D4
//power switch red :GND
int blueled = 2;//D2
int modesw = 4;//
long sumtime = 0;
int new_value = 0;
int old_value = 0;
bool btdown = false;
bool modeready = false;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(blueled, INPUT);
  pinMode(modesw, OUTPUT);
  /*digitalWrite(modesw,HIGH);  



    digitalWrite(modesw,LOW);  
    delay(5);
    digitalWrite(modesw,HIGH);  
    delay(1000);*/
    
    
    digitalWrite(modesw,LOW);
    delay(100);
    Serial.println("initing");
    digitalWrite(modesw,HIGH);
    delay(100);
    Serial.println("init");
}

void loop() {

  //digitalWrite(modesw,HIGH);
  // put your main code here, to run repeatedly:
  while(modeready==false)
  {
    digitalWrite(modesw,LOW);
    delay(10);
    digitalWrite(modesw,HIGH);
    Serial.println("blue on");
    new_value  =  digitalRead(blueled);
    sumtime = 0;
    while(new_value==1)
    {sumtime+=1;Serial.println(sumtime);delay(5);new_value  =  digitalRead(blueled);}
    Serial.println("blue off");
    if(sumtime < 110 && sumtime > 100){modeready=true;}

    delay(1000);
  }
  delay(1000);
  Serial.println("ready");
  
  /*new_value  =  digitalRead(blueled);
  if(old_value != new_value )
  {
     if((old_value == 0)&&(new_value == 1))
    {
      Serial.println("down");
      btdown = true;
    }
    if((old_value == 1)&&(new_value == 0))
    {
        Serial.println("up");
        Serial.println(sumtime);
        btdown = false;
        sumtime=0;
    }
  }
    if(new_value == 1 && btdown)
    {
      sumtime+=1;
      delay(10);
    }
  old_value = new_value;*/
  //delay(10);
}
