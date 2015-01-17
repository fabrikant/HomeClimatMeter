// Подключение библиотек
#include <LCD5110_Graph.h> //http://www.henningkarlsen.com/electronics/library.php?id=47
#include <DS1302.h>//http://www.henningkarlsen.com/electronics/library.php?id=5
#include <Adafruit_BMP085.h>//https://github.com/adafruit/Adafruit-BMP085-Library
#include <IRremote.h>//http://github.com/shirriff/Arduino-IRremote
#include <MenuSystem.h>//https://github.com/jonblack/arduino-menusystem
#include <Time.h> //http://playground.arduino.cc/Code/Time 
#include <TimeLord.h>//http://swfltek.com/arduino/timelord.html
#include "DHT.h"//https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.h
#include <Wire.h>
#include <EEPROM.h>

#define EEPROM_ADR_CONTRAST 1
#define EEPROM_ADR_LIGHT    2

//*******************************************************************************************
//Переменные для автоматического расчета восхода и заката.
const int TIMEZONE = 6; //PST
const float LATITUDE = 54.58, LONGITUDE = 73; // Omsk
TimeLord myLord; // TimeLord Object, Global variable
byte sunTime[]  = {0, 0, 0, 18, 12, 14}; // 18 dec 2014
int minNow, minLast = -1;

//*******************************************************************************************
//Настройка часов
#define DsCePin    10  // Chip Enable RST
#define DsIoPin    11  // Input/Output DAT
#define DsSclkPin  12  // Serial Clock SCLK

DS1302 rtc(DsCePin, DsIoPin, DsSclkPin); // Инициализация библиотеки с указанием пинов

//Время
Time t; // Инициализация структуры 

//*******************************************************************************************
//Настройка экрана
#define LCD_SCK   8 //Serial clock out (SCLK)
#define LCD_MOSI  7 //Serial data out (DIN)
#define LCD_DC    6 //Data/Command select (D/C)
#define LCD_CS    5 //LCD chip select (CS)
#define LCD_RST   4 //LCD reset (RST)

LCD5110 myGLCD(LCD_SCK,LCD_MOSI,LCD_DC,LCD_RST,LCD_CS); 

const int LCD_LIGHT = 3;//подсветка экрана
boolean lcdLightValue;//текущее состояние подсветки. 255 тускло. 0 ярко.
byte lcdContrast;

//Шрифты
extern uint8_t TinyFont[]; // Объявление внешних
extern uint8_t SmallFont[]; // массивов символов
extern uint8_t BigNumbers[];
extern uint8_t MediumNumbers[];

String settingsParametr;//строка в которой будут редактироваться параметры в меню. Например дата и время.
byte settingsCharNum;//Номер мигающего символа при редактировании параметра
boolean settingsCharTransparent;

//Глобальные переменные
int screenIntrerval = 5000;
int screenBegin;
byte currentScreen = 0;

#define CLOCK		1
#define SETTINGS        2
#define SETTIME         3
#define SETDATE         4
#define ABOUT           5
byte screenMode = CLOCK;

//*******************************************************************************************
//Инициализация пульта ДУ
const int RECEIVE_PIN = 9;
IRrecv irrecv(RECEIVE_PIN);
decode_results irButton;

#define IR_POWER  0x1FE48B7
#define IR_MUTE  0x1FE807F
#define IR_PLUS0  0x1FE58A7
#define IR_MINUS0  0x1FE40BF
#define IR_PLUS1  0x1FE7887
#define IR_MINUS1  0x1FEC03F
#define IR_1  0x1FE20DF
#define IR_2  0x1FEA05F
#define IR_3  0x1FE609F
#define IR_4  0x1FEE01F
#define IR_5  0x1FE10EF
#define IR_6  0x1FE906F
#define IR_7  0x1FE50AF
#define IR_8  0x1FED827
#define IR_9  0x1FEF807
#define IR_0  0x1FEB04F
#define IR_ZOOM  0x1FE30CF
#define IR_JUMP  0x1FE708F

//*******************************************************************************************
//Барометр-термометр
Adafruit_BMP085 bmp;

int temperature;
int pressure;
int humidity;

//*******************************************************************************************
//Гигрометр термометр
#define DHTPIN A2     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11 
DHT dht(DHTPIN, DHTTYPE);

