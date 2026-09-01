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
#define MOSFET_NTC_R25_OHM 10000.0f
#define MOSFET_NTC_BETA_K 3435.0f
#define MOSFET_NTC_T25_K 298.15f
#define MOSFET_NTC_R_FIXED_OHM 1000.0f
#define MOSFET_NTC_DIVIDER_SUPPLY_MV 3300.0f
#define VM_R_HIGH_OHM 330000.0f
#define VM_R_LOW_OHM 10000.0f
#define PHASE_VOLTAGE_R_HIGH_OHM 91000.0f
#define PHASE_VOLTAGE_R_LOW_OHM 4700.0f
#define CSA_GAIN_V_PER_V 20.0f
#define PHASE_SHUNT_OHM 0.002f
#define ADC_MAX_COUNTS 4095.0f
#define TELEMETRY_PROTOCOL_VERSION 2U

typedef struct
{
  uint16_t mosfet_temp;
  uint16_t pcb_temp;
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

static int32_t Telemetry_CdecFromCelsius(float temperature_c);

static uint8_t telemetry_rx_buffer[TELEMETRY_RX_BUFFER_SIZE];
static uint8_t telemetry_payload[TELEMETRY_PAYLOAD_SIZE];
static uint8_t telemetry_frame[TELEMETRY_FRAME_SIZE];
static uint32_t telemetry_next_ms;
static uint32_t telemetry_sequence;
static uint32_t telemetry_last_led_toggle_ms;
static uint8_t telemetry_tx_pending;

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

static int32_t Telemetry_MosfetTemperatureCdec(uint16_t adc_raw, uint32_t vdda_mv)
{
  float resistance_ohm;
  float temperature_c;
  float ntc_voltage_mv;

  if ((adc_raw == 0U) || (vdda_mv == 0U) || (adc_raw >= ADC_MAX_COUNTS))
  {
    return INT32_MIN;
  }

  ntc_voltage_mv = (float)adc_raw * (float)vdda_mv / ADC_MAX_COUNTS;
  if ((ntc_voltage_mv <= 0.0f) || (ntc_voltage_mv >= MOSFET_NTC_DIVIDER_SUPPLY_MV))
  {
    return INT32_MIN;
  }

  resistance_ohm = MOSFET_NTC_R_FIXED_OHM *
      (MOSFET_NTC_DIVIDER_SUPPLY_MV / ntc_voltage_mv - 1.0f);
  temperature_c = (1.0f / ((1.0f / MOSFET_NTC_T25_K) +
      logf(resistance_ohm / MOSFET_NTC_R25_OHM) / MOSFET_NTC_BETA_K)) - 273.15f;
  if (!isfinite(temperature_c))
  {
    return INT32_MIN;
  }

  return Telemetry_CdecFromCelsius(temperature_c);
}

static int32_t Telemetry_CdecFromCelsius(float temperature_c)
{
  if (!isfinite(temperature_c))
  {
    return INT32_MIN;
  }

  return (int32_t)(temperature_c * 10.0f +
      ((temperature_c >= 0.0f) ? 0.5f : -0.5f));
}

static int32_t Telemetry_PcbTemperatureCdec(uint16_t adc_raw, uint32_t vdda_mv)
{
  int32_t temperature_c;

  if ((adc_raw == 0U) || (vdda_mv == 0U))
  {
    return INT32_MIN;
  }

  temperature_c = __HAL_ADC_CALC_TEMPERATURE(vdda_mv, adc_raw, ADC_RESOLUTION_12B);
  return temperature_c * 10;
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

static int32_t Telemetry_PhaseCurrentMa(uint16_t adc_raw, uint32_t vdda_mv)
{
  float csa_output_mv;
  float current_ma;

  if (vdda_mv == 0U)
  {
    return 0;
  }

  /* Bidirectional CSA: SOx = VREF/2 + G * I * Rshunt. VREF is the same 3.3 V
   * analog rail as VDDA, so mid-rail is half of the measured VDDA. */
  csa_output_mv = (float)adc_raw * (float)vdda_mv / ADC_MAX_COUNTS;
  current_ma = (csa_output_mv - 0.5f * (float)vdda_mv) /
      (CSA_GAIN_V_PER_V * PHASE_SHUNT_OHM);
  return (int32_t)(current_ma + ((current_ma >= 0.0f) ? 0.5f : -0.5f));
}

static TelemetryAdcSamples Telemetry_ReadAdcs(void)
{
  static TelemetryAdcSamples samples;

  samples.curr_a = Telemetry_ReadAdcChannel(&hadc1, ADC_CHANNEL_1, samples.curr_a);
  samples.curr_b = Telemetry_ReadAdcChannel(&hadc1, ADC_CHANNEL_2, samples.curr_b);
  samples.curr_c = Telemetry_ReadAdcChannel(&hadc1, ADC_CHANNEL_3, samples.curr_c);
  samples.volt_b = Telemetry_ReadAdcChannel(&hadc1, ADC_CHANNEL_4, samples.volt_b);
  samples.volt_a = Telemetry_ReadAdcChannel(&hadc2, ADC_CHANNEL_13, samples.volt_a);
  samples.mosfet_temp = Telemetry_ReadAdcChannel(&hadc2, ADC_CHANNEL_3, samples.mosfet_temp);
  samples.vbus = Telemetry_ReadAdcChannel(&hadc2, ADC_CHANNEL_12, samples.vbus);
  samples.volt_c = Telemetry_ReadAdcChannel(&hadc2, ADC_CHANNEL_14, samples.volt_c);
  samples.pcb_temp = Telemetry_ReadAdcChannelWithSampling(&hadc1,
      ADC_CHANNEL_TEMPSENSOR_ADC1, ADC_SAMPLETIME_640CYCLES_5, samples.pcb_temp);
  return samples;
}

static uint8_t Telemetry_Encode(uint32_t now_ms, size_t *payload_length)
{
  bldc_Telemetry message = bldc_Telemetry_init_zero;
  TelemetryAdcSamples adc = Telemetry_ReadAdcs();
  uint32_t vdda_mv = Telemetry_ReadVddaMv();
  pb_ostream_t stream;

  message.protocol_version = TELEMETRY_PROTOCOL_VERSION;
  message.sequence = telemetry_sequence++;
  message.uptime_ms = now_ms;
  message.bus_voltage_mv = Telemetry_DividerVoltageMv(adc.vbus, vdda_mv,
                                                       VM_R_HIGH_OHM, VM_R_LOW_OHM);
  message.mosfet_temperature_cdec = Telemetry_MosfetTemperatureCdec(adc.mosfet_temp, vdda_mv);
  message.pcb_temperature_cdec = Telemetry_PcbTemperatureCdec(adc.pcb_temp, vdda_mv);
  message.curr_a_ma = Telemetry_PhaseCurrentMa(adc.curr_a, vdda_mv);
  message.curr_b_ma = Telemetry_PhaseCurrentMa(adc.curr_b, vdda_mv);
  message.curr_c_ma = Telemetry_PhaseCurrentMa(adc.curr_c, vdda_mv);
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
