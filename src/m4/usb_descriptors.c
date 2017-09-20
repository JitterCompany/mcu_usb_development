#include "usb_descriptors.h"
#include <string.h>

#define USB_WORD(x)	(x & 0xFF), ((x >> 8) & 0xFF)

/* Descriptor buffer. Stores USB descriptors.
 *
 * The purpose of this buffer is to pass descriptors to the USB library
 * when it calls the CALLBACK_USB_GetDescriptor() callback
 *
 * Device, Configuration, Interface and Endpoint descriptors
 * are generated by calling the appropriate API functions (see Descriptors.h)
 * and stored permanently in the buffer.
 *
 * String descriptors are built on the fly when requested,
 * using the remaining free space at the end of the buffer.
 */
static struct {
    const char *strings[DESC_MAX_STR_COUNT];
    uint8_t buffer[DESC_BUFF_SZ];
    uint16_t next_addr;
    uint16_t end_addr;
    uint8_t next_str;
    bool error_flag;
} desc_storage;

bool descriptor_ok()
{
    return (!desc_storage.error_flag);
}

// size of the language descriptor ("string descriptor" 0)
const size_t language_desc_len = 4;

USBDescriptorDevice *descriptor_make_device(uint16_t idVendor,
        uint16_t idProduct, uint16_t bcdDevice)
{
    memset((void *)&desc_storage, 0, sizeof(desc_storage));
    desc_storage.next_str = 1;
    desc_storage.end_addr = DESC_BUFF_SZ - language_desc_len;

    USBDescriptorDevice *device = (USBDescriptorDevice *)
                                         descriptor_storage_alloc(sizeof(USBDescriptorDevice), true);
    if(device == NULL) {
        desc_storage.error_flag = true;
        return NULL;
    }
    device->bLength = sizeof(USBDescriptorDevice);
    device->bDescriptorType = USB_DESCRIPTOR_TYPE_DEVICE;
    device->bcdUSB = 0x200; //USB_WORD(0x200);
    device->bDeviceClass = USB_CSCP_NoDeviceClass;
    device->bDeviceSubClass = USB_CSCP_NoDeviceSubclass;
    device->bDeviceProtocol = USB_CSCP_NoDeviceProtocol;

    device->bMaxPacketSize0 = CONTROL_ENDPOINT_SIZE;

    device->idVendor = idVendor;
    device->idProduct = idProduct;
    device->bcdDevice = bcdDevice;

    device->iManufacturer = MANUFACTURER_INDEX;
    device->iProduct = PRODUCT_INDEX;
    device->iSerialNumber = SERIAL_INDEX;

    device->bNumConfigurations = 0;
    return device;
}

USBDescriptorConfiguration *descriptor_make_configuration(
    USBDescriptorDevice *device,
    uint8_t bConfigurationValue, uint8_t bmAttributes, uint8_t bMaxPower)
{
    uint8_t len = sizeof(USBDescriptorConfiguration);
    USBDescriptorConfiguration *config =
        (USBDescriptorConfiguration *)descriptor_storage_alloc(
            len, true);

    if(device == NULL || config == NULL) {
        desc_storage.error_flag = true;
        return NULL;
    }

    device->bNumConfigurations++;

    config->bLength = len;
    config->bDescriptorType = USB_DESCRIPTOR_TYPE_CONFIGURATION;
    config->wTotalLength = len; //
    config->bNumInterfaces = 0; // updated when adding interfaces
    config->bConfigurationValue = bConfigurationValue;
    // bit 7 is reserved and must be high (see USB standard)
    config->bmAttributes = (1 << 7) | bmAttributes;
    config->bMaxPower = bMaxPower;

    return config;
}

