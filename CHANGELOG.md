# Change Log

[中文](doc/CHANGELOG/CHANGELOG_zh_CN.md)  

## V0.3.5
+ Improve BLE compatibility by adding support for WriteNoResponse operation (commit 0e5c82ea6c)
+ Improve serial port compatibility by allowing non-fatal errors when setting DTR/RTS in the connecting process (commit cae854bcdd)
+ Add Windows MSVC support for Bluetooth (commit 9760cccda8)
+ Add support for restoring dock widgets status (commit c88b47e1b0)
+ Add an option for enabling/disabling touch scrolling gesture (commit fda6389c31)
+ Replace "√"(U+221A) with "OK" in the status bar for the visual consistency across all platforms (commit 574360343e)
+ DataTab: Keep selection unchanged when appending received data (commit 86508ff76b)
+ Fix a few bugs

## V0.3.4
+ Android: Use adaptive launcher icon (commit da9f2caaf1)

## V0.3.3
+ Add support for specifying service UUID in Bluetooth client mode (commit 1bd23bf4e0)
+ Add support for showing IPv6 addresses on Android (commit 03a0e7938c)
+ Add option for remembering the data to be sent in DataTab (commit 00eb0e2e36)
+ Add option for clearing both received data and graph at the same time (commit a4779028d2)
+ Improve the permission request process on Android
+ Add point-only mode for plotting (commit daf34607c9)
+ Fix the missing separator when appending hex data with timestamp (commit 00eb0e2e36)

## V0.3.2
+ Fix 2 bugs in the Control Panel  
+ Fix a bug in the timestamp (commit c73e12c30ad)  
+ Add support for changing the theme without restarting the app  
+ Add the ability to merge timestamps with short interval (split packets based on timeout)  
+ Android: Add support for sharing a file to SerialTest then send it (requires Qt 5.15.10 or a higher version)  

## V0.3.1
+ Fix a build error (commit 9e9cdc1837)

## V0.3
(Deprecated, please use V0.3.1 or higher)
+ Support Rx timestamp
+ Support hardware serial port on Android (commit 47de23b11a)

## V0.2.4
+ Show the version number properly (commit 7b235fe40e)

## V0.2.3
+ Fix a crash (commit 40b6394681)
+ Fix a bug in AsyncCRC (commit de1933f5ee)
+ Show serial port VID and PID in Hex and Dec (commit b00aebd7bd)
+ Backward compatible to Qt 5.9
+ Refactor serial port history logic
+ More details when failed to open a serial port (commit b6dbd88b54)
+ Update QCustomPlot version to 2.1.1
+ Fix #22: Ignore close event of floating windows (commit c723e319fe)
+ Sync DTR and RTS from UI to serial port (commit ce576e45ed)

## V0.2.2
+ Add dark theme(from https://github.com/ColinDuquesnoy/QDarkStyleSheet)  
+ Add single orientation pinch gesture on plotter  
+ Add some useful snippets for control panel(in `demo/Control/`)  
+ Support dragging config file to control panel  

## V0.2.1
+ Add command line option to set config file path  
+ Fix some bugs in file transceiver  
+ Add touch screen gesture support for some scrollable widgets  

## V0.2
+ Supports TCP client/server, UDP, Bluetooth SPP client/server and Bluetooth BLE
+ All servers support 1:N connections. Sent to/Receive from/Disconnect any client as you want
+ Multiple NIC support. Specify local IP and port for TCP server, TCP client and UDP
+ Supports recording connection history with user specified alias. Click history item to reuse the arguments
+ File transceiver with fast CRC32 calculator and throttling
+ Import/Export configuration files, search configuration files in multiple directories
+ Support escape characters (\uHHHH, \nnn, \r, \n, \t, ...)
+ Change serial port arguments on the fly
+ Better UI, supports window translucency, selecting language, changing font
+ Better performance
+ Better gesture detection in PlotTab on Android
+ Share a piece of text to SerialTest then send it on Android

## V0.1.3 ~ V0.1.4
+ Fix some bugs  

## V0.1.2
+ Easier to use  
+ Use Json to storage control items in CtrlTab
+ Save splitter position in DataTab
+ Support serialport pinouts
+ Fix some bugs  

## V0.1.1
+ Support non-ascii characters in path
+ Support encodings
+ Save user preferences
+ Better UI
+ Hide/Show some graph in PlotTab
+ Find valid data in frame when Plotting
+ Fix some bugs

## V0.1
The first released version
