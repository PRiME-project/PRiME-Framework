/* This file is part of the PRiME Framework.
 * 
 * The PRiME Framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * The PRiME Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the PRiME Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Graeme Bragg
 */

#ifndef PRIME_APP_RANDOM_H
#define PRIME_APP_RANDOM_H

/* ==================================================================================== */
/* ============================= App Random ID generation ============================= */
/* ==================================================================================== */
/* Generate the compile-time random ID used by learning RTMs to track applications.
 * This ideally requires COMPILE_EPOCH and COMPILE_RAND to be defined externally but
 * can be generated here if not defined in the build system. In cmake, this can be 
 * done with:
 * set_target_properties(<app_target> PROPERTIES COMPILE_FLAGS "-DCOMPILE_RAND=$$(rand) -DCOMPILE_EPOCH=$$(date +%s)")
 */
 
#ifndef COMPILE_EPOCH
#define COMPILE_EPOCH (unsigned long int __TIME__)
#endif

#ifndef COMPILE_RAND
#define COMPILE_RAND (unsigned long int __COUNTER__)
#endif

// A(n insecure) random number generator modified from https://stackoverflow.com/a/17420032
#define UL unsigned long
#define znew  ((36969*(z&65535)+(z>>16))<<16)
#define wnew  ((COMPILE_RAND*(w&65535)+(w>>16))&65535)
#define MWC   (znew+wnew)
#define SHR3  (((jsr^(jsr<<17))^((jsr^(jsr<<17))>>13))^(((jsr^(jsr<<17))^((jsr^(jsr<<17))>>13))<<5))
#define CONG  (69069*jcong+COMPILE_RAND)
#define COMPILE_UR_ID  ((MWC^CONG)+SHR3)
static UL z=362436069*(unsigned long int)COMPILE_EPOCH, w=521288629*(unsigned long int)COMPILE_EPOCH, \
   jsr=123456789*(unsigned long int)COMPILE_EPOCH, jcong=380116160*(unsigned long int)COMPILE_EPOCH;
/* ==================================================================================== */

#endif
