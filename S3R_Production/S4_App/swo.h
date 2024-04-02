/*
 * swo.h
 *
 *  Created on: Mar 29, 2024
 *      Author: MarkNolan
 */

#ifndef SWO_H_
#define SWO_H_

#include <stdint.h>

void log_error(char *p);
void log_info(char *p);
void log_debug(char *p);
void log_debug_array(char const * const label, void const *array, uint16_t const len);

#endif /* SWO_H_ */
