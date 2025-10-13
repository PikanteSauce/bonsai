//      Librairies
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_intr_alloc.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"

// Personnal libs
#include "utils.h"
#include "config.h"

//      Manageurs de tâches
TaskHandle_t ledTaskHandleRed = NULL;
TaskHandle_t ledTaskHandleGreen = NULL;
TaskHandle_t ledTaskHandleBlue = NULL;
TaskHandle_t rangingSensorTaskHandle = NULL;
TaskHandle_t capteurHumiditeTaskHandle = NULL;
 
//      Fonctions prototypes
void setup();
void ledTaskRed(void *arg);
void ledTaskGreen(void *arg);
// void ledTaskBlue(void *arg);                vTaskDelay(pdMS_TO_TICKS(1000));
void capteurHumidite(void *arg);
void capteurDistance(void *arg);

// TODO : supprimer les boucle while(1) et remplacer par des timers et callbacks
//      Programme main
void app_main(){
    setup();
    xTaskCreatePinnedToCore(ledTaskRed, "led_red", 4096, NULL, 10, &ledTaskHandleRed, 1);
    xTaskCreatePinnedToCore(ledTaskGreen,"led_green", 4096, NULL, 9, &ledTaskHandleGreen, 1);
    // xTaskCreatePinnedToCore(ledTaskBlue,"led_blue", 4096, NULL, 8, &ledTaskHandleBlue, 1);
    xTaskCreatePinnedToCore(capteurDistance,"capteurDistance", 4096, NULL, 7, &rangingSensorTaskHandle, 1);
    xTaskCreatePinnedToCore(capteurHumidite, "capteurHumidite", 4096, NULL, 8, &capteurHumiditeTaskHandle, 1);
}

//      Tâche led rouge
void ledTaskRed(void *arg){
    while (1) {
        gpio_set_level(LED_RED, 0);
        vTaskDelay(100);
        gpio_set_level(LED_RED, 1);
        vTaskDelay(100);
    }   
}

//      Tâche led verte
void ledTaskGreen(void *arg){
    while (1) {
        gpio_set_level(LED_GREEN, 0);
        vTaskDelay(50);
        gpio_set_level(LED_GREEN, 1);
        vTaskDelay(50);
    }
}

void capteurDistance(void *arg){
    static const char *TAG = "ULTRASON";
    while (1) {
        float mesuresBrutes [TAILLE_TAB] = {};
        // float mesuresTriees [20] = {};
        for(int indexMesure = 0; indexMesure < (TAILLE_TAB) - 1; indexMesure ++) {
            // Envoi impulsion TRIG de 10 µs
            gpio_set_level(TRIG, 0);
            esp_rom_delay_us(2);
            gpio_set_level(TRIG, 1);
            esp_rom_delay_us(10);
            gpio_set_level(TRIG, 0);
            // Attente du front montant de ECHO
            int timeout = 30000;
            while (gpio_get_level(ECHO) == 0 && timeout > 0) {
                esp_rom_delay_us(1);
                timeout--;
            }
            if (timeout == 0) {
                ESP_LOGW(TAG, "TIMEOUT DE LA MESURE !");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            int64_t start = esp_timer_get_time();
            timeout = 30000;
            // Attente du front descendant de ECHO
            while (gpio_get_level(ECHO) == 1 && timeout > 0) {
                timeout--;
            }
            if (timeout == 0) {
                ESP_LOGW(TAG, "TIMEOUT DE LA MESURE !");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            int64_t end = esp_timer_get_time();
            // Calcul temps écoulé
            int64_t duration = end - start; // en µs
            // Calcul distance (vitesse son ~340 m/s = 0.034 cm/µs)
            mesuresBrutes[indexMesure] = (duration * 0.034) / 2;
            // ESP_LOGI(TAG, "Distance = %.2f cm", distance_cm);
            vTaskDelay(pdMS_TO_TICKS(50)); // pause en ms
           }
        // tri du tableau
        trierTableau(mesuresBrutes, TAILLE_TAB);
        // filtrage de la valeur
        float mesureFiltree = filtreMedianeMoy(mesuresBrutes, 2.0);
        if (mesureFiltree > DISTANCE_MAX) {
            ESP_LOGW(TAG, "DISTANCE MESUREE TROP GRAND !");
        }
        else {
            ESP_LOGI(TAG, "Distance = %.2f cm", mesureFiltree);
        }
    }
    vTaskDelete(NULL); // supprime la tâche   
}   

//      Initialisation des PINS
void setup(){
    // RED_LED
    gpio_reset_pin(LED_RED);                          // Reset de la conf de la PIN par sécurité
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT); 
    // GREEN_LED
    gpio_reset_pin(LED_GREEN);                        // Reset de la conf de la PIN par sécurité
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT); 
    // BLUE LED
    gpio_reset_pin(LEDC_OUTPUT_IO);
    // TRIG
    gpio_reset_pin(TRIG);
    gpio_set_direction(TRIG, GPIO_MODE_OUTPUT);
    // ECHO
    gpio_reset_pin(ECHO);
    gpio_set_direction(ECHO, GPIO_MODE_INPUT);
    //  HUMIDITE
    gpio_reset_pin(SENSOR_ANALOG_PIN);
    gpio_set_direction(SENSOR_ANALOG_PIN, GPIO_MODE_INPUT);       
}

void capteurHumidite(void *arg){
    int humidite = 0;
    static const char *TAG = "HUMIDITE";
    
    /*                        CREATION DE L'ADC                           */
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12, 
        };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, SENSOR_ANALOG_CHANNEL, &chan_config));

    /*          LECTURE DONNEES BRUTES ET CONVERSION EN VOLTS ET POURCENTAGE             */
    while(1) {
        float mesuresBrutes [TAILLE_TAB] = {};
        for (int i = 0; i < (TAILLE_TAB - 1); i++)
        {
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, SENSOR_ANALOG_CHANNEL, &humidite));
            mesuresBrutes[i] = conversion(humidite);
            vTaskDelay(pdMS_TO_TICKS(50));
        }   
        // tri du tableau
        trierTableau(mesuresBrutes, TAILLE_TAB);
        // filtrage de la valeur
        float mesureFiltree = filtreMedianeMoy(mesuresBrutes, 2.0);
        ESP_LOGI(TAG, "Humidite = %.2f %%", mesureFiltree);
    }
    adc_oneshot_del_unit(adc1_handle);      // supprime le handler pour l'ADC, libère la ressource
    vTaskDelete(NULL);                      // supprime la tâche
}
