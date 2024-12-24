#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <SPIFFS.h>


// Replace with your network credentials
const char* ssid = "NETWORK_SSID"; // Your Wi-Fi SSID
const char* password = "NETWORK_PASSWORD"; // Your Wi-Fi password

//TFT Display Instance
TFT_eSPI tft = TFT_eSPI();

// Spotify API credentials
const char* clientId = "CLIENT_ID";
const char* clientSecret = "CLIENT_SECRET";
String redirectUri = "YOUR_REDIRECT_URI"; // Change this to your redirect URI
String accessToken = "ACCESS_TOKEN"; // Replace with your actual access token
String refreshToken = "REFRESH_TOKEN"; // Replace with your actual refresh token

unsigned long previousMillis = 0; // Store the last time the track was checked
const long interval = 2500; // Interval to check the currently playing track 

//Default Song Information
const char* trackName = "Not Available";
const char* artistName = "Not Available";
const char* albumName = "Not Available";


// Function to refresh the access token
void refreshAccessToken() {
  HTTPClient http;
  http.begin("https://accounts.spotify.com/api/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String payload = "grant_type=refresh_token&refresh_token=" + refreshToken +
                   "&client_id=" + clientId +
                   "&client_secret=" + clientSecret;

  int httpResponseCode = http.POST(payload);
  if (httpResponseCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      accessToken = doc["access_token"].as<String>();
      Serial.println("New Access Token: " + accessToken);
    } else {
      Serial.print("JSON Deserialization Error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("Error refreshing token: ");
    Serial.println(httpResponseCode);
    String response = http.getString(); 
    Serial.println("Response: " + response);
  }
  http.end();
}

// Function to display the album art from a URL
void displayAlbumArt(const String& imageUrl) {
    HTTPClient http;
    http.begin(imageUrl); /

    int httpResponseCode = http.GET(); 
    if (httpResponseCode == 200) {
        
        int contentLength = http.getSize();
        if (contentLength > 0) {
            
            uint8_t* buffer = new uint8_t[contentLength];
            int bytesRead = http.getStream().readBytes(buffer, contentLength);

            if (bytesRead > 0) {
                
                TJpgDec.drawJpg(0, 0, buffer, bytesRead); 
            } else {
                Serial.println("Failed to read image data.");
            }

            delete[] buffer; 
        }
    } else {
        Serial.print("Error fetching image: ");
        Serial.println(httpResponseCode);
    }

    http.end(); 
}

// Function to get the currently playing track
void getCurrentPlayingTrack() {
  if (accessToken.length() == 0) {
    Serial.println("Access token is empty.");
    return;
  }

  HTTPClient http;
  http.begin("https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization", "Bearer " + accessToken);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    String response = http.getString();
    // Serial.println("Currently Playing Track: " + response); 

    // Parse the JSON response
    DynamicJsonDocument doc(2048); // Adjust size as needed
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      trackName = doc["item"]["name"]; // Track name
      artistName = doc["item"]["artists"][0]["name"]; 
      albumName = doc["item"]["album"]["name"]; // Album name
      String imageLink = doc["item"]["album"]["images"][0]["url"].as<String>(); // Get the image URL

      Serial.println("Track: " + String(trackName));
      Serial.println("Artist: " + String(artistName));
      Serial.println("Album: " + String(albumName));

      // Call the function to display the album art
      //displayAlbumArt(imageLink);
      
    } else {
      Serial.print("JSON Deserialization Error: ");
      Serial.println(error.c_str());
    }
  } else if (httpResponseCode == 401) {
    Serial.println("Access token expired, refreshing...");
    refreshAccessToken(); 
  } else {
    Serial.print("Error fetching currently playing track: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

// Function to connect to Wi-Fi
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}
void updatedisplay(const char* Track, const char* Artist, const char* Album){
    static String lastTrack = "";
    static String lastArtist = "";
    static String lastAlbum = "";

    // Only update the display if the information has changed
    if (lastTrack != Track || lastArtist != Artist || lastAlbum != Album) {
        tft.fillScreen(TFT_BLACK); // Clear the screen
        // Set text color and size
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        // Display track information
        tft.setCursor(10, 10);
        tft.print("Track: ");
        tft.println(Track);
        tft.setCursor(10, 40);
        tft.print("Artist: ");
        tft.println(Artist);
        tft.setCursor(10, 70);
        tft.print("Album: ");
        tft.println(Album);

        // Update the last displayed values
        lastTrack = Track;
        lastArtist = Artist;
        lastAlbum = Album;
    }
}




void setup() {
  Serial.begin(115200);
  connectToWiFi();
  tft.init();
  tft.setRotation(0);

  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK);

  updatedisplay(trackName, artistName, albumName);

}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 
    getCurrentPlayingTrack(); 
  }

  updatedisplay(trackName, artistName, albumName);

  delay(100);
  
}
