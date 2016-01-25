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

/* Choose your SPI library by setting one of the following definitions in
   the Makefile:
   - WIRINGPI for WiringPi (https://projects.drogon.net/raspberry-pi/wiringpi/)
   - BCM2835LIB for bcm2835 library (http://www.airspayce.com/mikem/bcm2835/)
*/

#ifdef BCM2835LIB
#include <bcm2835.h>
#endif

#ifdef WIRINGPI
#include <wiringPiSPI.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <sched.h>
#include <sys/mman.h>
#include "mcp320x.h"
#include "ring.h"

/* Default task priority */
#define DEFAULT_TASK_PRIORITY 49

/* Estimated maximum stack size */
#define MAX_STACK_SIZE (RING_SIZE*sizeof(struct ring_entry) + 1024) 

FILE *fout = NULL;

int task_priority;
struct timespec sampling_interval;
int spi_channel;
int spi_frequency;
enum channel_singleended adc_channel1;
enum channel_singleended adc_channel2;
double sampling_frequency;

struct ring the_ring;

pthread_t sampling_thread;
pthread_t logger_thread;

uint64_t max_delta = 0;

/**
 * Gracefully terminate the process.
 *
 * @param status process exit status
 */
void die(int status)
{
     if (fout != NULL)
	  fclose(fout);

     printf("Max. sampling interval was %llu ns\n", max_delta);

#ifdef BCM2835LIB
     bcm2835_spi_end();
     bcm2835_close();
#endif
     
     exit(status);
}

/**
 * SIGINT signal handler.
 */
void sig_int(int signo)
{
     pthread_cancel(sampling_thread);
     pthread_cancel(logger_thread);
}

/**
 * Print usage information.
 */
void usage(const char *appl)
{
     fprintf(stderr, "%s -s SPI_CHANNEL -f SPI_FREQUENCY -a ADC_CHANNEL1 "
	     "-b ADC_CHANNEL2 -F SAMPLING_FREQUENCY "
	     "-o LOGFILE [-p TASK_PRIORITY] \n", appl);
}

/**
 * Pref-fault stack memory for deterministic time to access stack memory.
 */
void stack_prefault(void)
{
     unsigned char dummy[MAX_STACK_SIZE];
     memset(dummy, 0, MAX_STACK_SIZE);
     return;
}

/**
 * Convert a frequency value to a time interval.
 *
 * @param frequency frequency in Hertz
 * @return time interval
 */
struct timespec frequency_to_interval(double frequency)
{
     struct timespec itimespec;
     
     /* Calculate interval in plain nanoseconds.
        Note: 64 bit, which is enough for thousands of years sampling 
	intervals. */
     uint64_t ins = (uint64_t) (1000000000.0/frequency + 0.5);

     itimespec.tv_sec = ins/1000000000ull;
     itimespec.tv_nsec = ins%1000000000ull;

     return itimespec;
}

/**
 * Calculate timestamp of next sample.
 *
 * @param tlast time of last sample
 * @param interval sampling interval
 * @return time of next sample
 * 
 */
struct timespec next_sampling_time(struct timespec tlast,
				   struct timespec interval)
{
     struct timespec tnext;
     
     tnext.tv_sec = tlast.tv_sec+interval.tv_sec;
     tnext.tv_nsec = tlast.tv_nsec+interval.tv_nsec;

     /* Normalize */
     if (tnext.tv_nsec >= 1000000000l) {
	  tnext.tv_sec++;
	  tnext.tv_nsec -= 1000000000l;
     }

     return tnext;
}

/**
 * Convert timespec values (sec, ns) to plain 64 bit nanosecond value.
 *
 * @param t timespec values
 * @return value in nanoseconds corresponding to timespec values
 */
uint64_t to_nanosec(struct timespec t)
{
     uint64_t t_ns = 1000000000ull*t.tv_sec;
     t_ns += t.tv_nsec;

     return t_ns;
}

/**
 * Write timestamped samples as to log file.
 *
 * Format: comma-separated values 
 * timestamp [nanoseconds], value 
 *
 * @param fout output file
 * @param sample sample value
 * @param tsample timestamp of sample
 */
