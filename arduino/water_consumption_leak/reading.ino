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
  
  if (millis() > 20000 and millis() < 240000){ // Four minuttes to callibrate what 3 liters are
    liter_conv = 3.00 / amount; 
    Serial.println(String(amount) + "," + String(liter_conv) + ",");
    }
    
  if (millis() > leak_millis + 10000){
    LR11 = R1; //Set LR11 every 10th second
  }
}

void temp_conv(){
  logR1 = log(R1);
  logR2 = log(R2);
  logR3 = log(R3);
  C1 = (1.0 / (cc1 + cc2*logR1 + cc3*logR1*logR1*logR1));
  C2 = (1.0 / (cc1 + cc2*logR2 + cc3*logR2*logR2*logR2));
  C3 = (1.0 / (cc1 + cc2*logR3 + cc3*logR3*logR3*logR3));
  C1 = C1 - 273.15;
  C2 = C2 - 273.15;
  C3 = C3 - 273.15;
}

