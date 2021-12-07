uint8_t i;
double sinwave[256];
double coswave[256];

void setup() {
  Serial.begin(115200);
  sinwave[0] = 0;
  coswave[0] = 0;
  for(i = 1; i != 0; i++)
  {
    sinwave[i] = sin((double)i / 256 * 4 * PI) * 20;
    coswave[i] = cos((double)i / 256 * 4 * PI) * 20 - 5;
  }
  i = 0;
}


void loop() {
  Serial.print(i);
  Serial.print(',');
  Serial.print(sinwave[i]);
  Serial.print(',');
  Serial.print(coswave[i]);
  Serial.print('\n');
  i++;
  delay(10);
  if(i == 0)
  Serial.print("cls\n");
}
