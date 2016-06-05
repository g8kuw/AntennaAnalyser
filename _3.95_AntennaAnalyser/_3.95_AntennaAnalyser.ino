#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library - Edited this file for width & height 480 x 320
//#include <TFTLCD.h> 
#include <TouchScreen.h>     // Add touch screen to 3.95" LCD

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define CH_A A6 //MEGA2560 analog pins for Channel A and B
#define CH_B A7

#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin

#define TS_MINX 150 // Set limits to the touch screen area
#define TS_MINY 175 
#define TS_MAXX 920
#define TS_MAXY 940

// For my 3.95" TFT, its 234 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 234);

#define tftx 480   // Width of 3.95" TFT in landscape mode (3) - origin at Top Left Corner
#define tfty 320   // Height of 3.95" TFT in landscape mode - origin at Top Left Corner
#define gridox 100 // X px Coord of Top Left of Grid
#define gridoy 10  // Y px Coord of Top Left of Grid
#define gridx 360  // Grid width in px
#define gridy 280  // Grid height in px
#define gratx 36   // Graticule width
#define graty 28   // Graticule height

// Colour defs for the 3.95" TFT    Red  Green  Blue
#define WHITE           0xFFFF // 11111 111111 11111
#define YELLOW          0xFFE0 // 11111 111111 00000
#define CYAN            0x07FF // 00000 111111 11111
#define GREEN           0x07E0 // 00000 111111 00000
#define	MAGENTA         0xF81F // 11111 000000 11111
#define	RED             0xF800 // 11111 000000 00000
#define	BLUE            0x001F // 00000 000000 11111
#define	BLACK           0x0000 // 00000 000000 00000
#define GREY            0XE71C // 11100 111100 11100
#define DARK            0X31CC // 01100 011100 01100

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// Define text string arrays for X and Y axis
char* freq20m[]={"14.0","14.1","14.2","14.3","14.4","14.5","14.6","14.7","14.8","14.9"};
char* freq30m[]={"10.0","10.1","10.2","10.3","10.4","10.5","10.6","10.7","10.8","10.9"};
char* swr[]={"inf","10:1","9:1","8:1","7:1","6:1","5:1","4:1","3:1","2:1","1:1"};

//Plot a bath curve for a test
float testswr[]={10, 9, 8, 7, 6, 5.1, 4.2, 3.3, 2.5, 2,2, 2.5, 3.3, 4.2, 5.1, 6, 7, 8, 9, 10}; 

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
//TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
void setup(void) { 
 // Set up analog inputs on Ch A and Ch B, internal reference voltage 
  pinMode(CH_A,INPUT);
  pinMode(CH_B,INPUT);
  analogReference(DEFAULT); //MEGA only
    // initialize serial communication at 57600 baud for debug
  Serial.begin(57600);
  Serial.print("Starting:");
  tft.reset();
  tft.begin(0x9341); //9341 chip set in the 3.95" TFT
  //tft.initDisplay();
  tft.fillScreen(BLACK); //CLS
  tft.setRotation(3); //Landscape view mode
  pinMode(13, OUTPUT);
  
  drawgrid(); // Draw a blue grid
  drawyaxis();
  drawx20maxis();
  writetext();
  
  button(10, 80,  " 20m");
  button(10, 140, " 30m");
  button(10, 200, "Scan");
  button(10, 260, "Clear");
}

void loop()
{
  digitalWrite(13, HIGH);
  Point p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // we have some minimum pressure, consider it a finger press
  // pressure of 0 means no finger!

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    // scale from 0->1023 to tft.width
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);
    p.y = map(p.y, TS_MINY, TS_MAXY, tft.width(), 0);
    
    p.x = 320 - p.x; //Invert the y axis for the 3.95" TFT landscape mode
    //Serial.print("Y = "); Serial.print(p.x);
    //Serial.print("\tX = "); Serial.print(p.y);
    //Serial.print("\tPressure = "); Serial.println(p.z);
  }

  // Check for button presses in a loop
  if ((p.y > 10 && p.y < 40) && ((p.x >10 && p.x < 50))){ //logo pressed?
    sinewave();
  } 
  
  if ((p.y > 10 && p.y < 40) && ((p.x >80 && p.x < 130))){ //20m button pressed?
    tft.fillRect( 100, 300, 355, 20, BLACK);
    drawx20maxis();
  }
  
  if ((p.y > 10 && p.y < 40) && ((p.x >140 && p.x < 190))){ //30m button pressed?
    tft.fillRect( 100, 300, 355, 20, BLACK);
    drawx30maxis();
  }

  if ((p.y > 10 && p.y < 40) && ((p.x >200 && p.x < 250))){ //Scan button pressed?
    //plotswr(); //Plot the scanned values
    plotChannelA();
  }

  if ((p.y > 10 && p.y < 40) && ((p.x >260 && p.x < 310))){ //Clear button pressed?
    tft.fillRect( gridox - 2, gridoy, gridx + 2, gridy, BLACK); //CLS
    drawgrid();
  }

}

