/*
 * arduino nano 
 * wangkang02 20170306
 * Wiring Guide:
1,red:VCC
2,black:GND
3,yellow:D2
4,green:GND
5,orange:D4*/
int blueled = 2;//D2
int modesw = 4;//
long sumtime = 0;
int new_value = 0;
bool btdown = false;
bool modeready = false;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(blueled, INPUT);
  pinMode(modesw, OUTPUT);
  digitalWrite(modesw,LOW);
  delay(100);
  Serial.println("initing");
  digitalWrite(modesw,HIGH);
  delay(100);
  Serial.println("inited!");

}

void loop() {
  while(modeready==false)
  {
    digitalWrite(modesw,LOW);
    delay(10);
    digitalWrite(modesw,HIGH);
    delay(100);
    Serial.println("blue on");
    new_value  =  digitalRead(blueled);
    sumtime = 0;
    while(new_value==1)
    {
      sumtime+=1;
      Serial.println(sumtime);
      delay(5);
      new_value  =  digitalRead(blueled);
      }
    Serial.println("blue off");
    if(sumtime < 283 && sumtime > 259){modeready=true;}
    else{Serial.println(sumtime);}

    delay(100);
  }
  
  delay(2000);
}
