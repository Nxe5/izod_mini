# Evil Portal Plugin

A WiFi captive portal plugin for authorized penetration testing, inspired by the [Flipper Zero Evil Portal project](https://github.com/bigbrodude6119/flipper-zero-evil-portal).

## ⚠️ IMPORTANT DISCLAIMER

**This tool is for educational and authorized testing purposes only.**

- Only use on networks you own or have explicit written permission to test
- Unauthorized use may violate local, state, and federal laws
- Always comply with applicable laws and regulations
- Use responsibly and ethically
- The developers are not responsible for misuse of this tool

## Features

### 🎯 Portal Templates

**Professional Portals (20+ available):**
- **Google Modern**: Modern Google account login
- **Apple ID**: Apple ID authentication portal
- **Microsoft**: Microsoft account login
- **Facebook**: Facebook social login
- **Twitter/X**: Twitter social platform
- **Amazon**: Amazon account portal
- **Starlink**: Starlink internet service
- **T-Mobile, Verizon, AT&T**: Major carrier logins
- **Spectrum**: Internet service provider
- **Airlines**: Southwest, Delta, United, American, JetBlue, Alaska, Spirit
- **Twitch**: Gaming platform login
- **Matrix**: Themed portal
- **Custom**: WiFi, Hotel, Corporate, Social Media, Email templates

All portals are sourced from the [Flipper Zero Evil Portal project](https://github.com/bigbrodude6119/flipper-zero-evil-portal/tree/main/portals) community.

### 🔧 Functionality
- **Selectable Portal Templates**: Choose from pre-built HTML templates
- **Credential Capture**: Automatically logs submitted credentials
- **Access Point Configuration**: Customizable SSID, channel, and security
- **Real-time Monitoring**: Live client count and activity tracking
- **Credential Management**: View, navigate, and save captured data
- **DNS Redirection**: Redirects all DNS queries to the portal
- **Captive Portal Detection**: Handles Android, iOS, and Windows detection

### 📱 User Interface
- **Menu Navigation**: Easy-to-use button-based interface
- **Status Display**: Real-time portal status and statistics
- **Settings Management**: Configure portal behavior and preferences
- **Credential Viewer**: Browse captured login attempts

## Usage

### Navigation Controls
- **Select Button**: Choose options, start/stop portal
- **Previous/Next Buttons**: Navigate through menus and lists
- **Play Button**: Go back to previous menu
- **Menu Button**: Exit plugin

### Basic Operation

1. **Select Portal Template**
   - Choose from available HTML templates
   - Templates are loaded from `/Pentest/portals/` directory

2. **Configure Access Point** (Optional)
   - Set SSID name (default: "Free WiFi")
   - Choose wireless channel (default: 1)
   - Configure security settings

3. **Start Portal**
   - Activates WiFi access point
   - Starts web server and DNS redirection
   - Portal becomes accessible to nearby devices

4. **Monitor Activity**
   - View connected client count
   - Track credential capture attempts
   - Monitor portal status in real-time

5. **View Captured Credentials**
   - Browse submitted login attempts
   - View usernames, passwords, IP addresses
   - See user agent strings and timestamps

6. **Stop Portal**
   - Safely shutdown access point and services
   - Optionally save captured credentials to file

## Portal Templates

### Creating Custom Templates

Portal templates are HTML files stored in `/Pentest/portals/`. To create a custom template:

1. Create an HTML file with your desired portal design
2. Use a GET form with `action="/get"` (standard format)
3. Include `email` and `password` input fields
4. Save the file in the portals directory

**Standard form structure (Flipper Zero Evil Portal format):**
```html
<form action="/get" method="GET">
    <input type="text" name="email" placeholder="Email" required>
    <input type="password" name="password" placeholder="Password" required>
    <button type="submit">Login</button>
</form>
```

**Requirements:**
- Single HTML file with embedded CSS/JS (no external dependencies)
- Maximum 20k characters per file
- Use `name="email"` and `name="password"` for form fields
- Mobile-responsive design recommended

### Template Features
- **Responsive Design**: Mobile-friendly layouts
- **Professional Styling**: Convincing visual design
- **Loading Animations**: Simulated authentication process
- **Input Validation**: Client-side form validation
- **Social Login Options**: Multiple authentication methods

## Technical Details

### Network Configuration
- **IP Range**: 192.168.4.0/24
- **Gateway**: 192.168.4.1
- **DNS Server**: Port 53 (redirects all queries)
- **Web Server**: Port 80 (configurable)
- **WiFi Mode**: Access Point (AP)

### File Structure
```
evil_portal/
├── manifest.json           # Plugin metadata
├── evil_portal.cpp         # Main plugin code
├── README.md              # This documentation
└── portals/               # Portal templates
    ├── wifi_login.html
    ├── hotel_wifi.html
    ├── corporate.html
    ├── social_media.html
    └── email_login.html
```

### Captured Data Format
Credentials are saved to `/Pentest/portals/credentials_[timestamp].txt` with the following format:
```
Entry 1:
Username: user@example.com
Password: password123
IP Address: 192.168.4.100
User Agent: Mozilla/5.0...
Timestamp: 1234567890
```

## Security Considerations

### For Authorized Testing
- Document all testing activities
- Obtain proper written authorization
- Follow responsible disclosure practices
- Respect privacy and data protection laws
- Use only for legitimate security assessments

### Detection Evasion
- Randomize SSID names to avoid suspicion
- Use realistic portal templates
- Monitor for security tools and countermeasures
- Limit testing duration and scope

## Legal Notice

This plugin is provided for educational and authorized security testing purposes only. Users are solely responsible for ensuring compliance with all applicable laws and regulations. The developers disclaim any liability for misuse of this software.

## Contributing

To contribute new portal templates or improvements:

1. Ensure templates are realistic but clearly for testing
2. Follow responsible disclosure principles
3. Test templates across different devices and browsers
4. Document any special features or requirements

## References

- [Flipper Zero Evil Portal](https://github.com/bigbrodude6119/flipper-zero-evil-portal)
- [OWASP Testing Guide](https://owasp.org/www-project-web-security-testing-guide/)
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)
