#include <iostream>
#include <curl/curl.h>

#include "HDC1080.h"
#include "MFRC522.h"  // Include the header file for MFRC522

char ip[] = "192.168.137.1";
char port[] = "3001";

void delay(int ms){
  usleep(ms*1000);  // Function to introduce a delay in milliseconds
}

void send_temp_hum(float temp, float hum){
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    char url[100];
    sprintf(url,"http://%s:%s/temphum", ip, port);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    char post_field[100];
    sprintf(post_field, "json= {\"temp\": %f,\"hum\": %f}", temp, hum);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}

void send_id(string id){
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    char url[100];
    sprintf(url,"http://%s:%s/id", ip, port);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    string post_field = "id="+ id;
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field.c_str());

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}
int main() {
    float temperature;
    float humidity;

    int i2c_fd = init_hdc1080();
    
    MFRC522 mfrc;  // Initialize an instance of the MFRC522 class
    mfrc.PCD_Init();  // Initialize the reader
    
    // Send a command to read data from the HDC1080 sensor
    while (1)
    {
        read_hdc1080(i2c_fd, temperature, humidity);
        std::cout << std::endl << "Temperature: " << temperature << " °C, Humidity: " << humidity << " %" << std::endl;
        send_temp_hum(temperature, humidity);
        
        // Look for a card
        if(!mfrc.PICC_IsNewCardPresent()) // Check if a new card is present
        continue;  // If no new card is present, continue to the next iteration

        if( !mfrc.PICC_ReadCardSerial())  // Attempt to read the card's serial number
        continue;  // If reading fails, continue to the next iteration

        std::cout << std::endl << mfrc.PICC_TakeStringID() << std::endl;  // Print the string ID of the card
        send_id(mfrc.PICC_TakeStringID());
        

        delay(1000);  // Introduce a delay of 1000 milliseconds (1 second)
    }
    close(i2c_fd); // Close the I2C connection
    return 0;
}