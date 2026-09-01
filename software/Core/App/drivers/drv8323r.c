#include "drv8323r.h"
#include "main.h"

#include <string.h>
extern SPI_HandleTypeDef hspi1;

#define DRV8323R_FAULT_FET_LC_OC   (1U << 0)
#define DRV8323R_FAULT_FET_HC_OC   (1U << 1)
#define DRV8323R_FAULT_FET_LB_OC   (1U << 2)
#define DRV8323R_FAULT_FET_HB_OC   (1U << 3)
#define DRV8323R_FAULT_FET_LA_OC   (1U << 4)
#define DRV8323R_FAULT_FET_HA_OC   (1U << 5)
#define DRV8323R_FAULT_OTSD        (1U << 6)
#define DRV8323R_FAULT_UVLO        (1U << 7)
#define DRV8323R_FAULT_GDF         (1U << 8)
#define DRV8323R_FAULT_VDS_OCP     (1U << 9)
#define DRV8323R_FAULT_FAULT       (1U << 10)
#define DRV8323R_FAULT_FETLC_VGS   (1U << 16)
#define DRV8323R_FAULT_FETHC_VGS   (1U << 17)
#define DRV8323R_FAULT_FETLB_VGS   (1U << 18)
#define DRV8323R_FAULT_FETHB_VGS   (1U << 19)
#define DRV8323R_FAULT_FETLA_VGS   (1U << 20)
#define DRV8323R_FAULT_FETHA_VGS   (1U << 21)
#define DRV8323R_FAULT_CPUV        (1U << 22)
#define DRV8323R_FAULT_OTW         (1U << 23)
#define DRV8323R_FAULT_SA_OC       (1U << 24)
#define DRV8323R_FAULT_SB_OC       (1U << 25)
#define DRV8323R_FAULT_SC_OC       (1U << 26)

#define DRV8323R_OCP_DEAD_TIME_100_NS       (1U << 8)
#define DRV8323R_OCP_MODE_LATCHED_SHUTDOWN  (0U << 6)
#define DRV8323R_OCP_DEGLITCH_4_US          (1U << 4)
#define DRV8323R_OCP_VDS_LEVEL_0_75_V       9U
#define DRV8323R_OCP_SAFE_STARTUP           \
	(DRV8323R_OCP_DEAD_TIME_100_NS |      \
	 DRV8323R_OCP_MODE_LATCHED_SHUTDOWN | \
	 DRV8323R_OCP_DEGLITCH_4_US |         \
	 DRV8323R_OCP_VDS_LEVEL_0_75_V)

static char Drv8323r_fault_buf[160];
static bool Drv8323r_transfer_failed;

static uint16_t Drv8323r_spi_exchange(uint16_t tx);
static uint16_t Drv8323r_build_frame(bool read, uint8_t reg, uint16_t data);
static uint16_t Drv8323r_spi_transfer(uint16_t frame);
static void Drv8323r_select(void);
static void Drv8323r_deselect(void);
static void Drv8323r_modify_reg(uint8_t reg, uint16_t clear_mask, uint16_t set_bits);

static uint16_t Drv8323r_build_frame(bool read, uint8_t reg, uint16_t data)
{
	uint16_t frame = 0;
	frame |= ((uint16_t)read & 0x01U) << 15;
	frame |= ((uint16_t)reg & 0x0FU) << 11;
	frame |= (data & 0x07FFU);
	return frame;
}

static uint16_t Drv8323r_spi_exchange(uint16_t tx)
{
	return Drv8323r_spi_transfer(tx);
}

static uint16_t Drv8323r_spi_transfer(uint16_t frame)
{
	uint16_t tx = frame;
	uint16_t rx = 0U;

	if (HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)&tx, (uint8_t *)&rx, 1U,
	                            HAL_MAX_DELAY) != HAL_OK) {
		Drv8323r_transfer_failed = true;
		return 0U;
	}

	return rx;
}

