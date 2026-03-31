/*
 * Copyright 2020-2026 AVSystem <avsystem@avsystem.com>
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

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <anjay/core.h>

void persistence_clear(void);
int persistence_mod_restore(anjay_t *anjay);
int persistence_mod_persist_if_required(anjay_t *anjay);

#ifdef ANJAY_WITH_CORE_PERSISTENCE
void persistence_core_clear(void);
anjay_t *persistence_core_try_anjay_new(const anjay_configuration_t *config,
                                        int *status);
void persistence_core_try_anjay_delete(anjay_t *anjay);
#endif // ANJAY_WITH_CORE_PERSISTENCE

#endif // PERSISTENCE_H