//Draw the buttons at bx by
void button(int bx, int by, String btext){ 
  tft.fillRect( bx, by, 54,34, DARK); // Face
  tft.fillRect( bx, by, 50,30, GREY); // Shadow
  tft.fillRect (bx, by, 50, 4, WHITE); // Top Highlight
  tft.fillRect (bx, by, 4, 28, WHITE); // Side Highlight
  tft.setCursor(bx+10, by+12); // Button Text
  tft.setTextColor(BLACK);
  tft.print(btext);   
}

//Plot the scan from the SWR array
void plotswr() {
  int i, plotx, ploty = 0;
  int prevx = gridox;
  int prevy = gridoy;
  
  for (i= 0; i < 19; i++) { //20 plots per band scan
    plotx = gridox + (i*18); //Graticule width is 36 pix so div by 2
     ploty = 318 - (testswr[i] * graty);  //Graticule height is 28 pix so invert y scale -318 (318 = gridy + gridoy + graticule height)
      tft.fillCircle(plotx, ploty, 2, RED); // Plot a red blob at the SWR coords    
      tft.drawLine (prevx, prevy, plotx, ploty, GREEN); //Join the dots
     prevx = plotx;
    prevy = ploty; 
  } 
 tft.drawLine (prevx, prevy, gridox + gridx, gridoy, GREEN); //Final line 
}

void plotChannelA() {
  double REV;
  int i;
  for (i = gridox; i < (gridox + gridx); i++) {
    REV = analogRead(CH_A);
    tft.fillCircle(i, (gridoy + REV), 2, RED); // Plot a red blob at the SWR coords
    Serial.println(int(REV));
    Serial.flush();
  } 
}

//display a pseudo sinewave waveform to test
void sinewave(){ 
  int i = 0; //horizontal index
  int n = 6; //horizontal multiplier
  int vert = 0;
  int pix;
  const float pi = 3.14159;
  float stepS = 0;
  float sinS[tftx];
  stepS = (2*pi) / tftx;
  
  for (i = gridox; i <= gridx + gridox; i++){
    sinS[i] = sin(i*stepS*n);
    vert = (sinS[i] * 50) + (gridy/2 + gridoy); 
    tft.drawPixel(i,vert,GREEN);
  }
}

//Draw Grid
void drawgrid(){
  for (int gx = gridox; gx <= gridox + gridx; gx +=gridx/10){ //10 Vertical blue lines of the X axis
    tft.drawFastVLine(gx, gridoy, gridy, BLUE); // x , y, height, colour
    //tft.drawVerticalLine(gx, gridoy, gridy, BLUE); // x , y, height, colour
  }
  for (int gy = gridoy; gy <= gridoy + gridy; gy +=gridy/10){ //10 Horizontal blue lines of the Y axis
    tft.drawFastHLine(gridox, gy, gridx, BLUE); // x , y, width, colour
    //tft.drawHorizontalLine(gridox, gy, gridx, BLUE); // x , y, width, colour
  } 
}

// Draw the Y SWR axis
void drawyaxis(){
  int i=0;
  tft.setCursor(55, 10); //Left Side line
  tft.setTextColor(MAGENTA);
  tft.println("SWR"); //SWR label
  
  for (int ya = 10; ya <= gridx; ya +=gridy/10){
    tft.setCursor(75, ya); //Left Side line
    tft.setTextColor(YELLOW);
    tft.println(swr[i]);
    i++;  
  } 
}

void drawx20maxis(){
  int i=0;
  for (int xa = gridox; xa <= (gridox + gridx)-1; xa +=gridx/10){
    tft.setCursor(xa, 300); //Bottom line
    tft.setTextColor(GREEN);
    tft.print(freq20m[i]); 
    i++;  
  } 
}

void drawx30maxis(){
  int i=0;
  for (int xa = gridox; xa <= (gridox + gridx)-1; xa +=gridx/10){
    tft.setCursor(xa, 300); //Bottom line
    tft.setTextColor(GREEN);
    tft.print(freq30m[i]); 
    i++;  
  } 
}

// Print Logo 
void writetext(){
  tft.setCursor((gridx + gridox)-1, gridy + 20); //Bottom line    
  tft.setTextColor(MAGENTA);
  tft.println("MHz");
  
  tft.setCursor(15, 20); //Top line    
  tft.setTextColor(BLUE);
  tft.println("G8KUW"); 
 
  tft.setCursor(15, 30); //Second line    
  tft.setTextColor(BLUE);
  tft.println("Antenna"); 

  tft.setCursor(15, 40); //Third line    
  tft.setTextColor(BLUE);
  tft.println("Analyser"); 
}
