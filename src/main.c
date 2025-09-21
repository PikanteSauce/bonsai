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
//      Leds classiques
#define LED_RED                 41
#define LED_GREEN               17
//      signal PWM
#define LEDC_OUTPUT_IO          5
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // résolution sur 13 bits
#define LEDC_DUTY               (4096) // Cycle à 50% de sa valeur max (2 ** 13) * 50% = 4096, évite le dépassement d'index du registre
#define LEDC_FREQUENCY          (4000) // Fréquence à 4kHz
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
//      Capteur ultrasons
#define TRIG                    45
#define ECHO                    47
#define DISTANCE_MAX            50 // en centimètres
#define VITESSE_SON             340 // en m.s⁻1
//      Capteur d'humidité
#define SENSOR_ANALOG_PIN       5  // Capteur relié à la PIN 5
#define SENSOR_ANALOG_CHANNEL  ADC_CHANNEL_4 // Voix ADC pour la PIN 5
#define MAX_RESOLUTION          4095 // résolution capteur en bits (12, selon datasheet esp32)
#define VOLTAGE                 3.3 // alimentation du capteur d'humidite
//      fonction trierTableau
#define TAILLE_TAB              20

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
void trierTableau(float *tab, int taille);
float filtreMedianeMoy(float* tab, float tolDistance);
float conversion(int valeur);

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

//      Tâche led bleue avec signal PWM (gestion intensité éclairage)
// void ledTaskBlue(void *arg){
//     ledc_timer_config_t ledc_timer = {
//         .speed_mode       = LEDC_MODE,
//         .duty_resolution  = LEDC_DUTY_RES,
//         .timer_num        = LEDC_TIMER,
//         .freq_hz          = LEDC_FREQUENCY,
//         .clk_cfg          = LEDC_AUTO_CLK 
//     };
//     ledc_timer_config(&ledc_timer);

//     ledc_channel_config_t ledc_channel = {
//         .speed_mode     = LEDC_MODE,
//         .channel        = LEDC_CHANNEL,
//         .timer_sel      = LEDC_TIMER,
//         .intr_type      = LEDC_INTR_DISABLE,
//         .gpio_num       = LEDC_OUTPUT_IO,
//         .duty           = 0,
//         .hpoint         = 0
//     };
//     ledc_channel_config(&ledc_channel);  

//     int step = 200;
//     while (1) {
//         // Fade in
//         for (int duty = 0; duty <= (1 << LEDC_DUTY_RES); duty += step) {
//             ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
//             ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
//             vTaskDelay(50 / portTICK_PERIOD_MS);
//         }
//         // Fade out
//         for (int duty = (1 << LEDC_DUTY_RES); duty >= 0; duty -= step) {
//             ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
//             ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
//             vTaskDelay(50 / portTICK_PERIOD_MS);
//         }
//     }
// }

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

// tri d'un tableau de float par ordre croissant de taile donnée
void trierTableau(float *tab, int taille) {
    int i, j, temp;
    for (i = 0; i < taille - 1; i++) {
        for (j = 0; j < taille - i - 1; j++) {
            if (tab[j] > tab[j + 1]) {
                // Échange
                temp = tab[j];
                tab[j] = tab[j + 1];
                tab[j + 1] = temp;
            }
        }
    }
}

float filtreMedianeMoy(float* tab, float tolDistance) {
    /* tolerance à donnée en cm */
    float mediane = tab[TAILLE_TAB/2];
    float borneMin = mediane - tolDistance;
    float borneMax = mediane + tolDistance;
    float mesuresAddi = 0.0;
    int nbMesureAcceptee = 0;
    for (int i = 0; i < (TAILLE_TAB - 1); i++){
        if (tab[i] >= borneMin && tab[i] <= borneMax) {
            mesuresAddi += tab[i];
            nbMesureAcceptee ++;
        }    
    }
    if (nbMesureAcceptee == 0) {
        return 0;
    }
    else {
        return (mesuresAddi / nbMesureAcceptee);
    }  
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

float conversion(int valeur) {
    // conversion de la valeur brute entière vers voltage lu puis vers % d'humidite
    float volts = valeur * (VOLTAGE / MAX_RESOLUTION);  // valeur en volts
    float pourcentHumidite = volts * 100 / 3.3;
    return pourcentHumidite;
}