uint32_t i;

void setup() {
  Serial.begin(115200);
  i = 0;
}


void loop() {
  Serial.print(i);
  Serial.print(',');
  Serial.print(sin((double)i / 256 * 4 * PI) * 20);
  Serial.print(',');
  Serial.print(cos((double)i / 256 * 4 * PI) * 20 - 5);
  Serial.print('\n');
  i++;
  delay(10);
}