//*******************************************************************************************
//Меню настроек
MenuSystem ms;
Menu mm("SETTINGS");
MenuItem mmTime("Time");
MenuItem mmDate("Date");
//MenuItem mmAbout("About");

Menu menuDayOfWeek("DOF");
MenuItem menuDayOfWeek_1("1");
MenuItem menuDayOfWeek_2("2");
MenuItem menuDayOfWeek_3("3");
MenuItem menuDayOfWeek_4("4");
MenuItem menuDayOfWeek_5("5");
MenuItem menuDayOfWeek_6("6");
MenuItem menuDayOfWeek_7("7");

Menu menuAutoLight("Auto light");
MenuItem menuAutoLight_1("on");
MenuItem menuAutoLight_2("off");

#define menuHighLine      9
#define menuCountLine     4
#define menuFirstLine    11
#define menuMaxOnScreen   4


//*******************************************************************************************
void setup()
{
  lcdContrast = EEPROM.read(EEPROM_ADR_CONTRAST);
  lcdLightValue = EEPROM.read(EEPROM_ADR_LIGHT);

  pinMode(LCD_LIGHT, OUTPUT);//Инициализация пина подсветки

  irrecv.enableIRIn();//Старт пульта
  bmp.begin();//Старт температуры барометра
  dht.begin();//Старт температуры гигрометра
  
  //старт часов
  rtc.halt(false); // Установка режимов 
  rtc.writeProtect(false); // работы часов

  myGLCD.InitLCD(lcdContrast); // Старт дисплея

  Serial.begin(9600);//Старт серийного порта
  screenBegin = millis();

  //Создание меню
  mm.add_item(&mmTime, &onTimeSelected);
  mm.add_item(&mmDate, &onDateSelected);
  mm.add_menu(&menuDayOfWeek);
  menuDayOfWeek.add_item(&menuDayOfWeek_1, &onDayOfWeek1Selected);
  menuDayOfWeek.add_item(&menuDayOfWeek_2, &onDayOfWeek2Selected);
  menuDayOfWeek.add_item(&menuDayOfWeek_3, &onDayOfWeek3Selected);
  menuDayOfWeek.add_item(&menuDayOfWeek_4, &onDayOfWeek4Selected);
  menuDayOfWeek.add_item(&menuDayOfWeek_5, &onDayOfWeek5Selected);
  menuDayOfWeek.add_item(&menuDayOfWeek_6, &onDayOfWeek6Selected);
  menuDayOfWeek.add_item(&menuDayOfWeek_7, &onDayOfWeek7Selected);

  mm.add_menu(&menuAutoLight);
  menuAutoLight.add_item(&menuAutoLight_1, &menuAutoLightOn);
  menuAutoLight.add_item(&menuAutoLight_2, &menuAutoLightOff);

  //mm.add_item(&mmAbout, &onAboutSelected);  
  ms.set_root_menu(&mm);

  myLord.TimeZone(TIMEZONE * 60);
  myLord.Position(LATITUDE, LONGITUDE);

  //showAbout();
  //delay(3000);
}

