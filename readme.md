RPi-Powermeter is a tool for measuring the power consumption of devices, in particular, battery-powered mobile devices, with a Raspberry Pi. RPi-Powermeter consists of a hardware and software part. The hardware is a board for measuring and sampling the voltage and current of the device using a 12 bit analog-to-digital converter (ADC) connected to the Raspberry Pi through SPI. The software runs on a Raspberry Pi with realtime extension (RT PREEMPT patch) to poll samples from the ADC with defined sampling frequency and log them concurrently to SD card for later post-processing. 

The key features of RPi-Powermeter are:

* Sampling of voltages up to 5 V and current up to 1 A to calculate the power consumption of the device under test (P = U*I). These ranges can be easily adapted by changing the values of only few hardware components (resistors and capacitors).
* Deterministic sampling frequency up to 1 kHz and concurrent data logging with single-core Raspberry Pi.
* Two alternative measurement boards using different methods to measure current, namely, a Hall sensor or shunt resistor. 
* Besides the power supply for the Raspberry Pi, no extra power supply is required. If the Raspberry Pi is powered from a battery pack, the measurement tool can be mobile to facilitate the evaluation of the energy efficiency of mobile devices.
* Simple hardware design (single-sided PCB, through-hole design) to simplify building measurement boards.
* Fully open source hardware and software design with liberal licensing (Apache License 2.0 and CERN Open Hardware License 1.2).

The following figure shows a measurement board with a Hall sensor connected to the Raspberry Pi.

![RPi-Powermeter measurement board connected to Raspberry Pi](/img/rpi_measurement_board_hall.jpg)

RPi-Powermeter was designed at University of Stuttgart for measuring the power consumption of mobile devices like smartphones to evaluate their energy-efficiency. We publish the design of RPi-Powermeter hoping that it will be useful also for others.

# Hardware: The Measurement Board

The measurement board contains the hardware to sample voltage V and current I of the device under test to calculate the power consumption as P = U*I. There are two versions of the measurement board, using different methods to measure current:

* Hall sensor: this board uses the ACS712 hall sensor, which outputs a voltage proportional to the current flowing through the sensor.  
* Shunt resistor: this board uses a 0.01 Ohm shunt resistor to measure the voltage drop along the resistor, which according to Ohm's law R = U/I is proportional to the current flowing through the shunt.

Both boards are dimensioned to measure voltages up to 5 V and current up to 1 A. These ranges can be adapted by changing the values of few components. 

Both boards are optimized such that the voltage drop along the current sensor is small to avoid a large variation of voltage at the device. Using the measurement board with 0.01 Ohm shunt resistor, the voltage drop along the shunt resistor is only 10 mV at 1 A. Using the Hall sensor with an internal resistance of 1.2 mOhm, the voltage drop is only 1.2 mV. In addition, we measure the voltage directly at the device rather than the power source to account for the voltage drop along the sensor.

Both boards use the MCP3208 12 bit analog-to-digital converter (ADC) to sample the voltage of the device and its current (actually, the amplified voltage of the hall sensor or shunt resistor). The MCP3208 is connected via SPI to the Raspberry Pi.

Schematics and board layouts for Eagle can be found in directory ```pcb/```.

## Design: Measurement Board with Hall Sensor

The following image shows the measurement board using the ACS712 Hall sensor. 

![Measurement Board with Hall sensor](/img/hall_measurement_board.jpg)

The ACS712 is on a separate PCB, which is available in slightly different versions from several vendors, so you might have to adapt the connector on the main measurement board to your specific ACS712 board.  

The ACS712 can measure current flowing in both directions through the Hall sensor. The output for I = 0 A is offset by 2.5 V (VCC/2). Since we are only interested in current flowing in one direction ("into" the device), we subtract 2.5 V from the sensor output using an op-amp as voltage subtractor (IC2B). 

After subtracting the 2.5 V offset, we amplify the voltage by a factor of 13.7 to increase the voltage gain of the Hall sensor from 180 mV/A to 2.34 V/A using a second op-amp (IC2C). Since we use a reference voltage of 2.5 V for the ADC, we can cover almost the full voltage range of the ADC assming a maximum current of 1 A. You can adjust this factor for other current ranges by changing the values of the two resistors R11 and R12 (amplification = 1 + R11/R12). If you change R11, you might also want to adjust C5 to adapt the low-pass filter (see below).

Since the output of the ACS712 is quite noisy (100 mA p-p) and to avoid interferences, we use a low-pass filter to reduce the amplification for higher frequencies by adding a capacitor (C5) in parallel to resistor R11 of IC2C. Thus, high frequencies will not be amplified. The cut-off frequency of the low-pass filter is calculated as f = 1 / (2*pi*R11*C5). Since we strive for a maximum sampling frequency of 1 kHz, we set f to 1950 Hz. You can adjust f by changing the value of capacitor C5.

A precise reference voltage of 2.5 V is provided by the LM385 diode (D1). IC1A acts as an impedance converter to provide a low impedance 2.5 V source.

