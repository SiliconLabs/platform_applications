/***************************************************************************//**
 * @file bt_types.h
 * @brief type definitions for minible library
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef BT_TYPES_H
#define BT_TYPES_H

#include <stdint.h>

typedef enum {
  btStatusSuccess                         = 0x00,
  btStatusUnkownConnectionIdentifier      = 0x02,
  btStatusMemoryCapacityExceeded          = 0x07,
  btStatusTimeout                         = 0x08,
  btStatusUnsupportedFeature              = 0x11,
  btStatusInvalidParameter                = 0x12,
  btStatusRemoteTermination               = 0x13,
  btStatusLocalTermination                = 0x16,
  btStatusInstantPassed                   = 0x28,
  btStatusFailedToEstablish               = 0x3e,
} BtStatus_t;

typedef enum {
  attStatusSuccess                        = 0x00,
  attStatusInvalidHandle                  = 0x01,
  attStatusReadNotPermitted               = 0x02,
  attStatusWriteNotPermitted              = 0x03,
  attStatusInvalidPdu                     = 0x04,
  attStatusInsufficientAuthentication     = 0x05,
  attStatusRequestNotSupported            = 0x06,
  attStatusInvalidOffset                  = 0x07,
  attStatusAttributeNotFound              = 0x0a,
  attStatusInvalidAttributeValueLength    = 0x0d,
  attStatusUnlikelyError                  = 0x0e,
  attStatusInsufficientResources          = 0x11,
  attStatusApplication                    = 0x80,
} AttStatus_t;

typedef struct Uuid {
  const void *data;
  uint8_t length;
} Uuid_t;

#define UUIDX(dataPtr, dataLength)  { (const void *)(dataPtr), (dataLength) }
#define UUID16(dataPtr)             UUIDX(dataPtr, 2)
#define UUID128(dataPtr)            UUIDX(dataPtr, 16)

typedef uint16_t AttHandle_t;

typedef struct BtDatabase BtDatabase_t;

typedef enum btAddress_type {
  btAddressTypePublic = 0,
  btAddressTypeRandom = 1,
} BtAddressType_t;

typedef struct btAddress {
  uint8_t address[6];
  BtAddressType_t type : 1;
} BtAddress_t;

typedef struct {
  int16_t txPower;   // (0 => 0 dBm, 15 = 1.5 dBm)
  BtAddress_t *address;
} BtConfig_t;

typedef enum {
  attPropertyNone                 = 0x00,
  attPropertyBroadcast            = 0x01,
  attPropertyRead                 = 0x02,
  attPropertyWriteWithoutResponse = 0x04,
  attPropertyWrite                = 0x08,
  attPropertyNotify               = 0x10,
  attPropertyIndicate             = 0x20,
} AttProperty_t;

typedef struct {
  const void *ptr;
  uint16_t length;
} AttributeValue_t;

// start of internal structures
typedef struct GattAttribute {
  struct GattAttribute *next;
  Uuid_t type;
  AttProperty_t properties;
  AttributeValue_t value;
} GattAttribute_t;
typedef struct { GattAttribute_t attribute[1]; } GattServiceInternal_t;
typedef struct { GattAttribute_t attribute[2]; uint8_t declaration[19]; } GattCharacteristicInternal_t;
typedef struct { GattAttribute_t attribute[1]; } GattDescriptorInternal_t;
// end of interal structures

typedef struct {
  Uuid_t uuid;
  GattServiceInternal_t _internal;
} GattService_t;

typedef struct {
  Uuid_t uuid;
  AttProperty_t properties;
  AttributeValue_t value;
  GattCharacteristicInternal_t _internal;
} GattCharacteristic_t;

typedef struct {
  Uuid_t type;
  AttProperty_t properties;
  AttributeValue_t value;
  GattDescriptorInternal_t _internal;
} GattDescriptor_t;

typedef struct {
  GattAttribute_t *attribute;
  AttHandle_t size;
} GattDatabase_t;

typedef enum {
  attributeValueOpRead,
  attributeValueOpWrite,
} AttributeValueOp_t;

typedef AttStatus_t (*AttributeValueCallback_t)(AttributeValueOp_t op, AttributeValue_t *params);

#endif
