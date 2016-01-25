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

#include "ring.h"

void ring_init(struct ring *r)
{
     r->entrycnt = 0;
     
     pthread_mutex_init(&r->mutex, NULL);
     
     pthread_cond_init(&r->notempty, NULL);
     pthread_cond_init(&r->notfull, NULL);
}

void ring_destroy(struct ring* r)
{
     pthread_cond_destroy(&r->notempty);
     pthread_cond_destroy(&r->notfull);

     pthread_mutex_destroy(&r->mutex);
}

void ring_put(struct ring *r, const struct ring_entry *e)
{
     pthread_mutex_lock(&r->mutex);
     
     while (r->entrycnt == RING_SIZE) {
	  pthread_cond_wait(&r->notfull, &r->mutex);
     }

     r->entries[r->head] = *e;
     r->entrycnt++;
     r->head = (r->head+1) & RING_SIZE_MODMASK;

     pthread_cond_signal(&r->notempty);
     
     pthread_mutex_unlock(&r->mutex);
}

void ring_get(struct ring *r, struct ring_entry *e)
{
     pthread_mutex_lock(&r->mutex);

     while (r->entrycnt == 0) {
	  pthread_cond_wait(&r->notempty, &r->mutex);
     }

     *e = r->entries[r->tail];
     r->entrycnt--;
     r->tail = (r->tail+1) & RING_SIZE_MODMASK;
     
     pthread_cond_signal(&r->notfull);
	  
     pthread_mutex_unlock(&r->mutex);
}

