/*
 * Copyright 2020 AVSystem <avsystem@avsystem.com>
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

// LwM2M client works as custom client in used ST's library
#define CUSTOMCLIENT_THREAD_STACK_SIZE (32768U)
#define CUSTOMCLIENT_THREAD_PRIO osPriorityNormal
#define CUSTOMCLIENT_THREAD 1 /* Number of threads created by CUSTOM_CLIENT */

#define NOTIFICATION_THREAD_STACK_SIZE (512U)
#define NOTIFICATION_THREAD_PRIO osPriorityNormal
#define NOTIFICATION_THREAD 1 /* Number of threads created by NOTIFICATION */

// Do not change, ST's Trace interface uses it
#define USE_CUSTOM_CLIENT 1
#define USE_TRACE_CUSTOM_CLIENT 1

#endif // PLF_CUSTOM_CONFIG_H
