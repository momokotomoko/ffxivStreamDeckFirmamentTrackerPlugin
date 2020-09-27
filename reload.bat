taskkill /IM StreamDeck.exe /F
timeout /t 1
call repackage.bat
rmdir /s /q "C:\Users\%USERNAME%\AppData\Roaming\Elgato\StreamDeck\Plugins\com.elgato.ffxivfirmament.sdPlugin"
start "" "Release/com.elgato.ffxivfirmament.streamDeckPlugin"
pause