// Elektrotehnicki fakultet u Sarajevu
// Master arduino kod projekta za seminarski rad iz predmeta Mehatronika (Bogilovic Adnan)
#include "MPU6050.h"
#include "I2Cdev.h"
#include "math.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

unsigned long tajmer = 0;
float time_step = 0.025;

MPU6050 mpu_prst;
MPU6050 mpu_ruka(0x69);

struct dugme_podaci{
    int pin, stanje, proslo_stanje, vrijeme_snimanja, n_pritisaka;
    unsigned long snimaj_start_time;
} dugme;


struct senzor_podaci{
    float roll, pitch;  // roll i pitch uglovi senzora
} sp_prst, sp_ruka;


struct raw_senzor_podaci{
    int16_t ax, ay, az;  // neobradjeni podaci akcelometra sa senzora MPU6050 (x,y,z)
    int16_t gx, gy, gz;  // neobradjeni podaci ziroskopa sa senzora MPU6050 (x,y,z)
} rsp_prst, rsp_ruka;


void azuriranje_podataka_senzora(MPU6050 *mpu_senzor, struct senzor_podaci *sp, struct raw_senzor_podaci *rsp){
    mpu_senzor->getMotion6(&(rsp->ax), &(rsp->ay), &(rsp->az), &(rsp->gx), &(rsp->gy), &(rsp->gz));
    
    sp->pitch = 0.9*(sp->pitch + rsp->gx*time_step/4098) + 
        0.1*atan2(float(rsp->ay), sqrt(float(rsp->ax)*float(rsp->ax) + float(rsp->az)*float(rsp->az)))*180/M_PI;
    
    sp->roll = 0.9*(sp->roll + rsp->gy*time_step/4098) + 
        0.1*atan2(float(rsp->ax), sqrt(float(rsp->ay)*float(rsp->ay) + float(rsp->az)*float(rsp->az)))*180/M_PI;
}


struct serial_podaci{
    // Maksimalna velicina serial buffera je 64 byte-a!
    // 3*1 = 3 bytes
    uint8_t ugao_servo0, ugao_servo1, ugao_servo2; // uglovi za servo motore 0-180 stepeni
    // 1 byte
    uint8_t servo3_grip_direkcija;
    // 1 byte
    bool servo0_rotira; // servo0 vertikalno osovinski motor za rotaciju cijele ruke
} serial_podaci;


struct akcije{
    bool servo0_rotira = true;      // odabiranje moda rotacija oko osse ili pokretanje dijelova antropomorfne ruke
    uint8_t servo3_grip_direkcija = 0;   // 0 ako ne gripuje (1 za hvatanje, 2 za pustanje)
} akcije;


void azuriranje_serial_podataka(){
    serial_podaci.servo0_rotira = akcije.servo0_rotira;
    serial_podaci.servo3_grip_direkcija = akcije.servo3_grip_direkcija;

    uint8_t ugao;
    if(!akcije.servo0_rotira){
        ugao = uint8_t(sp_ruka.pitch) + 10;
        ugao = 180 - ugao;
        if(ugao > 180) ugao = 180;
        serial_podaci.ugao_servo1 = ugao;
        
        ugao = uint8_t(sp_prst.pitch) + 45;
        ugao = 180 - ugao;
        if(ugao > 180) ugao = 180;
        serial_podaci.ugao_servo2 = 180 - ugao;
    }else{
        ugao = uint8_t(sp_ruka.roll) + 90;
        if(ugao > 180) ugao = 180;
        else if(ugao < 0) ugao = 0;
        serial_podaci.ugao_servo0 = ugao;
    }
}


void azuriranje_gestura_dugmeta(){
    dugme.proslo_stanje = dugme.stanje;
    dugme.stanje = digitalRead(dugme.pin);
    
    if(dugme.stanje == HIGH && dugme.stanje!=dugme.proslo_stanje){
        if(dugme.n_pritisaka == 0){
            dugme.snimaj_start_time = tajmer;
        }
        dugme.n_pritisaka += 1;
    }

    if(tajmer >= dugme.snimaj_start_time + dugme.vrijeme_snimanja){
        if(dugme.n_pritisaka == 1 && dugme.stanje == LOW && dugme.proslo_stanje == LOW){
            akcije.servo0_rotira = !akcije.servo0_rotira;
        }else if(dugme.n_pritisaka == 1 && dugme.stanje == HIGH){
            akcije.servo3_grip_direkcija = 1;
        }else if(dugme.n_pritisaka == 2 && dugme.stanje == HIGH){
            akcije.servo3_grip_direkcija = 2;
        }

        if(dugme.stanje == LOW){
            dugme.n_pritisaka = 0;
            akcije.servo3_grip_direkcija = 0;
        }
    }
}


void slanje_podataka_bluetoothom(){
      //JEDAN PAKET
      if(Serial.availableForWrite()>=11){
          // Poruka kodirana kao paket sa medjubajtovima zbog gubitaka bajtova pri blutut prenosu. Prihvata se samo valjani paket!
          byte paket[11] = {(byte)250, 
                        (byte)serial_podaci.ugao_servo0, 
                        (byte)251, 
                        (byte)serial_podaci.ugao_servo1, 
                        (byte)252, 
                        (byte)serial_podaci.ugao_servo2,
                        (byte)253,
                        (byte)serial_podaci.servo3_grip_direkcija,
                        (byte)254,
                        (byte)serial_podaci.servo0_rotira,
                        (byte)255};
          Serial.write(paket, 11);
      }
}

void setup() {
    Serial.begin(9600);
    
    // pridruziti I2C bus (I2Cdev biblioteka ne radi to automatski)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    mpu_prst.initialize();
    mpu_ruka.initialize();
    
    dugme = {9, LOW, LOW, int(12*time_step*1000), 0, 0};
    pinMode(dugme.pin, INPUT);
}

void loop() {
    tajmer = millis();
    
    azuriranje_podataka_senzora(&mpu_prst, &sp_prst, &rsp_prst);
    azuriranje_podataka_senzora(&mpu_ruka, &sp_ruka, &rsp_ruka);
    azuriranje_gestura_dugmeta();
    azuriranje_serial_podataka();
    slanje_podataka_bluetoothom();
    
    delay(time_step*1000 - (millis()-tajmer));
}
