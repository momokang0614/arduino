#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"

MPU6050 accelgyro;
const int BTD2 = 2;
const int BTD3 = 3;
int BTD2_ST = 1;
int BTD3_ST = 1;
unsigned long now, lastTime = 0;
float dt;                                  

int16_t ax, ay, az, gx, gy, gz;
float aax=0, aay=0,aaz=0, agx=0, agy=0, agz=0;
long axo = 0, ayo = 0, azo = 0;
long gxo = 0, gyo = 0, gzo = 0;

float pi = 3.1415926;
float AcceRatio = 16384.0;           
float GyroRatio = 131.0;               

uint8_t n_sample = 8;                  
float aaxs[8] = {0}, aays[8] = {0}, aazs[8] = {0};      
long aax_sum, aay_sum,aaz_sum;           

float a_x[10]={0}, a_y[10]={0},a_z[10]={0} ,g_x[10]={0} ,g_y[10]={0},g_z[10]={0}; 
float Px=1, Rx, Kx, Sx, Vx, Qx;      
float Py=1, Ry, Ky, Sy, Vy, Qy;            
float Pz=1, Rz, Kz, Sz, Vz, Qz;           

int Dirx=0,Diry=0;
void setup()
{
    Wire.begin();
    Serial.begin(115200);
    accelgyro.initialize();   
    unsigned short times = 200;          
    for(int i=0;i<times;i++)
    {
        accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); 
        axo += ax; ayo += ay; azo += az;     
        gxo += gx; gyo += gy; gzo += gz;
    
    }
    
    axo /= times; ayo /= times; azo /= times; 
    gxo /= times; gyo /= times; gzo /= times; 
}

void loop()
{
    unsigned long now = millis();            
    dt = (now - lastTime) / 1000.0;    
    lastTime = now;                        
    //delay(100);
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float accx = ax / AcceRatio;             
    float accy = ay / AcceRatio;          
    float accz = az / AcceRatio;             

    aax = atan(accy / accz) * (-180) / pi; 
    aay = atan(accx / accz) * 180 / pi;      
    aaz = atan(accz / accy) * 180 / pi;    

    aax_sum = 0;                           
    aay_sum = 0;
    aaz_sum = 0;
  
    for(int i=1;i<n_sample;i++)
    {
        aaxs[i-1] = aaxs[i];
        aax_sum += aaxs[i] * i;
        aays[i-1] = aays[i];
        aay_sum += aays[i] * i;
        aazs[i-1] = aazs[i];
        aaz_sum += aazs[i] * i;
    
    }
    
    aaxs[n_sample-1] = aax;
    aax_sum += aax * n_sample;
    aax = (aax_sum / (11*n_sample/2.0)) * 9 / 7.0;
    aays[n_sample-1] = aay;                        
    aay_sum += aay * n_sample;              
    aay = (aay_sum / (11*n_sample/2.0)) * 9 / 7.0;
    aazs[n_sample-1] = aaz; 
    aaz_sum += aaz * n_sample;
    aaz = (aaz_sum / (11*n_sample/2.0)) * 9 / 7.0;

    float gyrox = - (gx-gxo) / GyroRatio * dt; 
    float gyroy = - (gy-gyo) / GyroRatio * dt; 
    float gyroz = - (gz-gzo) / GyroRatio * dt; 
    agx += gyrox;                          
    agy += gyroy;                           
    agz += gyroz;
    
    /* kalman start */
    Sx = 0; Rx = 0;
    Sy = 0; Ry = 0;
    Sz = 0; Rz = 0;
    
    for(int i=1;i<10;i++)
    {                
        a_x[i-1] = a_x[i];                   
        Sx += a_x[i];
        a_y[i-1] = a_y[i];
        Sy += a_y[i];
        a_z[i-1] = a_z[i];
        Sz += a_z[i];
    
    }
    
    a_x[9] = aax;
    Sx += aax;
    Sx /= 10;                                 
    a_y[9] = aay;
    Sy += aay;
    Sy /= 10;                               
    a_z[9] = aaz;
    Sz += aaz;
    Sz /= 10;

    for(int i=0;i<10;i++)
    {
        Rx += sq(a_x[i] - Sx);
        Ry += sq(a_y[i] - Sy);
        Rz += sq(a_z[i] - Sz);
    
    }
    
    Rx = Rx / 9;                             
    Ry = Ry / 9;                        
    Rz = Rz / 9;
  
    Px = Px + 0.0025;                         
    Kx = Px / (Px + Rx);                      
    agx = agx + Kx * (aax - agx);            
    Px = (1 - Kx) * Px;                      

    Py = Py + 0.0025;
    Ky = Py / (Py + Ry);
    agy = agy + Ky * (aay - agy); 
    Py = (1 - Ky) * Py;
  
    Pz = Pz + 0.0025;
    Kz = Pz / (Pz + Rz);
    agz = agz + Kz * (aaz - agz); 
    Pz = (1 - Kz) * Pz;
    
    typedef struct sdata
    {
      char  h1;
      char  h2;
      char len;
      int x;
      int y;
      int w;
      char t1;
      char t2;
    }sdata;
    sdata data= {0xaa, 0xaa, 0x06,int(agx),int(agy),1,0xbb,0xbb};
    byte b[sizeof(sdata)];
    memcpy(b,&data,sizeof(sdata));
    //Serial.write(b,sizeof(sdata));
    Serial.print("aax:\t");Serial.print(aax);Serial.print("\t");
    Serial.print("aay:\t");Serial.print(aay);Serial.print("\t");
    Serial.print("aaz:\t");Serial.print(aaz);Serial.print("\t");
    /*uint8_t data_to_send[8];
    data_to_send[0]=0xAA;
    data_to_send[1]=0xAA;
    data_to_send[2]=0x03;
    data_to_send[3]=char(int(agx));
    data_to_send[4]=char(int(agy));
    data_to_send[5]=0xFF;
    data_to_send[6]=0xBB;
    data_to_send[7]=0xBB;
    Serial.write(data_to_send,8);*/
    
    /*if(btd2_st==0 && btd3_st==1){    Serial.print(dirx);Serial.print("\t");Serial.print(diry);Serial.print("\t"); Serial.println(btd3_st^1);}
    else if(btd2_st==1 && btd3_st!=BTD3_ST){BTD3_ST = btd3_st;Serial.print(0);Serial.print("\t");Serial.print(0);Serial.print("\t"); Serial.println(btd3_st^1);}*/
    
    /*if( BTD2_ST==0  && btd2_st==0 && btd3_st==1)
    {
      if(Dirx!=dirx || Diry!=diry)
      {
        Dirx=dirx; 
        Diry=diry;
        Serial.print(dirx);Serial.print("\t");Serial.print(diry);Serial.print("\t"); Serial.println(btd3_st^1);
      }
    }
    else if(BTD2_ST==0  && btd2_st==1){ Serial.print(0);Serial.print("\t");Serial.print(0);Serial.print("\t"); Serial.println(btd3_st^1);}
    else if( BTD2_ST==1  && btd2_st==1 && btd3_st!=BTD3_ST){Serial.print(0);Serial.print("\t");Serial.print(0);Serial.print("\t"); Serial.println(btd3_st^1);}
    BTD2_ST = btd2_st;
    BTD3_ST = btd3_st;*/
 // Serial.print(BTD2_ST);Serial.print("\t");Serial.print(btd2_st);
     
     
    /*Serial.print(agx);Serial.print("\t");
    Serial.print(agy);Serial.print("\t");
    Serial.print(agz);
    */
    //Serial.println(accz);
    
}


