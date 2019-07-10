// Arduino HWMonitor - numeric info and the graph
// (C)2019 Pawel A. Hernik
// YouTube Video: https://youtu.be/_XbgCrL8Fp4

// N5110 LCD pinout from left:
// #1 RST      - Pin  9 or any digital
// #2 CS/CE    - Pin 10 or any digital
// #3 DC       - Pin  8 or any digital
// #4 MOSI/DIN - Pin 11 / SPI
// #5 SCK/CLK  - Pin 13 / SPI
// #6 VCC 3.3V
// #7 LIGHT (200ohm to GND)
// #8 GND

/*
 Use PC HardwareSerialMonitor from:
  https://cdn.hackaday.io/files/19018813666112/HardwareSerialMonitor_v1.1.zip
*/

// comment out for load graph
#define CLOCK_GRAPH

#include "N5110_SPI.h"
// define USESPI in above header for HW SPI version

N5110_SPI lcd(9,10,8); // RST,CS,DC

#if USESPI==1
#include <SPI.h>
#endif

// from PropFonts library
#include "c64enh_font.h"
#include "small5x7bold_font.h"

String inputString = "";
char buf[30];
String cpuLoadString;
String cpuTempString;
String cpuClockString;
String ramString;
int cpuLoad=0;
int cpuClock=0;
int inp=0;

void setup() 
{
  Serial.begin(9600);
  inputString.reserve(200);
  
  lcd.init();
  lcd.clrScr();
  lcd.setFont(c64enh);
  lcd.printStr(ALIGN_CENTER, 2, "Connecting ...");
}

int readSerial() 
{
  while (Serial.available()) {
    char ch = (char)Serial.read();
    inputString += ch;
    if(ch == '|') {  // full info chunk received
      int st,en;
      st = inputString.indexOf("CHC");  // CPU clock: "CHC1768"
      if(st>=0) {
        en = inputString.indexOf("|", st);
        cpuClockString = inputString.substring(st+3, en);
        cpuClock = cpuClockString.toInt();
        inp=3;
      } else {

        st = inputString.indexOf("R");  // used RAM: "R6.9"
        if(st>=0) {
          en = inputString.indexOf("|", st);
          ramString = inputString.substring(st+1 , en-1);
          st = ramString.indexOf(",");
          if(st>=0) ramString.setCharAt(st,'.');
          inp=2;
        }

        int cpuTempStart = inputString.indexOf("C"); // CPU temperature: "C52"
        int cpuLoadStart = inputString.indexOf("c"); // CPU load: "c18%"
        if(cpuLoadStart>=0 && cpuTempStart>=0) {
          en = inputString.indexOf("|");
          cpuTempString = inputString.substring(cpuTempStart+1, cpuLoadStart);
          cpuLoadString = inputString.substring(cpuLoadStart+1, en-1);
          cpuLoad = cpuLoadString.toInt();
          inp=1;
        }
      }
      inputString = "";
      return 1;
    }
  }
  return 0;
}

#define MIN_CLOCK 400
#define MAX_CLOCK 2900
#define NUM_VAL (21)
int valTab[NUM_VAL];
int i,ght=16;
int x=0;

void addVal()
{
  for(i=0;i<NUM_VAL-1;i++) valTab[i]=valTab[i+1];
#ifdef CLOCK_GRAPH
  if(cpuClock<400) cpuClock=400;
  if(cpuClock>2900) cpuClock=2900;
  valTab[NUM_VAL-1] = (long)(cpuClock-MIN_CLOCK)*ght/(MAX_CLOCK-MIN_CLOCK);
#else
  valTab[NUM_VAL-1] = cpuLoad*ght/100;
#endif
}

// --------------------------------------------------------------------------
byte scr[84*2];  // frame buffer
byte scrWd = 84;
byte scrHt = 2;

void clrBuf()
{
  for(int i=0;i<scrWd*scrHt;i++) scr[i]=0;
}

void drawPixel(int16_t x, int16_t y, uint16_t color) 
{
  if((x < 0) || (x >= scrWd) || (y < 0) || (y >= scrHt*8)) return;
  switch (color) {
    case 1: scr[x+(y/8)*scrWd] |=  (1 << (y&7)); break;
    case 0: scr[x+(y/8)*scrWd] &= ~(1 << (y&7)); break; 
    case 2: scr[x+(y/8)*scrWd] ^=  (1 << (y&7)); break; 
  }
}

void drawLineH(uint8_t x0, uint8_t x1, uint8_t y, uint8_t step)
{
  if(step>1) { if(((x0&1)==1 && (y&1)==0) || ((x0&1)==0 && (y&1)==1)) x0++; }
  if(x1>x0) for(uint8_t x=x0; x<=x1; x+=step) drawPixel(x,y,1);
  else      for(uint8_t x=x1; x<=x0; x+=step) drawPixel(x,y,1);
}

void drawLineV(int x, int y0, int y1, int step)
{
  if(step>1) { if(((x&1)==1 && (y0&1)==0) || ((x&1)==0 && (y0&1)==1)) y0++; }
  if(y1>y0)for(int y=y0; y<=y1; y+=step) drawPixel(x,y,1);
  else     for(int y=y1; y<=y0; y+=step) drawPixel(x,y,1);
}

// --------------------------------------------------------------------------

void drawGraphBar()
{
  drawLineH(0,83,0,2);
  drawLineH(0,83,15,2);
  drawLineV(0,0,15,2);
  drawLineV(83,0,15,2);
  for(i=0;i<NUM_VAL;i++) {
    drawLineH(i*4,(i+1)*4-1,15-valTab[i],1);
    drawLineV(i*4+0,15-valTab[i],15,2);
    drawLineV(i*4+1,15-valTab[i],15,2);
    drawLineV(i*4+2,15-valTab[i],15,2);
    drawLineV(i*4+3,15-valTab[i],15,2);
    if(i>0) drawLineV(i*4,15-valTab[i-1],15-valTab[i],1);
  }
}

void loop() 
{
  //readSerial();
  if(readSerial()) 
  {
    int xs=38;
    lcd.setFont(c64enh);
    lcd.printStr(0, 0, "Temp: ");
    x=lcd.printStr(xs, 0, (char*)cpuTempString.c_str());
    lcd.printStr(x, 0, "'C  ");
    lcd.printStr(0, 1, "Load: ");
    snprintf(buf,20,"%d %",cpuLoad);
    x=lcd.printStr(xs, 1, buf);
    lcd.printStr(x, 1, "%  ");
    lcd.printStr(0, 3, "RAM: ");
    x=lcd.printStr(xs, 3, (char*)ramString.c_str());
    lcd.printStr(x, 3, " GB  ");
    lcd.printStr(0, 2, "Clock: ");
    x=lcd.printStr(xs, 2, (char*)cpuClockString.c_str());
    lcd.setFont(Small5x7PLBold);
    lcd.printStr(x, 2, " MHz    ");
#ifdef CLOCK_GRAPH
    i = 2;
#else
    i = 1;
#endif
    lcd.fillWin(83,i,1,1,B00111110);
    lcd.fillWin(82,i,1,1,B00011100);
    lcd.fillWin(81,i,1,1,B00001000);

    if(inp==3) addVal();
    clrBuf();
    drawGraphBar();
    lcd.drawBuf(scr,0,4,scrWd,scrHt);
  }
  if(inp>=3) { delay(1000); inp=0; }
}

