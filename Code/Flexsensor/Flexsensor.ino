const int flexSensorPin = A0;  // Flexsensor an Pin A0 angeschlossen
int sensorValue = 0;          // Variable zum Speichern des Sensorwerts

void setup() {
  Serial.begin(9600);        // Initialisiere die serielle Kommunikation mit 9600 Baud
}

void loop() {
  sensorValue = analogRead(flexSensorPin); // Lese den analogen Wert vom Flexsensor

  Serial.print("Sensorwert: ");
  Serial.println(sensorValue);           // Gib den Sensorwert auf der Konsole aus

  delay(100);                           // Warte 100 Millisekunden, bevor die nächste Messung erfolgt
}