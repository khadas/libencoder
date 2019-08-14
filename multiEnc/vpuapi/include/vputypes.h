
/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef _VPU_TYPES_H_
#define _VPU_TYPES_H_

#include <stdint.h>
#include <stddef.h>

#define STATIC          static
/**
* @brief    This type is an 8-bit unsigned integral type, which is used for declaring pixel data.
*/
typedef uint8_t         Uint8;

/**
* @brief    This type is a 32-bit unsigned integral type, which is used for declaring variables with wide ranges and no signs such as size of buffer.
*/
typedef uint32_t        Uint32;

/**
* @brief    This type is a 16-bit unsigned integral type.
*/
typedef uint16_t        Uint16;

/**
* @brief    This type is an 8-bit signed integral type.
*/
typedef int8_t          Int8;

/**
* @brief    This type is a 32-bit signed integral type.
*/
typedef int32_t         Int32;

/**
* @brief    This type is a 16-bit signed integral type.
*/
typedef int16_t         Int16;
#if defined(_MSC_VER)
typedef unsigned __int64 Uint64;
typedef __int64          Int64;
#else
typedef uint64_t        Uint64;
typedef int64_t         Int64;
#endif
#ifndef PhysicalAddress
/**
* @brief    This is a type for representing physical addresses which are recognizable by VPU. 
In general, VPU hardware does not know about virtual address space 
which is set and handled by host processor. All these virtual addresses are 
translated into physical addresses by Memory Management Unit. 
All data buffer addresses such as stream buffer and frame buffer should be given to
VPU as an address on physical address space.
*/
typedef Uint32 PhysicalAddress;
#endif
#ifndef BYTE
/**
* @brief This type is an 8-bit unsigned integral type.
*/
typedef unsigned char   BYTE;
#endif
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE            1
#endif /* TRUE */
#ifndef FALSE
#define FALSE           0
#endif /* FALSE */
#ifndef NULL
#define NULL	0
#endif

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P)          \
    /*lint -save -e527 -e530 */ \
{ \
    (P) = (P); \
} \
    /*lint -restore */
#endif

#ifdef __GNUC__
#define UNREFERENCED_FUNCTION __attribute__ ((unused))
#else
#define UNREFERENCED_FUNCTION
#endif

#endif	/* _VPU_TYPES_H_ */
 