static void Drv8323r_select(void)
{
	HAL_GPIO_WritePin(SPI1_NSCS_GPIO_Port, SPI1_NSCS_Pin, GPIO_PIN_RESET);
}

static void Drv8323r_deselect(void)
{
	HAL_GPIO_WritePin(SPI1_NSCS_GPIO_Port, SPI1_NSCS_Pin, GPIO_PIN_SET);
	for (volatile uint32_t index = 0U; index < 16U; index++)
	{
		__NOP();
	}
}

static void Drv8323r_modify_reg(uint8_t reg, uint16_t clear_mask, uint16_t set_bits)
{
	uint16_t value = Drv8323r_ReadRegister(reg);
	value &= clear_mask;
	value |= set_bits;
	Drv8323r_WriteRegister(reg, value);
}

static void Drv8323r_fault_append(const char *text, size_t *offset)
{
	size_t remaining = sizeof(Drv8323r_fault_buf) - *offset;
	if (remaining <= 1U) {
		return;
	}

	size_t text_len = strlen(text);
	size_t copy_len = text_len;
	if (copy_len >= remaining) {
		copy_len = remaining - 1U;
	}

	memcpy(&Drv8323r_fault_buf[*offset], text, copy_len);
	*offset += copy_len;
	Drv8323r_fault_buf[*offset] = '\0';
}

bool Drv8323r_Init(void)
{
	uint16_t csa_control;

	Drv8323r_transfer_failed = false;
	HAL_GPIO_WritePin(SPI1_NSCS_GPIO_Port, SPI1_NSCS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SPI1_ENABLE_GPIO_Port, SPI1_ENABLE_Pin, GPIO_PIN_SET);
	HAL_Delay(100);

	/* Start with overcurrent protection enabled in latched-shutdown mode. */
	Drv8323r_WriteRegister(5, DRV8323R_OCP_SAFE_STARTUP);
	Drv8323r_WriteRegister(3, 0x03AFU);
	Drv8323r_WriteRegister(4, 0x07AFU);
	Drv8323r_SetCurrentSenseGain(Drv8323rCurrentSenseGain20VPerV);

	/* Clear any startup fault latches. */
	Drv8323r_ClearFaults();
	csa_control = Drv8323r_ReadRegister(6U);

	return (!Drv8323r_transfer_failed) &&
	       ((csa_control & 0x00C0U) == 0x0080U);
}

uint16_t Drv8323r_ReadRegister(uint8_t reg)
{
	uint16_t command = Drv8323r_build_frame(true, reg, 0x007FU);

	Drv8323r_select();
	(void)Drv8323r_spi_exchange(command);
	Drv8323r_deselect();

	Drv8323r_select();
	uint16_t response = Drv8323r_spi_exchange(command);
	Drv8323r_deselect();

	return response;
}

void Drv8323r_WriteRegister(uint8_t reg, uint16_t data)
{
	uint16_t command = Drv8323r_build_frame(false, reg, data);

	Drv8323r_select();
	(void)Drv8323r_spi_exchange(command);
	Drv8323r_deselect();
}

void Drv8323r_SetOvercurrentAdjustment(uint8_t value)
{
	if (value > 15U)
	{
		value = 15U;
	}

	uint16_t reg = Drv8323r_ReadRegister(5);
	reg &= 0xFFF0U;
	reg |= (uint16_t)value;
	Drv8323r_WriteRegister(5, reg);
}

void Drv8323r_SetOvercurrentMode(Drv8323rOvercurrentMode mode)
{
	if (mode > Drv8323rOvercurrentLatchShutdown)
	{
		mode = Drv8323rOvercurrentLatchShutdown;
	}

	uint16_t reg = Drv8323r_ReadRegister(5);
	reg &= 0xFF3FU;
	reg |= ((uint16_t)mode & 0x03U) << 6;
	Drv8323r_WriteRegister(5, reg);
}

