// Tested on ESP32
// For Arduino UNO, set ARRAY_LEN to 320 to take less RAM
// #define ARRAY_LEN 320
#define ARRAY_LEN 512

uint16_t i;
double sinTable[ARRAY_LEN];

void setup()
{
  // faster
  Serial.begin(921600);
  for (i = 0; i < ARRAY_LEN; i++)
  {
    sinTable[i] = sinf((double)i / ARRAY_LEN * 2 * PI) * 20;
  }
  i = 0;
}

void loop()
{
  Serial.print(i);
  Serial.print(',');
  Serial.print(sinTable[i]);
  Serial.print(',');
  Serial.print(sinTable[(i + ARRAY_LEN / 3) % ARRAY_LEN]);
  Serial.print(',');
  Serial.print(sinTable[(i + ARRAY_LEN / 3 * 2) % ARRAY_LEN]);
  Serial.print(',');
  Serial.print(i % (ARRAY_LEN / 4) + 20);
  Serial.print('\n');
  i++;
  i %= ARRAY_LEN;
}

// set Data Num to 5
// set Frame Splitter to \n
// set Data Splitter to String, then input ","
// X Type can be anything
