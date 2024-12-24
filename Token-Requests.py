import requests
import webbrowser
from urllib.parse import urlencode, urlparse, parse_qs

# Spotify API credentials
CLIENT_ID = 'YOUR_CLIENT_ID'
CLIENT_SECRET = 'YOUR_CLIENT_SECRET'
REDIRECT_URI = 'YOUR_REDIRECT_URI'
SCOPE = 'user-read-private user-read-email'  # Adjust scopes as needed

# Step 1: Get Authorization Code
def get_authorization_code():
    auth_url = 'https://accounts.spotify.com/authorize?' + urlencode({
        'response_type': 'code',
        'client_id': CLIENT_ID,
        'redirect_uri': REDIRECT_URI,
        'scope': 'user-read-private user-read-email user-read-playback-state user-modify-playback-state user-read-currently-playing'
    })

    # Open the authorization URL in the web browser
    webbrowser.open(auth_url)
    print("Please authorize the application in the browser and copy the URL you are redirected to.")

    # Wait for the user to authorize and paste the redirected URL
    redirected_url = input("Paste the redirected URL here: ")
    parsed_url = urlparse(redirected_url)
    code = parse_qs(parsed_url.query)['code'][0]
    return code

# Step 2: Exchange Authorization Code for Access and Refresh Tokens
def exchange_code_for_tokens(code):
    token_url = 'https://accounts.spotify.com/api/token'
    payload = {
        'grant_type': 'authorization_code',
        'code': code,
        'redirect_uri': REDIRECT_URI,
        'client_id': CLIENT_ID,
        'client_secret': CLIENT_SECRET
    }

    response = requests.post(token_url, data=payload)
    tokens = response.json()

    if 'error' in tokens:
        print("Error obtaining tokens:", tokens['error'])
        return None, None

    access_token = tokens['access_token']
    refresh_token = tokens['refresh_token']
    print("Access Token:", access_token)
    print("Refresh Token:", refresh_token)
    return access_token, refresh_token

# Main function
def main():
    code = get_authorization_code()
    access_token, refresh_token = exchange_code_for_tokens(code)

    # You can now use access_token and refresh_token as needed in the main .ino file
    # preferably save them to a .env file for security

if __name__ == '__main__':
    main()