void log_sample(FILE *fout, int16_t sample1, int16_t sample2, uint64_t t)
{
     fprintf(fout, "%llu,%d,%d\n", t, sample1, sample2);

     /*
     uint8_t buffer[10];
     memcpy(buffer, &t, 8);
     memcpy(&buffer[8], &sample, 2);
     fwrite(buffer, 1, 10, fout);
     */

     /*
     fflush(fout);
     */
}

/**
 * Main loop of sampling thread.
 */
void *sampling_thread_loop(void *args)
{
     /* Set high ("realtime") priority for sampling thread */

     struct sched_param schedparam;
     schedparam.sched_priority = task_priority;
     if (sched_setscheduler(0, SCHED_FIFO, &schedparam) == -1) {
	  perror("sched_setscheduler failed");
	  die(-1);
     }
    
     /* Start sampling in an infinite loop (until thread is canceled) */
     
     struct timespec tsample;
     clock_gettime(CLOCK_MONOTONIC, &tsample);
     uint64_t tprevious_ns = to_nanosec(tsample);
     while (true) {
	  // Take a sample 
	  int16_t sample1 = get_sample_singleended(adc_channel1, spi_channel);
	  int16_t sample2 = get_sample_singleended(adc_channel2, spi_channel);
	  
	  // Timestamp sample
	  struct timespec tnow;
      	  clock_gettime(CLOCK_MONOTONIC , &tnow);

	  uint64_t tnow_ns = to_nanosec(tnow);
	  uint64_t delta = tnow_ns-tprevious_ns;
	  if (delta > max_delta) {
	       max_delta = delta;
	  }
	  tprevious_ns = tnow_ns;

	  if (sample1 == -1 || sample2 == -1) {
	       fprintf(stderr, "Error while taking sample\n");
	  } else {
	       struct ring_entry entry;
	       entry.timestamp = to_nanosec(tnow);
	       entry.value1 = sample1;
	       entry.value2 = sample2;
	       ring_put(&the_ring, &entry);
	  }
	  
	  // Sleep until next sampling time
	  tsample = next_sampling_time(tsample, sampling_interval);
	  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tsample, NULL);
     }
}

/**
 * Main loop of logger thread.
 */
void *logger_thread_loop(void *args)
{
     /* Set high ("realtime") priority for sampling thread */

     struct sched_param schedparam;
     /* Give logger thread a lower priority than sampling thread, so on a 
        single core system it does not get into the way of the sampling
	thread. */ 
     schedparam.sched_priority = task_priority-1;
     if (sched_setscheduler(0, SCHED_FIFO, &schedparam) == -1) {
	  perror("sched_setscheduler failed");
	  die(-1);
     }
     
     while (true) {
	  struct ring_entry entry;
	  ring_get(&the_ring, &entry);
	  log_sample(fout, entry.value1, entry.value2, entry.timestamp);
     }
}

/**
 * Convert ADC channel number to MCP320x channel definition.
 * 
 * @param channel number
 * @param mcp320x_channel single-ended MCP320x channel definition
 * @return 0 if channel is in valid range [0,7]; -1 otherwise
 */
int to_mcp320x_channel(int channel, enum channel_singleended *mcp320x_channel)
{
     switch (channel) {
     case 0:
	  *mcp320x_channel = CH0;
	  break;
     case 1:
	  *mcp320x_channel = CH1;
	  break;
     case 2:
	  *mcp320x_channel = CH2;
	  break;
     case 3:
	  *mcp320x_channel = CH3;
	  break;
     case 4:
	  *mcp320x_channel = CH4;
	  break;
     case 5:
	  *mcp320x_channel = CH5;
	  break;
     case 6:
	  *mcp320x_channel = CH6;
	  break;
     case 7:
	  *mcp320x_channel = CH7;
	  break;
     default:
	  return -1;
     }

     return 0;
}


/**
 * The main function.
 */
