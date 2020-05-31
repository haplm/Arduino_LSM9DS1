/*Test program for Arduino__LSM9DS1 Library version 2.0 extensions
 * Written by Femme Verbeek Pijnacker the Netherlands 23 may 2020.
 * Run through all the new set and get functions
 */

#include <Arduino_LSM9DS1.h>

void setup() {
   Serial.begin(115200);
   while(!Serial);
   Serial.println();
     if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1); }

   for (int i = 0;i<=3;i++){
      if (IMU.setAccelFS(i))
         printResult ("Accelleration Full Scale range param = ", i ,IMU.getAccelFS()," g ");
      else Serial.println ("failed setting accelleration scale value "); } 

   for (int i = 0;i<=3;i++){
      if (IMU.setGyroFS(i))  
        printResult ("Gyroscope Full Scale range param = ",i,IMU.getGyroFS()," deg/s ");
      else Serial.println ("failed setting gyroscope scale value ");}
 
   for (int i = 0;i<=3;i++){
      if (IMU.setMagnetFS(i))
         printResult ("magnetic range param = ", i ,IMU.getMagnetFS()," µT ");
      else Serial.println ("failed setting magnetic field scale value "); } 
      
   for (int i = 0;i<=7;i++){
      if (IMU.setAccelODR(i))
      {  printResult ("accelleration sample rate param = ", i , IMU.getAccelODR()," Hz ");
       //  printResult ("Gyroscope sample rate param = ", i , IMU.getGyroODR()," Hz ");
         Serial.println(" automatic bandwidth setting (Hz) "+ String (IMU.getAccelBW()));
         for (int j = 0;j<=3;j++)
         {   IMU.setAccelBW(j);    // override automatic bandwith
             printResult("Accel bandwidth override ", j ,IMU.getAccelBW(), "Hz " );
         }
      } 
      else Serial.println ("failed setting accelleration sample rate ");}
 
   for (int i = 0;i<=7;i++){
      if (IMU.setGyroODR(i)){
        printResult ("gyroscope sample rate param = ", i , IMU.getGyroODR(),"  Hz" );
         for (int j = 0;j<=3;j++)
         {   IMU.setGyroBW(j);    // override automatic bandwith
             printResult("Gyro bandwidth setting ", j ,IMU.getGyroBW(), "Hz " );
         }    
      }
      else Serial.println ("failed setting gyroscope sample rate "); }
 
   for (int i = 0;i<=7;i++){
      if (IMU.setMagnetODR(i))
        printResult ("magnetic field sample rate param = ", i , IMU.getMagnetODR()," Hz ");
      else Serial.println ("failed setting magnetic field sample rate ");}      
}

void loop() {

}

void printResult (String msg, int nr,float value, String dimension)
      { Serial.print (msg+String(nr));
      //  Serial.print (nr); 
        Serial.print(" setting "+String(value));
    //    Serial.print(value);
        Serial.println(dimension);}
