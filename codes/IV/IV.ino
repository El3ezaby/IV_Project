const int pwmPin1 = 3;      // PWM Output 1
const int pwmPin2 = 11;     // PWM Output 2
const int voltagePin = A0;  // Voltage Input
const int currentPin = A1;  // Current Input

String mode = "X";  // Default mode: Stop
int pwmValue = 0;   // PWM Value from Python
float voltage = 0;  // Measured Voltage
float current = 0;  // Measured Current

void setup() {
    Serial.begin(115200);
    pinMode(pwmPin1, OUTPUT);
    pinMode(pwmPin2, OUTPUT);
    pinMode(voltagePin, INPUT);
    pinMode(currentPin, INPUT);
    TCCR2A = B10100011; //fast PWM on both A and B
    TCCR2B = B00000001; //no prescale on timer2
    Serial.println("[INFO] Arduino Ready");
}

void loop() {
    if (Serial.available() > 0) {
        String receivedData = Serial.readStringUntil('\n'); // Read incoming data
        receivedData.trim(); // Remove unwanted spaces or line breaks
        if (receivedData.length() > 0) {
            processInput(receivedData); // Process mode and PWM input
        }
    }

    if (mode == "F") {
        sweepForwardBias(); // Forward Bias Sweep
    } 
    else if (mode == "I") {
        analogWrite(pwmPin1, pwmValue);
        analogWrite(pwmPin2, pwmValue);
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
    } else {
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
            // analogWrite(pwmPin1, pwmValue);
            // analogWrite(pwmPin2, pwmValue);
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
        analogWrite(pwmPin1, pwm);
        analogWrite(pwmPin2, pwm);
        readAndSendData();
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
        analogWrite(pwmPin1, pwm);
        analogWrite(pwmPin2, pwm);
        readAndSendData();
        delay(300);
    }
    Serial.print("*");
    mode="X";
}
// Send voltage & current for Instant Mode
void sendInstantValues()
{
    if (mode != "X") readAndSendData();
}

// Read voltage & current, send to Python
float readVoltage() {
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(voltagePin);
        delay(5);
    }
    return (sum / 10) * 5.0 / 1023.0;
}

float readCurrent() {
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(currentPin);
        delay(5);
    }
    return (sum / 10) * 5.0 / 1023.0;
}

void readAndSendData() {
    voltage = readVoltage();
    current = readCurrent();
    Serial.print(voltage, 3);
    Serial.print(",");
    Serial.println(current, 3);
}
