#include <DFRobot_LCD.h>
#include <Wire.h>

/*
 * Program to control the Bogus Finger Apparatus
 * This device can be controlled with the on-board controls or through remote serial commands
 * On boards commands have higher priority
 * 
 */


const int number_func = 5;


long X=0;
long NX=0;
int MoveSpeed=600; //microseconds between steps
String     inputString = ""; // a string to hold incoming data
boolean stringComplete = true;     // whether the string is completet
boolean        ComData = false;    // whether com data is on when motors are moving will slow them down
float forceLimit = 2.0; // force in Newton to stop
boolean applyForce = false;

long maxLowPos = 110000;
long maxHighPos = 0;
  
long loadCellValue = 0;  
int offset = 17;   //16 PJR 3axis, 18 b&k 1 axes
float N = 0.00980665; // 1 gr newton conversion
float sumLc = 0;
float current_force = 0;
float output = 0;
float lc =0;
int cntprint = 0;
int btn_pwr_status = LOW;
int btn_down_status= LOW;
int btn_up_status  = LOW;
int btn_stop_status= LOW;
boolean generalStop = true;

DFRobot_LCD lcd(16,2);
int currentLCDFunction = 0; 
String lcdStringR0 = "";
String lcdStringR1 = "";
int timeRefresh = 500;
long lastRefresh = 0;
boolean functionChanged = true;
boolean standby = true;

# define X_EN_5v  53 //ENA+(+5V) stepper motor enable , active low    (yellow)
# define X_DIR_5v 51 //DIR+(+5v) axis stepper motor direction control (dark-green)
# define X_STP_5v 49 //PUL+(+5v) axis stepper motor step control      (ligh-green)
# define load_cell A4 // load cell analog port

#define BTN_STOP 52
#define BTN_DOWN 48
#define BTN_UP 46
#define BTN_PWR 50

#define LED_CHECK 47

#define F1_READ_FORCE 0
#define F2_SET_FORCE  1
#define F3_MOVE_POS   2
#define F4_MOVE_FORCE 3
#define F5_SET_ZERO   4

void setup() {

    Serial.begin(2000000);

    // MOTOR CONTROLLER SETUP  

    pinMode (X_EN_5v ,OUTPUT); //ENA+(+5V)
    pinMode (X_DIR_5v,OUTPUT); //DIR+(+5v)
    pinMode (X_STP_5v,OUTPUT); //PUL+(+5v)
    pinMode (LED_CHECK,OUTPUT);
    
    pinMode(BTN_STOP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_PWR, INPUT_PULLUP);
    
    digitalWrite (X_EN_5v, HIGH); //ENA+(+5V) 
    digitalWrite (X_DIR_5v, LOW); //DIR+(+5v)
    digitalWrite (X_STP_5v, LOW); //PUL+(+5v)
    digitalWrite (LED_CHECK, LOW);

    // LOAD CELL SETUP
    analogReference(EXTERNAL);
    
    // LCD SETUP 
    lcd.init();
    lcd.print("Init");

}

/* Manage input serial commands */
void serialEvent()
{ while (Serial.available()) 
  {
    char inChar = (char)Serial.read();            // get the new byte
    if (inChar > 0)     {inputString += inChar;}  // add it to the inputString
    if (inChar == '\n') { stringComplete = true;} // flag command is ready 
  }
}

/* Send command description through Serial channel*/
void Help(){
 Serial.println("Commands available");
 Serial.println("check             --> verify the serial connection");
 Serial.println("on                --> turns command motor (TB6600) ON");
 Serial.println("off               --> turns command motor (TB6600) OFF");
 Serial.println("stop              --> stop current the movement");
 Serial.println("zero              --> set current position as Zero");
 Serial.println("home              --> go to zero position");
 Serial.println("getNextPos        --> return the next position set");
 Serial.println("getCurrPos        --> return the current position");
 Serial.println("getSpeed          --> return the current time in microsecond between steps");
 Serial.println("getForce          --> return the force stop set");
 Serial.println("p+Number(+/-60000)--> set the next position in steps (e.g. p10000)");
 Serial.println("f+Number(0-2000)  --> set the new force to stop in cents of Newton (e.g. f1050 --> 10.5 N)");
 Serial.println("t+Number(0-2000)  --> set Microseconds betwean steps (e.g. t500)");
 Serial.println("mp                --> motor moves to new postion");
 Serial.println("mf                --> motor moves down until it get the force set");
 Serial.println("verbose_on        --> print position data while moving");
 Serial.println("verbose_off       --> not print position data while moving");

 inputString="";
}

