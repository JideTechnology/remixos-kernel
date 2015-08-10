/*COPYRIGHT**
 * -------------------------------------------------------------------------
 *               INTEL CORPORATION PROPRIETARY INFORMATION
 *  This software is supplied under the terms of the accompanying license
 *  agreement or nondisclosure agreement with Intel Corporation and may not
 *  be copied or disclosed except in accordance with the terms of that
 *  agreement.
 *        Copyright (C) 2008-2014 Intel Corporation.  All Rights Reserved.
 * -------------------------------------------------------------------------
**COPYRIGHT*/

/*
    Description:    Definitions shared between SEP3UserClient (kernel) and
                    SEP3UserClientTool (userland).
*/

#ifndef _USERKERNELSHARED_H_
#define _USERKERNELSHARED_H_

#define SEPDriverClassName          com_intel_driver_SEP3_16

#define kSEPDriverClassName         "com_intel_driver_SEP3_16"
#define kSEPPCIDriverClassName      "com_intel_driver_SEP3_16PCIDriver"

// Data structure passed between the tool and the user client. This structure and its fields need to have
// the same size and alignment between the user client, 32-bit processes, and 64-bit processes.
// To avoid invisible compiler padding, align fields on 64-bit boundaries when possible
// and make the whole structure's size a multiple of 64 bits.

#define kMyStopListeningMessage     555555
#define SEP_PROCESSED_MARKER        0xDEADBEEF

// Keeping it to be same as ModuleInfoMemoryType
#define ModuleInfoAsyncCall         4800

// Command Gate command defines...
#define ENQUEUE_DATA                1
#define GET_DATA                    2
#define ASYNCHRONOUS_STOP           3
#define NORMAL_STOP                 4

#define MAX_CPUS                    24
// 16*512*1024 = 8388608
// 16*16*1024  = 262144
// 16*2048*1024= 33554432
#define CPU_DATA                    ((MAX_CPUS)*512*1024)
// 2048*1024= 2097152
// 512*1024 = 524288
// 16*1024  = 16384
#define CPU_PERF_DATA               524288
#define IOCTL_DATA                  524288
#define SEP_MODULE_DATA             131072
#define SEP_MARKER_DATA             ((MAX_CPUS)*1024)
#define BUCKET_COUNT                4
#define MOD_BUCKET_COUNT            2
#define MARKER_BUCKET_COUNT         2

// 2ndry thread processing commands.
#define PROCESS_DATA                1
#define TERMINATE_EXIT              2

// We are using these high numbers to make sure we map after
// CPUs. Each cpu will map to its own memory in clientMemoryForType
// These memory types should come AFTER those types.

enum {
    kIODriverMemoryType             = 4098,
    kIODriverDataMemoryType_0       = 4099,
    kIODriverDataMemoryType_1       = 4100,
    kIODriverDataMemoryType_2       = 4101,
    kIODriverDataMemoryType_3       = 4102,
    kIODriverAsyncMemoryType        = 4103,
    kIODriverModuleInfoMemoryType_0 = 4801,
    kIODriverModuleInfoMemoryType_1 = 4802,
    kIODriverMarkerInfoMemoryType_0 = 4901,
    kIODriverMarkerInfoMemoryType_1 = 4902,
};


// User client method dispatch selectors.
enum abstract_DriverMethods {
    abstract_Open_Driver,
    abstract_Close_Driver,
    kMyCheckForData,
    kMyExecuteMethod,
    kMyScalarIScalarOMethod,
    kMyModuleInfoData,
    kMyMarkerInfoData,
    kNumberOfMethods            // Must be last
};

struct SEP_Data_S {
    volatile uint64_t DataSize;
    volatile uint32_t BucketCounter;
};

typedef struct SEP_Data_S        SEP_Data;
typedef        SEP_Data          *SEP_Data_p;

struct INTR_Data_S {
    volatile uint64_t CPU;
    volatile uint32_t BucketCounter;
};

typedef struct INTR_Data_S        INTR_Data;
typedef        INTR_Data          *INTR_Data_p;

enum {
    kSEPIOModuleInfoSetup                     = 0x10,
    kSEPIOModuleInfoData                      = 0x20,
    kSEPIOModuleInfoIsRequestingClose         = 0x30,
    kSEPIOModuleInfoIsRequestingCloseWithData = 0x40,
};

enum asyncModInfoCommands {
    kSEPIOModInfoDummy       = 0,        // When there is nothing to send use dummy
    kSEPIOModInfoBuffer      = 0,
    kSEPIOModInfoBufferLenth = 1,        // Send buffer length
    kSEPIOModInfoFileDesc    = 1,        // In case of setup, send file descriptor.
    kSEPIOModInfoCommand     = 2,        // This will carry type of command (Setup, data, close etc..)
    kNumberofModuleInfoCommands          // Must be last
};

enum {
    kSEPIOMarkerInfoSetup                     = 0x10,
    kSEPIOMarkerInfoData                      = 0x20,
    kSEPIOMarkerInfoIsRequestingClose         = 0x30,
    kSEPIOMarkerInfoIsRequestingCloseWithData = 0x40,
};

enum asyncMarkerInfoCommands {
    kSEPIOMarkerInfoDummy       = 0,     // When there is nothing to send use dummy
    kSEPIOMarkerInfoBuffer      = 0,     //
    kSEPIOMarkerInfoBufferLenth = 1,     // Send buffer length
    kSEPIOMarkerInfoFileDesc    = 1,     // In case of setup, send file descriptor.
    kSEPIOMarkerInfoCommand     = 2,     // This will carry type of command (Setup, data, close etc..)
    kNumberofMarkerInfoCommands          // Must be last
};

#endif

