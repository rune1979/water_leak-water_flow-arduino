void read_termister(){
  Voa = analogRead(ThermistorPin1);
  Vob = analogRead(ThermistorPin2);
  Voc = analogRead(ThermistorPin3);
  RA1 = R01 * (1023.0 / (float)Voa - 1.0);
  RA2 = R02 * (1023.0 / (float)Vob - 1.0);
  RA3 = R03 * (1023.0 / (float)Voc - 1.0);
  if (!R1 and ! R2 and !R3){
    R1 = RA1;
    R2 = RA2;
    R3 = RA3;
  }
  R1 = ((R1 * 9) + RA1) / 10;
  R2 = ((R2 * 9) + RA2) / 10;
  R3 = ((R3 * 9) + RA3) / 10;
  cond = abs(R1 - R3);
  cond = cond / 300;
  cond = 1 / cond;
  tol = R1 * term_tolerance;
  diff = R1 - R2; // deviation between termistor 1 (in) and two (out)
  diff = diff * cond; // expecting only 90% of the temperature difference to be carried over
  if (millis() > 120000 and millis() < 350000){
    liter_conv = 5.00 / amount; 
    Serial.println(String(amount) + "," + String(liter_conv) + ",");
  }
       
}

