int ir0 = A0;
int ir1 = A1;
int ir2 = A2;
int ir3 = A3;
byte ir_data[13]  = {0x0a,0x0a,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0b,0x0b};

void setup() {
  Serial1.begin(115200);
  
}

void loop() {
  int d1 = analogRead(ir0);
  int d2 = analogRead(ir1);
  int d3 = analogRead(ir2);
  int d4 = analogRead(ir3);
  ir_data[3] = highByte(d1);ir_data[4] = lowByte(d1);
  ir_data[5] = highByte(d2);ir_data[6] = lowByte(d2);
  ir_data[7] = highByte(d3);ir_data[8] = lowByte(d3);
  ir_data[9] = highByte(d4);ir_data[10] = lowByte(d4);
  Serial1.write(&ir_data[0],13);
  delay(20);
  /*Serial.println();
  Serial.print(d1);Serial.print('\t');
  Serial.print(d2);Serial.print('\t');
  Serial.print(d3);Serial.print('\t');
  Serial.print(d4);Serial.print('\t');
  Serial.println();*/

}