USBDescriptorInterface *descriptor_make_interface(
    USBDescriptorConfiguration *config,
    uint8_t bInterfaceNumber, uint8_t bAlternateSetting)
{
    if(config == NULL) {
        desc_storage.error_flag = true;
        return NULL;
    }
    /*
     * Descriptors should be in the same order as they are sent to the host.
     * The referenced config descriptor should be the last one in the buffer
     * up to here.
     *
     * Referencing a config descriptor before the last one would
     * either overwrite these existing descriptor(s) or
     * need to shift the existing descriptors down in the buffer, invalidating
     * pointers that the user program may still hold
     */

    uint16_t config_addr = ((uint8_t *)config - desc_storage.buffer);
    if(desc_storage.next_addr != (config_addr + config->wTotalLength)) {
        desc_storage.error_flag = true;
        return NULL;
    }

    uint8_t len = sizeof(USBDescriptorInterface);
    USBDescriptorInterface *interface =
        (USBDescriptorInterface *)descriptor_storage_alloc(
            len, true);

    if(interface == NULL) {
        desc_storage.error_flag = true;
        return NULL;
    }
    config->wTotalLength+= len;
    config->bNumInterfaces++;

    interface->bLength = len;
    interface->bDescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE;
    interface->bInterfaceNumber = bInterfaceNumber;
    interface->bAlternateSetting = bAlternateSetting;
    interface->bNumEndpoints = 0;
    interface->bInterfaceClass = USB_CSCP_VendorSpecificClass;
    interface->bInterfaceSubClass = USB_CSCP_VendorSpecificSubclass;
    interface->bInterfaceProtocol = USB_CSCP_VendorSpecificProtocol;

    return interface;
}

bool descriptor_make_endpoint(
    USBDescriptorConfiguration *config,
    USBDescriptorInterface *interface,
    uint8_t bEndpointAddress, uint8_t bmAttributes,
    uint16_t wMaxPacketSize, uint8_t bInterval)
{
    if(config == NULL || interface == NULL) {
        desc_storage.error_flag = true;
        return NULL;
    }

    uint8_t len = sizeof(USBDescriptorEndpoint);

    /*
     * No next descriptor can be present: adding endpoints to the current
     * interface would either overwrite these existing descriptor(s) or
     * need to shift the existing descriptors down, invalidating
     * pointers that the user program may still hold
     */
    uint16_t expected_next = ((uint8_t *)interface - desc_storage.buffer);
    expected_next+= interface->bLength;
    expected_next+= (interface->bNumEndpoints*len);
    if(desc_storage.next_addr > expected_next) {
        desc_storage.error_flag = true;
        return NULL;
    }

    USBDescriptorEndpoint *endpoint_desc =
        (USBDescriptorEndpoint *)descriptor_storage_alloc(
            len, true);

    // if(endpoint == NULL) {
    //     desc_storage.error_flag = true;
    //     return NULL;
    // }

    config->wTotalLength+= len;
    interface->bNumEndpoints++;
    endpoint_desc->bLength = len;
    endpoint_desc->bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT;

    endpoint_desc->bEndpointAddress = bEndpointAddress;
    endpoint_desc->bmAttributes = bmAttributes;
    endpoint_desc->wMaxPacketSize = wMaxPacketSize;
    endpoint_desc->bInterval = bInterval;

    return true; //endpoint;
}


uint8_t descriptor_string(const char *const string)
{
    if(desc_storage.next_str < DESC_MAX_STR_COUNT) {
        /* ensure the descriptor buffer has enough space
         * for the largest created string descriptor
         * by moving the end_addr marker
         */
        uint16_t new_end_addr = DESC_BUFF_SZ - descriptor_string_size(string);
        if(new_end_addr < desc_storage.end_addr) {
            if(desc_storage.next_addr < new_end_addr) {
                desc_storage.end_addr = new_end_addr;
            } else {
                desc_storage.error_flag = true;
                return NO_DESCRIPTOR;
            }
        }
        uint8_t assigned_index = desc_storage.next_str;
        desc_storage.next_str++;
        desc_storage.strings[assigned_index] = string;

        return assigned_index;
    }
    desc_storage.error_flag = true;
    return NO_DESCRIPTOR;
}

// void *descriptor_find(uint8_t bDescriptorType, uint8_t skip_count)
// {
//     uint8_t *currPtr = desc_storage.buffer;
//     uint8_t *endPtr = &(desc_storage.buffer[desc_storage.next_addr]);
//     while(currPtr < endPtr) {
//         uint8_t len = *currPtr;
//         uint8_t type = *(currPtr+1);
//         if(type == bDescriptorType) {
//             if(skip_count == 0) {
//                 return currPtr;
//             }
//             skip_count--;
//         }
//         currPtr+=len;
//     }
//     return NULL;
// }


// /** Internal API **/

size_t descriptor_string_size(const char *const string)
{
    return 2 + (2*strlen(string));
}

