#ifndef UART_TASK_H
#define UART_TASK_H

#include <Arduino.h>
extern bool inKeypadTestMode;  // Khai báo để uart_task.cpp sử dụng
void uartTask();
void availableCommands();

#endif // UART_TASK_H