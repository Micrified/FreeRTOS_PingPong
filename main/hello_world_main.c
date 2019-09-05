#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"


#define A_BIT   0x1
#define B_BIT   0x2

TaskHandle_t handle_task_a;
TaskHandle_t handle_task_b;

void a_task (void *pvParameters) {
    BaseType_t xResult;
    uint32_t value;

    while (1) {

        // Block waiting for notification
        xResult = xTaskNotifyWait(pdFALSE, // Don't clear bits on entry
                                 A_BIT, // Clear own bit on exit
                                 &value, // Save value 
                                 portMAX_DELAY); // Block indefinitely

        if (xResult != pdPASS) {
            fprintf(stderr, "Err: Task A bad notify!\n");
            continue;
        }

        // Otherwise wait a second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Ignore if bit not set
        if ((value & A_BIT) == 0) {
            continue;
        }

        // Show message
        printf("Task A: Ping!\n");

        // Dispatch notification to task B
        xTaskNotify(handle_task_b, B_BIT, eSetBits);
    }
}

void b_task (void *pvParameters) {
    BaseType_t xResult;
    uint32_t value;

    while (1) {

        // Block waiting for notification
        xResult = xTaskNotifyWait(pdFALSE, // Don't clear bits on entry
                                 B_BIT, // Clear own bit on exit
                                 &value, // Save value 
                                 portMAX_DELAY); // Block indefinitely

        if (xResult != pdPASS) {
            fprintf(stderr, "Err: Task B bad notify!\n");
            continue;
        }

        // Otherwise wait a second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Ignore if bit not set
        if ((value & B_BIT) == 0) {
            continue;
        }

        // Show message
        printf("Task B: Pong!\n");

        // Dispatch notification to task A
        xTaskNotify(handle_task_a, A_BIT, eSetBits);
    }
}


void app_main(void)
{

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    // Create task A 
    if(xTaskCreate(a_task, "Task A", 8096, NULL, tskIDLE_PRIORITY, &handle_task_a) != pdPASS) {
        fprintf(stderr, "Error creating task A!\n");
        return;
    }

    // Create task B
    if (xTaskCreate(b_task, "Task B", 8096, NULL, tskIDLE_PRIORITY, &handle_task_b) != pdPASS) {
        fprintf(stderr, "Error creating task B!\n");
        return;
    }

    // Kick off by notifying task A
    xTaskNotify(handle_task_a, A_BIT, eSetBits);

}
