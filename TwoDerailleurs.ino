/* 2016/8/21
 *  Control electrical bicycle shifting device
 *  Front derailleur control: servo motor
 *  Rear derailleur control: stepper motor
 */
#include <EEPROM.h>
#include <SoftwareSerial.h>

//SoftwareSerial getRPM(11, 12);//RX:11 , TX:12, communication with second UNO board
int frontD;
int rearD;
volatile boolean send_flag = false;
volatile unsigned long PF_prev_time = 0;
volatile unsigned long PF_time_escape = 0;
volatile int countPF = 0;
void setup() {
// reset EEPROM info; store chain position
//    EEPROM.write(1, 9);
//    EEPROM.write(0, 0);
//5~8 for stepper
  for (int i = 5; i <= 8; i++) {
    pinMode(i, OUTPUT);
  }
// address 0 store front chain position, 1 store rear chain position
  frontD = EEPROM.read(0);
  rearD = EEPROM.read(1);
  // phase-correct PWM mode, prescaler 1024, TCNT 156, OCR2B 7~14
  // 4.5% duty cycle to 9% duty cycle
  pinMode(3, OUTPUT); // servo signal 
  // timer generating PWM setting
  TCCR2A |= _BV(COM2A0) | _BV(WGM20) | _BV(COM2B1);
  TCCR2B |= _BV(WGM22) | _BV(CS22) | _BV(CS21) | _BV(CS20);
  OCR2A = 156;
  if (frontD == 0)
    OCR2B = 6; // small chainring
  else
    OCR2B = 14; // large chainring
// attach pedal rate(cadence) interrupt
  attachInterrupt(digitalPinToInterrupt(2), measure_PF, FALLING);
  TIMSK1 &= ~(_BV(TOIE1)); // disable timer1 overflow interrupt
  TIMSK1 &= ~(_BV(OCIE1A) | _BV(OCIE1B)); // disable output compare match interrupt
  TCCR1A &= ~(_BV(WGM11) | _BV(WGM10));// normal mode
  TCCR1B &= ~(_BV(WGM12) | _BV(WGM13));
  TCCR1B |= (_BV(CS12) | _BV(CS10)); // clk prescaler 1024
  TCCR1B &= ~(_BV(CS11));
  TIMSK1 |= _BV(TOIE1); // enable timer1 overflow interrupt
  TCNT1 = 32767;
// get button input
  pinMode(4, INPUT); // button control front derailleur
  pinMode(9, INPUT); // button control rear derailleur
  pinMode(10, INPUT); // button control rear derailleur
  //control stepper motor relay module
  pinMode(13, OUTPUT);
  //default UART bluetooth speed
  Serial.begin(38400);
// bit rate communicate with second board
//  getRPM.begin(38400);
  pinMode(2, OUTPUT); // servo relay
  pinMode(12, OUTPUT); // solenoid relay
}
// stepper motor phase delay
int delayT = 6;
//(L298n input1) ->pin 8  arduino UNO
//(L298n input2) ->pin 7 arduino UNO
//(L298n input3) ->pin 6 arduino UNO
//(L298n input4) ->pin 5 arduino UNO
//(L298n output1) ->black A
//(L298n output2) ->green A'
//(L298n output3) ->red B
//(L298n output4) ->blue B'
int looptime = 65;// 10 gears, 9 gaps; gap = 600/9 ≈ 67
// looptime = 50 equal to 50 *4 steps equal to 1 circle

