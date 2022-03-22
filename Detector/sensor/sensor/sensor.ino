/*
  ZMPT101B - AC Voltage sensor
  Calculate Voltage

  modified on 7 Sep 2020
  by Mohammad Reza Akbari @ Electropeak
  Home
*/


double maximum = 0;
double minimum = 0;

void setup() {
  Serial.begin(9600);
  maximum = analogRead(A0);
  minimum = maximum;
}

void loop() {
  double current = analogRead(A0);
  if (current > maximum){
    maximum = current;
  }
  if (minimum > current) {
    minimum = current;
  }
  Serial.print("Max: ");
  Serial.println(maximum);
  Serial.print("Min: ");
  Serial.println(minimum);
  delay(100);
}