void Drv8323r_SetCurrentSenseGain(Drv8323rCurrentSenseGain gain)
{
	uint16_t reg = Drv8323r_ReadRegister(6);
	reg &= ~(0x03U << 6);

	switch (gain) {
	case Drv8323rCurrentSenseGain5VPerV:
		reg |= (0U << 6);
		break;
	case Drv8323rCurrentSenseGain10VPerV:
		reg |= (1U << 6);
		break;
	case Drv8323rCurrentSenseGain20VPerV:
		reg |= (2U << 6);
		break;
	case Drv8323rCurrentSenseGain40VPerV:
		reg |= (3U << 6);
		break;
	default:
		break;
	}

	Drv8323r_WriteRegister(6, reg);
}

void Drv8323r_SetDcCalibration(bool enabled)
{
	Drv8323r_modify_reg(6, 0xFFFBU, enabled ? (1U << 2) : 0U);
}

uint32_t Drv8323r_ReadFaults(void)
{
	uint32_t status1 = Drv8323r_ReadRegister(0);
	uint32_t status2 = Drv8323r_ReadRegister(1);

	return status1 | (status2 << 16);
}

const char *Drv8323r_FaultsToString(uint32_t faults)
{
	size_t offset = 0;
	Drv8323r_fault_buf[0] = '\0';

	if (faults == 0U) {
		strcpy(Drv8323r_fault_buf, "No DRV8323R faults");
		return Drv8323r_fault_buf;
	}

	Drv8323r_fault_append("|", &offset);

	if (faults & DRV8323R_FAULT_FET_LC_OC) { Drv8323r_fault_append(" FETLC_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_HC_OC) { Drv8323r_fault_append(" FETHC_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_LB_OC) { Drv8323r_fault_append(" FETLB_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_HB_OC) { Drv8323r_fault_append(" FETHB_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_LA_OC) { Drv8323r_fault_append(" FETLA_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_HA_OC) { Drv8323r_fault_append(" FETHA_OC |", &offset); }
	if (faults & DRV8323R_FAULT_OTSD) { Drv8323r_fault_append(" OTSD |", &offset); }
	if (faults & DRV8323R_FAULT_UVLO) { Drv8323r_fault_append(" UVLO |", &offset); }
	if (faults & DRV8323R_FAULT_GDF) { Drv8323r_fault_append(" GDF |", &offset); }
	if (faults & DRV8323R_FAULT_VDS_OCP) { Drv8323r_fault_append(" VDS OCP |", &offset); }
	if (faults & DRV8323R_FAULT_FAULT) { Drv8323r_fault_append(" FAULT |", &offset); }
	if (faults & DRV8323R_FAULT_FETLC_VGS) { Drv8323r_fault_append(" FETLC VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETHC_VGS) { Drv8323r_fault_append(" FETHC VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETLB_VGS) { Drv8323r_fault_append(" FETLB VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETHB_VGS) { Drv8323r_fault_append(" FETHB VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETLA_VGS) { Drv8323r_fault_append(" FETLA VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETHA_VGS) { Drv8323r_fault_append(" FETHA VGS |", &offset); }
	if (faults & DRV8323R_FAULT_CPUV) { Drv8323r_fault_append(" CPU V |", &offset); }
	if (faults & DRV8323R_FAULT_OTW) { Drv8323r_fault_append(" OTW |", &offset); }
	if (faults & DRV8323R_FAULT_SA_OC) { Drv8323r_fault_append(" AMP A OC |", &offset); }
	if (faults & DRV8323R_FAULT_SB_OC) { Drv8323r_fault_append(" AMP B OC |", &offset); }
	if (faults & DRV8323R_FAULT_SC_OC) { Drv8323r_fault_append(" AMP C OC |", &offset); }

	return Drv8323r_fault_buf;
}

void Drv8323r_ClearFaults(void)
{
	uint16_t reg = Drv8323r_ReadRegister(2);
	reg |= 1U;
	Drv8323r_WriteRegister(2, reg);
}
