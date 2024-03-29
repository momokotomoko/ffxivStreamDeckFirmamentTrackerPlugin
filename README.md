# FFXIV Firmament Tracker Plugin for the Elgato StreamDeck

Note: This application is now obsolete, as all the firmament progress in every server has now been completed:
[Ishgardian Restoration Builders' Progress Report](https://na.finalfantasyxiv.com/lodestone/ishgardian_restoration/builders_progress_report/)

However, if in the future fresh servers are added where the restoration has not been completed, then this plugin can still be used.

StreamDeck is an external LCD key macro device that allows the installation of plugins to improve productivity.

Final Fantasy XIV is a MMORPG video game.

## Description

`FFXIV Firmament Tracker Plugin` reads the FFXIV Ishguardian Restoration [Builders' Progress Report](https://na.finalfantasyxiv.com/lodestone/ishgardian_restoration/builders_progress_report) and displays server progress percentages on your StreamDeck.

The website updates every hour, so the plugin updates every 1m after the hour and on initialization.

You can drag and drop multiple plugins to display more than one server if desired.

Clicking the button will open up the Builders' Progress Report web page.

Compatible with Windows.

Note: There is currently no API for reading the firmament progress. Instead, this plugin parses the html page directly. This works for now, but if SE changes the webpage in some significant way, this plugin may stop working until I update it.

![](screenshot.png)

## Installation

In the Release folder, you can find the file `com.elgato.ffxivfirmament.streamDeckPlugin`. If you double-click this file on your machine, StreamDeck will install the plugin.

Folder: https://github.com/momokotomoko/ffxivStreamDeckFirmamentTrackerPlugin/Release

If an installation was previously present, you must delete the plugin folder located in Elgato's AppData folder. For example: `C:\Users\<username>\AppData\Roaming\Elgato\StreamDeck\Plugins`

## Settings

![](settings.png)

`Title`

The title is automatically set by the plugin to display the server name and progress percentage.

`Server Select`

Select a server from the dropdown menu to track.

`Server`

Textbox where you can type the name of the server to track, in case it is not in the dropdown menu. It is case sensitive.

`Button URL`

A custom URL per button can be set such that when the StreamDeck button is pressed, the webpage is opened by the default browser.

By default, the webpage is set to the [Ishgardian Restoration Builders' Progress Report](https://na.finalfantasyxiv.com/lodestone/ishgardian_restoration/builders_progress_report/)

`Image Select`

Select a StreamDeck image from an included set. If set to Default, you can still use the StreamDeck UI to upload your own image as usual.

![](Sources/com.elgato.ffxivfirmament.sdPlugin/Images/Icons/ish1.png) ![](Sources/com.elgato.ffxivfirmament.sdPlugin/Images/Icons/ish2.png) ![](Sources/com.elgato.ffxivfirmament.sdPlugin/Images/Icons/ish3.png)

`Font`

Servers with longer names can use the `T` drop down menu to select a different font size.

`Firmament URL`

An advanced setting that allows the user to specify the location of the firmament progress page in case it ever changes. This is a global setting and it is reflected across all instances of this plugin on the StreamDeck.

## Source Code

The source code can be found in the Sources folder.

## Developed By

[Momoko Tomoko from Sargatanas](https://na.finalfantasyxiv.com/lodestone/character/1525660/)

[Youtube](https://www.youtube.com/channel/UCAqH9TEBLONg22Espxyw-Rg)

[Twitter](https://twitter.com/momoko_tomoko)

[![buymeacoffee](https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png)](https://www.buymeacoffee.com/momokoffxiv)