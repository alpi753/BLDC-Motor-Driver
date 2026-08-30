#include "cdc_usb_bridge.h"

#include "cdc_nanopb.h"
#include "main.h"
#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t AppCdc_Transmit(uint8_t *buffer, uint16_t length)
{
  return CDC_Transmit_FS(buffer, length);
}

static void AppCdc_RearmReception(void)
{
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, CdcNanopb_RxBuffer());
  (void)USBD_CDC_ReceivePacket(&hUsbDeviceFS);
}

void AppCdc_Init(void)
{
  CdcNanopb_Init(AppCdc_Transmit);
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

void AppCdc_OnTransmitComplete(void)
{
  CdcNanopb_OnTransmitComplete();
  HAL_GPIO_TogglePin(LEDA_GPIO_Port, LEDA_Pin);
  HAL_GPIO_TogglePin(LEDB_GPIO_Port, LEDB_Pin);
  HAL_GPIO_TogglePin(LEDC_GPIO_Port, LEDC_Pin);
}
