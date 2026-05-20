int speakerPin = 8;
int poti = A0;
int toneFrq = 0;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  int potiValue = analogRead(poti);
  int toneFreq = map(potiValue, 0, 1023, 131, 330); //die Eingangswerte potiValue sind im Bereich von 0 bis 1023 und werden auf den Bereich 131 bis 330 umgerechnet (Tenorfrequenzbereich)
  tone(speakerPin, toneFreq);
}

