#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <wininet.h>
#pragma comment(lib, "wininet.lib") 

// Function to check if a process is running
bool IsProcessRunning(const std::wstring & processName)
{
    bool isRunning = false;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &processEntry))
        {
            do
            {
                if (processName == processEntry.szExeFile)
                {
                    isRunning = true;
                    break;
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }
    }
    CloseHandle(hSnapshot);
    return isRunning;
}

bool CheckIfGameRunning(const std::filesystem::path& cleanupFile)
{
    if (!IsProcessRunning(L"VRChat.exe"))
    {
        std::cout << "VRChat no longer running, exiting!" << std::endl;
        if (std::filesystem::exists(cleanupFile))
        {
            std::filesystem::remove(cleanupFile);
            std::cout << "Cleaned up file: " << cleanupFile << std::endl;
        }
        return false;
    }
    return true;
}


bool IsFileInUse(const std::filesystem::path& filePath)
{
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return GetLastError() == ERROR_SHARING_VIOLATION;
    }
    CloseHandle(hFile);
    return false;
}

std::string GetDefaultWebBrowser()
{
    HKEY hKey;
    const char* subKey = R"(Software\Microsoft\Windows\Shell\Associations\UrlAssociations\http\UserChoice)";
    const char* valueName = "ProgId";
    char value[256];
    DWORD valueLength = sizeof(value);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, valueName, nullptr, nullptr, (LPBYTE)value, &valueLength) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            std::string progId(value);
            if (progId.find("ChromeHTML") != std::string::npos)
                return "chrome";
            else if (progId.find("FirefoxURL") != std::string::npos)
                return "firefox";
            else if (progId.find("MSEdgeHTM") != std::string::npos)
                return "edge";
            else if (progId.find("VivaldiHTM") != std::string::npos)
                return "vivaldi";
            else if (progId.find("OperaHTML") != std::string::npos)
                return "opera";
            else if (progId.find("ChromiumHTM") != std::string::npos)
                return "chromium";
            else if (progId.find("BraveHTML") != std::string::npos)
                return "brave";
            else
                return {};
        }
        RegCloseKey(hKey);
    }
    return {};
}

