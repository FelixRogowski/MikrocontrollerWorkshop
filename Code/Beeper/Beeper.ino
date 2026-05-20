int speakerPin = 8;
int testTon = 131; //tiefes Tenor C
unsigned long vergangeneZeit = 0;
unsigned long aktuelleZeit = 0;

void setup() {
  // put your setup code here, to run once:


}

void loop() {
  // put your main code here, to run repeatedly:
  aktuelleZeit = millis();
  if(aktuelleZeit - vergangeneZeit > 2000){
    tone(speakerPin, testTon, 500);
    vergangeneZeit = aktuelleZeit;
  }
}

