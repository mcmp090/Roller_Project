#include <Wire.h>

#include <SPI.h> // SPI-Bibiothek hinzufügen

#include <MFRC522.h> // RFID-Bibiothek hinzufügen

#define SS_PIN 10 // SDA an Pin 10 (bei MEGA anders)

#define RST_PIN 9 // RST an Pin 9 (bei MEGA anders)

MFRC522 mfrc522(SS_PIN, RST_PIN); // RFID-Empfänger benennen


byte val;
long code=0;
void setup() // Beginn des Setups:

{

Serial.begin(9600); // Serielle Verbindung starten (Monitor)

Wire.begin(8);                // join i2c bus with address #8

Wire.onRequest(requestEvent); // register event
  
SPI.begin(); // SPI-Verbindung aufbauen

mfrc522.PCD_Init(); // Initialisierung des RFID-Empfängers

}



void loop() // Hier beginnt der Loop-Teil

{

  delay(100);

}

void requestEvent() {
code=0;
if ( ! mfrc522.PICC_IsNewCardPresent()) // Wenn keine Karte in Reichweite ist...

{val = 0;}else{

if ( ! mfrc522.PICC_ReadCardSerial()) // Wenn kein RFID-Sender ausgewählt wurde

{}else{
Serial.print("Die ID des RFID-TAGS lautet:"); // "Die ID des RFID-TAGS lautet:" wird auf den Serial Monitor geschrieben.



for (byte i = 0; i < mfrc522.uid.size; i++)

{

code=((code+mfrc522.uid.uidByte[i])*10); // Dann wird die UID ausgelesen, die aus vier einzelnen Blöcken besteht und der Reihe nach an den Serial Monitor gesendet. Die Endung Hex bedeutet, dass die vier Blöcke der UID als HEX-Zahl (also auch mit Buchstaben) ausgegeben wird

Serial.print(" "); // Der Befehl „Serial.print(" ");“ sorgt dafür, dass zwischen den einzelnen ausgelesenen Blöcken ein Leerzeichen steht.

}


Serial.println(code); // Mit dieser Zeile wird auf dem Serial Monitor nur ein Zeilenumbruch gemacht.
 // ...springt das Programm zurück vor die if-Schleife, womit sich die Abfrage wiederholt.
}
 // ...springt das Programm zurück vor die if-Schleife, womit sich die Abfrage wiederholt.
 if(code == 202702880){
val = 1;
 }else{
  val = 0;
 }

  
}
  
  Wire.write(val); // respond with message of 6 bytes
  // as expected by master
}