void CheckConnection(){
 Serial.println("cmdAck_Serial_OK");
 inputString="";
}

void ENAXON(){ 
 Serial.println("cmdAck_MOTOR_ENABLED");
 digitalWrite (X_EN_5v, LOW);
 inputString="";
}
void ENAXOFF(){  
 Serial.println("cmdAck_MOTOR_DISABLED");
 digitalWrite (X_EN_5v, HIGH);
 inputString="";
}
void SetSpeed(){
 inputString.setCharAt(0,' ');
 MoveSpeed=inputString.toInt();
 Serial.print("cmdAck_Speed=");
 Serial.println(MoveSpeed);
 inputString="";
}
void ComDataON(){
 ComData=true;
 Serial.println("cmdAck_verbose_ON");
 inputString="";
}
void ComDataOFF(){
 ComData=false;
 Serial.println("cmdAck_verbose_OFF");
 inputString="";
}
void SetNextPos(){ 
 inputString.setCharAt(0,' ');
 NX=inputString.toInt();
 Serial.print("cmdAck_Next_Position=");
 Serial.println(NX);
 inputString="";
}

void GetNextPos(){
  Serial.print("cmdAck_Next_Position=");
  Serial.println(NX);
  inputString="";
}

void GetCurrPos(){
  Serial.print("cmdAck_Current_Position=");
  Serial.println(X);
  inputString="";
}

void SetZero(){
  X =0;
  NX =0;
  Serial.print("cmdAck_New_Zero_Set");
  Serial.flush();
  delay(1000);
  inputString="";
}

void GetSpeed(){
  Serial.print("cmdAck_Current_Speed=");
  Serial.println(MoveSpeed);
  inputString="";
}

void SetForce(){
  inputString.setCharAt(0,' ');
  forceLimit=inputString.toInt()/100.0;
  Serial.print("cmdAck_New_Force=");
  Serial.println(forceLimit);
  inputString="";
}

void GetForce(){
  Serial.print("cmdAck_Current_Force=");
  Serial.println(forceLimit);
  inputString="";
}


void Moving(){
  generalStop = false; 
  Serial.println("Event_Move");  
}

void StopMe(){
  generalStop = true;
  Serial.println("Event_Stop"); 
}

void StopMeRemote(){
  generalStop = true;
  Serial.println("cmdAck_Stop"); 
}

boolean isMoving(){
  return !generalStop;
}

void nextLCDFunction(){
  currentLCDFunction++;
  currentLCDFunction= currentLCDFunction % number_func;
  lcd.clear();
  delay(500);
  functionChanged = true;
  Serial.println("Event_Function_changed"); 
}

/*Read loadcell value and convert in force stored in global variable "current_force"*/
void readLoadCell(){
  
  loadCellValue = analogRead(load_cell);
  loadCellValue -= offset;
  lc = map(loadCellValue, 0, (1023 - offset) , 0, 1023);
  if (lc > 2){ // it is reading some force
    for (int i=0; i<100; i++){
      loadCellValue = analogRead(load_cell);
      loadCellValue -= offset;
      lc = map(loadCellValue, 0, (1023 - offset) , 0, 1023);
      sumLc += lc;
    }
    lc = sumLc / 100;
    sumLc =0;
  }
     
  //values calculated on 3 axis accelerometer  (no support)
  if (lc < 50){
    output = 0.0057*pow(lc,3)- 0.5*pow(lc, 2) +17.467*lc + 15.67;
  }else{
    output = 3.3507*lc+ 152.58;
  }
  cntprint += 1;
  current_force = output*N;
  if (cntprint == 10){
    //Serial.println(output);
    cntprint = 0;
  } 
}




