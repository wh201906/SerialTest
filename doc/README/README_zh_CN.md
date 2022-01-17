# 串口助手
![downloads](https://img.shields.io/github/downloads/wh201906/SerialTest/total)  
跨平台串口测试工具  

[English](../../README.md)  

# 特点
+ 可在Windows, Ubuntu, Android, Raspbian上使用  
+ 快速响应  
（可通过关闭接收框实时显示进一步提速）  
+ 低内存占用  
（在原始数据导出模式下接收字节数与内存消耗约为1:1）  
（导出大量串口数据时很有用）  
+ 导出原始二进制数据/选中文本数据  
+ 实时绘图  
+ 自定义控制面板  
+ 支持Android蓝牙串口  

# 预览
![port](../previews/port_zh_CN.png)  
![port_android](../previews/port_android_zh_CN.jpg)  
![data](../previews/data_zh_CN.png)  
![plot](../previews/plot_zh_CN.png)  

[更多预览](../previews/previews_zh_CN.md)  

## 手动编译
编译时需要[手动下载](https://www.qcustomplot.com/release/2.1.0fixed/QCustomPlot-source.tar.gz)qcustomplot.c和qcustomplot.h并放到/src目录下编译。  