//*******************************************************************************************
void loop()
{

  long irCom = 0;
  if (irrecv.decode(&irButton)) {
    irCom = irButton.value;
    executeCommand(irCom);
    delay(50);
    irrecv.resume();// Receive the next value
  }

  if (screenMode == SETTINGS){//Отображение меню
    showDisplayMenu();
  }
  else if (screenMode == SETTIME|screenMode == SETDATE){//Отображение редактирования настроек

    screenIntrerval = 500;
    String  strScr = settingsParametr;

    myGLCD.clrScr(); // Очистка экрана
    myGLCD.setFont(SmallFont); // Установка набора символов
    int now = millis();
    if (now - screenBegin >= screenIntrerval){
      settingsCharTransparent = true;
      strScr[settingsCharNum] = '_';
    }
    if (now - screenBegin >= screenIntrerval*2){
      settingsCharTransparent = false;
      screenBegin = now;
    }
    myGLCD.print(strScr, CENTER, 20);
    myGLCD.update(); // Вывод вместимого буфера на дисплей
  }
  else if (screenMode == ABOUT){
    showAbout();
  }
  else if (screenMode == CLOCK){//Отображение  основных экранов
    
    t = rtc.getTime(); // Получение времени и его запись в структуру t
    temperature = int(bmp.readTemperature());
    pressure = int(0.00750061683 *bmp.readSealevelPressure());
    humidity = dht.readHumidity();
    
    int now = millis();
    if (now - screenBegin >= screenIntrerval){
      screenBegin = now;
      currentScreen++;
      if (currentScreen > 3)  currentScreen = 0;
    }

    boolean autoLight = EEPROM.read(EEPROM_ADR_LIGHT);
    if (autoLight){
      minNow = t.hour*60+t.min;
      if (minNow != minLast){

        minLast = minNow;
        /* Sunrise: */
        sunTime[3] = t.date; // Uses the Time library to give Timelord the current date
        sunTime[4] = t.mon;
        sunTime[5] = t.year-2000;
        myLord.SunRise(sunTime); // Computes Sun Rise. Prints:
        int mSunrise = sunTime[2] * 60 + sunTime[1];

        /* Sunset: */
        sunTime[3] = t.date; // Uses the Time library to give Timelord the current date
        sunTime[4] = t.mon;
        sunTime[5] = t.year-2000;
        myLord.SunSet(sunTime); // Computes Sun Set. Prints:
        int mSunset = sunTime[2] * 60 + sunTime[1];

        if (minNow >= mSunset || minNow <= mSunrise){
          lcdLightSet(true);
        }
        else{
          lcdLightSet(false);
        };
      }
    }

    if (currentScreen == 0){

      screenIntrerval = 5000;
      myGLCD.clrScr(); // Очистка экрана
      myGLCD.setFont(BigNumbers); // Установка набора символов

      myGLCD.printNumI(int(t.hour), 7, 0, 2, '0'); // Вывод часов
      myGLCD.print("-", 35, 0);
      myGLCD.printNumI(int(t.min), 49, 0, 2, '0'); // Вывод минут
      myGLCD.setFont(SmallFont); // Установка набора символов
      myGLCD.printNumI(int(t.sec), CENTER, 0 ,2, '0' ); // Вывод секунд

      myGLCD.print(rtc.getDOWStr(), CENTER, 30); // Вывод дня недели
      myGLCD.print(rtc.getDateStr(), CENTER, 40); // Вывод даты

      //myGLCD.setFont(TinyFont); // Установка набора символов
      //myGLCD.print("Temp.= "+String(temperature)+"; pres.= "+String(pressure), CENTER, 42);

      myGLCD.update(); // Вывод вместимого буфера на дисплей
    } 
    else if (currentScreen == 1){
      screenIntrerval = 2500;
      myGLCD.clrScr(); // Очистка экрана
      myGLCD.setFont(SmallFont); // Установка набора символов
      myGLCD.print("TEMP, C~ ", CENTER, 0);
      myGLCD.print(strTime(t), CENTER, 40);
      //myGLCD.print(String(t.hour), CENTER, 38);


      myGLCD.setFont(BigNumbers); // Установка набора символов
      myGLCD.printNumI(temperature, CENTER, 10, 2, '0');

      myGLCD.update(); // Вывод вместимого буфера на дисплей
    }
    else if (currentScreen == 2){
      screenIntrerval = 2500;
      myGLCD.clrScr(); // Очистка экрана
      myGLCD.setFont(SmallFont); // Установка набора символов
      myGLCD.print("PRESS, mmHg", CENTER, 0);
      myGLCD.print(strTime(t), CENTER, 40);

      myGLCD.setFont(BigNumbers); // Установка набора символов
      myGLCD.printNumI(pressure, CENTER, 10);
      myGLCD.update(); // Вывод вместимого буфера на дисплей
    }
   else if (currentScreen == 3){
      screenIntrerval = 2500;
      myGLCD.clrScr(); // Очистка экрана
      myGLCD.setFont(SmallFont); // Установка набора символов
      myGLCD.print("Humidity, %", CENTER, 0);
      myGLCD.print(strTime(t), CENTER, 40);

      myGLCD.setFont(BigNumbers); // Установка набора символов
      myGLCD.printNumI(humidity, CENTER, 10);
      myGLCD.update(); // Вывод вместимого буфера на дисплей
    }
  }
}

//*******************************************************************************************
//Обработчики нажатий меню
void on_item1_selected(MenuItem* p_menu_item)
{
  //Serial.println("Item1 Selected");
}

