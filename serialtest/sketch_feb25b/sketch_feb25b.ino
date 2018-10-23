

String comdata = "";

void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
}

void loop() {
  int dir = 0;
  while(Serial.available() >0 )  
  {
   comdata += char(Serial.read());
   delay(1);
  } 
  int len = comdata.length();
   if (len == 6 ||  len > 6)
    {
        Serial.print((unsigned char)comdata[2]+1);
        Serial.print((unsigned char)comdata[3]);
        comdata = "";
    }
}
