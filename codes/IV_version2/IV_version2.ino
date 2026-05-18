
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/********************************************** Macros For Diode Circuit*******************************************************/
//Pwm Pins
#define PWM_PIN11     3      // PWM Output 1
#define PWM_PIN3     11     // PWM Output 2
//Measuring Voltage Pins
#define VOLTAGE_A0         A0  // Voltage Input
#define VOLTAGE_A1         A1  // Current Input

#define VOLTAGE_A2         A2  // Voltage Input
#define VOLTAGE_A3         A3  // Current Input


/********************************************** Macros For RGB & IR LED Pins ****************************************************/
#define COMON_CATHOD       0
#define REDLED             5   
#define GREENLED           6
#define BLUELED            7
#define UVLED              9
#define IRLED              10

/**************************************** Variables Related To  measuring voltage and current of diode **************************/
String mode = "X";  // Default mode: Stop
int pwmValue = 0;   // PWM Value from Python
float Voltage_Diode = 0;  // Measured Voltage
float Voltage_Shunt = 0;  // Measured Current

/*************************************** Set the I2C address (0x27 or 0x3F depending on the module)******************************/
LiquidCrystal_I2C lcd(0x27, 16, 4);  // 16x4 LCD

void LCD_Init(void)
{

  lcd.init();       // Initialize LCD
  lcd.backlight();  // Turn on backlight
}

void setup() {
    Serial.begin(115200);
    LCD_Init();
    lcd.setCursor(0, 0); 
    lcd.print("App is Starting");


    pinMode(PWM_PIN11, OUTPUT);
    pinMode(PWM_PIN3, OUTPUT);


    pinMode(Voltage_Diode, INPUT);
    pinMode(Voltage_Shunt, INPUT);
    pinMode(VOLTAGE_A2, INPUT);
    pinMode(VOLTAGE_A3, INPUT);

    pinMode(REDLED, OUTPUT);
    pinMode(GREENLED, OUTPUT);
    pinMode(BLUELED, OUTPUT);
    pinMode(IRLED, OUTPUT);
    pinMode(UVLED, OUTPUT);


    TCCR2A = B10100011; //fast PWM on both A and B
    TCCR2B = B00000001; //no prescale on timer2
    Serial.println("[INFO] Arduino Ready");
    delay(1000);
    lcd.clear();
    lcd.print("Arduino Ready");

}

