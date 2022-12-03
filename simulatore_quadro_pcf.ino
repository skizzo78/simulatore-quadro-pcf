//scritto da Moratelli Denis   aggiornato il 04/12/2022
//sketch per scheda prototipo su base arduino per simulare sensori per prova schede madri quadro


#include <EEPROM.h>
#include <Wire.h>
#include "posizione_magneti.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "PCF8575.h"//pcf
PCF8575 PCF (0x20);//pcf
Adafruit_SSD1306 display(128, 64, &Wire, 4);

#define disp2 display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE)
#define disp3 display.clearDisplay(); display.setTextSize(3); display.setTextColor(WHITE)
#define disp4 display.clearDisplay(); display.setTextSize(4); display.setTextColor(WHITE)

//   uscite su modulo pcf
#define pin_rd 15       // rifasatore discesa
#define pin_id 14       // sensore id - 61u     
#define pin_is 13       // sensore is - 61n     
#define pin_rs 12       // rifasatore salita   
#define pin_iz 11       // zona porte     
#define pin_ds 10       // sensore DS (rallentamento solamente per SEA oleo)    
#define pin_porta 9     // contatto porte cabina 
//#define pin 8     // non utilizzabile
#define pin_fcc 7       //  finecorsa chiusura                     
#define pin_fca 6       //  finecorsa apertura
#define pin_sorve 5     // contatto sorveglianza porte 81/20                      
#define pin_co 2        //  controllo teleruttori porte (elmi)                     
#define pin_sp 1        //  start permit                      



//   ingressi arduino

#define pin_d 2
#define pin_s 3
#define pin_bv  4
#define pin_av 5
#define pin_ch 6
#define pin_ap 7
//pin 8 non connesso
//pin 9 non connesso
//pin 10 non connesso
//pin 11 non connesso
//pin 12 non connesso
//pin 13 non connesso
//pin   A0 non connesso
//pin   A1 non connesso
//pin   A4 seriale I2c SDA
//pin   A5 seriale I2c SCL
//pin   A6 non connesso
#define pin_menu A7    // ingresso analogico pulsanti menu


bool iz = 1 , is = 1 , id = 1 , rs = 1 , rd = 0 , ds = 0 , sp = 1 , fcc = 0 , fca = 1 , co = 1 , ap , ch , s , d , av , bv;

int pos = 0 , vel = 0 , stato = 0 ;

bool attesa = true;           //flag attesa entrata menu
bool first = true;            //flag per lettura eeprom & gestione menu
bool flag_porta_chiusa = true;       //flag per tempo chiusura
bool flag_porta_aperta = true;       //flag per tempo apertura
bool flag_tempo = true;       //flag per avanzamento tempo conteggio vano
int set = EEPROM.read(0);    //   0 epb    1 systemlift - elmi   2 OTIS LB2     3 sea fune    4 sea oleo

unsigned long previousMillis = 0 , millis_porta = 0 , millis_tempo = 0;

