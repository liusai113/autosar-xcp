/* Copyright (C) 2010 Joakim Plate
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Xcp.h"
#include "Xcp_Internal.h"

#if(XCP_FEATURE_DIO == STD_ON)
#include "Dio.h"
#endif

#define XCP_DEBUG_MEMORY

#ifdef XCP_DEBUG_MEMORY
uint8_t g_XcpDebugMemory[1024];
#endif

/**
 * Read a character from MTA
 * @return
 */
static uint8 Xcp_MtaGetMemory(Xcp_MtaType* mta)
{
    return *(uint8*)(mta->address++);
}

/**
 * Write a character to MTA
 * @param val
 */
static void Xcp_MtaPutMemory(Xcp_MtaType* mta, uint8 val)
{
    *(uint8*)(mta->address++) = val;
}

#if(XCP_FEATURE_DIO == STD_ON)
/**
 * Read a character from DIO
 * @return
 */
static uint8 Xcp_MtaGetDio(Xcp_MtaType* mta)
{
    unsigned int offset = mta->address % sizeof(Dio_PortLevelType);
    Dio_PortType port   = mta->address / sizeof(Dio_PortLevelType);

    if(offset == 0) {
        mta->buffer = Dio_ReadPort(port);
    }
    mta->address++;
    return (mta->buffer >> offset * 8) & 0xFF;
}

/**
 * Flush data in buffer to DIO
 * @return
 */
static void Xcp_MtaFlushDio(Xcp_MtaType* mta)
{
    Dio_PortType port = mta->address / sizeof(Dio_PortLevelType);
    Dio_WritePort(port, mta->buffer);
    mta->buffer = 0;
}

/**
 * Write a character to DIO
 * @param val
 */
static void Xcp_MtaPutDio(Xcp_MtaType* mta, uint8 val)
{
    unsigned int offset = mta->address % sizeof(Dio_PortLevelType);
    mta->buffer = (mta->buffer & ~(0xFF << offset)) | (val << offset);
    mta->address++;
    if(offset == 0) {
        Xcp_MtaFlushDio(mta);
    }
}


#endif

/**
 * Generic function that writes character to mta using put
 * @param val
 */
static void Xcp_MtaWriteGeneric(Xcp_MtaType* mta, uint8* data, int len)
{
    while(len-- > 0) {
        mta->put(mta, *(data++));    }
}

/**
 * Generic function that reads buffer from mta using get
 * @param val
 */
static void Xcp_MtaReadGeneric(Xcp_MtaType* mta, uint8* data, int len)
{
    while(len-- > 0) {
        *(data++) = mta->get(mta);
    }
}

/**
 * Set the MTA pointer to given address on given extension
 * @param address
 * @param extension
 */
void Xcp_MtaInit(Xcp_MtaType* mta, intptr_t address, uint8 extension)
{
    Xcp_MtaFlush(mta);
    mta->address   = address;
    mta->extension = extension;
    mta->read      = Xcp_MtaReadGeneric;
    mta->write     = Xcp_MtaWriteGeneric;
    mta->flush     = NULL;

    if(extension == XCP_MTA_EXTENSION_MEMORY) {
        mta->get   = Xcp_MtaGetMemory;
        mta->put   = Xcp_MtaPutMemory;
#ifdef XCP_DEBUG_MEMORY
    } else if(extension == XCP_MTA_EXTENSION_DEBUG) {
        mta->address = (intptr_t)g_XcpDebugMemory + address;
        mta->get   = Xcp_MtaGetMemory;
        mta->put   = Xcp_MtaPutMemory;
#endif
    } else if(extension == XCP_MTA_EXTENSION_FLASH) {
        mta->get   = Xcp_MtaGetMemory;
        mta->put   = NULL;
        mta->read  = Xcp_MtaReadGeneric;
        mta->write = NULL;
#if(XCP_FEATURE_DIO == STD_ON)
    } else if(extension == XCP_MTA_EXTENSION_DIO) {
        mta->get   = Xcp_MtaGetDio;
        mta->put   = Xcp_MtaPutDio;
        mta->flush = Xcp_MtaFlushDio;
        /* if not aligned to start of port, we must fill buffer */
        unsigned int offset = address % sizeof(Dio_PortLevelType);
        mta->address -= offset;
        while(offset--)
            Xcp_MtaGetDio(mta);
#endif
    } else {
        mta->get   = NULL;
        mta->put   = NULL;
        mta->read  = NULL;
        mta->write = NULL;
    }
}

