#include "cdc_usb_bridge.h"

#include "cdc_nanopb.h"
#include "main.h"
#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

static uint8_t AppCdc_Transmit(uint8_t *buffer, uint16_t length)
{
  return CDC_Transmit_FS(buffer, length);
}

static void AppCdc_RearmReception(void)
{
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, CdcNanopb_RxBuffer());
  (void)USBD_CDC_ReceivePacket(&hUsbDeviceFS);
}

static uint16_t AppCdc_AdcReadChannel(ADC_HandleTypeDef *hadc, uint32_t channel,
                                      uint16_t previous_value)
{
  ADC_ChannelConfTypeDef config = {0};

  config.Channel = channel;
  config.Rank = ADC_REGULAR_RANK_1;
  config.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  config.SingleDiff = ADC_SINGLE_ENDED;
  config.OffsetNumber = ADC_OFFSET_NONE;
  config.Offset = 0;
  if (HAL_ADC_ConfigChannel(hadc, &config) != HAL_OK || HAL_ADC_Start(hadc) != HAL_OK)
  {
    return previous_value;
  }

  if (HAL_ADC_PollForConversion(hadc, 10U) == HAL_OK)
  {
    previous_value = (uint16_t)HAL_ADC_GetValue(hadc);
  }

  (void)HAL_ADC_Stop(hadc);
  return previous_value;
}

static void AppCdc_AdcReadSamples(CdcNanopbAdcSamples *samples)
{
  static CdcNanopbAdcSamples last_samples;

  last_samples.curr_a = AppCdc_AdcReadChannel(&hadc1, ADC_CHANNEL_1, last_samples.curr_a);
  last_samples.curr_b = AppCdc_AdcReadChannel(&hadc1, ADC_CHANNEL_2, last_samples.curr_b);
  last_samples.curr_c = AppCdc_AdcReadChannel(&hadc1, ADC_CHANNEL_3, last_samples.curr_c);
  last_samples.volt_b = AppCdc_AdcReadChannel(&hadc1, ADC_CHANNEL_4, last_samples.volt_b);
  last_samples.volt_a = AppCdc_AdcReadChannel(&hadc2, ADC_CHANNEL_13, last_samples.volt_a);
  last_samples.ntc_pcb = AppCdc_AdcReadChannel(&hadc2, ADC_CHANNEL_3, last_samples.ntc_pcb);
  last_samples.vbus = AppCdc_AdcReadChannel(&hadc2, ADC_CHANNEL_12, last_samples.vbus);
  last_samples.volt_c = AppCdc_AdcReadChannel(&hadc2, ADC_CHANNEL_14, last_samples.volt_c);
  *samples = last_samples;
}


void AppCdc_AdcCalibrate(void){
  if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }
}

void AppCdc_Init(void)
{
	AppCdc_AdcCalibrate();
  CdcNanopb_Init(AppCdc_Transmit, AppCdc_AdcReadSamples);
  AppCdc_RearmReception();
}

void AppCdc_Task(uint32_t now_ms)
{
  if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
  {
    CdcNanopb_Task(now_ms);
  }
}

uint8_t AppCdc_OnReceive(uint8_t *buffer, uint32_t length)
{
  CdcNanopb_OnReceive(buffer, length);
  AppCdc_RearmReception();
  return USBD_OK;
}

void AppCdc_StateIndicator(uint32_t now_ms)
{
	static uint32_t last_led_toggle_ms = 0U;
	if ((uint32_t)(now_ms - last_led_toggle_ms) >= 500U)
	{
		HAL_GPIO_TogglePin(LEDC_GPIO_Port, LEDC_Pin);
		last_led_toggle_ms = now_ms;
	}
}

void AppCdc_OnTransmitComplete(void)
{
  CdcNanopb_OnTransmitComplete();
	AppCdc_StateIndicator(HAL_GetTick());
}
