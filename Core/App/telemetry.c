#include "telemetry.h"

#include <limits.h>
#include <math.h>

#include "main.h"
#include "pb_encode.h"
#include "protocol/bldc.pb.h"
#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#define TELEMETRY_INTERVAL_MS 250U
#define TELEMETRY_RX_BUFFER_SIZE 1024U
#define TELEMETRY_PAYLOAD_SIZE bldc_Telemetry_size
#define TELEMETRY_FRAME_SIZE (TELEMETRY_PAYLOAD_SIZE + 2U)
#define PCB_NTC_R25_OHM 10000.0f
#define PCB_NTC_BETA_K 3435.0f
#define PCB_NTC_T25_K 298.15f
#define PCB_NTC_R_FIXED_OHM 1000.0f
#define PCB_NTC_DIVIDER_SUPPLY_MV 3300.0f
#define VM_R_HIGH_OHM 330000.0f
#define VM_R_LOW_OHM 10000.0f
#define PHASE_VOLTAGE_R_HIGH_OHM 91000.0f
#define PHASE_VOLTAGE_R_LOW_OHM 4700.0f
#define ADC_MAX_COUNTS 4095.0f

typedef struct
{
  uint16_t ntc_pcb;
  uint16_t curr_a;
  uint16_t curr_b;
  uint16_t curr_c;
  uint16_t volt_a;
  uint16_t volt_b;
  uint16_t volt_c;
  uint16_t vbus;
} TelemetryAdcSamples;

extern USBD_HandleTypeDef hUsbDeviceFS;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

static uint8_t telemetry_rx_buffer[TELEMETRY_RX_BUFFER_SIZE];
static uint8_t telemetry_payload[TELEMETRY_PAYLOAD_SIZE];
static uint8_t telemetry_frame[TELEMETRY_FRAME_SIZE];
static uint32_t telemetry_next_ms;
static uint32_t telemetry_sequence;
static uint32_t telemetry_prng = 0x6d2b79f5U;
static uint32_t telemetry_last_led_toggle_ms;
static uint8_t telemetry_tx_pending;

static uint32_t Telemetry_Random(void)
{
  uint32_t value = telemetry_prng;

  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  telemetry_prng = value;
  return value;
}

static size_t Telemetry_CobsEncode(const uint8_t *input, size_t input_length,
                                   uint8_t *output, size_t output_capacity)
{
  size_t input_index = 0U;
  size_t output_index = 1U;
  size_t code_index = 0U;
  uint8_t code = 1U;

  if (output_capacity == 0U)
  {
    return 0U;
  }

  while (input_index < input_length)
  {
    if (input[input_index] == 0U)
    {
      if (output_index >= output_capacity) return 0U;
      output[code_index] = code;
      code = 1U;
      code_index = output_index++;
    }
    else
    {
      if (output_index >= output_capacity) return 0U;
      output[output_index++] = input[input_index];
      code++;
      if (code == 0xffU)
      {
        if (output_index >= output_capacity) return 0U;
        output[code_index] = code;
        code = 1U;
        code_index = output_index++;
      }
    }
    input_index++;
  }

  output[code_index] = code;
  return output_index;
}

