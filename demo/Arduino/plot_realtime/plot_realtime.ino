// Tested on ESP32
#define ARRAY_LEN 256

uint16_t i;

void setup() {
  Serial.begin(115200);
  i = 0;
}


void loop() {
  Serial.print(i);
  Serial.print(',');
  Serial.print(sin((double)i / ARRAY_LEN * 2 * PI) * 20);
  Serial.print(',');
  Serial.print(cos((double)i / ARRAY_LEN * 2 * PI) * 20 - 5);
  Serial.print('\n');
  i++;
  delay(10);
}

// set Data Num to 3
// set Frame Splitter to \n
// set Data Splitter to String, then input ","
// X Type can be anything
