/*
 * Copyright 2020-2025 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PLF_CUSTOM_CONFIG_H
#define PLF_CUSTOM_CONFIG_H

// Do not change, ST's library uses it
#define USE_DEFAULT_SETUP 1

// LwM2M client threads created with static allocation
#ifdef USE_AIBP
#define LWM2M_THREAD_STACK_SIZE (5224U)
#else
#define LWM2M_THREAD_STACK_SIZE (5096U)
#endif
#define LWM2M_THREAD_PRIO osPriorityNormal

#define BOARD_BUTTONS_THREAD_STACK_SIZE (256U)
#define BOARD_BUTTONS_THREAD_PRIO osPriorityBelowNormal

// Number of datacache event callbacks defined by application
#define APPLICATION_DATACACHE_NB 1

// Number of datacache event entires defined by application
#define APPLICATION_DATACACHE_ENTRIES_NB 1

// Separate Debug channel for application
#define USE_DBG_CHAN_APPLICATION 1

// Do not change, ST's Trace interface uses it
#define USE_CUSTOM_CLIENT 1
#define USE_TRACE_CUSTOM_CLIENT 1

#define USE_SENSORS 1

#endif // PLF_CUSTOM_CONFIG_H