static uint16_t Telemetry_ReadAdcChannelWithSampling(ADC_HandleTypeDef *hadc, uint32_t channel,
												uint32_t sampling_time, uint16_t previous_value)
{
  ADC_ChannelConfTypeDef config = {0};

  config.Channel = channel;
  config.Rank = ADC_REGULAR_RANK_1;
  config.SamplingTime = sampling_time;
  config.SingleDiff = ADC_SINGLE_ENDED;
  config.OffsetNumber = ADC_OFFSET_NONE;
  if ((HAL_ADC_ConfigChannel(hadc, &config) != HAL_OK) || (HAL_ADC_Start(hadc) != HAL_OK))
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

static uint16_t Telemetry_ReadAdcChannel(ADC_HandleTypeDef *hadc, uint32_t channel,
												uint16_t previous_value)
{
	return Telemetry_ReadAdcChannelWithSampling(hadc, channel, ADC_SAMPLETIME_2CYCLES_5, previous_value);
}

static uint32_t Telemetry_ReadVddaMv(void)
{
  uint16_t vrefint_adc;

  vrefint_adc = Telemetry_ReadAdcChannelWithSampling(&hadc1, ADC_CHANNEL_VREFINT,
                                         ADC_SAMPLETIME_640CYCLES_5, 0U);
  if (vrefint_adc == 0U)
  {
    return 0U;
  }

  return __HAL_ADC_CALC_VREFANALOG_VOLTAGE(vrefint_adc, ADC_RESOLUTION_12B);
}

static int32_t Telemetry_NtcPcbTemperatureCdec(uint16_t adc_raw, uint32_t vdda_mv)
{
  float resistance_ohm;
  float temperature_c;
  float ntc_voltage_mv;

  if ((adc_raw == 0U) || (vdda_mv == 0U) || (adc_raw >= ADC_MAX_COUNTS))
  {
    return INT32_MIN;
  }

  ntc_voltage_mv = (float)adc_raw * (float)vdda_mv / ADC_MAX_COUNTS;
  if ((ntc_voltage_mv <= 0.0f) || (ntc_voltage_mv >= PCB_NTC_DIVIDER_SUPPLY_MV))
  {
    return INT32_MIN;
  }

  resistance_ohm = PCB_NTC_R_FIXED_OHM *
      (PCB_NTC_DIVIDER_SUPPLY_MV / ntc_voltage_mv - 1.0f);
  temperature_c = (1.0f / ((1.0f / PCB_NTC_T25_K) +
      logf(resistance_ohm / PCB_NTC_R25_OHM) / PCB_NTC_BETA_K)) - 273.15f;
  if (!isfinite(temperature_c))
  {
    return INT32_MIN;
  }

  return (int32_t)(temperature_c * 10.0f +
      ((temperature_c >= 0.0f) ? 0.5f : -0.5f));
}

static uint32_t Telemetry_DividerVoltageMv(uint16_t adc_raw, uint32_t vdda_mv,
                                           float high_side_ohm, float low_side_ohm)
{
  float adc_voltage_mv;

  if (vdda_mv == 0U)
  {
    return 0U;
  }

  adc_voltage_mv = (float)adc_raw * (float)vdda_mv / ADC_MAX_COUNTS;
  return (uint32_t)(adc_voltage_mv * ((high_side_ohm + low_side_ohm) / low_side_ohm) +
      0.5f);
}

static TelemetryAdcSamples Telemetry_ReadAdcs(void)
{
  static TelemetryAdcSamples samples;

  samples.curr_a = Telemetry_ReadAdcChannel(&hadc1, ADC_CHANNEL_1, samples.curr_a);
  samples.curr_b = Telemetry_ReadAdcChannel(&hadc1, ADC_CHANNEL_2, samples.curr_b);
  samples.curr_c = Telemetry_ReadAdcChannel(&hadc1, ADC_CHANNEL_3, samples.curr_c);
  samples.volt_b = Telemetry_ReadAdcChannel(&hadc1, ADC_CHANNEL_4, samples.volt_b);
  samples.volt_a = Telemetry_ReadAdcChannel(&hadc2, ADC_CHANNEL_13, samples.volt_a);
  samples.ntc_pcb = Telemetry_ReadAdcChannel(&hadc2, ADC_CHANNEL_3, samples.ntc_pcb);
  samples.vbus = Telemetry_ReadAdcChannel(&hadc2, ADC_CHANNEL_12, samples.vbus);
  samples.volt_c = Telemetry_ReadAdcChannel(&hadc2, ADC_CHANNEL_14, samples.volt_c);
  return samples;
}

static uint8_t Telemetry_Encode(uint32_t now_ms, size_t *payload_length)
{
  bldc_Telemetry message = bldc_Telemetry_init_zero;
  TelemetryAdcSamples adc = Telemetry_ReadAdcs();
  uint32_t vdda_mv = Telemetry_ReadVddaMv();
  pb_ostream_t stream;

  message.protocol_version = 1U;
  message.sequence = telemetry_sequence++;
  message.uptime_ms = now_ms;
  message.bus_voltage_mv = Telemetry_DividerVoltageMv(adc.vbus, vdda_mv,
                                                       VM_R_HIGH_OHM, VM_R_LOW_OHM);
  message.phase_current_ma = (int32_t)(Telemetry_Random() % 20001U) - 10000;
  message.motor_rpm = Telemetry_Random() % 6001U;
  message.mosfet_temperature_cdec = 250 + (int32_t)(Telemetry_Random() % 551U);
  message.ntc_pcb_temperature_cdec = Telemetry_NtcPcbTemperatureCdec(adc.ntc_pcb, vdda_mv);
  message.curr_a_adc_raw = adc.curr_a;
  message.curr_b_adc_raw = adc.curr_b;
  message.curr_c_adc_raw = adc.curr_c;
  message.volt_a_mv = Telemetry_DividerVoltageMv(adc.volt_a, vdda_mv,
                                                  PHASE_VOLTAGE_R_HIGH_OHM,
                                                  PHASE_VOLTAGE_R_LOW_OHM);
  message.volt_b_mv = Telemetry_DividerVoltageMv(adc.volt_b, vdda_mv,
                                                  PHASE_VOLTAGE_R_HIGH_OHM,
                                                  PHASE_VOLTAGE_R_LOW_OHM);
  message.volt_c_mv = Telemetry_DividerVoltageMv(adc.volt_c, vdda_mv,
                                                  PHASE_VOLTAGE_R_HIGH_OHM,
                                                  PHASE_VOLTAGE_R_LOW_OHM);

  stream = pb_ostream_from_buffer(telemetry_payload, sizeof(telemetry_payload));
  if (!pb_encode(&stream, bldc_Telemetry_fields, &message)) return 0U;

  *payload_length = stream.bytes_written;
  return 1U;
}

void Telemetry_Init(void)
{
  if ((HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) ||
      (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK))
  {
    Error_Handler();
  }

  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, telemetry_rx_buffer);
  (void)USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  telemetry_next_ms = 0U;
  telemetry_tx_pending = 0U;
}

