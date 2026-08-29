/*
 * Olive Community Edition - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBOLIVECORE_CPUOPTIMIZE_H
#define LIBOLIVECORE_CPUOPTIMIZE_H

#if defined(__x86_64__) || defined(__i386__)
#define OLIVE_PROCESSOR_X86
#include <xmmintrin.h>
#elif defined(__aarch64__)
#define OLIVE_PROCESSOR_ARM
#include "sse2neon.h"
#endif

#endif // LIBOLIVECORE_CPUOPTIMIZE_H
