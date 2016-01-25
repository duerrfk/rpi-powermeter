/**
 * This file is part of RPi-Powermeter.
 *
 * Copyright 2015 University of Stuttgart
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MCP320X_H
#define MCP320X_H

#include <stdint.h>

/**
 * MCP320x channels in single-ended mode.
 */
enum channel_singleended {CH0, CH1, CH2, CH3, CH4, CH5, CH6, CH7};

/**
 * MCP320x channels in differential mode.
 * In differential mode, two channels are used for IN+ and IN-, respectively.
 * The first channel is IN+, second IN-.
 */
enum channel_differential {CH0CH1, CH1CH0, CH2CH3, CH3CH2, CH4CH5, CH5CH4,
			   CH6CH7, CH7CH6};

/**
 * Sample a channel in single-ended mode.
 *
 * @param adc_channel the MCP320x channel to be sampled 
 * @param spi_channel the SPI channel to be used. RPi has two channels (0, 1).
 * @return 12 bit sample value, or -1 in case of an error.
 */
int16_t get_sample_singleended(enum channel_singleended adc_channel,
			       int spi_channel);

/**
 * Sample in differential mode.
 *
 * @param adc_channel the MCP320x channel pair to be sampled
 * @param spi_channel the SPI channel to be used. RPi has two channels (0, 1).
 * @return 12 bit sample value, or -1 in case of an error. 
 */
int16_t get_sample_diff(enum channel_differential channel,
			int spi_channel);

#endif
