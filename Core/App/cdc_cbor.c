#include "cdc_cbor.h"

#include "cbor.h"

#define CDC_CBOR_INTERVAL_MS 250U
#define CDC_CBOR_RX_BUFFER_SIZE 1024U
#define CDC_CBOR_PAYLOAD_SIZE 96U
#define CDC_CBOR_FRAME_SIZE 128U

static uint8_t cdc_cbor_rx_buffer[CDC_CBOR_RX_BUFFER_SIZE];
static uint8_t cdc_cbor_payload[CDC_CBOR_PAYLOAD_SIZE];
static uint8_t cdc_cbor_frame[CDC_CBOR_FRAME_SIZE];
static CdcCborTransmitFn cdc_cbor_transmit;
static uint32_t cdc_cbor_next_telemetry_ms;
static uint32_t cdc_cbor_sequence;
static uint32_t cdc_cbor_prng = 0x6d2b79f5U;
static uint8_t cdc_cbor_tx_pending;

static uint32_t CdcCbor_Random(void)
{
  uint32_t value = cdc_cbor_prng;

  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  cdc_cbor_prng = value;
  return value;
}

static size_t CdcCbor_CobsEncode(const uint8_t *input, size_t input_length,
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
      if (output_index >= output_capacity)
      {
        return 0U;
      }
      output[code_index] = code;
      code = 1U;
      code_index = output_index++;
    }
    else
    {
      if (output_index >= output_capacity)
      {
        return 0U;
      }
      output[output_index++] = input[input_index];
      code++;

      if (code == 0xffU)
      {
        if (output_index >= output_capacity)
        {
          return 0U;
        }
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

static CborError CdcCbor_EncodeTelemetry(uint32_t now_ms, size_t *payload_length)
{
  CborEncoder encoder;
  CborEncoder message;
  CborEncoder telemetry;
  CborError error;
  const uint32_t voltage_mv = 22000U + (CdcCbor_Random() % 3001U);
  const int32_t current_ma = (int32_t)(CdcCbor_Random() % 20001U) - 10000;
  const uint32_t motor_rpm = CdcCbor_Random() % 6001U;
  const int32_t mosfet_temperature_cdec = 250 + (int32_t)(CdcCbor_Random() % 551U);

  cbor_encoder_init(&encoder, cdc_cbor_payload, sizeof(cdc_cbor_payload), 0);
  error = cbor_encoder_create_map(&encoder, &message, 5U);
  if (error != CborNoError) return error;

  error = cbor_encode_uint(&message, 0U); /* protocol version */
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&message, 1U);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&message, 1U); /* message type: telemetry */
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&message, 2U);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&message, cdc_cbor_sequence++);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&message, 3U);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&message, now_ms);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&message, 4U);
  if (error != CborNoError) return error;

  error = cbor_encoder_create_map(&message, &telemetry, 4U);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&telemetry, 0U); /* DC bus voltage, mV */
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&telemetry, voltage_mv);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&telemetry, 1U); /* Phase current, mA */
  if (error != CborNoError) return error;
  error = cbor_encode_int(&telemetry, current_ma);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&telemetry, 2U); /* Motor speed, RPM */
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&telemetry, motor_rpm);
  if (error != CborNoError) return error;
  error = cbor_encode_uint(&telemetry, 3U); /* MOSFET temperature, 0.1 C */
  if (error != CborNoError) return error;
  error = cbor_encode_int(&telemetry, mosfet_temperature_cdec);
  if (error != CborNoError) return error;
  error = cbor_encoder_close_container(&message, &telemetry);
  if (error != CborNoError) return error;
  error = cbor_encoder_close_container(&encoder, &message);
  if (error != CborNoError) return error;

  *payload_length = cbor_encoder_get_buffer_size(&encoder, cdc_cbor_payload);
  return CborNoError;
}

void CdcCbor_Init(CdcCborTransmitFn transmit)
{
  cdc_cbor_transmit = transmit;
  cdc_cbor_next_telemetry_ms = 0U;
  cdc_cbor_tx_pending = 0U;
}

void CdcCbor_Task(uint32_t now_ms)
{
  size_t payload_length;
  size_t frame_length;

  if ((cdc_cbor_transmit == NULL) || (cdc_cbor_tx_pending != 0U) ||
      ((int32_t)(now_ms - cdc_cbor_next_telemetry_ms) < 0))
  {
    return;
  }

  if (CdcCbor_EncodeTelemetry(now_ms, &payload_length) != CborNoError)
  {
    return;
  }

  frame_length = CdcCbor_CobsEncode(cdc_cbor_payload, payload_length,
                                     cdc_cbor_frame, sizeof(cdc_cbor_frame) - 1U);
  if (frame_length == 0U)
  {
    return;
  }
  cdc_cbor_frame[frame_length++] = 0U;

  if (cdc_cbor_transmit(cdc_cbor_frame, (uint16_t)frame_length) == 0U)
  {
    cdc_cbor_tx_pending = 1U;
    cdc_cbor_next_telemetry_ms = now_ms + CDC_CBOR_INTERVAL_MS;
  }
}

void CdcCbor_OnReceive(const uint8_t *buffer, uint32_t length)
{
  (void)buffer;
  (void)length;
}

void CdcCbor_OnTransmitComplete(void)
{
  cdc_cbor_tx_pending = 0U;
}

uint8_t *CdcCbor_RxBuffer(void)
{
  return cdc_cbor_rx_buffer;
}