size_t descriptor_from_string(const USBDescriptorString **result_desc,
                              const char *const string)
{
    if(string == NULL) {
        return 0;
    }

    size_t descriptor_size = descriptor_string_size(string);
    uint8_t *dest_buffer = descriptor_storage_alloc(descriptor_size, true);
    if(dest_buffer == NULL) {
        return 0;
    }
    *result_desc = (void *)dest_buffer;

    *(dest_buffer++) = descriptor_size;
    *(dest_buffer++) = USB_DESCRIPTOR_TYPE_STRING;
    size_t str_len = strlen(string);
    for(size_t i=0; i<str_len; i++) {
        *dest_buffer = string[i];
        dest_buffer+= 2;
    }
    return descriptor_size;
}

uint8_t *descriptor_storage_alloc(uint16_t requested_num_bytes, bool commit)
{
    if(desc_storage.next_addr < desc_storage.end_addr) {
        uint16_t bytes_remaining =
            desc_storage.end_addr - desc_storage.next_addr;
        if(bytes_remaining >= requested_num_bytes) {
            uint8_t *ptr =
                &(desc_storage.buffer[desc_storage.next_addr]);
            if(commit) {
                desc_storage.next_addr+= requested_num_bytes;
            }
            memset(ptr, 0, requested_num_bytes);
            return ptr;
        }
    }
    desc_storage.error_flag = true;
    return NULL;
}



// /** This function is called by the library when in device mode
//  * and must be overridden (see library "USB Descriptors" documentation)
//  * by the application code. Its purpose is to provide the descriptors
//  * to the USB library.
//  * When the device receives a Get Descriptor request on the control endpoint,
//  * this function is called so that the descriptor details can be passed back,
//  * enabling the USB library to send the appropriate descriptor(s) to the host.
//  */
// uint16_t CALLBACK_USB_GetDescriptor(uint8_t corenum,
//                                     const uint16_t wValue,
//                                     const uint8_t wIndex,
//                                     const void **const result_addr)
// {
//     const uint8_t  descriptor_type   = (wValue >> 8);
//     const uint8_t  descriptor_num = (wValue & 0xFF);

//     const void *addr = NULL;
//     uint16_t    size = NO_DESCRIPTOR;

//     switch(descriptor_type) {
//         case DTYPE_Device: {
//             addr = descriptor_find(DTYPE_Device, 0);
//             if(addr != NULL) {
//                 size = ((const USB_StdDescriptor_Device_t *)addr)->bLength;
//             }
//             break;
//         }
//         case DTYPE_Configuration: {
//             /*
//              * By default, the hosts sets descriptor_num to zero, in that
//              * case just return the first configuration found.
//              *
//              * Otherwise, search for a configuration matching descriptor_num.
//              * TODO: not sure if the host uses descriptor_num in this way
//              */
//             uint8_t search_offset = 0;
//             while(true) {
//                 addr = descriptor_find(DTYPE_Configuration, search_offset);
//                 if(addr == NULL) {
//                     break;
//                 }
//                 const USB_StdDescriptor_Configuration_Header_t *cfg =
//                     (const USB_StdDescriptor_Configuration_Header_t *)addr;
//                 if(!descriptor_num
//                         || cfg->bConfigurationValue == descriptor_num) {
//                     size = cfg->wTotalLength;
//                     break;
//                 }
//                 search_offset++;
//             }
//             break;
//         }
//         case DTYPE_String: {
//             if(descriptor_num == 0) {
//                 /* string descriptor 0 is not actually a string, it is
//                  * a list of 16-bit language IDs. For now, return a hardcoded
//                  * LANGUAGE_ID_ENG (0x0409, in little endian)
//                  */
//                 uint8_t *dest_buffer = descriptor_storage_alloc(
//                                            language_desc_len, false);
//                 if(dest_buffer != NULL) {
//                     addr = dest_buffer;
//                     size = language_desc_len;

//                     *(dest_buffer+0) = size;
//                     *(dest_buffer+1) = DTYPE_String;
//                     *(dest_buffer+2) = 0x09;
//                     *(dest_buffer+3) = 0x04;
//                 }
//             } else if(descriptor_num < DESC_MAX_STR_COUNT) {
//                 size = descriptor_from_string(&addr,
//                                               desc_storage.strings[descriptor_num]);
//             }
//             break;
//         }
//     }
//     if(addr == NULL) {
//         return NO_DESCRIPTOR;
//     }
//     *result_addr = addr;
//     return size;
// }