/* Move to de desired position NX*/
void MoveX(){ 
  int xt;
  if (NX>X)
  {xt=NX-X; digitalWrite (X_DIR_5v,LOW);xt=1;}
  else
  {xt=X-NX; digitalWrite (X_DIR_5v,HIGH);xt=-1;}
  Moving();
  Serial.println("cmdAck_MovePos");
  for (; X !=NX; X=X+xt)
  {     
       btn_stop_status = digitalRead(BTN_STOP);
       if ((btn_stop_status == LOW)|| (generalStop)){
        Serial.println("Event_Stop"); 
        break; 
       }
       digitalWrite (X_STP_5v, HIGH);
       delayMicroseconds (MoveSpeed);
       digitalWrite (X_STP_5v, LOW);
       delayMicroseconds (MoveSpeed);
       if (ComData==true){
        Serial.print("Event_CurrPos=");
        Serial.println(X);
       }
  }
  Serial.print("Event_Positon_Reached=");
  Serial.println(X);
  inputString="";
}

/* Move to zero position*/
void GoHome(){
  NX=0;
  MoveX();
  Serial.println("cmdAck_Home");
}


/*  Move down until you reach the forceLimit, then maintain */
void MoveForce(){ 
  digitalWrite (X_DIR_5v,LOW); // direction is down, fixed
  Moving();
  Serial.println("cmdAck_MoveForce");
  int current_speed = MoveSpeed;
  boolean isHolding = false;
  while (true) {  
    

     serialEvent(); 
     if (stringComplete){
        if (inputString=="stop\n")       {StopMeRemote();}
        inputString = ""; 
         stringComplete = false;
     }
      btn_stop_status = digitalRead(BTN_STOP);
     if ((btn_stop_status == LOW) || (generalStop)){
        Serial.println("Event_Stop");
        break; 
     }else{
       readLoadCell();
       
      //if you send the force value add a 2ms delay to avoid broken packets
      //String str = "Force_" + String(current_force);
      //Serial.println(str);
      //delay(2);
       

        
       
       if (current_force < (forceLimit*1.01)){
          if (current_force < (forceLimit*0.8)){ current_speed = 20;} 
          else{ current_speed = 200; }  
          applyForce = false;
          if (!isHolding){
              applyForce = true;
          }else{
            if (current_force < (forceLimit*0.99)){
              applyForce = true;
            }  
          }
          if (applyForce){
              digitalWrite (X_STP_5v, HIGH);
              delayMicroseconds (current_speed);
              digitalWrite (X_STP_5v, LOW);
              delayMicroseconds (current_speed);
              isHolding = false;    
              X = X+1;
          }
       }else{
          // force applied, start to hold 
          if (!isHolding){
            Serial.println("Event_Hold");
          }
          isHolding = true;
       }
     }
  }
  inputString="";
}


/* UPDATE LCD SCREEN */
void updateLCD(){
  boolean displayActive = true;
  switch (currentLCDFunction) {
    case F1_READ_FORCE:
      lcdStringR0 ="READ FORCE";
      readLoadCell();
      lcdStringR1 =String(current_force, 2);
      break;
    case F2_SET_FORCE:
      lcdStringR0 ="SET FORCE";
      lcdStringR1 =String(forceLimit, 2);
      break;
    case F3_MOVE_POS:
      displayActive = false;
      lcdStringR0 ="MOVE NO CHECK!";
      lcdStringR1 ="Press UP or DOWN";
      break;
    case F4_MOVE_FORCE:
      displayActive = false;
      lcdStringR0 ="MOVE UNTIL "+String(forceLimit, 2);
      lcdStringR1 ="Press DOWN";
      break;
    case F5_SET_ZERO:
      lcdStringR0 ="SET ZERO "+String(X);
      lcdStringR1 ="Press UP+DOWN";
      break;
  }
  if (((millis() - timeRefresh) > lastRefresh) && ((displayActive) || (functionChanged)) && !standby){
    functionChanged = false;
    lcd.setCursor(0,0);
    lcd.print(lcdStringR0);
    lcd.setCursor(0,1);
    lcd.print(lcdStringR1);
    lastRefresh = millis();
  }
}

