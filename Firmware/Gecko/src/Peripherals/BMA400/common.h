/**
 * Copyright (C) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _BMA400_COMMON_H_
#define _BMA400_COMMON_H_

#include "bma400.h"
#include "system.h"

extern int8_t interfaceResult;

float lsb_to_ms2(int16_t accel_data, uint8_t g_range, uint8_t bit_width);
int8_t bma400_interface_init(struct bma400_dev *bme, uint8_t intf);

#if __DEVELOPMENT_BOARD__
    int8_t bma400_read_i2c(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void* intf_ptr);
    int8_t bma400_write_i2c(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void* intf_ptr);
#else
    int8_t bma400_read_spi(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
    int8_t bma400_write_spi(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
#endif

void bma400_delay_us(uint32_t period_us, void* intf_ptr);
void bma400_check_rslt(const char api_name[], int8_t rslt);
void bma400_coines_deinit(void);

#endif