void Telemetry_Task(uint32_t now_ms)
{
  size_t payload_length;
  size_t frame_length;

  if ((hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) || (telemetry_tx_pending != 0U) ||
      ((int32_t)(now_ms - telemetry_next_ms) < 0))
  {
    return;
  }
  if (Telemetry_Encode(now_ms, &payload_length) == 0U) return;

  frame_length = Telemetry_CobsEncode(telemetry_payload, payload_length, telemetry_frame,
                                       sizeof(telemetry_frame) - 1U);
  if (frame_length == 0U) return;

  telemetry_frame[frame_length++] = 0U;
  if (CDC_Transmit_FS(telemetry_frame, (uint16_t)frame_length) == USBD_OK)
  {
    telemetry_tx_pending = 1U;
    telemetry_next_ms = now_ms + TELEMETRY_INTERVAL_MS;
  }
}

uint8_t Telemetry_OnReceive(uint8_t *buffer, uint32_t length)
{
  (void)buffer;
  (void)length;
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, telemetry_rx_buffer);
  (void)USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  return USBD_OK;
}

void Telemetry_OnTransmitComplete(void)
{
  telemetry_tx_pending = 0U;
  if ((uint32_t)(HAL_GetTick() - telemetry_last_led_toggle_ms) >= 500U)
  {
    HAL_GPIO_TogglePin(LEDC_GPIO_Port, LEDC_Pin);
    telemetry_last_led_toggle_ms = HAL_GetTick();
  }
}