int main(int argc, char *argv[])
{
     /* Parse arguments */
     
     char *spi_channel_arg = NULL;
     char *adc_channel1_arg = NULL;
     char *adc_channel2_arg = NULL;
     char *spi_frequency_arg = NULL;
     char *sampling_frequency_arg = NULL;
     char *logfile_arg = NULL;
     char *task_priority_arg = NULL;
     int c;
     while ((c = getopt(argc, argv, "s:a:b:f:F:o:p:")) != -1) {
	  switch (c) {
	  case 's' :
	       spi_channel_arg = malloc(strlen(optarg)+1);
	       strcpy(spi_channel_arg, optarg);
	       break;
	  case 'a' :
	       adc_channel1_arg = malloc(strlen(optarg)+1);
	       strcpy(adc_channel1_arg, optarg);
	       break;
	  case 'b' :
	       adc_channel2_arg = malloc(strlen(optarg)+1);
	       strcpy(adc_channel2_arg, optarg);
	       break;
	  case 'f' :
	       spi_frequency_arg = malloc(strlen(optarg)+1);
	       strcpy(spi_frequency_arg, optarg);
	       break;
	  case 'F' :
	       sampling_frequency_arg = malloc(strlen(optarg)+1);
	       strcpy(sampling_frequency_arg, optarg);
	       break;
	  case 'o' :
	       logfile_arg = malloc(strlen(optarg)+1);
	       strcpy(logfile_arg, optarg);
	       break;
	  case 'p' :
	       task_priority_arg = malloc(strlen(optarg)+1);
	       strcpy(task_priority_arg, optarg);
	       break;
	  case '?':
	       fprintf(stderr, "Unknown option\n");
	       usage(argv[0]);
	       die(-1);
	       break;
	  }
     }
	      
     if (spi_frequency_arg == NULL || adc_channel1_arg == NULL ||
	 adc_channel2_arg == NULL || spi_channel_arg == NULL ||
	 sampling_frequency_arg == NULL ||
	  logfile_arg == NULL) {
	  usage(argv[0]);
	  die(-1);
     }

     spi_channel = atoi(spi_channel_arg);
     spi_frequency = atoi(spi_frequency_arg);
     sampling_frequency = strtod(sampling_frequency_arg, NULL);

     uint8_t channel = atoi(adc_channel1_arg);
     if (to_mcp320x_channel(channel, &adc_channel1) == -1) {
	  fprintf(stderr, "ADC channel must in in range 0 to 7\n");
	  die(-1);
     }

     channel = atoi(adc_channel2_arg);
     if (to_mcp320x_channel(channel, &adc_channel2) == -1) {
	  fprintf(stderr, "ADC channel must in in range 0 to 7\n");
	  die(-1);
     }
	  
     sampling_interval = frequency_to_interval(sampling_frequency);

     if (task_priority_arg != NULL) {
	  task_priority = atoi(task_priority_arg);
     } else {
	  task_priority = DEFAULT_TASK_PRIORITY;
     }
     
     /* Setup SPI */

     /* with WiringPi */
#ifdef WIRINGPI
     int ret;
     ret = wiringPiSPISetup(spi_channel, spi_frequency);
     if (ret < 0) {
	  perror("Could not setup SPI");
	  die(-1);
     }
#endif
     
#ifdef BCM2835LIB
     /* with BCM2835 library */
     bcm2835_init();
     bcm2835_spi_begin();
     
     if (spi_channel == 0)
	  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
     else
	  bcm2835_spi_chipSelect(BCM2835_SPI_CS1);

     // Set divider according to requested spi frequency
     uint16_t divider = (uint16_t) ((double) 250000000/spi_frequency + 0.5);
     bcm2835_spi_setClockDivider(divider);

     // SPI 0,0 as per MCP3208 data sheet
     bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);

     bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);

     bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
#endif
     
     /* Open log file */

     fout = fopen(logfile_arg, "w");
     if (fout == NULL) {
	  perror("Could not open log file");
	  die(-1);
     }

     // Sampling and logging thread communicate through the ring buffer.

     ring_init(&the_ring);

     /* Lock memory and prefault stack */

     if (mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
	  perror("mlockall failed");
	  die(-1);
     }

     stack_prefault();
     
     /* Create threads */
     
     if (pthread_create(&sampling_thread, NULL, sampling_thread_loop, NULL)) {
	  perror("Could not create sampling thread");
	  die(-1);
     }

     if (pthread_create(&logger_thread, NULL, logger_thread_loop, NULL)) {
	  perror("Could not create logger thread");
	  die(-1);
     }

     /* Install SIGINT signal handler for graceful termination */
     
     if (signal(SIGINT, sig_int) == SIG_ERR) {
	  perror("Could not set signal handler for SIGINT");
	  die(-1);
     }
     
     pthread_join(logger_thread, NULL);
     pthread_join(sampling_thread, NULL);

     die(0);
}
