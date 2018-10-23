int SW1 = 7;
void setup() {
  Serial.begin(115200);
  pinMode(SW1,OUTPUT);
 
  digitalWrite(SW1, LOW);
  delay(5000);
  digitalWrite(SW1,HIGH);
  delay(1000);
  digitalWrite(SW1, LOW);
  
}
void loop() {
}
