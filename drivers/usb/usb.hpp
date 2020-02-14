#pragma once

#include <stdint.h>

namespace usb
{
    enum {
        requestRecipientDevice = 0,
        requestRecipientInteface = (1 << 0),
        requestRecipientEndpoint = (2 << 0),
        requestRecipientOther = (3 << 0),

        requestTypeStandard = 0,
        requestTypeClass = (1 << 5),
        requestTypeVendor = (2 << 5),

        requestDirectionHostToDevice = 0,
        requestDirectionDeviceToHost = (1 << 7),
    };

    enum {
        requestGetStatus = 0,
        requestClearFeature = 1,
        requestSetFeature = 3,
        requestSetAddress = 5,
        requestGetDescriptor = 6,
        requestSetDescriptor = 7,
        requestGetConfiguration = 8,
        requestSetConfiguration = 9,
        requestGetInterface = 10,
        requestSetInterface = 11,
        requestSynchFrame = 12,
        requestSetSel = 48,
        requestSetIsochDelay = 49,
    };

    enum {
        descriptorTypeDevice = 1,
        descriptorTypeConfiguration = 2,
        descriptorTypeString = 3,
        descriptorTypeInterface = 4,
        descriptorTypeEndpoint = 5,
        descriptorTypeInterfacePower = 8,
        descriptorTypeOtg = 9,
        descriptorTypeDebug = 10,
        descriptorTypeInterfaceAssociation = 11,
        descriptorTypeBos = 15,
        descriptorTypeDeviceCapability = 16,
        descriptorTypeSuperSpeedUseEndpointCompanion = 48,
    };

    struct [[gnu::packed]] packet {
        uint8_t request_type;
        uint8_t request;
        uint16_t value;
        uint16_t index;
        uint16_t length;
    };

    struct [[gnu::packed]] device_descriptor {
        uint8_t length;
        uint8_t descriptor_type;
        uint16_t usb_version;
        uint8_t device_class;
        uint8_t device_sub_class;
        uint8_t protocol;
        uint8_t max_packet_size;
        uint16_t vendor_id;
        uint16_t product_id;
        uint16_t device_release;
        uint8_t manufacturer;
        uint8_t product;
        uint8_t serial_number;
        uint8_t num_configs;
    };

    struct [[gnu::packed]] string_langid_descriptor {
        uint8_t length;
        uint8_t type;
        uint16_t langids[(256 - 2) / 2];
    };

    struct [[gnu::packed]] string_unicode_descriptor {
        uint8_t length;
        uint8_t type;
        char16_t str[(256 - 2) / 2];
    };
} // namespace usb