int main()
{
    std::cout << "Checking if VRChat is running." << std::endl;

    int count = 0;
    while (!IsProcessRunning(L"VRChat.exe"))
    {
        if (count >= 30) // seconds
        {
            std::cout << "Exceeded maximum wait time. VRChat.exe not found." << std::endl;
            Sleep(5000);
            return 0;
        }
        if (!count)
        {
            std::cout << "VRChat.exe not open. Waiting 30 seconds." << std::endl;
        }
        Sleep(1000);
        count++;
    }

    std::cout << "VRChat found." << "\n" << std::endl;

    // Retrieve the AppData path using _dupenv_s
    char* appDataPath = nullptr;
    size_t len = 0;
    if (_dupenv_s(&appDataPath, &len, "LOCALAPPDATA") != 0 || appDataPath == nullptr)
    {
        std::cerr << "Error: Unable to retrieve LOCALAPPDATA environment variable." << std::endl;
        return 0;
    }

    std::string appDataPathStr(appDataPath);
    free(appDataPath); // Free the allocated memory
    std::filesystem::path ytDlpConfig = std::filesystem::weakly_canonical(appDataPathStr + R"(\..\Roaming\yt-dlp\config)");
    if (std::filesystem::exists(ytDlpConfig))
    {
        std::ifstream file(ytDlpConfig);
        if (!file.is_open())
        {
            std::cerr << "Error opening file: " << ytDlpConfig << "\n" << std::endl;
            return 0;
        }
        std::cout << "yt-dlp config found at " << ytDlpConfig << "\nFile Contents:" << std::endl;
        std::ostringstream buffer;
        std::string line;
        bool containsCookiesFromBrowser = false;
        bool containsSleepRequests = false;
        bool containsMinSleepInterval = false;
        bool containsMaxSleepInterval = false;
        // Read the file and check for parameters
        while (std::getline(file, line))
        {
            buffer << line << "\n";
            std::cout << line << std::endl;
            if (line.find("--cookies-from-browser") != std::string::npos)
            {
                containsCookiesFromBrowser = true;
            }
            if (line.find("--sleep-requests") != std::string::npos)
            {
                containsSleepRequests = true;
            }
            if (line.find("--min-sleep-interval") != std::string::npos)
            {
                containsMinSleepInterval = true;
            }
            if (line.find("--max-sleep-interval") != std::string::npos)
            {
                containsMaxSleepInterval = true;
            }
        }
        std::cout << std::endl;
        file.close();
        // Open the file for writing and prepend missing parameters
        std::ofstream outFile(ytDlpConfig, std::ios::trunc);
        if (!outFile.is_open())
        {
            std::cerr << "Error opening file for writing: " << ytDlpConfig << "\n" << std::endl;
            return 0;
        }
        if (!containsCookiesFromBrowser)
        {
            std::cout << "--cookies-from-browser parameter not located in config. Checking system registry for default web browser ProgId." << std::endl;
            std::string defaultBrowser = GetDefaultWebBrowser();
            if (defaultBrowser.empty())
            {
                std::cerr << "ERROR: Failed to detect default web browser from registry or browser not recognized." << std::endl;
                return 0;
            }
            std::cout << "Browser detected as: " << defaultBrowser << std::endl;
            outFile << "--cookies-from-browser " << defaultBrowser << " ";
            std::cout << "\"--cookies-from-browser " << defaultBrowser << "\" prepended to the yt-dlp config file." << std::endl;
        }
        if (!containsSleepRequests)
        {
            outFile << "--sleep-requests 1.5 ";
            std::cout << "\"--sleep-requests 1.5\" prepended to the yt-dlp config file." << std::endl;
        }
        if (!containsMinSleepInterval)
        {
            outFile << "--min-sleep-interval 15 ";
            std::cout << "\"--min-sleep-interval 15\" prepended to the yt-dlp config file." << std::endl;
        }
        if (!containsMaxSleepInterval)
        {
            outFile << "--max-sleep-interval 45 ";
            std::cout << "\"--max-sleep-interval 45\" prepended to the yt-dlp config file." << std::endl;
        }
        // Write the original content after the prepended parameters
        outFile << buffer.str();
        outFile.close();
    }
    else
    {
        std::cout << "yt-dlp config file not found. Creating default config..." << "\n" << std::endl;

        std::string defaultBrowser = GetDefaultWebBrowser();
        if (defaultBrowser.empty())
        {
            std::cerr << "ERROR: Failed to detect default web browser from registry or browser not recognized." << std::endl;
            return 0;
        }
        std::ofstream outFile(ytDlpConfig);
        if (!outFile.is_open())
        {
            std::cerr << "Error creating file: " << ytDlpConfig << "\n" << std::endl;
            return 0;
        }
        // Write the default configuration with the default browser
        outFile << "--cookies-from-browser " << defaultBrowser << " --sleep-requests 1.5 --min-sleep-interval 15 --max-sleep-interval 45";
        outFile.close();
    }

    if (!IsProcessRunning(L"VRChat.exe"))
    {
        std::cout << "\nVRChat no longer running, exiting!" << std::endl;
        return 0;
    }

    // Construct full paths and check files
    const std::filesystem::path ytDlpPath = std::filesystem::weakly_canonical(appDataPathStr + R"(\..\LocalLow\VRChat\VRChat\Tools\yt-dlp.exe)");
    if (std::filesystem::exists(ytDlpPath))
    {
        std::filesystem::remove(ytDlpPath);
        std::cout << "\nDeleting VRChat's custom YT-DLP from " << ytDlpPath << std::endl;
    }

    if (std::filesystem::exists(ytDlpPath.parent_path() / "yt-dlp-latest.exe"))
    {
        std::filesystem::remove(ytDlpPath.parent_path() / "yt-dlp-latest.exe");
        std::cout << "Deleting stale official YT-DLP from previous run." << std::endl;
    }

    if (!ytDlpPath.empty())
    {
        std::cout << "Downloading latest yt-dlp.exe from https://github.com/yt-dlp/yt-dlp" << std::endl;

        const std::string downloadUrl = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe";
        const std::filesystem::path destinationPath = ytDlpPath.parent_path() / "yt-dlp-latest.exe";
        // Use WinINet or another library to download the file
        HINTERNET hInternet = InternetOpen(L"yt-dlp-downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hInternet)
        {
            HINTERNET hUrl = InternetOpenUrl(hInternet, std::wstring(downloadUrl.begin(), downloadUrl.end()).c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (hUrl)
            {
                std::ofstream outFile(destinationPath, std::ios::binary);
                if (outFile.is_open())
                {
                    char buffer[4096];
                    DWORD bytesRead;
                    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
                    {
                        outFile.write(buffer, bytesRead);
                    }
                    outFile.close();
                    std::cout << "yt-dlp.exe Download complete." << std::endl;
                }
                else
                {
                    std::cerr << "Failed to open file for writing: " << destinationPath << std::endl;
                }
                InternetCloseHandle(hUrl);
            }
            else
            {
                std::cerr << "Failed to open URL: " << downloadUrl << std::endl;
            }
            InternetCloseHandle(hInternet);
        }
        else
        {
            std::cerr << "Failed to initialize WinINet." << std::endl;

        }
    }


    //cleanup downloads leftover in INetCache 
    std::filesystem::path targetDirectory = std::filesystem::path(appDataPathStr) / "Microsoft\\Windows\\INetCache\\IE";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(targetDirectory))
    {
        if (entry.is_regular_file() && entry.path().filename().string().rfind("yt-dlp", 0) == 0) // Check if file starts with "yt-dlp"
        {
            bool warned = false;
            while (IsFileInUse(entry.path()))
            {
                if (!warned)
                {
                    warned = true;
                    std::cout << "Leftover file is still in use. Waiting..." << std::endl;
                }
                Sleep(100); //wait 1/10th of a second.
            }
            std::filesystem::remove(entry.path());
            std::cout << "Cleaned up leftover yt-dlp download in INetCache: " << entry.path() << std::endl;
        }
    }

    int duration = 0;
    std::cout << "\nWaiting for VRChat to regenerate its custom YT-DLP." << std::endl;

    while (!std::filesystem::exists(ytDlpPath))
    {
        if (!CheckIfGameRunning(ytDlpPath.parent_path() / "yt-dlp-latest.exe"))
        {
            return 0;
        }
        if (duration > 15)
        {
            std::cout << "Waited a minute without VRChat replacing YT-DLP, proceeding." << "\n" << std::endl;
            break;
        }
        Sleep(1000);
        duration++;
    }

    if (std::filesystem::exists(ytDlpPath))
    {
        bool warned = false;
        while (IsFileInUse(ytDlpPath))
        {
            if (!warned)
            {
                warned = true;
                std::cout << "Waiting for VRChat to finish writing its custom YT-DLP." << std::endl;
            }
            if (!CheckIfGameRunning(ytDlpPath.parent_path() / "yt-dlp-latest.exe"))
            {
                return 0;
            }
            Sleep(100); //wait 1/10 of a second.
        }
        
        std::filesystem::remove(ytDlpPath);
        std::cout << "Deleting VRChat's custom YT-DLP again lol." << std::endl;
    }

    if (std::filesystem::exists(ytDlpPath.parent_path() / "yt-dlp-latest.exe"))
    {
        std::filesystem::rename(ytDlpPath.parent_path() / "yt-dlp-latest.exe", ytDlpPath.parent_path() / "yt-dlp.exe");
        std::cout << "Finished replacing VRChat's YT-DLP with the official version." << std::endl;
    }

    return 1;
}
