/*!
    \file    main.c
    \brief   ADC oversample and shift 

    \version 2017-02-10, V1.0.0, firmware for GD32F30x
    \version 2018-10-10, V1.1.0, firmware for GD32F30x
    \version 2018-12-25, V2.0.0, firmware for GD32F30x
    \version 2020-09-30, V2.1.0, firmware for GD32F30x
*/


#include "gd32f30x.h"
#include "systick.h"
#include <stdio.h>
#include "main.h"
#include "gd32f303c_eval.h"


#define BOARD_ADC_CHANNEL   ADC_CHANNEL_1
#define ADC_GPIO_PORT_RCU   RCU_GPIOA
#define ADC_GPIO_PORT       GPIOA
#define ADC_GPIO_PIN        GPIO_PIN_1

uint16_t adc_value = 0;

void rcu_config(void);
void gpio_config(void);
void adc_config(void);

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/

int main(void)
{
    /* system clocks configuration */
    rcu_config();
    /* systick configuration */
    systick_config();  
    /* GPIO configuration */
    gpio_config();
    /* ADC configuration */
    adc_config();
    /* configures COM port */
    gd_eval_com_init(EVAL_COM1); 
  
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
      usart_data_transmit(EVAL_COM1, 'A');
    printf("Hello World\r\n");
    while(1){
        adc_flag_clear(ADC0, ADC_FLAG_EOC);
        while(SET != adc_flag_get(ADC0, ADC_FLAG_EOC)){
        }
        
        adc_value = ADC_RDATA(ADC0);
        printf("16 times sample, 4 bits shift: 0x%x\r\n", adc_value);
        delay_1ms(500);        
    }
}

/*!
    \brief      configure the different system clocks
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rcu_config(void)
{
    /* enable GPIOA clock */
    rcu_periph_clock_enable(ADC_GPIO_PORT_RCU);
    /* enable ADC0 clock */
    rcu_periph_clock_enable(RCU_ADC0);
    /* config ADC clock */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV4);
}

/*!
    \brief      configure the GPIO peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
void gpio_config(void)
{
    /* config the GPIO as analog mode */
    gpio_init(ADC_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_MAX, ADC_GPIO_PIN);
}

/*!
    \brief      configure the ADC peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
void adc_config(void)
{
    /* ADC continuous function enable */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);  
    /* ADC trigger config */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE); 
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    /* ADC mode config */
    adc_mode_config(ADC_MODE_FREE); 
    /* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);
 
    /* ADC regular channel config */
    adc_regular_channel_config(ADC0, 0, BOARD_ADC_CHANNEL, ADC_SAMPLETIME_55POINT5);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
  
    /* 16 times sample, 4 bits shift */
    adc_oversample_mode_config(ADC0, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_8B, ADC_OVERSAMPLING_RATIO_MUL256);
    adc_oversample_mode_enable(ADC0);
  
    /* enable ADC interface */
    adc_enable(ADC0);
    delay_1ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);
}
/* retarget the gcc's C library printf function to the USART */
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

int _read(int file, char *data, int len) {
    // wait until we get a receive interrupt
    while(RESET == usart_flag_get(EVAL_COM1, USART_FLAG_RBNE))
        ;
    // receive data
    int i = 0;
    data[i++] = (uint8_t) usart_data_receive(EVAL_COM1);
    // return 1 byte. this really isn't the smartest thing to do
    // (we could wait for more bytes sent after this), 
    // but the upper stdio layer will just again call into
    // this function to read the next byte.
    // we might miss a few bytes though.
    return i;
}

int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }

    for (int i = 0; i < len; i++)
    {
        usart_data_transmit(EVAL_COM1, (uint8_t)data[i]);
        while (RESET == usart_flag_get(EVAL_COM1, USART_FLAG_TBE))
            ;
    }

    // return # of bytes written - as best we can tell
    return len;
}

/* retarget the C library printf function to the USART */
#if 0
int fputc(int ch, FILE *f)
{
    usart_data_transmit(EVAL_COM1, (uint8_t) ch);
    while (RESET == usart_flag_get(EVAL_COM1,USART_FLAG_TBE));
    return ch;
}
#endif