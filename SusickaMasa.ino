// Arduino LCD 16x2 Shield
#include "HX711.h"

#define VAHYCELKEM 2

int vahyArray[VAHYCELKEM][5] = {
  ///pSCK, pDT, slope, offset,  location
//{ 46, 47, 1, 00, 1 },
//{ 48, 49, 1, 00, 2 },
{ 50, 51, 1, 00, 3 },
{ 52, 53, 1, 00, 4 }
};


byte LB[8] =                    
{            
  B11000,           
  B11000,           
  B11000,           
  B11000,           
  B00000,           
  B00000,           
  B00000,           
  B00000,           
};

byte LF[8] =                    
{            
  B00011,           
  B00011,           
  B00011,           
  B00011,           
  B00000,           
  B00000,           
  B00000,           
  B00000,           
};

byte RB[8] =                    
{            
  B00000,           
  B00000,           
  B00000,           
  B00000,
  B11000,           
  B11000,           
  B11000,                      
  B11000,                      
};

byte RF[8] =                    
{            
  B00000,           
  B00000,           
  B00000,           
  B00000,
  B00011,           
  B00011,           
  B00011,           
  B00011,                      
};
#define DEFAULTPERCENT 60;

typedef struct _maso 
{
    int ID;
    int pSCK;
    int pDT;
    long offset;
    long scale;
    bool enable;
    long weight;
    long startWeight;
    int perc;
    int curPerc;
    int location;
    HX711 vaha;
} maso;

// připojení knihovny pro LCD
#include <LiquidCrystal.h>
// inicializace LCD displeje
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
// nastavení čísla propojovacího pinu
// pro osvětlení LCD displeje
#define lcdSvit 10

maso masoStruct[VAHYCELKEM];

int vahyInUse = 0;
int prevVahyInUse=0;

String TISKRADEK[2*VAHYCELKEM];

