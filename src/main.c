//      Librairies standard
#include <stdio.h>
#include <stdlib.h>
//      Librairies ESP-IDF
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_intr_alloc.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "esp_timer.h"

//      Librairies FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

// Personnal libs
#include "utils.h"
#include "config.h"

//      Manageurs de tâches
TaskHandle_t rangingSensorTaskHandle = NULL;
TaskHandle_t capteurHumiditeTaskHandle = NULL;
TaskHandle_t moteurArrosageTaskHandle = NULL;

//      Nom
static const char* TAG = "example";
//      Callbacks
static void humidite_callback(TimerHandle_t xTimer);
static void distance_callback(TimerHandle_t xTimer);

 
//      Fonctions prototypes
void setup();
void capteurHumidite(void *arg);
void capteurDistance(void *arg);
void pompeArrosage(void* arg);

// TODO : supprimer les boucle while(1) et remplacer par des timers et callbacks
//      Programme main
void app_main(){
    setup();
    xTaskCreate(capteurDistance,"capteurDistance", 4096, NULL, 7, &rangingSensorTaskHandle);
    xTaskCreate(capteurHumidite, "capteurHumidite", 4096, NULL, 8, &capteurHumiditeTaskHandle);
    xTaskCreate(pompeArrosage, "pompeArrosage", 4096, NULL, 9, &moteurArrosageTaskHandle);


    /* GESTION DU TIMER */
    TimerHandle_t HumiditeTimer = xTimerCreate(
        "humiditeTimer",                            // Nom
        pdMS_TO_TICKS(TPS_HUMIDITE_S * 1000),       // Période (ms)
        pdTRUE,                                     // Périodique
        (void*)0,                                   // ID (optionnel)
        humidite_callback                           // Callback
    );
    if (HumiditeTimer == NULL) {
        ESP_LOGE(TAG, "Erreur de création du timer capteur humidite !");
        return;
    }
    TimerHandle_t DistanceTimer = xTimerCreate(
        "DistanceTimer",                            // Nom
        pdMS_TO_TICKS(TPS_DISTANCE_S * 1000),       // Période (ms)
        pdTRUE,                                     // Périodique
        (void*)1,                                   // ID (optionnel)
        distance_callback                           // Callback
    );

    if (DistanceTimer == NULL) {
    ESP_LOGE(TAG, "Erreur de création du timer capteur de distance !");
        return;
    }
     // Démarrage des timers
    if (xTimerStart(HumiditeTimer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Erreur au démarrage du timer !");
    }   
    if (xTimerStart(DistanceTimer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Erreur au démarrage du timer !");
    }   
}


//      Initialisation des PINS
void setup(){
    // TRIG
    gpio_reset_pin(TRIG);
    gpio_set_direction(TRIG, GPIO_MODE_OUTPUT);
    // ECHO
    gpio_reset_pin(ECHO);
    gpio_set_direction(ECHO, GPIO_MODE_INPUT);
    //  HUMIDITE
    gpio_reset_pin(SENSOR_ANALOG_PIN);
    gpio_set_direction(SENSOR_ANALOG_PIN, GPIO_MODE_INPUT);
    //  MOTEUR
    gpio_reset_pin(PUMP_PIN);
}

void capteurDistance(void *arg){
    static const char *TAG = "ULTRASON";
    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
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
        // vTaskDelete(NULL); // supprime la tâche   
    }

}   


void capteurHumidite(void *arg){
    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
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
        // while(1) {
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
        if (mesureFiltree < 60.0) {
           xTaskNotifyGiveIndexed(moteurArrosageTaskHandle, 0 ); 
        }
        // }
        adc_oneshot_del_unit(adc1_handle);      // supprime le handler pour l'ADC, libère la ressource
        // vTaskDelete(NULL);                   // supprime la tâche
    }
}

void pompeArrosage(void* arg) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = PUMP_MODE,
        .duty_resolution  = PUMP_DUTY_RES,
        .timer_num        = PUMP_TIMER,
        .freq_hz          = PUMP_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK 
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = PUMP_MODE,
        .channel        = PUMP_CHANNEL,
        .timer_sel      = PUMP_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PUMP_PIN,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel); 
    // ledc_set_duty(PUMP_MODE, PUMP_CHANNEL, 2000);
    // ledc_update_duty(PUMP_MODE, PUMP_CHANNEL);
    ledc_stop(PUMP_MODE, PUMP_CHANNEL, 0); 
    int32_t step = 200;
    for(;;) {
        // Fade in
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        ledc_set_duty(PUMP_MODE, PUMP_CHANNEL, 8000);
        ledc_update_duty(PUMP_MODE, PUMP_CHANNEL);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        ledc_stop(PUMP_MODE, PUMP_CHANNEL, 0);

        // for (int duty = 0; duty <= (1 << PUMP_DUTY_RES); duty += step) {
        //     ledc_set_duty(PUMP_MODE, PUMP_CHANNEL, duty);
        //     ledc_update_duty(PUMP_MODE, PUMP_CHANNEL);
        //     vTaskDelay(50 / portTICK_PERIOD_MS);
        // }
        // // Fade out
        // for (int duty = (1 << PUMP_DUTY_RES); duty >= 0; duty -= step) {
        //     ledc_set_duty(PUMP_MODE, PUMP_CHANNEL, duty);
        //     ledc_update_duty(PUMP_MODE, PUMP_CHANNEL);
        //     vTaskDelay(50 / portTICK_PERIOD_MS);
        // }
    }
}

static void humidite_callback(TimerHandle_t xTimer) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(capteurHumiditeTaskHandle, &xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(); // pour forcer le réveil immédiat /* Voir avec le numéro de priorité des tâches, notamment avec le futur serveur web */
}

static void distance_callback(TimerHandle_t xTimer) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(rangingSensorTaskHandle, &xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(); // pour forcer le réveil immédiat /* Voir avec le numéro de priorité des tâches, notamment avec le futur serveur web */
}