//*******************************************************************************************
void menuAutoLightOn(MenuItem* p_menu_item){
  EEPROM.write(EEPROM_ADR_LIGHT, true);
}

//*******************************************************************************************
void menuAutoLightOff(MenuItem* p_menu_item){
  EEPROM.write(EEPROM_ADR_LIGHT, false);
}

//*******************************************************************************************
void onAboutSelected(MenuItem* p_menu_item){
  showAbout();
  screenMode = ABOUT;
}

//*******************************************************************************************
void onTimeSelected(MenuItem* p_menu_item){
  t = rtc.getTime();
  settingsParametr = strTime(t);
  settingsCharNum = 0;
  screenMode = SETTIME;

}

//*******************************************************************************************
void onDateSelected(MenuItem* p_menu_item){
  settingsParametr = rtc.getDateStr();
  settingsCharNum = 0;
  screenMode = SETDATE;
}

//*******************************************************************************************
void onDayOfWeek1Selected(MenuItem* p_menu_item)
{
  rtc.setDOW(1); 
  showOKScreen();  
}

//*******************************************************************************************
void onDayOfWeek2Selected(MenuItem* p_menu_item)
{
  rtc.setDOW(2);   
  showOKScreen();
}

//*******************************************************************************************
void onDayOfWeek3Selected(MenuItem* p_menu_item)
{
  rtc.setDOW(3); 
  showOKScreen();  
}

//*******************************************************************************************
void onDayOfWeek4Selected(MenuItem* p_menu_item)
{
  rtc.setDOW(4);   
  showOKScreen();
}

//*******************************************************************************************
void onDayOfWeek5Selected(MenuItem* p_menu_item)
{
  rtc.setDOW(5); 
  showOKScreen();  
}

//*******************************************************************************************
void onDayOfWeek6Selected(MenuItem* p_menu_item)
{
  rtc.setDOW(6);   
  showOKScreen();
}

//*******************************************************************************************
void onDayOfWeek7Selected(MenuItem* p_menu_item)
{
  rtc.setDOW(7); 
  showOKScreen();  
}

//*******************************************************************************************
void showDisplayMenu() {

  //menuMaxOnScreen
  int screenLine = 0;
  boolean curMenuIsShow = false;

  String menuItem;
  myGLCD.clrScr(); // Очистка экрана
  myGLCD.setFont(SmallFont); // Установка набора символов


  // Display the menu
  Menu const* cp_menu = ms.get_current_menu();
  myGLCD.print(cp_menu->get_name(), CENTER, 0);

  MenuComponent const* cp_menu_sel = cp_menu->get_selected();
  for (int i = 0; i < cp_menu->get_num_menu_components(); ++i)
  {
    if (screenLine==menuMaxOnScreen) {
      if (curMenuIsShow) {
        break;
      }
      else{
        screenLine = 0;
        myGLCD.clrScr(); // Очистка экрана
        myGLCD.print(cp_menu->get_name(), CENTER, 0);
      }
    }
    MenuComponent const* cp_m_comp = cp_menu->get_menu_component(i);
    if (cp_menu_sel == cp_m_comp){
      menuItem = ">>";
      curMenuIsShow = true;
    }
    else{
      menuItem = "  ";
    }
    menuItem = menuItem + cp_m_comp->get_name();
    myGLCD.print(menuItem, LEFT, menuFirstLine+menuHighLine*screenLine);
    ++screenLine;
  }
  myGLCD.update(); // Вывод вместимого буфера на дисплей

}