///////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  PCF.begin();//pcf
  for (int i = 0 ; i < 15 ; i++) {
    PCF.write(i , 0);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display(); //display.clearDisplay();

  disp2; display.setCursor(0, 0); display.println(set); display.display();

  delay(2000);


  PCF.write(pin_porta, 1);

  pinMode(pin_ap, INPUT_PULLUP);
  pinMode(pin_ch, INPUT_PULLUP);
  pinMode(pin_av, INPUT_PULLUP);
  pinMode(pin_bv, INPUT_PULLUP);
  pinMode(pin_s, INPUT_PULLUP);
  pinMode(pin_d, INPUT_PULLUP);
  pinMode(pin_menu, INPUT);


  if (analogRead(pin_menu) > 250 && analogRead(pin_menu) < 260) {   //leggo pin per entrare nel menu
    stato = 1;                                                      //premere "ok" durante il boot
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  switch (stato) {
    case 0: simulazione(); break;
    case 1: menu_exit(); break;
    case 2: menu_tipo(); break;
    case 21: sub_menu_tipo_epb(); break;
    case 211: sub_menu_tipo_epb_2v(); break;
    case 212: sub_menu_tipo_epb_oleo(); break;
    case 213: sub_menu_tipo_epb_3vf(); break;
    case 22: sub_menu_tipo_elmi(); break;
    case 221: sub_menu_tipo_elmi_2v(); break;
    case 222: sub_menu_tipo_elmi_oleo(); break;
    case 223: sub_menu_tipo_elmi_3vf(); break;
    case 23: sub_menu_tipo_systemlift(); break;
    case 24: sub_menu_tipo_otis_lb2(); break;
    case 25: sub_menu_tipo_sea(); break;
    case 251: sub_menu_tipo_sea_2v(); break;
    case 252: sub_menu_tipo_sea_oleo(); break;
    case 253: sub_menu_tipo_sea_3vf(); break;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void simulazione() {

  if (first == true) {
    set = EEPROM.read(0);
    first = false;
  }

  if (millis() - previousMillis >= 100) { // trasmetto al display ogni 100Ms

    previousMillis = millis();
    disp3; display.setCursor(20, 35); display.println(pos);
    if (!ap) {
      display.setTextSize(2); display.setCursor(80, 0); display.print("AP");
    }
    if (!ch) {
      display.setTextSize(2); display.setCursor(100, 0); display.print("CH");
    }
    if (!s) {
      display.setTextSize(2); display.setCursor(0, 0); display.print("S");
    }
    if (!d) {
      display.setTextSize(2); display.setCursor(20, 0); display.print("D");
    }
    if (!av) {
      display.setTextSize(2); display.setCursor(35, 0); display.print("AV");
    }
    if (!bv) {
      display.setTextSize(2); display.setCursor(60, 0); display.print("BV");
    }
    display.display();
  }

  ap = digitalRead(pin_ap);
  ch = digitalRead(pin_ch);
  s = digitalRead(pin_s);
  d = digitalRead(pin_d);
  av = digitalRead(pin_av);
  bv = digitalRead(pin_bv);

  PCF.write(pin_is , is);
  PCF.write(pin_id , id);
  PCF.write(pin_rs , rs);
  PCF.write(pin_rd , rd);
  PCF.write(pin_sp , sp);
  PCF.write(pin_iz , iz);
  PCF.write(pin_co , co);
  PCF.write(pin_fcc , fcc);
  PCF.write(pin_fca , fca);
  PCF.write(pin_ds , ds);

  co = 1;

  if (ap == 0) {
    PCF.write (pin_porta , 0);    // apro porte
    PCF.write (pin_sorve , 0);    // apro sorveglianza porte
    fcc = 1;      // apro finecorsa chiusura
    co = 0;      //apro controllo teleruttore porte
    flag_porta_chiusa = true;

    if (flag_porta_aperta == true) {
      millis_porta = millis();
      flag_porta_aperta = false;
    }
    if (millis() - millis_porta > 4000) {
      fca = 0;
    }
  }

  //////

  if (ch == 0) {                     //chiudo porte
    co = 0;
    fca = 1;
    flag_porta_aperta = true;
    if (flag_porta_chiusa == true) {
      millis_porta = millis();
      flag_porta_chiusa = false;
    }
    if (millis() - millis_porta > 4000) {    //tempo di chiusura
      PCF.write(pin_porta , 1);    // chiudo contatto porte
      PCF.write (pin_sorve , 1);    // chiudo sorveglianza porte
      fcc = 0;
    }
  }


  if (s == 0 || d == 0) {                                 //quando pin salita o discesa sono attivi

    if (vel >= 40 && av == 0 && flag_tempo == true) {             //regola  velocita alta
      vel = vel -= 20 ;                                           //accelerazione
    }

    if (vel >= 110 && (vel >= 110 && bv == 0) && av == 1 && flag_tempo == true) { //regola bassa velocita
      vel = vel -= 10 ;                                           //accelerazione
    }

    if (vel <= 100 && av == 1  && flag_tempo == true) {            //passo da velocita alta a bassa
      vel = vel += 10 ;                                            //decelerazione
    }
  }


  if (s == 1 && d == 1 ) {
    vel = 300 ;                                                    //velocita partenza
  }

  if (s == 0  && pos < 500 ) {                                     //attivo salita e

    if (flag_tempo == true) {
      millis_tempo = millis();
      flag_tempo = false;
    }
    if (millis() - millis_tempo > vel) {
      flag_tempo = true;
      pos = ++pos;                                                //aumenta posizione
    }
  }

  if (d == 0 && pos > 0 ) {                                       //attivo discesa e
    if (flag_tempo == true) {
      millis_tempo = millis();
      flag_tempo = false;
    }
    if (millis() - millis_tempo > vel) {
      flag_tempo = true;
      pos = --pos;                                                   //diminuisco posizione
    }
  }


  iz = LOW; id = LOW; is = LOW; rd = HIGH; rs = HIGH; sp = HIGH; ds = LOW;

  if (s == 0 || d == 0) {                                            // funzione startpermit
    sp = LOW;
  }

  if (set == 0 || set == 1) {
    posizione_magneti();            //  chiamata funzione per lettura magneti
  }

  if (set == 3 || set == 4) {
    posizione_magneti();            //  chiamata funzione per lettura magneti  SEA FUNE(3)/OLEO(4)
  }


  if (set == 2) {
    posizione_magneti_lb2();
  }


}

//////////////////////////////////////////////////////////////////////////////////////////////

void setStato(int s) {
  stato = s;
  first = true;
  display.clearDisplay();
  delay(300);
}

////////////////////////////////////////////////////////////////////////////////////////////////

void menu_exit() {
  if (attesa) {
    disp2; display.setCursor(0, 10); display.println("ATTENDERE PREGO"); display.display();
    attesa = false;
    delay(4000);
  }
  if (first) {
    disp2;
    display.setCursor(0, 10);
    display.println("EXIT&SAVE");
    display.setCursor(0, 30);    display.println("UP");
    display.setCursor(0, 50);    display.println("DOWN");
    display.display();
    first = false;
  }

  int key = readkey();
  switch (key) {
    case 1:
      EEPROM.update(0, set);
      disp2; display.setCursor(0, 10); display.println("SALVO IN  EEPROM"); display.display();
      delay(4000);
      setStato(0); break;
    case 2: setStato(2); break;
    case 3: setStato(2); break;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////


void menu_tipo() {
  if (first) {
    disp2; display.setCursor(0, 0); display.println("TIPO      QUADRO"); display.display();
    first = false;
    delay(200);
  }
  int key = readkey();
  switch (key) {
    case 1: setStato(21); break;
    case 2: setStato(1); break;
    case 3: setStato(1); break;
  }
}

//////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_epb() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("EPB 2.4"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 0 ; setStato(211); break;
    case 2: setStato(25); break;
    case 3: setStato(22); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_epb_2v() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("2vel A0.02"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 0 ; setStato(2); break;
    case 2: setStato(212); break;
    case 3: setStato(213); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_epb_oleo() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("oleo A0.05"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 0 ; setStato(2); break;
    case 2: setStato(213); break;
    case 3: setStato(211); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_epb_3vf() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("3vf A0.08"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 0 ; setStato(2); break;
    case 2: setStato(211); break;
    case 3: setStato(212); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_elmi() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("ELMI"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 1 ; setStato(221); break;
    case 2: setStato(21); break;
    case 3: setStato(23); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_elmi_2v() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("2 vel"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 1 ; setStato(2); break;
    case 2: setStato(222); break;
    case 3: setStato(223); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_elmi_oleo() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("oleo"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 1 ; setStato(2); break;
    case 2: setStato(223); break;
    case 3: setStato(221); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_elmi_3vf() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("3vf"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 1 ; setStato(2); break;
    case 2: setStato(221); break;
    case 3: setStato(222); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_systemlift() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("SystemLift"); display.display();
    first = false;
    delay(200);
  }


  int key = readkey();
  switch (key) {
    case 1:
      set = 1 ; setStato(2); break;
    case 2: setStato(22); break;
    case 3: setStato(24); break;
  }
}



/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_otis_lb2() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("OTIS LB2"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 2 ; setStato(2); break;
    case 2: setStato(23); break;
    case 3: setStato(25); break;
  }
}




void sub_menu_tipo_sea() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("SEA"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 1 ; setStato(251); break;
    case 2: setStato(24); break;
    case 3: setStato(21); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_sea_2v() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("2 vel"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 3 ; setStato(2); break;
    case 2: setStato(252); break;
    case 3: setStato(253); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_sea_oleo() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("oleo"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 4 ; setStato(2); break;
    case 2: setStato(253); break;
    case 3: setStato(251); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo_sea_3vf() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("3vf"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1:
      set = 3 ; setStato(2); break;
    case 2: setStato(251); break;
    case 3: setStato(252); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////



int readkey() {
  // return   pulsante ok=1    pulsante su = 2    pulsante giu = 3    nessun pulsante = 0 \\

  if (analogRead(pin_menu) > 250 && analogRead(pin_menu) < 260) {
    return 1;
  }
  else if (analogRead(pin_menu) > 500 && analogRead(pin_menu) < 515) {
    return 2;
  }
  else if (analogRead(pin_menu) > 760 && analogRead(pin_menu) < 770) {
    return 3;
  }
  else return 0;
}


////////////////////////////////FINE\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
