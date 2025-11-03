#ifndef CONFIG_H
#define CONFIG_H

#include "driver/ledc.h"   

/*****************************************************
 *                 PARAMÈTRES GÉNÉRAUX
 *****************************************************/
#define TAILLE_TAB              20
#define VOLTAGE                 3.3
#define MAX_RESOLUTION          4095
#define HUMIDITE_TOLERANCE_CM   2.0

/*****************************************************
 *                 LEDS
 *****************************************************/
#define LED_RED                 41
#define LED_GREEN               17

/*****************************************************
 *                 POMPE / PWM
 *****************************************************/
#define LEDC_OUTPUT_IO          5
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY          4000
#define LEDC_DUTY               4096
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE

/*****************************************************
 *                 CAPTEUR ULTRASON
 *****************************************************/
#define TRIG                    45
#define ECHO                    47
#define DISTANCE_MAX            50      // cm
#define VITESSE_SON             340.0   // m/s
#define TPS_DISTANCE_S          10      // secondes

/*****************************************************
 *                 CAPTEUR D’HUMIDITÉ
 *****************************************************/
#define SENSOR_ANALOG_PIN       5
#define SENSOR_ANALOG_CHANNEL   ADC_CHANNEL_4
#define TPS_HUMIDITE_S          5       // secondes

/*****************************************************
 *                 TEMPS / DELAIS
 *****************************************************/
#define DELAI_LED_ROUGE_MS      100
#define DELAI_LED_VERTE_MS      50

#endif

