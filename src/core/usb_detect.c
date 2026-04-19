/*
 * Rufus Linux - USB Detection Module
 * Copyright © 2026 Port Project
 *
 * USB device detection using libusb-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usb_detect.h"

// Initialize libusb and return context
libusb_context* usb_init(void)
{
    libusb_context* ctx = NULL;
    int result = libusb_init(&ctx);
    if (result != LIBUSB_SUCCESS) {
        fprintf(stderr, "Erreur: libusb_init() a échoué: %s\n", libusb_error_name(result));
        return NULL;
    }
    
    // Enable debug output
    #ifdef DEBUG
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    #endif
    
    return ctx;
}

// Cleanup libusb context
void usb_cleanup(libusb_context* ctx)
{
    if (ctx) {
        libusb_exit(ctx);
    }
}

// Get string descriptor from device
static int get_string_descriptor(libusb_device_handle* handle, uint8_t desc_index, 
                                unsigned char langid, char* buffer, size_t buffer_size)
{
    if (desc_index == 0) {
        buffer[0] = '\0';
        return 0;
    }
    
    int result = libusb_get_string_descriptor_ascii(handle, desc_index, 
                                                   (unsigned char*)buffer, buffer_size);
    if (result < 0) {
        buffer[0] = '\0';
        return -1;
    }
    
    return result;
}

// Check if device is a storage device (Mass Storage Class)
int usb_is_storage_device(const usb_device_info_t* device)
{
    // USB Mass Storage Class: 0x01, 0x06, 0x50
    return (device->device_class == 0x01 && device->device_subclass == 0x06 && device->device_protocol == 0x50) ||
           (device->device_class == 0x00 && device->device_subclass == 0x00 && device->device_protocol == 0x00);
}

// Scan for USB devices and populate device list
int usb_scan_devices(libusb_context* ctx, usb_device_list_t* device_list)
{
    if (!ctx || !device_list) {
        return -1;
    }
    
    memset(device_list, 0, sizeof(usb_device_list_t));
    
    libusb_device** devices;
    ssize_t device_count = libusb_get_device_list(ctx, &devices);
    if (device_count < 0) {
        fprintf(stderr, "Erreur: libusb_get_device_list() a échoué: %s\n", 
                libusb_error_name(device_count));
        return -1;
    }
    
    printf("Scan USB: %zd périphériques trouvés\n", device_count);
    
    for (ssize_t i = 0; i < device_count && device_list->count < MAX_USB_DEVICES; i++) {
        libusb_device* device = devices[i];
        libusb_device_handle* handle = NULL;
        usb_device_info_t* device_info = &device_list->devices[device_list->count];
        
        // Get device descriptor
        struct libusb_device_descriptor desc;
        int result = libusb_get_device_descriptor(device, &desc);
        if (result != LIBUSB_SUCCESS) {
            fprintf(stderr, "Erreur: impossible d'obtenir le descripteur du périphérique %zd: %s\n", 
                    i, libusb_error_name(result));
            continue;
        }
        
        // Open device for string descriptors
        result = libusb_open(device, &handle);
        if (result != LIBUSB_SUCCESS) {
            // Try to continue without string descriptors
            handle = NULL;
        }
        
        // Fill device info
        device_info->bus_number = libusb_get_bus_number(device);
        device_info->device_address = libusb_get_device_address(device);
        device_info->vendor_id = desc.idVendor;
        device_info->product_id = desc.idProduct;
        device_info->device_class = desc.bDeviceClass;
        device_info->device_subclass = desc.bDeviceSubClass;
        device_info->device_protocol = desc.bDeviceProtocol;
        device_info->num_configurations = desc.bNumConfigurations;
        
        // Get string descriptors if possible
        if (handle) {
            get_string_descriptor(handle, desc.iManufacturer, 0x0409, 
                                 device_info->manufacturer, MAX_STRING_LENGTH);
            get_string_descriptor(handle, desc.iProduct, 0x0409, 
                                 device_info->product, MAX_STRING_LENGTH);
            get_string_descriptor(handle, desc.iSerialNumber, 0x0409, 
                                 device_info->serial_number, MAX_STRING_LENGTH);
        }
        
        // Check if it's a storage device
        device_info->is_storage_device = usb_is_storage_device(device_info);
        
        if (handle) {
            libusb_close(handle);
        }
        
        device_list->count++;
        
        printf("  [%d] Bus %03d Device %03d: ID %04x:%04x %s %s %s\n", 
               device_list->count,
               device_info->bus_number,
               device_info->device_address,
               device_info->vendor_id,
               device_info->product_id,
               device_info->manufacturer,
               device_info->product,
               device_info->is_storage_device ? "[STORAGE]" : "");
    }
    
    libusb_free_device_list(devices, 1);
    
    printf("Scan terminé: %d périphériques USB trouvés\n", device_list->count);
    return device_list->count;
}

// Print device information
void usb_print_device_info(const usb_device_info_t* device)
{
    printf("Périphérique USB:\n");
    printf("  Bus: %03d\n", device->bus_number);
    printf("  Adresse: %03d\n", device->device_address);
    printf("  VID:PID: %04x:%04x\n", device->vendor_id, device->product_id);
    printf("  Fabricant: %s\n", device->manufacturer[0] ? device->manufacturer : "Inconnu");
    printf("  Produit: %s\n", device->product[0] ? device->product : "Inconnu");
    printf("  Numéro de série: %s\n", device->serial_number[0] ? device->serial_number : "Inconnu");
    printf("  Classe: 0x%02x:%02x:%02x\n", device->device_class, device->device_subclass, device->device_protocol);
    printf("  Configurations: %d\n", device->num_configurations);
    printf("  Périphérique de stockage: %s\n", device->is_storage_device ? "Oui" : "Non");
}

// Print all devices in list
void usb_print_device_list(const usb_device_list_t* device_list)
{
    printf("\n=== Liste des Périphériques USB ===\n");
    printf("Total: %d périphériques\n\n", device_list->count);
    
    for (int i = 0; i < device_list->count; i++) {
        printf("[%d] ", i + 1);
        usb_print_device_info(&device_list->devices[i]);
        printf("\n");
    }
    
    // Show only storage devices
    printf("=== Périphériques de Stockage ===\n");
    int storage_count = 0;
    for (int i = 0; i < device_list->count; i++) {
        if (device_list->devices[i].is_storage_device) {
            printf("[%d] %04x:%04x - %s %s\n", 
                   ++storage_count,
                   device_list->devices[i].vendor_id,
                   device_list->devices[i].product_id,
                   device_list->devices[i].manufacturer,
                   device_list->devices[i].product);
        }
    }
    
    if (storage_count == 0) {
        printf("Aucun périphérique de stockage trouvé\n");
    }
}
