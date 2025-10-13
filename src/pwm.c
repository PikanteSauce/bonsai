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