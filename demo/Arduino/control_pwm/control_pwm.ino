// measure the DC voltage on this pin
// or you can measure the AC waveform on this pin,
// the duty cycle changes.
int analogPin = 5;

// 0->0V, 255->VCC
uint8_t analogVal = 0;
bool isSending = false;
String cmd;

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello from SerialTest control_pwm demo");
  Serial.println("Please send commands with \\n");
  Serial.println("Commands:");
  Serial.println("axxx");
  Serial.println("- Set the analog output value, xxx is 0~255");
  Serial.println("Start");
  Serial.println("- Start to send message continuously");
  Serial.println("Stop");
  Serial.println("- Stop to send message continuously");
}

void loop()
{
  if (Serial.available())
  {
    cmd = Serial.readStringUntil('\n');
    Serial.println("Commands: " + cmd);
    if (cmd[0] == 'a') // set analog val
    {
      analogVal = cmd.substring(1).toInt() & 0xFF;
      analogWrite(analogPin, analogVal);
      Serial.println("New analog val: " + String(analogVal));
    }
    else if (cmd == "Start") // start to send message continuously
    {
      isSending = true;
    }
    else if (cmd == "Stop") // stop to send message
    {
      isSending = false;
    }
    else
    {
      Serial.println("Invalid command!");
    }
  }
  else
  {
    if (isSending)
    {
      delay(1000);
      Serial.println("Message from demo");
    }
  }
}

// Goto control panel, click "import", then choose the control_pwm.json
// Then you can send "Start", "Stop" and "axxx" easily