void loop(){

  // read onboard buttons status
  btn_pwr_status = digitalRead(BTN_PWR);
  btn_stop_status = digitalRead(BTN_STOP);
  btn_up_status = digitalRead(BTN_UP);
  btn_down_status = digitalRead(BTN_DOWN);

  //MANAGE PWR BUTTON FUNCTION
  if (btn_pwr_status == LOW){
    digitalWrite (LED_CHECK, HIGH);
    digitalWrite (X_EN_5v, LOW);
    if (standby){
       standby = false;
       functionChanged = true;
       lcd.clear();
    }
  }else{
    digitalWrite (LED_CHECK, LOW);
    digitalWrite (X_EN_5v, HIGH);
    standby = true;
    lcd.setCursor(0,0);
    lcd.print("Standby         ");
    lcd.setCursor(0,1);
    lcd.print("Press ON btn    ");
    
    
  
  }

  //MANAGE STOP BUTTON FUNCTION
  // if motor is moving stop it, else roll around funcions
  if (btn_stop_status == LOW){
    Serial.println("OnBoard_btn_stop");
    if (isMoving()){
      StopMe();
      delay(200);
    }else{
      nextLCDFunction();
    }
  }

  //UPDATE LCD SCREEN 
  updateLCD();


  

  //MANAGE BUTTON UP FUNCTIONS
  if (btn_up_status == LOW){
    Serial.println("OnBoard_btn_up");
    switch (currentLCDFunction) {
      case F1_READ_FORCE:
        break;
      case F2_SET_FORCE:
        forceLimit = forceLimit +0.1; 
        delay(150);
        break;
      case F3_MOVE_POS:
      case F4_MOVE_FORCE:
        digitalWrite (X_DIR_5v,HIGH);
        digitalWrite (X_STP_5v, HIGH);
        delayMicroseconds (20);
        digitalWrite (X_STP_5v, LOW);
        delayMicroseconds (20);
        break;    
      case F5_SET_ZERO:
        if (btn_down_status == LOW){
          SetZero();
        }
        break;
     }
  }
   
  
  //MANAGE BUTTON DOWN FUNCTIONS     
  if (btn_down_status == LOW){
    //Serial.println("down");
    Serial.println("OnBoard_btn_down");
    switch (currentLCDFunction) {
      case F1_READ_FORCE: 
        break; 
      case F2_SET_FORCE:
        forceLimit = forceLimit -0.1;
        delay(150); 
        break;
      case F3_MOVE_POS:
        digitalWrite (X_DIR_5v,LOW);
        digitalWrite (X_STP_5v, HIGH);
        delayMicroseconds (20);
        digitalWrite (X_STP_5v, LOW);
        delayMicroseconds (20);
        break;
      case F4_MOVE_FORCE:
        MoveForce();
        break;
      case F5_SET_ZERO:
        break;
    }   
  }

  //MANAGE REMOTE COMMANDS
  serialEvent(); 
  if (stringComplete){
    if (inputString=="check\n")      {CheckConnection();}  
    if (inputString=="on\n")         {ENAXON();}   
    if (inputString=="off\n")        {ENAXOFF();}
    if (inputString=="verbose_on\n") {ComDataON();}
    if (inputString=="verbose_off\n"){ComDataOFF();}
    if (inputString=="mp\n")         {MoveX();}
    if (inputString=="mf\n")         {MoveForce();}
    if (inputString.charAt(0)=='t')  {SetSpeed();}
    if (inputString.charAt(0)=='p')  {SetNextPos();} 
    if (inputString.charAt(0)=='f')  {SetForce();} 
    if (inputString=="getCurrPos\n") {GetCurrPos();}
    if (inputString=="getNextPos\n") {GetNextPos();}
    if (inputString=="getForce\n")   {GetForce();}
    if (inputString=="getSpeed\n")   {GetSpeed();}
    if (inputString=="zero\n")       {SetZero();}
    if (inputString=="stop\n")       {StopMeRemote();}
    if (inputString=="home\n")       {GoHome();}
    if (inputString=="help\n")       {Help();}
    if (inputString !="") {Serial.println("BAD_COMMAND="+inputString);}
       
    inputString = ""; 
    stringComplete = false;
  }
}
