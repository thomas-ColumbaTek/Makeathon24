/*  Simple Stepper Motor Control
 *      
 *  by Achim pieters, www.studiopieters.nl
 *  
 */
 
// Defines pins numbers
const int stepPin = 7;
const int dirPin = 8; 

int customDelay,customDelayMapped; // Defines variables
 
void setup() {
  // Sets the two pins as Outputs
  pinMode(stepPin,OUTPUT);
  pinMode(dirPin,OUTPUT);
 
  digitalWrite(dirPin,HIGH); //Enables the motor to move in a particular direction
}
void loop() {
  
  customDelayMapped = speedUp(); // Gets custom delay values from the custom speedUp function
  // Makes pules with custom delay, depending on the Potentiometer, from which the speed of the motor depends
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(customDelayMapped);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(customDelayMapped);
}
// Function for reading the Potentiometer
int speedUp() {
  int customDelay = analogRead(A0); // Reads the potentiometer
  int newCustom = map(customDelay, 0, 1023, 300,4000); // Convrests the read values of the potentiometer from 0 to 1023 into desireded delay values (300 to 4000)
  return newCustom;  
}