void loop() {
    if(send_flag){ // timer time's up, send PF(cadence) to smart phone
        PF_time_escape /= countPF; // get average pedal time
        countPF = 0;
        int PF = 60000 / PF_time_escape; // get RPM of cadence
        PF_time_escape = 0;
        String PF_string = String(PF); // convert int to string
        String P = "P"; // header of pedal rate(cadence)
        String temp = "";
        if(PF > 99){ // padding PF info to 4 digit. e.g. P374
            temp = P + PF_string;
        }
        else if(PF <= 99 && PF > 9){ // e.g. P@65
            temp = P + "@" + PF_string;
        }
        else{ // e.g. P@@3
            temp = P + "@@" + PF_string;
        }
        Serial.print(temp);
        send_flag = false;
    }
    if(digitalRead(4) == HIGH){
//        Serial.println(digitalRead(4));
        delay(50);
        if(digitalRead(4) == LOW){
            if(frontD == 1){
                frontD = 0;
                EEPROM.write(0, 0); // write EEPROM
                digitalWrite(12, HIGH);
                delay(20);
                digitalWrite(2, HIGH);
                OCR2B = 14;
                delay(500);
                OCR2B = 6;
                delay(1000);
                digitalWrite(12, LOW);
                digitalWrite(2, LOW);
                Serial.write("f@@0"); // inform smart phone
            }
            else{
                frontD = 1;
                EEPROM.write(0, 1); // write EEPROM
                digitalWrite(12, HIGH);
                delay(20);
                digitalWrite(2, HIGH);
                OCR2B = 14;
                delay(1500);
                digitalWrite(12, LOW);
                delay(500);
                digitalWrite(2, LOW);
                Serial.write("f@@1"); // inform smart phone
            }
        }
    }
    if(digitalRead(9) == HIGH){
        delay(50);
        if(digitalRead(9) == LOW){
            if(rearD == 10);
            else{
                digitalWrite(13, HIGH);
                ccw();
                rearD++;
                String num;
                if (rearD == 10) {
                    num = "r@10";
                }
                else {
                    num = "r@@" + String(rearD);
                }
                EEPROM.write(1, rearD);
                digitalWrite(13, LOW);
                Serial.print(num);// ack message
            }
        }
    }
    if(digitalRead(10) == HIGH){
        delay(50);
        if(digitalRead(10) == LOW){
            if(rearD == 1);
            else{
                digitalWrite(13, HIGH);
                cw();
                rearD--;
                String num = "r@@" + String(rearD);
                EEPROM.write(1, rearD);
                digitalWrite(13, LOW);
                Serial.print(num);
            }
        }
    }
  // get second board's information
//  if (getRPM.available()) {
//    String b = getRPM.readString();
//    Serial.print(b);
//  }

  // get bluetooth information
//  if (Serial.available()) {
//    char buf[3] = {Serial.read(), '#', '#'};
//    delayMicroseconds(300);// wait > speed of uart communication
//    buf[1] = Serial.read();
//    delayMicroseconds(300);
//    buf[2] = Serial.read();
//    //        Serial.println(buf);
//    if (strncmp(buf, "tup", 3) == 0) {
//      looptime = 5;
//      digitalWrite(13, HIGH);
//      ccw();
//      digitalWrite(13, LOW);
//      Serial.print("t@up");
//      looptime = 65;
//    }
//    else if (strncmp(buf, "tdn", 3) == 0) {
//      looptime = 5;
//      digitalWrite(13, HIGH);
//      cw();
//      digitalWrite(13, LOW);
//      Serial.print("t@dn");
//      looptime = 65;
//    }
//    else if (strncmp(buf, "0up", 3) == 0) {
//      if (frontD == 1);
//      else {
//        frontD = 1;
//        EEPROM.write(0, 1);
//        digitalWrite(12, HIGH);
//        delay(20);
//        digitalWrite(2, HIGH);
//        OCR2B = 14;
//        delay(1500);
//        digitalWrite(12, LOW);
//        delay(100);
//        digitalWrite(2, LOW);
//        Serial.print("f@@1");// ack message
//      }
//    }
//    else if (strncmp(buf, "0dn", 3) == 0) {
//      if (frontD == 1) {
//        frontD = 0;
//        EEPROM.write(0, 0);
//        digitalWrite(12, HIGH);
//        delay(20);
//        digitalWrite(2, HIGH);
//        OCR2B = 14;
//        delay(500);
//        OCR2B = 6;
//        delay(1000);
//        digitalWrite(12, LOW);
//        digitalWrite(2, LOW);
//        Serial.print("f@@0");// ack message
//      }
//      else;
//    }
//    else if (strncmp(buf, "1up", 3) == 0) {
//
//      if (rearD == 10);
//      else {
//        digitalWrite(13, HIGH);
//        ccw();
//        rearD++;
//        String num;
//        if (rearD == 10) {
//          num = "r@10";
//        }
//        else {
//          num = "r@@" + String(rearD);
//        }
//        EEPROM.write(1, rearD);
//        digitalWrite(13, LOW);
//        Serial.print(num);// ack message
//      }
//    }
//    else if (strncmp(buf, "1dn", 3) == 0) {
//      if (rearD == 1);
//      else {
//        digitalWrite(13, HIGH);
//        cw();
//        rearD--;
//        String num = "r@@" + String(rearD);
//        EEPROM.write(1, rearD);
//        digitalWrite(13, LOW);
//        Serial.print(num);// ack message
//      }
//    }
//  }
}

void ccw() {
  for (int i = 0; i < looptime; i++) {
    digitalWrite(5, LOW);
    digitalWrite(6, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    delay(delayT);
    digitalWrite(5, LOW);
    digitalWrite(6, HIGH);
    digitalWrite(7, HIGH);
    digitalWrite(8, LOW);
    delay(delayT);
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    digitalWrite(7, HIGH);
    digitalWrite(8, LOW);
    delay(delayT);
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    delay(delayT);
  }
}

void cw() {
  for (int i = 0; i < looptime; i++) {
    digitalWrite(5, LOW);
    digitalWrite(6, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    delay(delayT);
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    delay(delayT);
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    digitalWrite(7, HIGH);
    digitalWrite(8, LOW);
    delay(delayT);
    digitalWrite(5, LOW);
    digitalWrite(6, HIGH);
    digitalWrite(7, HIGH);
    digitalWrite(8, LOW);
    delay(delayT);
  }
}

void measure_PF(){
    /* get time difference between last pedal and present
       pedal. And sum the time difference    */
    PF_time_escape += millis() - PF_prev_time;
    PF_prev_time = millis(); // get present time
    countPF++; // pedal count + 1
}

ISR(TIMER1_OVF_vect){
//    send_flag = true;
    TCNT1 = 32767;
}