The input voltage is divided by 2 by a voltage divider (resistors R13, R14) since we aim for a voltage range up to 5 V, but the ADC maximum input voltage is 2.5 V. Therefore, you need to multiply voltage samples by 2!

We would also like to note that instead of using the LM324 op-amp, you can also use other op-amps such as the MCP6004. Note that offset of the LM324 increases quite fast if it sinks current. In particular, this is a problem for the subtractor implemented by IC2B. A large offset to GND means that you cannot measure small currents anymore (note that the voltage slope of the Hall sensor is only 180 mV/A). In order to alleviate this problem, we use "pull-down" resistor R3 to avoid sinking significant current by the LM324. This reduces the offset to GND to few mV. This resistor might not be necessary for op-amps with smaller offsets like the MCP6004.

In order to ensure stability, the 10 Ohm resistors R7 and R9 decouple the op-amp outputs from the internal sampling capacitor of the ADC. Such small values will not affect the precision of the ADC.

## Design: Measurement Board with Shunt Resistor

The following image shows the measurement board using a 0.01 Ohm shunt resistor. 

![Measurement Board with Shunt Resistor](/img/shunt_measurement_board.jpg)

To indirectly measure current, we measure the voltage drop along the shunt resistor, which is connected in series with the device under test. With a 0.01 Ohm shunt resistor, this voltage is intentionally very small to keep the voltage at the device close to the supply voltage. 

For precise measurements, we amplify the voltage of the shunt resistor using an instrumentation amplifier (INA126P) with gain 100. The instrumentation amplifier has a very small offset voltage (100 uV) and high common mode rejection ratio. A small offset voltage is crucial at higher gains since also the offset voltage will be amplified. On the downside, the bandwidth of the instrumentation amplifier is not very high (9 kHz at gain 100, 1.8 kHz at gain 500). Therefore, we restrict the gain of the instrumentation amplifier to 100 to stay well above our desired 1 kHz sampling frequency, and use a second op amp with a small gain of 2.49 to get a total gain of 249 (actually, it is an inverting amplifier with gain -2.49 since the voltage drop along the shunt is negative). With a gain of 249, we get about 2.5 V output at 1 A, which is also the reference voltage of the ADC (MCP6004). The second amplification stage also implements a low pass filter with 2341 Hz cut-off frequency to further remove noise. To adapt the gain or cut-off frequency, change the values of the resistors and the capacitor of the low-pass filter. 

The 2.5 V reference voltage is provided by an LM385 diode. An op amp acts as an impedance converter to provide a low impedance 2.5 V source.     

The input voltage is divided by 2 by a voltage divider since we aim for a voltage range up to 5 V, but the ADC maximum input voltage is 2.5 V. Therefore, you need to multiply voltage samples by 2!

For the instrumentation amplifier and the op amp we need a symmetric power supply with a voltage range large enough to compensate for the rail offsets of the instrumentation amplifier and the op amp. To this end, we added a DC/DC converter, which generates +-12 V from the 5 V provided by the Raspberry Pi (+-5 V should also work). Since the voltage of the 1 W DC/DC converter is specified at a minimum load of 10 %, we also added an LED and resistor to provide a minimum load and at the same time a power indicator. 

# Software

The main software of RPi-Powermeter is the Powermeter command line application. Powermeter polls the ADC at given sampling intervals and logs samples concurrently to SD card. 

## Compiling Powermeter

The source code of Powermeter can be found in directory ```src/```. You can compile it using the following command:

    make powermeter

## Using Powermeter

Powermeter has a number of parameters:

* ```-f SPI_FREQUENCY```: SPI clock rate, e.g., 500000 for 500 kHz SPI clock rate. Since we operate the MCP3208 at 3.3 V, the clock rate should be <= 1 MHz (cf. MCP3208 datasheet).
* ```-s SS_PIN```: SPI slave select pin. The Raspberry Pi has two SS pins called CE0 and CE1. 0 selects CE0; 1 selects CE1. The measurement board uses CE0. 
* ```-a ADC_CHANNEL``` and ```-b ADC_CHANNEL```: ADC channels for voltage and current sampling. The measurement board uses ADC channel 0 for current sampling and ADC channel 1 for voltage sampling. 
* ```-F SAMPLING_FREQUENCY```: Sampling frequency. 1 kHz seems to be a safe upper bound where the Raspberry Pi can still deterministically meet the 1 ms sampling interval. Using much smaller sampling intervals is not reasonable since the measurement board implements a low-pass filter with 2 kHz cut-off frequency.
* ```-o FILE```: Output file for logging samples.

The output file format is CSV (comma-separated values). The first value is the actual timestamp of the samples followed by the raw 12 bit values (0-4095) of the two ADC channels, which need to be translated to V [V] and I [A] (see calibration) to calculate the power consumption P = V*I.

## Design of Powermeter

To ensure that samples are taken at precisely defined time intervals, Powermeter relies on a realtime operating system, namely, Linux with RT PREEMPT patch [1]). We refer to the website [2] to show how to install a RT PREEMPT kernel for Raspberry Pi.