void setup()
{
    Serial.begin(9600);
    Serial.println("Inicializace LCD" );
    initDisplay();
    createPosChars();
    Serial.println("LCD inicializovano" );
    
    Serial.println("Inicializace vahovych portu" );
    for ( int i = 0; i < VAHYCELKEM; i++) {
        initPortuVahy( i, vahyArray[i][0], vahyArray[i][1], vahyArray[i][2], vahyArray[i][3], vahyArray[i][4]);
    }
    Serial.println("Vahove porty inicializovany" );  
    
    Serial.println("Pre-calibrace");
    preCalibrateRun();
    Serial.println("Pre-calibrace done");
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Dej maso na vahy");
    lcd.setCursor(0,1);
    lcd.print("a stiskni SELECT");
    
    while ( nactiTlacitka() != 5 ) {
        delay(100);
    }

    Serial.println("Priprava jednotlivych senzoru");
    for ( int i = 0; i < VAHYCELKEM; i++) {
        initVahy(i);
        prevVahyInUse = vahyInUse;
    }
    Serial.println("Senzory inicializovany");
    
    String UsedIDs;
    for ( int i = 0; i < VAHYCELKEM; i++) {
        if ( masoStruct[i].enable == true ) {
            UsedIDs+=String(i)+",";
        }
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Obsazene vahy: "+UsedIDs);
    delay(2000);
}


void loop() {   
    decideMenuScreen(nactiTlacitka());
    updateVahy();
    drawScreen();
}

void initVahy(int i) {
    Serial.println("Priprava vahy ID:" + String(i));
    //long hmotnost=masoStruct[i].vaha.get_units(10);
    long hmotnost=spusteniMereni(i);
    Serial.println("Hmotnost: " + String(hmotnost));
    noveMaso ( i, hmotnost );
    
    if ( masoStruct[i].enable == true ) {
      Serial.println("ID: " + String(masoStruct[i].ID) + ", enable: " + String(masoStruct[i].enable) + ", Vaha: " + String(masoStruct[i].weight) + ", Startovaci vaha: " + String(masoStruct[i].startWeight) + ", Cilova vaha: " + String(masoStruct[i].startWeight * masoStruct[vahyInUse].perc/100));
      vahyInUse++;
      } 
    else {
       Serial.println("Na vaze ID " + String( i ) + " neni zadne maso...");
    };
}

void initPortuVahy(int ID, int pSCK, int pDT, long offset, long scale, int location ) {
    Serial.println("ID: " + String(ID) + ", pSCK: " + String(pSCK) + ", pDT: " + String(pDT) + + ", scale: " + String(scale) + ", offset: " + String(offset) + ", location:" + String(location));
    masoStruct[ID].pSCK = pSCK;
    masoStruct[ID].pDT = pDT;
    masoStruct[ID].offset = offset;
    masoStruct[ID].scale = scale;
    masoStruct[ID].location = location;
    
    masoStruct[ID].vaha.begin(pDT, pSCK);
    masoStruct[ID].vaha.set_scale(offset);
    masoStruct[ID].vaha.tare();
    masoStruct[ID].vaha.power_down();
}


void initDisplay() {
    lcd.begin(16, 2);
    pinMode(lcdSvit, OUTPUT);
    digitalWrite(lcdSvit, HIGH);
}

int nactiTlacitka() {
    int retval;
    int analog = analogRead(0);
    if (analog < 50) retval = 6;
    if ((analog > 700) && (analog < 1024)) retval = 0;
    if ( (analog > 95) && (analog < 150) ) retval = 8;
    if ( (analog > 250) && (analog < 350) ) retval = 2;
    if ( (analog > 400) && (analog < 500) ) retval = 4;
    if ( (analog > 600) && (analog < 750) ) retval = 5;
    return retval;
}

long spusteniMereni( int ID ){
    long w1 = 0;
    // read until stable
    masoStruct[ID].vaha.power_up();
    w1 = masoStruct[ID].vaha.get_units(10);
    masoStruct[ID].vaha.power_down();
    Serial.println("Vaha: " + String(w1));
    return w1;
}

void noveMaso ( int ID,  int weight ) {
    masoStruct[ID].enable = false;
    masoStruct[ID].ID = ID;
    masoStruct[ID].perc = DEFAULTPERCENT;
    masoStruct[ID].curPerc = 100;
    masoStruct[ID].weight = weight;
    masoStruct[ID].startWeight = weight;
    if ( weight > 500 ) {
        masoStruct[ID].enable = true;
    }
};

void updateVahy() {
    for ( int i = 0; i < VAHYCELKEM; i++) {
        if ( masoStruct[i].enable == true ){
            masoStruct[i].weight = spusteniMereni(i);
            masoStruct[i].curPerc = masoStruct[i].weight*100/masoStruct[i].startWeight;
            Serial.println("Vaha: " + String(masoStruct[i].weight));
        }
    }
}

void calibrate(int ID){
    masoStruct[ID].vaha.set_scale();
    //masoStruct[ID].vaha.tare();

    long b = masoStruct[ID].vaha.read_average();
    Serial.println("Read average: "+ String(b));
    Serial.println("Dej na vahu ID" + String(ID) + "1kg a stiskni SELECT");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("1kg na vahu ID:" + String(ID));
    lcd.setCursor(0,0);
    lcd.print("a stiskni SELECT");
    
     while ( nactiTlacitka() != 5 ) {
        delay(100);
    }
        
    long x = masoStruct[ID].vaha.read_average();
    long y = 1000;
    double m = (double)(y-b)/x;

    Serial.println("b: " + String(b));
    Serial.println("x: " + String(x));
    Serial.println("y: " + String(y));
    Serial.println("m: " + String(m));
    Serial.println(" --------- ");
    Serial.println("scale[" + String(ID) + "].set_offset=" + String(b));
    Serial.println(" --------- ");
    Serial.println("scale["+ String(ID) + "].set_scale="+String(m));
    Serial.println(" --------- ");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("offset=" + String(b) +" || " + String(masoStruct[ID].offset));
    lcd.setCursor(0,1);
    lcd.print("scale=" + String(m))  +" || " + String(masoStruct[ID].scale);
    
    while ( nactiTlacitka() != 5 ) {
        delay(100);
    }
}

void preCalibrateRun() {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("OK pro kalibrace");
    for (int i = 0; i < VAHYCELKEM; i++) {
        lcd.setCursor(0,1);
        lcd.print("vahy ID "+ String(i));
        int a=0;
        while ( a < 100 ) {
            if ( nactiTlacitka() == 5 ) {
                calibrate( i );
                break;
            }
            delay(10);
            a++;
        }
    }
}

void decideMenuScreen( int klavesa ) {
    
    if ( klavesa != 5  ) {
        return;
    }

    String mainMenu[4][2] = { 
        {"Kalibrace vahy"," "},
        {"Nastavit start"," "},
        {"Nastavit cil"," "},
        {"Re-init vahy"," "},
    };
    int choise;    
    choise=drawMenuItems( 4, mainMenu );
    switch (choise) {
    case 0:
        //kalibrace
        break;
    case 1:
        //nastavit start
        break;
    case 2:
        //nastavit cil
        break;
    case 3:
        //reinit vahy
        break;
    }
    return;
}

void drawScreen() {
    for (int i = 0; i < VAHYCELKEM; i++) {
        if ( masoStruct[i].enable == false ) { continue; }
        // considering enabled weights only
        lcd.clear();
        lcd.setCursor(0,0);
        if (  masoStruct[i].curPerc < masoStruct[i].perc ) {
            lcd.print("ID="+String(i)+ "; - DONE -" );
        }
        else {
            lcd.print("ID="+String(i)+";Cil:"+masoStruct[i].perc+"%");
        }
        lcd.setCursor(0,1);

        lcd.print(String(masoStruct[i].weight)+"g|"+String(masoStruct[i].curPerc)+"%");
        
        lcd.setCursor(15,0);
        lcd.write(byte(masoStruct[i].location));              

        delay(2000);
    }
}

void createPosChars() {
    lcd.createChar(1, LB);
    lcd.createChar(2, LF);
    lcd.createChar(3, RB);
    lcd.createChar(4, RF);
}

int drawMenuItems( int totalItems, String menuItems[][2]  ) {
    bool reDraw=false;    
    int menuID=0;

    while ( true ) {
        int tlacitko=nactiTlacitka();
        if ( tlacitko == 5 ) { 
            return menuID; 
        }
        if ( tlacitko == 8 ) { 
            ++menuID;
            if ( menuID == totalItems ) { menuID = 0; }
            reDraw=true;
        }
        if ( tlacitko == 2 ) { 
            --menuID;
            if ( menuID == -1 ) { menuID = totalItems-1; }
            reDraw=true;
        }
        
        if ( reDraw == true ) {
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(String(menuItems[menuID][0]));
            lcd.setCursor(0,1);
            lcd.print(String(menuItems[menuID][1]));
        }

        reDraw=false;
        delay(100);
    }
} 

int updateNumber( int znak, int radek, int startNumber, int maxNum) {
    bool reDraw = true;
    int value=startNumber;
    int rad=1;
    
    while ( true ) {
        int tlacitko = nactiTlacitka();
        
        if ( tlacitko == 5 ) { return value; }
        else if ( tlacitko == 6 ) { rad*=10; }
        else if ( tlacitko == 4 ) {
            rad/=10;
            if ( rad < 1 ) { rad = 1; }
        }
        else 
        {
            if ( tlacitko == 8 ) {
                value += rad; 
                if ( maxNum != 0 && maxNum < value ) { value = maxNum; }
                reDraw = true;
            }
            if ( tlacitko == 2 ) {
                value -= rad;
                if ( value < 0 ) { value = 0 ;}
                reDraw=true; 
            }
            if ( reDraw == true ) {
                lcd.setCursor(znak,radek);
                lcd.print(String(value));
            }
        }
        
        delay(500);
    }
}







