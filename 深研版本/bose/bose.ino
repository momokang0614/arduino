const int power = 3;
const int vol_up = 4;
const int bt = 5;
void setup() {
  // put your setup code here, to run once:
  //Serial.begin(115200);
  pinMode(power, OUTPUT);
  pinMode(vol_up, OUTPUT);
  pinMode(bt, OUTPUT);
  
  //Serial.println("Starting power on !!");
  delay(1000);
  //Serial.println("Power key down ~");
  digitalWrite(power,HIGH);
  delay(500);
  //Serial.println("Power key up ~");
  digitalWrite(power,LOW);
  //Serial.println("Power on finished!");
  delay(2000);
  //Serial.println("Disable auto turnoff !!");
  for(int i = 0;i < 10; i++)
  {
    //Serial.println("bt&vol+  key down !!");
    //Serial.println(i);
    digitalWrite(vol_up,HIGH);
    digitalWrite(bt,HIGH);
    delay(500);
    }
  //Serial.println("bt&vol+  key up !!");
  digitalWrite(vol_up,LOW);
  digitalWrite(bt,LOW);
  delay(500);
  
  //Serial.println("Setting volume !!");
  for(int i = 0;i < 100; i++)
  {
    //Serial.println("Volume+ key down !!");
    //Serial.println(i);
    digitalWrite(vol_up,HIGH);
    delay(200);
    digitalWrite(vol_up,LOW);
    delay(200);
    }
  //Serial.println("Volume+ key up !!");
  digitalWrite(vol_up,LOW);
  delay(500);
  
  
}
void loop() {
 
 
}

