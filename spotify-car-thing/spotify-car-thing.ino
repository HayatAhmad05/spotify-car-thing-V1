#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <SPIFFS.h>

// Replace with your network credentials
const char* ssid = "WIFI_NAME"; 
const char* password = "WIFI_PASSWORD"; 

// Spotify API credentials
const char* clientId = "CLIENT_ID";
const char* clientSecret = "CLIENT_SECRET";
String redirectUri = "CALLBACK_URL"; 
String accessToken = "ACCESS_TOKEN"; 
String refreshToken = "REFREASH_TOKEN"; // Replace with your actual refresh token

//Default Song Information
const char* trackName = "Not Available";
const char* artistName = "Not Available";
const char* albumName = "Not Available";

// Add these global variables
int currentSongPosition = 0;  // Current position in milliseconds
int songDuration = 0;        // Total duration in milliseconds
unsigned long lastProgressUpdate = 0;  // For tracking progress updates
char timeBuffer[16];  // Buffer to store the time string
String imageLink; // Declare globally to store the image URL
String PrevImageLink = ""; //Optimize the SPIFFS Download
int imageScaleSize = 2; 
unsigned long previousMillis = 0; // Store the last time the track was checked
const long interval = 500; // Interval to check the currently playing track (0.5 seconds)

//TFT Display Instance
TFT_eSPI tft = TFT_eSPI();

// First, add this callback function outside of updatedisplay
// This is required for TJpg_Decoder to work with your TFT display
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
    tft.pushImage(x, y, w, h, bitmap);
    return true;
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
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, response);

        if (!error) {
            trackName = doc["item"]["name"];
            artistName = doc["item"]["artists"][0]["name"]; 
            albumName = doc["item"]["album"]["name"];
            currentSongPosition = doc["progress_ms"].as<int>();
            songDuration = doc["item"]["duration_ms"].as<int>();

            // Get the smallest suitable image URL (300x300)
            JsonArray images = doc["item"]["album"]["images"];
            int selectedWidth = 0;
            for (JsonVariant image : images) {
                int width = image["width"].as<int>();
                
                if (width>= 300 && width <= 400) {  
                    imageLink = image["url"].as<String>();
                    selectedWidth = width;
                    break;
                }
            }
            
            
            if (imageLink.length() == 0 && images.size() > 0) {
                imageLink = images[0]["url"].as<String>();  // Changed to largest image
                selectedWidth = images[0]["width"].as<int>();
            }

            Serial.println("Selected image size: " + String(selectedWidth) + "x" + String(selectedWidth));
            if (PrevImageLink != imageLink)
            {
                // Save the album art to SPIFFS using our custom function
                saveAlbumArtToSPIFFS(imageLink);
            }
            
            PrevImageLink = imageLink;
            updatedisplay(trackName, artistName, albumName);
            
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


void saveAlbumArtToSPIFFS(const String& imageUrl) {
    HTTPClient http;
    http.begin(imageUrl);
    
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
        // If an old image exists, remove it first
        if(SPIFFS.exists("/albumArt.jpg")) {
            SPIFFS.remove("/albumArt.jpg");
        }

       
        File file = SPIFFS.open("/albumArt.jpg", "w");
        if(!file) {
            Serial.println("Failed to open file for writing");
            return;
        }

        
        WiFiClient *stream = http.getStreamPtr();
        
        // Write data to SPIFFS file
        uint8_t buff[1024];
        size_t size = http.getSize();
        size_t remaining = size;

        while(http.connected() && (remaining > 0)) {
            
            size_t available = stream->available();
            size_t bytesToRead = min(available, sizeof(buff));
            
            if(bytesToRead > 0) {
                size_t bytesRead = stream->readBytes(buff, bytesToRead);
                if(bytesRead > 0) {
                    file.write(buff, bytesRead);
                    remaining -= bytesRead;
                }
            }
            yield(); 
        }

        file.close();
        Serial.println("Album art saved to SPIFFS");
    } else {
        Serial.print("Error downloading image: ");
        Serial.println(httpResponseCode);
    }
    http.end();
}




void updatedisplay(const char* Track, const char* Artist, const char* Album) {
    static String lastTrack = "";
    static String lastArtist = "";
    static String lastAlbum = "";

    // Only update if the song has changed
    if (lastTrack != Track || lastArtist != Artist || lastAlbum != Album) {
        tft.fillScreen(TFT_BLACK); 

        // Update album art
        if(SPIFFS.exists("/albumArt.jpg")) {
            // Initialize JPEG decoder
            TJpgDec.setJpgScale(imageScaleSize);    
            TJpgDec.setSwapBytes(true);
            TJpgDec.setCallback(tft_output);

            // Calculate center position for image
            int imageX = (tft.width() - 150) / 2;
            int imageY = 20;

            // Draw the JPEG image from SPIFFS
            TJpgDec.drawFsJpg(imageX, imageY, "/albumArt.jpg");
        }

        // Update text
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);

        tft.setCursor(10, 200);
        tft.print("Track: ");
        tft.println(Track);

        tft.setCursor(10, 220);
        tft.print("Artist: ");
        tft.println(Artist);

        tft.setCursor(10, 240);
        tft.print("Album: ");
        tft.println(Album);

        // Update last known values
        lastTrack = Track;
        lastArtist = Artist;
        lastAlbum = Album;
    }

    // Always draw the progress bar
    drawProgressBar();
}


void drawProgressBar() {
    if (songDuration <= 0) return;  

    // Define progress bar dimensions
    int barWidth = tft.width() - 20;  
    int barHeight = 10;
    int barX = 10;  
    int barY = tft.height() - barHeight - 20;  
    int cornerRadius = 4;  

    // Draw background bar (with rounded corners)
    tft.fillRoundRect(barX, barY, barWidth, barHeight, cornerRadius, TFT_DARKGREY);

    // Calculate and draw progress
    float progress = (float)currentSongPosition / songDuration;
    int progressWidth = (int)(barWidth * progress);
    
    // Draw the progress bar (with rounded corners)
    if (progressWidth > 0) {
        tft.fillRoundRect(barX, barY, progressWidth, barHeight, cornerRadius, TFT_GREEN);
    }

    // Draw border (with rounded corners)
    tft.drawRoundRect(barX, barY, barWidth, barHeight, cornerRadius, TFT_WHITE);
    
    int elapsedSeconds = currentSongPosition / 1000;
    int totalSeconds = songDuration / 1000;
    
    
    tft.fillRect(barX, barY - 15, barWidth, 15, TFT_BLACK);
    
    // Set text properties before drawing
    tft.setTextColor(TFT_WHITE, TFT_BLACK);  
    tft.setTextSize(1);
    tft.setCursor(barX, barY - 15);
    
    
    sprintf(timeBuffer, "%02d:%02d / %02d:%02d",
            (elapsedSeconds / 60) % 60, elapsedSeconds % 60,
            (totalSeconds / 60) % 60, totalSeconds % 60);
    
    tft.print(timeBuffer);
}


void setup() {
    Serial.begin(115200);
    
    if(!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    connectToWiFi();
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    // JPEG decoder
    TJpgDec.setJpgScale(imageScaleSize);
    TJpgDec.setCallback(tft_output);
    
    updatedisplay(trackName, artistName, albumName);
}

void loop() {
    unsigned long currentMillis = millis();
    
    // Update track info every interval
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        getCurrentPlayingTrack();
    }
    
    if (currentMillis - lastProgressUpdate >= 500) {  // Update every 500ms
        lastProgressUpdate = currentMillis;
        if (songDuration > 0) {
            drawProgressBar();
        }
    }
}
