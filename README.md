# VRChat-YT-DLP-Fix
This program fixes VRChat's YT-DLP "Sign in to confirm you're not a bot" warning by replacing their stripped down version with the official version of YT-DLP, allowing VRChat to operate using your actual web-browser's sign in cookies.
_________

### **This requires [VRCX](https://github.com/vrcx-team/VRCX ) to load.**

________

### Usage: 
1) Extract the .exe file wherever you want.
1) Open VRCX, navigate to App Launcher.
1) Turn on the "Enable" slider.
1) Click the Auto-Launch Folder button.
1) Create a shortcut to VRChat-YT-DLP-Fix.exe in this folder. **(If you extract the .exe into this folder, you still have to create a shortcut.)**

And you're set! Everything's automatic from there.
   
![Screenshot 2025-05-31 225412](https://github.com/user-attachments/assets/850cc3a0-4e54-4e40-8b56-96a12c8157c3)

![Screenshot 2025-05-31 225626](https://github.com/user-attachments/assets/8cd42bc6-f27d-48d1-b900-f91d3c3af38d)


- If you are using browser containers (if you don't know what that means, then you most likely aren't using them), you will have to manually edit AppData/Roaming/yt-dlp/config and specify what container you want to pull cookies from - ie `--cookies-from-browser firefox::"youtube tabs"`

--------



#### For a bit more in depth explanation of what this program does;
1) Waits for VRChat to open.
2) Deletes VRChat's custom/stripped down YT-DLP from `AppData\LocalLow\VRChat\VRChat\Tools`
3) Checks if an existing yt-dlp config file exists in `AppData/Roaming/yt-dlp`

    a) If it does exist, it checks if the config is properly set to pull cookies from the default web browser.
    - It also checks if the sleep between downloads feature is enabled (and if it isn't, setting both the min & max sleep times randomizes the delay, which is critical to preventing youtube from flagging the traffic as a scraping bot. 
    
    b) If the config file does not exist, generate one with all the above parameters.
4) Downloads the latest release binary of YT-DLP from [YT-DLP's repo](https://github.com/yt-dlp/yt-dlp) 
5) Waits for VRChat to automatically regenerate its custom YT-DLP file during user login.
6) Delete VRChat's stripped down YT-DLP file (again.)
7) Places the official version of YT-DLP in `AppData\LocalLow\VRChat\VRChat\Tools`, fully replacing VRChat's stripped down version.

--------------

Made by Afevis, released under GPL V3. 

------------
No warranties are made for this software. Using YT-DLP is against Youtube's TOS, which you're already violating by using video players in VRChat - usage may or may not present additional risk by tying your video activity to your actual YouTube account (really it depends on if YouTube feels like being a dick and actually caring.)

