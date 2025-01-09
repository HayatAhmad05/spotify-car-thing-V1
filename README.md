
# Spotify Desktop Assisstant - V1

An ESP32 project allowing you to display your current Spotify song information onto a TFT display, usinf the Spotify API.

- Song information displayed :

- Track Name 

- Artist

- Album

- Album Artwork

- Real-time progress bar

# Hardware Requirements

- ESP32

- ILI9341 TFT Touch display

For V2 a Rotary Encoder will be implemented for Play/Pause and Volume Functionality

# Setup Guide

1) **Spotify Developer Application Setup**

    Go to the Spotify Developer Dashboard

    Create a new application

    Set a Redirect URI (http://localhost:8888/callback)

    Note down your:
    - Client ID
    - Client Secret

2) **Obtaining Access and Refresh Tokens**

Use the **Token-Requests.py** file, add your **Client ID, Client Secret** and  **Redirect URL** to the fields.

Next run the script and it will open a browser tab which you will then copy and paste into your terminal of your IDE. The URL you copied contains the Spotify Authorization code and the script will parse the URL and use the AUTH code to gain the Access and Refreash Tokens.

The terminal will then provide the Access and Refreash Tokens, save these 2 codes somewhere safe, like a .env file.

# Project File Configuration

In the **.ino** file replace the placeholders with your information:

- WIFI SSID
- WIFI Password
- Client ID and Secret
- Refreash and Access Token
- Redirect URL

**Download the following Libraries into your project environment**

These Libraries are crutial for the project and cannot work without them. 

If you are using the Arduino IDE, go to the top left side of the screen, go to **Sketch > Include Library > Manage Libraries.** From here search the following Libraries and install them: 

- WiFi
- HTTPClient
- ArduinoJson
- TFT_eSPI
- TJpg_Decoder
- SPIFFS

# Display Configuration

For any TFT Display to work with an ESP32, the User_Setup.h file needs to be edited to match your pinout for the display and ESP32.

The file can be found in the following location: 

**C:\Users\USERNAME\Documents\Arduino\libraries\TFT_eSPI**

Change the pinout for your TFT Display and save it.

# Troubleshooting

- Ensure all tokens and credentials are correctly copied
- Check WiFi connectivity
- Verify Spotify app permissions
- Ensure you have an active Spotify playback device

For any further qustions or help contact sh2ahmad@uwaterloo.ca








