#include "cdc_nanopb.h"

#include "pb_encode.h"
#include "protocol/bldc.pb.h"

#define CDC_NANOPB_INTERVAL_MS 250U
#define CDC_NANOPB_RX_BUFFER_SIZE 1024U
#define CDC_NANOPB_PAYLOAD_SIZE bldc_Telemetry_size
#define CDC_NANOPB_FRAME_SIZE (CDC_NANOPB_PAYLOAD_SIZE + 2U)

static uint8_t cdc_nanopb_rx_buffer[CDC_NANOPB_RX_BUFFER_SIZE];
static uint8_t cdc_nanopb_payload[CDC_NANOPB_PAYLOAD_SIZE];
static uint8_t cdc_nanopb_frame[CDC_NANOPB_FRAME_SIZE];
static CdcNanopbTransmitFn cdc_nanopb_transmit;
static uint32_t cdc_nanopb_next_telemetry_ms;
static uint32_t cdc_nanopb_sequence;
static uint32_t cdc_nanopb_prng = 0x6d2b79f5U;
static uint8_t cdc_nanopb_tx_pending;

static uint32_t CdcNanopb_Random(void)
{
  uint32_t value = cdc_nanopb_prng;

  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  cdc_nanopb_prng = value;
  return value;
}

static size_t CdcNanopb_CobsEncode(const uint8_t *input, size_t input_length,
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

static uint8_t CdcNanopb_EncodeTelemetry(uint32_t now_ms, size_t *payload_length)
{
  bldc_Telemetry telemetry = bldc_Telemetry_init_zero;
  pb_ostream_t stream;

  telemetry.protocol_version = 1U;
  telemetry.sequence = cdc_nanopb_sequence++;
  telemetry.uptime_ms = now_ms;
  telemetry.bus_voltage_mv = 22000U + (CdcNanopb_Random() % 3001U);
  telemetry.phase_current_ma = (int32_t)(CdcNanopb_Random() % 20001U) - 10000;
  telemetry.motor_rpm = CdcNanopb_Random() % 6001U;
  telemetry.mosfet_temperature_cdec = 250 + (int32_t)(CdcNanopb_Random() % 551U);

  stream = pb_ostream_from_buffer(cdc_nanopb_payload, sizeof(cdc_nanopb_payload));
  if (!pb_encode(&stream, bldc_Telemetry_fields, &telemetry))
  {
    return 0U;
  }

  *payload_length = stream.bytes_written;
  return 1U;
}

void CdcNanopb_Init(CdcNanopbTransmitFn transmit)
{
  cdc_nanopb_transmit = transmit;
  cdc_nanopb_next_telemetry_ms = 0U;
  cdc_nanopb_tx_pending = 0U;
}

void CdcNanopb_Task(uint32_t now_ms)
{
  size_t payload_length;
  size_t frame_length;

  if ((cdc_nanopb_transmit == NULL) || (cdc_nanopb_tx_pending != 0U) ||
      ((int32_t)(now_ms - cdc_nanopb_next_telemetry_ms) < 0))
  {
    return;
  }

  if (CdcNanopb_EncodeTelemetry(now_ms, &payload_length) == 0U)
  {
    return;
  }

  frame_length = CdcNanopb_CobsEncode(cdc_nanopb_payload, payload_length,
                                       cdc_nanopb_frame, sizeof(cdc_nanopb_frame) - 1U);
  if (frame_length == 0U)
  {
    return;
  }
  cdc_nanopb_frame[frame_length++] = 0U;

  if (cdc_nanopb_transmit(cdc_nanopb_frame, (uint16_t)frame_length) == 0U)
  {
    cdc_nanopb_tx_pending = 1U;
    cdc_nanopb_next_telemetry_ms = now_ms + CDC_NANOPB_INTERVAL_MS;
  }
}

void CdcNanopb_OnReceive(const uint8_t *buffer, uint32_t length)
{
  (void)buffer;
  (void)length;
}

void CdcNanopb_OnTransmitComplete(void)
{
  cdc_nanopb_tx_pending = 0U;
}

uint8_t *CdcNanopb_RxBuffer(void)
{
  return cdc_nanopb_rx_buffer;
}
