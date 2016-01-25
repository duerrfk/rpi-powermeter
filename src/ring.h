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

#ifndef RING_H
#define RING_H

#include <pthread.h>
#include <stdint.h>

/* At a sampling rate of 1000 Hz, we can buffer more than 8 seconds of samples.
   If the logging thread cannot chatch up in this timespan, the systems is
   definitely too slow for the sampling rate. */
#define RING_SIZE 8192
#define RING_SIZE_MODMASK ((RING_SIZE)-1)

struct ring_entry {
     uint64_t timestamp;
     uint16_t value1;
     uint16_t value2;
};

struct ring {
     struct ring_entry entries[RING_SIZE];

     unsigned int head;
     unsigned int tail;

     unsigned int entrycnt;
     
     pthread_cond_t notempty;
     pthread_cond_t notfull;

     pthread_mutex_t mutex;
};

/**
 * Initialize new ring.
 * 
 * @param r the ring to be initialized
 */
void ring_init(struct ring *r);

/**
 * Destroy a ring.
 *
 * @param r the ring to be destroyed
 */
void ring_destroy(struct ring* r);

/**
 * Add an entry to a ring.
 * 
 * @param r the ring
 * @param e the entry to be added
 */
void ring_put(struct ring *r, const struct ring_entry *e);

/**
 * Get and remove an entry from a ring.
 * 
 * @param r the ring
 * @param e structure to copy the entry to.
 */
void ring_get(struct ring *r, struct ring_entry *e);

#endif