//*******************************************************************************************
void executeCommand(long irCom)
{
  if (irCom == IR_POWER){
    if(screenMode == CLOCK) {
      screenMode = SETTINGS;
    } 
    else {
      screenMode = CLOCK;
    }
  }  

  if (screenMode == SETTINGS){//открыто меню
    if (irCom == IR_PLUS0) {
      ms.prev();
      showDisplayMenu();
    }
    else if (irCom == IR_MINUS0) {
      ms.next();
      showDisplayMenu();

    }
    else if (irCom == IR_PLUS1) {
      ms.select();
      showDisplayMenu();

    }
    else if (irCom == IR_MINUS1) {
      ms.back();
      showDisplayMenu();
    }
  } 
  else if (screenMode == SETTIME|screenMode == SETDATE){//открыто окно установки времени
    if (irCom == IR_MINUS1) {
      screenMode = SETTINGS;
    }
    else if (irCom == IR_PLUS1){
      setNewTimeDate();
    }
    else if (irCom == IR_1){
      changeSettingsParametr('1');
    }
    else if (irCom == IR_2){
      changeSettingsParametr('2');
    }
    else if (irCom == IR_3){
      changeSettingsParametr('3');
    }
    else if (irCom == IR_4){
      changeSettingsParametr('4');
    }
    else if (irCom == IR_5){
      changeSettingsParametr('5');
    }
    else if (irCom == IR_6){
      changeSettingsParametr('6');
    }
    else if (irCom == IR_7){
      changeSettingsParametr('7');
    }
    else if (irCom == IR_8){
      changeSettingsParametr('8');
    }
    else if (irCom == IR_9){
      changeSettingsParametr('9');
    }
    else if (irCom == IR_0){
      changeSettingsParametr('0');
    }
  }
  else if (screenMode == ABOUT){
    screenMode = CLOCK;
  }
  else if (screenMode == CLOCK){//открыто основное окно
    if (irCom == IR_PLUS0) {
      if (lcdContrast < 120) lcdContrast++;
      myGLCD.setContrast(lcdContrast);
      EEPROM.write(EEPROM_ADR_CONTRAST, lcdContrast);
    }
    else if (irCom == IR_MINUS0) {
      if (lcdContrast > 0) lcdContrast--;
      myGLCD.setContrast(lcdContrast);
      EEPROM.write(EEPROM_ADR_CONTRAST, lcdContrast);
    }
    else if (irCom == IR_PLUS1) {
      lcdLightSet(true);
    }
    else if (irCom == IR_MINUS1) {
      lcdLightSet(false);
    }
  }
}

//*******************************************************************************************
void setNewTimeDate(){
  if (screenMode == SETTIME){
    rtc.setTime(settingsParametr.substring(0,2).toInt(), settingsParametr.substring(3,5).toInt(), settingsParametr.substring(6).toInt());
  }
  else if (screenMode == SETDATE){
    rtc.setDate(settingsParametr.substring(0,2).toInt(), settingsParametr.substring(3,5).toInt(), settingsParametr.substring(6).toInt());   
  }
  showOKScreen();
}

//*******************************************************************************************
void showOKScreen(){
  myGLCD.clrScr(); // Очистка экрана
  myGLCD.setFont(SmallFont); // Установка набора символов
  myGLCD.print("OK", CENTER, 20);
  myGLCD.update(); // Вывод вместимого буфера на дисплей
  delay(1500);

  screenMode = SETTINGS;  
}

//*******************************************************************************************
String formatTimePart(String par) {
  String str = "0"+String(par);
  return str.substring(str.length()-2, str.length());
}

//*******************************************************************************************
String strTime(Time t){
  return ""+formatTimePart(String(t.hour))+":"+formatTimePart(String(t.min))+":"+formatTimePart(String(t.sec));  
}

//*******************************************************************************************
void lcdLightSet(boolean i){

  if(i){
    digitalWrite(LCD_LIGHT, LOW);    
  }
  else{
    digitalWrite(LCD_LIGHT, HIGH);
  }
}

//*******************************************************************************************
void changeSettingsParametr(char ch){
  settingsParametr[settingsCharNum] = ch;
  if (settingsCharNum == 1 | settingsCharNum == 4) {
    settingsCharNum = settingsCharNum+2;
  }
  else if (settingsCharNum == settingsParametr.length()-1) {
    settingsCharNum = 0;
  }
  else {
    ++settingsCharNum;
  }
}

//*******************************************************************************************
void showAbout(){
/*
  myGLCD.clrScr(); // Очистка экрана
  myGLCD.setFont(SmallFont); // Установка набора символов
  myGLCD.print("Home", CENTER, 0);
  myGLCD.print("Climat", CENTER, 10);
  myGLCD.print("Meter", CENTER, 20);
  myGLCD.print("ver. 1.1", CENTER, 30);
  myGLCD.setFont(TinyFont); // Установка набора символов
  myGLCD.print("Artem Banderov, 2014", RIGHT, 40);
  myGLCD.update(); // Вывод вместимого буфера на дисплей
*/   
}
