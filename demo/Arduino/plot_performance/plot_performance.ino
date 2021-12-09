#define ARR_LEN 512
uint32_t i;
float sinTable[ARR_LEN];

void setup() {
  Serial.begin(921600);
  for(i = 0; i < ARR_LEN; i++)
  {
    sinTable[i] = sinf((double)i / ARR_LEN * 4 * PI) * 20;
  }
  i = 0;
}


void loop() {
  Serial.print(i);
  Serial.print(',');
  Serial.print(sinTable[i]);
  Serial.print(',');
  Serial.print(sinTable[(i + ARR_LEN / 3) % ARR_LEN]);
  Serial.print(',');
  Serial.print(sinTable[(i + ARR_LEN / 3 * 2) % ARR_LEN]);
  Serial.print(',');
  Serial.print(i % (ARR_LEN / 4) + 20);
  Serial.print('\n');
  i++;
  i %= ARR_LEN;
}