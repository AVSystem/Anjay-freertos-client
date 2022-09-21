/*
 * Copyright 2020-2022 AVSystem <avsystem@avsystem.com>
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

#ifndef AI_BRIDGE_H
#define AI_BRIDGE_H

#include <stddef.h>

#define AI_BRIDGE_ANOMALY_HEADER 'A'
#define AI_BRIDGE_CLASSIFIER_HEADER 'C'
#define AI_BRIDGE_CLASSIFIER_CLASSES 'D'

#define AI_BRIDGE_ANOMALY_MSG 'a'
#define AI_BRIDGE_CLASSIFIER_MSG 'c'

#define AI_BRIDGE_UNKNOWN_TYPE -1
#define AI_BRIDGE_ANOMALY_TYPE 0
#define AI_BRIDGE_CLASSIFIER_TYPE 1

extern char **ai_bridge_classes;
extern int ai_bridge_num_of_classes;
extern int ai_bridge_type;

uint8_t ai_bridge_get_type();
uint8_t ai_bridge_get_classes();
void ai_bridge_learn(void);
void ai_bridge_read_anomaly(char *buffer, float *anomaly);
void ai_bridge_start_it(void);

#endif // AI_BRIDGE_H
