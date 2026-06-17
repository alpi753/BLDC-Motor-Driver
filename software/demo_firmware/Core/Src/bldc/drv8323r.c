#include "bldc.h"
#include "main.h"

#include <stdbool.h>
#include <string.h>
extern SPI_HandleTypeDef hspi1;

enum {
	DRV8323R_OC_DISABLED = 0,
	DRV8323R_OC_LIMIT = 1,
	DRV8323R_OC_LATCH_SHUTDOWN = 2,
};

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

static char drv8323r_fault_buf[160];

static uint16_t drv8323r_spi_exchange(uint16_t tx);
static uint16_t drv8323r_build_frame(bool read, uint8_t reg, uint16_t data);
static uint16_t drv8323r_spi_transfer(uint16_t frame);
static void drv8323r_select(void);
static void drv8323r_deselect(void);
static void drv8323r_modify_reg(uint8_t reg, uint16_t clear_mask, uint16_t set_bits);

static uint16_t drv8323r_build_frame(bool read, uint8_t reg, uint16_t data)
{
	uint16_t frame = 0;
	frame |= ((uint16_t)read & 0x01U) << 15;
	frame |= ((uint16_t)reg & 0x0FU) << 11;
	frame |= (data & 0x07FFU);
	return frame;
}

static uint16_t drv8323r_spi_exchange(uint16_t tx)
{
	return drv8323r_spi_transfer(tx);
}

static uint16_t drv8323r_spi_transfer(uint16_t frame)
{
	uint8_t tx[2] = {(uint8_t)(frame >> 8), (uint8_t)(frame & 0xFFU)};
	uint8_t rx[2] = {0U, 0U};

	if (HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2U, HAL_MAX_DELAY) != HAL_OK) {
		return 0U;
	}

	return (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
}

static void drv8323r_select(void)
{
	HAL_GPIO_WritePin(DRV8323R_CS_GPIO_Port, DRV8323R_CS_Pin, GPIO_PIN_RESET);
}

static void drv8323r_deselect(void)
{
	HAL_GPIO_WritePin(DRV8323R_CS_GPIO_Port, DRV8323R_CS_Pin, GPIO_PIN_SET);
}

static void drv8323r_modify_reg(uint8_t reg, uint16_t clear_mask, uint16_t set_bits)
{
	uint16_t value = bldc_drv8323r_read_reg(reg);
	value &= clear_mask;
	value |= set_bits;
	bldc_drv8323r_write_reg(reg, value);
}

static void drv8323r_fault_append(const char *text, size_t *offset)
{
	size_t remaining = sizeof(drv8323r_fault_buf) - *offset;
	if (remaining <= 1U) {
		return;
	}

	size_t text_len = strlen(text);
	size_t copy_len = text_len;
	if (copy_len >= remaining) {
		copy_len = remaining - 1U;
	}

	memcpy(&drv8323r_fault_buf[*offset], text, copy_len);
	*offset += copy_len;
	drv8323r_fault_buf[*offset] = '\0';
}

void bldc_drv8323r_init(void)
{
	HAL_GPIO_WritePin(DRV8323R_EN_GPIO_Port, DRV8323R_EN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DRV8323R_CS_GPIO_Port, DRV8323R_CS_Pin, GPIO_PIN_SET);
	HAL_Delay(100);

	/* Safe startup defaults inspired by the upstream DRV8323S driver. */
	bldc_drv8323r_write_reg(5, 0x00D0U);
	bldc_drv8323r_write_reg(3, 0x03AFU);
	bldc_drv8323r_write_reg(4, 0x07AFU);
	bldc_drv8323r_set_current_amp_gain(10);

	/* Clear any startup fault latches. */
	bldc_drv8323r_reset_faults();
}

uint16_t bldc_drv8323r_read_reg(uint8_t reg)
{
	uint16_t command = drv8323r_build_frame(true, reg, 0x007FU);

	drv8323r_select();
	(void)drv8323r_spi_exchange(command);
	drv8323r_deselect();

	drv8323r_select();
	uint16_t response = drv8323r_spi_exchange(command);
	drv8323r_deselect();

	return response;
}

void bldc_drv8323r_write_reg(uint8_t reg, uint16_t data)
{
	uint16_t command = drv8323r_build_frame(false, reg, data);

	drv8323r_select();
	(void)drv8323r_spi_exchange(command);
	drv8323r_deselect();
}

void bldc_drv8323r_set_oc_adj(int val)
{
	if (val < 0) {
		val = 0;
	} else if (val > 15) {
		val = 15;
	}

	uint16_t reg = bldc_drv8323r_read_reg(5);
	reg &= 0xFFF0U;
	reg |= (uint16_t)val & 0x000FU;
	bldc_drv8323r_write_reg(5, reg);
}

