/*
 * Rufus Linux - USB Detection Module
 * Copyright © 2026 Port Project
 *
 * USB device detection using libusb-1.0
 */

#ifndef USB_DETECT_H
#define USB_DETECT_H

#include <stdint.h>
#include <libusb-1.0/libusb.h>

#define MAX_USB_DEVICES 32
#define MAX_STRING_LENGTH 256

typedef struct {
    uint8_t bus_number;
    uint8_t device_address;
    uint16_t vendor_id;
    uint16_t product_id;
    char manufacturer[MAX_STRING_LENGTH];
    char product[MAX_STRING_LENGTH];
    char serial_number[MAX_STRING_LENGTH];
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t num_configurations;
    int is_storage_device;
} usb_device_info_t;

typedef struct {
    usb_device_info_t devices[MAX_USB_DEVICES];
    int count;
} usb_device_list_t;

// Initialize libusb and return context
libusb_context* usb_init(void);

// Cleanup libusb context
void usb_cleanup(libusb_context* ctx);

// Scan for USB devices and populate device list
int usb_scan_devices(libusb_context* ctx, usb_device_list_t* device_list);

// Print device information
void usb_print_device_info(const usb_device_info_t* device);

// Print all devices in list
void usb_print_device_list(const usb_device_list_t* device_list);

// Check if device is a storage device
int usb_is_storage_device(const usb_device_info_t* device);

#endif // USB_DETECT_H
