# Demo | 示例程序

## Directory structure | 目录结构
+ demo/
    + Arduino/
        + control_pwm/
        + plot_cls/
        + plot_performance/
        + plot_realtime/
    + STC8G/
        + plot_realtime/
    + STM32F103C8/
        + CubeMX/
            + plot_realtime/  
        + StdPeriph/  
            + plot_realtime/  
    + STM32G431KB/  
        + plot_realtime/  

## Notes | 提示
Demos with prefix "plot_" is for plot panel  
Demos with prefix "control_" is for control(shortcut) panel  
All demos are implemented on Arduino platform  
For other platforms, please refer to "plot_realtime" to write your code  
Demos on STM32 use printf() to send data in strings. check these projects to know how to use printf() on USART  
I implement itoa() in STC8G/plot_realtime/ project. For more conversion functions, see [this file](https://github.com/wh201906/CubeMX_Lib/blob/main/Module/UTIL/util.c)  

***

含有plot_前缀的示例项目用于演示实时绘图功能  
含有control_前缀的示例项目用于演示快捷发送功能（在“控制”面板当中）  
Arduino文件夹下有所有的示例，其它平台仅提供plot_realtime项目以便于移植  
STM32平台的项目中实现了用printf()函数在USART串口上输出数值的功能  
STC8G平台的项目中实现了itoa()函数，其它各种转换函数可参考[此文件](https://github.com/wh201906/CubeMX_Lib/blob/main/Module/UTIL/util.c)  