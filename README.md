# 带键盘事件的计算器（实验 1）

## 功能
- 四则运算（加、减、乘、除），支持小数
- 键盘输入：数字、\+ - * /\、\Enter/= \ 计算、\Backspace\ 退格、\Esc\ 清除
- 鼠标点击与键盘输入复用同一套处理逻辑
- 除数为 0、连续小数点、连续运算符等异常输入处理

## 构建
用 Qt Creator 打开 \Calculator.pro\，选择 Qt 6.9.2 MinGW 64-bit 套件构建运行。

## 文件说明
- \calculator.ui\：界面布局（Qt Designer 可直接打开）
- \calculator.h/.cpp\：计算器状态管理与事件处理
- \style.qss\：界面样式表
