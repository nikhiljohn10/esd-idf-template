#ifndef ADC_H
#define ADC_H

int get_sample_value(adc_oneshot_unit_handle_t adc_handle, adc_channel_t channel, int *out_raw);
adc_oneshot_unit_handle_t setup_light_sensor(adc_channel_t channel);

#endif // ADC_H
