# Change Log

[中文](doc/CHANGELOG/CHANGELOG_zh_CN.md)  

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