void bldc_drv8323r_set_oc_mode(int mode)
{
	if (mode < DRV8323R_OC_DISABLED) {
		mode = DRV8323R_OC_DISABLED;
	} else if (mode > DRV8323R_OC_LATCH_SHUTDOWN) {
		mode = DRV8323R_OC_LATCH_SHUTDOWN;
	}

	uint16_t reg = bldc_drv8323r_read_reg(5);
	reg &= 0xFF3FU;
	reg |= ((uint16_t)mode & 0x03U) << 6;
	bldc_drv8323r_write_reg(5, reg);
}

void bldc_drv8323r_set_current_amp_gain(int gain)
{
	uint16_t reg = bldc_drv8323r_read_reg(6);
	reg &= ~(0x03U << 6);

	switch (gain) {
	case 5:
		reg |= (0U << 6);
		break;
	case 10:
		reg |= (1U << 6);
		break;
	case 20:
		reg |= (2U << 6);
		break;
	case 40:
		reg |= (3U << 6);
		break;
	default:
		break;
	}

	bldc_drv8323r_write_reg(6, reg);
}

void bldc_drv8323r_dccal_on(void)
{
	drv8323r_modify_reg(6, 0xFFFBU, (1U << 2));
}

void bldc_drv8323r_dccal_off(void)
{
	drv8323r_modify_reg(6, 0xFFFBU, 0U);
}

uint32_t bldc_drv8323r_read_faults(void)
{
	uint32_t status1 = bldc_drv8323r_read_reg(0);
	uint32_t status2 = bldc_drv8323r_read_reg(1);

	return status1 | (status2 << 16);
}

char* bldc_drv8323r_faults_to_string(uint32_t faults)
{
	size_t offset = 0;
	drv8323r_fault_buf[0] = '\0';

	if (faults == 0U) {
		strcpy(drv8323r_fault_buf, "No DRV8323R faults");
		return drv8323r_fault_buf;
	}

	drv8323r_fault_append("|", &offset);

	if (faults & DRV8323R_FAULT_FET_LC_OC) { drv8323r_fault_append(" FETLC_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_HC_OC) { drv8323r_fault_append(" FETHC_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_LB_OC) { drv8323r_fault_append(" FETLB_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_HB_OC) { drv8323r_fault_append(" FETHB_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_LA_OC) { drv8323r_fault_append(" FETLA_OC |", &offset); }
	if (faults & DRV8323R_FAULT_FET_HA_OC) { drv8323r_fault_append(" FETHA_OC |", &offset); }
	if (faults & DRV8323R_FAULT_OTSD) { drv8323r_fault_append(" OTSD |", &offset); }
	if (faults & DRV8323R_FAULT_UVLO) { drv8323r_fault_append(" UVLO |", &offset); }
	if (faults & DRV8323R_FAULT_GDF) { drv8323r_fault_append(" GDF |", &offset); }
	if (faults & DRV8323R_FAULT_VDS_OCP) { drv8323r_fault_append(" VDS OCP |", &offset); }
	if (faults & DRV8323R_FAULT_FAULT) { drv8323r_fault_append(" FAULT |", &offset); }
	if (faults & DRV8323R_FAULT_FETLC_VGS) { drv8323r_fault_append(" FETLC VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETHC_VGS) { drv8323r_fault_append(" FETHC VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETLB_VGS) { drv8323r_fault_append(" FETLB VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETHB_VGS) { drv8323r_fault_append(" FETHB VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETLA_VGS) { drv8323r_fault_append(" FETLA VGS |", &offset); }
	if (faults & DRV8323R_FAULT_FETHA_VGS) { drv8323r_fault_append(" FETHA VGS |", &offset); }
	if (faults & DRV8323R_FAULT_CPUV) { drv8323r_fault_append(" CPU V |", &offset); }
	if (faults & DRV8323R_FAULT_OTW) { drv8323r_fault_append(" OTW |", &offset); }
	if (faults & DRV8323R_FAULT_SA_OC) { drv8323r_fault_append(" AMP A OC |", &offset); }
	if (faults & DRV8323R_FAULT_SB_OC) { drv8323r_fault_append(" AMP B OC |", &offset); }
	if (faults & DRV8323R_FAULT_SC_OC) { drv8323r_fault_append(" AMP C OC |", &offset); }

	return drv8323r_fault_buf;
}

void bldc_drv8323r_reset_faults(void)
{
	uint16_t reg = bldc_drv8323r_read_reg(2);
	reg |= 1U;
	bldc_drv8323r_write_reg(2, reg);
}

