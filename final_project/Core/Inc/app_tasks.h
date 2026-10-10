#ifndef APP_TASKS_H
#define APP_TASKS_H

/**
 * @brief 建立所有 subscriber queue、呼叫 subscribe()、建立所有 FreeRTOS task。
 *        必須在 pubsub_init() 之後、vTaskStartScheduler() 之前呼叫。
 */
void start_app_tasks(void);

#endif /* APP_TASKS_H */
