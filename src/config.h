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
 *                 POMPE / PWM
 *****************************************************/
#define PUMP_PIN                6
#define PUMP_CHANNEL            LEDC_CHANNEL_0
#define PUMP_DUTY_RES           LEDC_TIMER_13_BIT
#define PUMP_FREQUENCY          1000    // a tester pour 1kHz, puis augmenter jusqu'à 2kHz max
#define PUMP_DUTY               4096
#define PUMP_TIMER              LEDC_TIMER_0
#define PUMP_MODE               LEDC_LOW_SPEED_MODE
typedef enum {
    CMD_ARRETER = 0,                    // 0 = stop
    CMD_DEMARRER = 1,                   // 1 = on
} pompe_cmd_t;
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
 *                 PLANTES
 *****************************************************/
typedef struct {
    int8_t tresSec ;
} TauxHumidite;
               
#endif

