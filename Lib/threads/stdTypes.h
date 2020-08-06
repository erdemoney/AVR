/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *         stdThread type translations on the AVR
 */

#ifndef stdTypes_INCLUDED
#define stdTypes_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------- Types ----------------------------------*/

#include <inttypes.h>

typedef unsigned char    Char;
typedef unsigned char    Byte;
typedef unsigned char    Bool;

typedef uint16_t        uInt;
typedef  int16_t         Int;
typedef uint32_t        uInt32;
typedef  int32_t         Int32;
typedef uint16_t        uInt16;
typedef  int16_t         Int16;
typedef uint8_t         uInt8;
typedef  int8_t          Int8;

typedef void*           Pointer;
typedef Char*           String;
typedef float           Float;

/*--------------------------------- Constants --------------------------------*/

#define Null            0
#define False           0
#define True            1


#ifdef __cplusplus
}
#endif

#endif

