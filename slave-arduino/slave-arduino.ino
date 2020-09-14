// Elektrotehnicki fakultet u Sarajevu
// Slave arduino kod projekta za seminarski rad iz predmeta Mehatronika (Bogilovic Adnan)
#include "Servo.h"

Servo servo0, servo1, servo2, servo3;
int pin_servo0 = 9, pin_servo1 = 10, pin_servo2 = 11, pin_servo3 = 5;
uint8_t ugao_servo0 = 90, ugao_servo1 = 90, ugao_servo2 = 90, ugao_servo3 = 180;
uint8_t servo3_grip_direkcija = 0;
bool servo0_rotira = true;


uint8_t pomjeri_servo(Servo* servo,uint8_t ugao_servo, uint8_t prosli_ugao){
    if(prosli_ugao!=ugao_servo) servo->write(ugao_servo);
    return ugao_servo;
}

uint8_t pomjeri_griper(uint8_t smijer){
    if(smijer == 1){
        ugao_servo3 += 1;
        if(ugao_servo3 > 180) ugao_servo3 = 180;
        else servo3.write(ugao_servo3);
    }else if(smijer == 2){
        ugao_servo3 -= 1;
        if(ugao_servo3 < 95) ugao_servo3 = 95;
        else servo3.write(ugao_servo3);
    }
    return ugao_servo3;
}

void setup(){
    Serial.begin(9600);
    
    servo0.attach(pin_servo0); servo1.attach(pin_servo1);
    servo2.attach(pin_servo2); servo3.attach(pin_servo3);
    
    servo0.write(ugao_servo0); servo1.write(ugao_servo1);
    servo2.write(ugao_servo2); servo3.write(ugao_servo3);
}

void loop(){
    if(Serial.available()>10){
        if((uint8_t)Serial.read() == 250){
            uint8_t buff[10];
            Serial.readBytes(buff, 10);
            // ako je dosao validan paket
            if(buff[1] == 251 && buff[3] == 252 && buff[5] == 253 && buff[7] == 254 && buff[9] == 255){
                // ako smo u mode-u da servo0 rotira (tj ako se cijela antropomorfna ruka okrece po z osi)
                // kada se prelazi iz jednog u drugi mode, mora se vratiti u okolinu prethodnog stanja skupine motora tog moda iz kojeg se izaslo (abs)
                if((bool)buff[8]){
                    if(!servo0_rotira && abs(buff[0]-ugao_servo0) <= 8){
                        ugao_servo0 = pomjeri_servo(&servo0, buff[0], ugao_servo0);
                        servo0_rotira = true;
                    }else if(servo0_rotira) ugao_servo0 = pomjeri_servo(&servo0, buff[0], ugao_servo0);
                }else{
                    if(servo0_rotira && abs(buff[2]-ugao_servo1) <= 8 && abs(buff[4]-ugao_servo2) <= 8){
                        ugao_servo1 = pomjeri_servo(&servo1, buff[2], ugao_servo1);
                        ugao_servo2 = pomjeri_servo(&servo2, buff[4], ugao_servo2);
                        servo0_rotira = false;
                    }else if(!servo0_rotira){
                        ugao_servo1 = pomjeri_servo(&servo1, buff[2], ugao_servo1);
                        ugao_servo2 = pomjeri_servo(&servo2, buff[4], ugao_servo2);
                    }
                }
                ugao_servo3 = pomjeri_griper(buff[6]);
                delay(15);
            }
        }
    }
}
