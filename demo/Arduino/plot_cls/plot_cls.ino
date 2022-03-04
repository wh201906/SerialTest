// Tested on ESP32
// For Arduino UNO, set ARRAY_LEN to 160 to take less RAM
// #define ARRAY_LEN 160
#define ARRAY_LEN 256

uint16_t i;
double sinTable[ARRAY_LEN];
double cosTable[ARRAY_LEN];

void setup()
{
  Serial.begin(115200);
  for (i = 0; i < ARRAY_LEN; i++)
  {
    sinTable[i] = sin((double)i / ARRAY_LEN * 2 * PI) * 20;
    cosTable[i] = cos((double)i / ARRAY_LEN * 2 * PI) * 20 - 5;
  }
  i = 0;
}

void loop()
{
  Serial.print(i);
  Serial.print(',');
  Serial.print(sinTable[i]);
  Serial.print(',');
  Serial.print(cosTable[i]);
  Serial.print('\n');
  i++;
  i %= ARRAY_LEN;
  delay(10);
  if (i == 0)
    // clear the plot panel
    Serial.print("cls\n");
}

// set Data Num to 3
// set Frame Splitter to \n
// set Data Splitter to String, then input ","
// set Clear to String, then input "cls"
// X Type can be anything