void loop() {
    if (Serial.available() > 0) {
        String receivedData = Serial.readStringUntil('\n'); // Read incoming data
        receivedData.trim(); // Remove unwanted spaces or line breaks
        if (receivedData.length() > 0) {
            processInput(receivedData); // Process mode and PWM input
        }
    }

    if (mode == "F") 
    {
          lcd.clear();
          lcd.setCursor(0, 0); 
          lcd.print("Forward Biase");

          sweepForwardBias(); // Forward Bias Sweep
    } 
    else if (mode == "I") 
    {
      
        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("Measure instant");   
        analogWrite(PWM_PIN3, pwmValue);
        analogWrite(PWM_PIN11, 0);
        sendInstantValues(); // Continuous measurement
    } 
    else if (mode == "R") {
        // Serial.println("[INFO] Reverse Bias Mode, No Measurement");
        //  Serial.println("[INFO] Reverse ") ;

        sweepReverseBias() ;
    } 
    else if (mode == "X") {
        // Serial.println("[INFO] Measurement Stopped");
        return;
    }

    delay(100); // Small delay to prevent flooding
}
void ControlLED(char receivedMode)
{
  #ifdef COMON_CATHOD
    switch (receivedMode)
    {
      case 'A':
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, LOW);
        digitalWrite(BLUELED, LOW);
        digitalWrite(IRLED, LOW);
        digitalWrite(UVLED, LOW);

        break;
      case 'B':
        digitalWrite(REDLED, HIGH);
        digitalWrite(GREENLED, HIGH);
        digitalWrite(BLUELED, HIGH);
        digitalWrite(IRLED, HIGH);
        digitalWrite(UVLED, HIGH);

        break;
      case '0':
        digitalWrite(REDLED, HIGH);

        break;
      case '1':
        digitalWrite(REDLED, LOW);
      case '2':
        digitalWrite(GREENLED, HIGH);

        break;
      case '3':
        digitalWrite(GREENLED, LOW);

        break;
      case '4':
        digitalWrite(BLUELED, HIGH);
        break;
      case '5':
        digitalWrite(BLUELED, LOW);
      case '6':
        digitalWrite(IRLED, HIGH);
      case '7':
        digitalWrite(IRLED, LOW);
      case '8':
        digitalWrite(UVLED, HIGH);
      case '9':
        digitalWrite(UVLED, LOW);
      default:
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, LOW);
        digitalWrite(BLUELED, LOW);
        digitalWrite(IRLED, LOW);
        digitalWrite(UVLED, LOW);
        break;
    }
  #else
    switch (receivedMode)
    {
      case 'A':
        digitalWrite(REDLED, HIGH);
        digitalWrite(GREENLED, HIGH);
        digitalWrite(BLUELED, HIGH);
        digitalWrite(IRLED, HIGH);
        digitalWrite(UVLED, HIGH);

        break;
      case 'B':
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, LOW);
        digitalWrite(BLUELED, LOW);
        digitalWrite(IRLED, LOW);
        digitalWrite(UVLED, LOW);

        break;
      case '0':
        digitalWrite(REDLED, LOW);

        break;
      case '1':
        digitalWrite(REDLED, HIGH);
      case '2':
        digitalWrite(GREENLED, LOW);

        break;
      case '3':
        digitalWrite(GREENLED, HIGH);

        break;
      case '4':
        digitalWrite(BLUELED, LOW);
        break;
      case '5':
        digitalWrite(BLUELED, HIGH);
      case '6':
        digitalWrite(IRLED, LOW);
      case '7':
        digitalWrite(IRLED, HIGH);
      case '8':
        digitalWrite(UVLED, LOW);
      case '9':
        digitalWrite(UVLED, HIGH);
      default:
        digitalWrite(REDLED, HIGH);
        digitalWrite(GREENLED, HIGH);
        digitalWrite(BLUELED, HIGH);
        digitalWrite(IRLED, HIGH);
        digitalWrite(UVLED, HIGH);
        break;
    }
  #endif

}
void processInput(String input) {
    input.trim(); // Remove unwanted spaces or line breaks
    if (input.length() < 1) {
        Serial.println("[ERROR] Empty command received.");
        return;
    }

    char receivedMode = input.charAt(0); // Get the first character as mode

    if (receivedMode == 'F' || receivedMode == 'R' || receivedMode == 'I') {
       //[INFO] Mode Set:
        mode = String(receivedMode); // Update mode
        //  Serial.print("mode is :");
        // Serial.println(mode);
    }
    else if(receivedMode == '0'||receivedMode == '1' || receivedMode == '2' || receivedMode == '3'|| receivedMode == '4'||   //R:0,1 ... G:2,3  ...B:4,5 ..IR:6,7 ...UV:8,9
            receivedMode == '5'|| receivedMode == '6' || receivedMode == '7'|| receivedMode == '8'||receivedMode == '9' ||
            receivedMode == 'A'|| receivedMode == 'B')     // All off :A ........ ALL on :B
    {
        // Turn on LED 
        ControlLED( receivedMode);
        return;
    }
     else  {
        Serial.println("[ERROR] Invalid Mode");
        Serial.println(input);
        return;
    }

    // Check if "I" mode has a PWM value
    int commaIndex = input.indexOf(',');
    if (mode == "I" && commaIndex != -1) {
        int newPwmValue = input.substring(commaIndex + 1).toInt();
        if (newPwmValue >= 0 && newPwmValue <= 255)
         {
            //[INFO] PWM Set:
            pwmValue = newPwmValue;
            // analogWrite(PWM_PIN11, pwmValue);
            // analogWrite(PWM_PIN3, pwmValue);
        } 
        else 
        {
          Serial.println("[ERROR] Invalid PWM Value");
          Serial.println(input);
        }
    }
}
// Sweep PWM from 0 to 255 in Forward Bias
void sweepForwardBias() 
{
    for (int pwm = 0; pwm <= 255; pwm +=3)
    {
        if (mode == "X") return; // Stop if mode is set to X
        analogWrite(PWM_PIN3, pwm);
        analogWrite(PWM_PIN11, 0);
        readAndSendData('F');
        delay(300);


    }
    Serial.print("*");
    mode="X";
}
void sweepReverseBias() 
{
    for (int pwm = 0; pwm <= 255; pwm +=3)
    {
        if (mode == "X") return; // Stop if mode is set to X
        analogWrite(PWM_PIN11, pwm);
        analogWrite(PWM_PIN3, 0);
        readAndSendData('R');
        delay(300);
    }
    Serial.print("*");
    mode="X";
}
// Send voltage & current for Instant Mode
void sendInstantValues()
{
    if (mode != "X") readAndSendData('F');
}

// Read voltage & current, send to Python
float ReadVoltage0() {
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(VOLTAGE_A0);
        delay(5);
    }
    return (sum / 10) * 5.0 / 1023.0;
}

float ReadVoltage1() {
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(VOLTAGE_A1);
        delay(5);
    }
    return (sum / 10) * 5.0 / 1023.0;
}

float ReadVoltage2() {
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(VOLTAGE_A2);
        delay(5);
    }
    return (sum / 10) * 5.0 / 1023.0;
}

float ReadVoltage3() {
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(VOLTAGE_A3);
        delay(5);
    }
    return (sum / 10) * 5.0 / 1023.0;
}

void readAndSendData(char Status) 
{
    if (Status=='R')
    {
      Voltage_Diode = ReadVoltage0();
      Voltage_Shunt = ReadVoltage1();
    }
    else
    {
      Voltage_Diode = ReadVoltage2();
      Voltage_Shunt = ReadVoltage3();
     
    }

    Serial.print(Voltage_Diode, 3);
    Serial.print(",");
    Serial.println(Voltage_Shunt, 3);

    lcd.setCursor(0, 1); 
    lcd.print("VD"+String(Voltage_Diode)+"..VS="+String(Voltage_Shunt));
}
