#include <Crypto.h>
#include <SHA256.h>

void setup() {
  SHA256 sha256;
  const char* password = "1234";  // Password to be checked
  byte hash[32];  // Array to store the hash result

  Serial.begin(9600);

  sha256.update((byte*)password, strlen(password));  // Convert password to bytes and hash it
  sha256.finalize(hash, sizeof(hash));  // Finalize the hash and store the result in 'hash'

  for (int i = 0; i < sizeof(hash); i++) {
    Serial.print(hash[i] < 0x10 ? "0" : "");  // Ensure leading zero for values less than 0x10
    Serial.print(hash[i], HEX);  // Print the hash value as hexadecimal
  }
  Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:
}

