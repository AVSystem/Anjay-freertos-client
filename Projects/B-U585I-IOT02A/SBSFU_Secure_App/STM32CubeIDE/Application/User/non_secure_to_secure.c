/*
 * Copyright 2020-2024 AVSystem <avsystem@avsystem.com>
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


#include "main.h"
#include "non_secure_to_secure.h"
#include "stm32u5xx_ll_utils.h"

CMSE_NS_ENTRY uint32_t Secure_GetUID_Word(int word_number)
{
	uint32_t ret_val = 0;

	switch(word_number){
	case 0:
		ret_val = LL_GetUID_Word0();
		break;
	case 1:
		ret_val = LL_GetUID_Word1();
		break;
	case 2:
		ret_val = LL_GetUID_Word2();
		break;
	default:
		break;
	}

	return ret_val;
}