In order to achieve deterministic sampling intervals, it is important to remove I/O operations from the critical path executing time-sensitive operations. To this end, Powermeter uses two threads: the time-sensitive sampling thread runs at high priority to poll the ADC at given time intervals. The logging thread concurrently writes samples to a file on SD card. The logging thread runs at lower priority than the sampling thread. If the logging thread is blocked by a long-running IO operation (for instance, when buffers are flushed to disk), the operating system can assign the CPU to the sampling thread. If the blocking I/O call were on the time-sensitive path in a single thread, it would prevent samples from being taken while this thread is blocked. Thus, multi-threading is beneficial also on a single-core machine! Both threads communicate through a ring buffer, which can hold several seconds of samples at 1 kHz, leaving plenty of time for the logging thread to catch up and write samples to disk. Tests showed that with this separation of realtime sampling and background logging, it is possible to achieve deterministic time bounds with 1 kHz sampling frequency on a single-core Raspberry Pi. 

To interface with the ADC through SPI, we use the bcm2835 library [3]. Using WiringPi [4] instead did not lead to deterministic time-bounds. Therefore, we recommend to use bcm2835 by adding ```-DBCM2835LIB``` to the CFLAGS of the Makefile (default setting).

# Calibration

As any measurement tool, also RPi-Powermeter needs to be calibrated first. In order to calculate power as P = V*I, we first must translate raw 12 bit ADC values of ADC channel 0 and 1 to current and voltage values, respectively. Translating ADC voltage samples is quite straightforward since the input values are usually quite large (e.g., several Volts for typical batteries). It is usually enough to consider that input voltage is divided by 2 by the voltage divider on the measurement board. Thus, you can calculate voltage as V = (s*2.5V/4096) * 2, where s is the voltage sample. However, if you like you can also use a similar calibration method as described for current values next.

For calibrating the translation of samples to current values, we first measure the true current, for instance, using a multimeter for several current values, while logging the ADC output using the powermeter tool. To this end, you can connect a resistor to the load connector of the measurement board and a bench power supply connected to the power connector. For instance, we used a 4.7 Ohm resistor rated for 9 W. Be sure not to exceed the power rating of your resistor and the limits of the measurement board (5 V / 1A)! For instance, at 4.7 V, a current of 1 A will flow through the resistor, so the power rating of the resistor should be greater than P = 4.7 V * 1 A = 4.7 W. 

Collect a sufficent number of ADC values for each known (true) current value. For instance, at 1 kHz sampling rate you will get 10000 samples after running powermeter for 10 s, which should be more than sufficient. Then, average the ADC values to remove noise. Now, you should have several pairs of (true current value [mA], averaged ADC value), e.g., (0, 116.8604), (101, 376.9123), (201, 673.4908), (302, 983.9772), (400, 1274.473), (500, 1588.119), (600, 1889.262), (700, 2187.948), (800, 2500.642).

The goal is now to derive a linear function 

    y(x) = x*m + c

where x is the ADC value, and y(x) is the current value calculated from the ADC value. To calculate the unknowns m and c, we can use linear regression, for instance, using R [5]:

    $ x<-c(116.8604, 376.9123, 673.4908, 983.9772, 1274.473, 1588.119, 1889.262, 2187.948, 2500.642)
    $ y<-c(0, 101, 201, 302, 400, 500, 600, 700, 800) 
    $ fm <- lm(y~x) 
    $ plot(x, y, type = "p") 
    $ abline(fm, col = "red") 
    $ fm  

The values used in this example are real values collected with a RPi-Powermeter board with Hall sensor. Although this sensor is quite noisy, you will see that the averaged samples fit a linear model quite nicely. 

All measurements, also including values for the shunt-based board, can be found in the folder ```calibration````.

# Evaluation

Finally, we evaluate the performance of both measurement boards with respect to noise. All measurements can be found in the folder ```calibration````. 

We measured a set of ADC values for a constant current of 201 mA for both boards with Hall sensor and shunt resistor, respectively. After calibration, one LSB of the ADC corresponds to about 332 uA for the hall sensor board and 248 uA for the shunt resistor board. The following results show the peak-to-peak values for both sensors: 

* Hall sensor board: min = 261; max = 1210; noise = 315 mA p-p
* Shunt resistor board: min = 763; max = 774; noise = 2.8 mA p-p

Note that the noise of the Hall sensor further increases if we leave out the low-pass filter. The depicted values are with low-pass filter.

In conclusion, the measurement board with the shunt resistor exhibits much lower noise and, thus, should be preferred as long as the main feature of the hall sensor (isolation of the power supply of the device under test from the measurement circuit/Raspberry Pi) is not required.

# References

* [1] RT PREEMPT: https://rt.wiki.kernel.org/index.php/Main_Page
* [2] Raspberry Pi Going Realtime with RT Preempt: http://www.frank-durr.de/?p=203
* [3] bcm2825 library: http://www.airspayce.com/mikem/bcm2835/ 
* [4] Wiring Pi library: http://wiringpi.com/
* [5] The R Project: https://www.r-project.org/
