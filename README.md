# SerialTest
![downloads](https://img.shields.io/github/downloads/wh201906/SerialTest/total)  
A cross-platform serial port test tool.  

[中文介绍](doc/README/README_zh_CN.md)  

## Features
+ Tested on Windows, Ubuntu, Android, Raspbian  
+ Fast response  
(Disable "Realtime" option in "data" tab for higher speed)  
+ Low memory cost  
(around 1:1 memory cost in raw dump mode)  
(useful when dumping serial data)  
+ Rich encoding support  
(UTF-8/16/32, GB18030, BIG5, KOI8-R, EUC-JP, EUC-KR, …)  
+ Remember all preferences  
(All preferences are saved in one file on PC, portable and clean)  
+ Export raw binary data or selected text  
+ Real-time plot  
+ Customized controllers  
+ Android Bluetooth SPP support  

## Previews
![port](doc/previews/port.png)  
![port_android](doc/previews/port_android.jpg)  
![data](doc/previews/data.png)  
![plot](doc/previews/plot.png)  

[more previews](doc/previews/previews.md)  

## Tutorials[WIP]
[1.Connect](doc/tutorials/connect/connect_zh_CN.md)  
[2.Send&Receive Data](doc/tutorials/data/data_zh_CN.md)  
[3.Plot](doc/tutorials/plot/plot_zh_CN.md)  

## Change Log
[Change Log](CHANGELOG.md)

## Build
You need to [download](https://www.qcustomplot.com/release/2.1.0fixed/QCustomPlot-source.tar.gz) qcustomplot.c and qcustomplot.h manually, put them in the /src folder, then build.  