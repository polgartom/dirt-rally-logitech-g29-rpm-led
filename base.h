#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}

char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s)
{
    return rtrim(ltrim(s)); 
}

void trim_trailing_slash(char *path) {
    int len = strlen(path);
    while (len > 0 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
        path[len - 1] = '\0';
        len--;
    }
}

FILE * fopen_mkdir(const char *path, const char *mode)
{
    char *p = strdup(path);
    char *sep = strchr(p+1, '\\');
    while(sep != NULL)
    {
        *sep = '\0';
        if (mkdir(p, 0755) && errno != EEXIST)
        {
            fprintf(stderr, "error while trying to create %s\n", p);
        }
        *sep = '\\';
        sep = strchr(sep+1, '\\');
    }
    free(p);
    return fopen(path, mode);
}

/////////////////////////////////////////////////////////////////////////////

#include "include/hidapi_winapi.h"
#include "include/iniparser.h"

#include "telementry.h"


#define FIELD_INDEX(type, field) (offsetof(type, field) / sizeof(((type *)0)->field))

#define MAX_STR 255

#define DRT1_RPM 37
#define DRT1_MAX_RPM 63
#define DRT1_MIN_RPM 64

// /*
// Setting should be a number from 0 to 31

//     From outside in, mirrored on each side.

//     0 = No LEDs
//     1 = Green One
//     2 = Green Two
//     4 = Orange One
//     8 = Orange Two
//     16 = Red

//     31 = All LEDs
// */

#define DRT1_RPM_LED_NONE (0)
#define DRT1_RPM_LED_GREEN_ONE (1<<0)
#define DRT1_RPM_LED_GREEN_TWO (1<<1)
#define DRT1_RPM_LED_ORANGE_ONE (1<<2)
#define DRT1_RPM_LED_ORANGE_TWO (1<<3)
#define DRT1_RPM_LED_RED (1<<4)
#define DRT1_RPM_LED_ALL \
    (DRT1_RPM_LED_GREEN_ONE | DRT1_RPM_LED_GREEN_TWO | DRT1_RPM_LED_ORANGE_ONE | DRT1_RPM_LED_ORANGE_TWO | DRT1_RPM_LED_RED)

void print_hid_report_descriptor_from_device(hid_device *device) {
	unsigned char descriptor[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
	int res = 0;

	printf("  Report Descriptor: ");
#if HID_API_VERSION >= HID_API_MAKE_VERSION(0, 14, 0)
	res = hid_get_report_descriptor(device, descriptor, sizeof(descriptor));
#else
	(void)res;
#endif
	if (res < 0) {
		printf("error getting: %ls", hid_error(device));
	}
	else {
		printf("(%d bytes)", res);
	}
	for (int i = 0; i < res; i++) {
		if (i % 10 == 0) {
			printf("\n");
		}
		printf("0x%02x, ", descriptor[i]);
	}
	printf("\n");
}

const char *hid_bus_name(hid_bus_type bus_type) {
	static const char *const HidBusTypeName[] = {
		"Unknown",
		"USB",
		"Bluetooth",
		"I2C",
		"SPI",
	};

	if ((int)bus_type < 0)
		bus_type = HID_API_BUS_UNKNOWN;
	if ((int)bus_type >= (int)(sizeof(HidBusTypeName) / sizeof(HidBusTypeName[0])))
		bus_type = HID_API_BUS_UNKNOWN;

	return HidBusTypeName[bus_type];
}

void print_device(struct hid_device_info *cur_dev) {
	printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
	printf("\n");
	printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
	printf("  Product:      %ls\n", cur_dev->product_string);
	printf("  Release:      %hx\n", cur_dev->release_number);
	printf("  Interface:    %d\n",  cur_dev->interface_number);
	printf("  Usage (page): 0x%hx (0x%hx)\n", cur_dev->usage, cur_dev->usage_page);
	printf("  Bus type: %u (%s)\n", (unsigned)cur_dev->bus_type, hid_bus_name(cur_dev->bus_type));
	printf("\n");
}

void print_devices(struct hid_device_info *cur_dev) {
	for (; cur_dev; cur_dev = cur_dev->next) {
		print_device(cur_dev);
	}
}

hid_device* curr_wheel = NULL;
unsigned char last_led_bits = 0;

hid_device* init_logi_g29_wheel_hid() {
	int res;
	unsigned char buf[256];
	wchar_t wstr[MAX_STR];
	hid_device *handle;
	int i;

    curr_wheel = NULL;

	// Initialize the hidapi library
	res = hid_init();

    handle = hid_open(0x46D, 0xC24F, NULL);
	if (!handle) {
		printf("Unable to open device\n");
		hid_exit();
 		return NULL;
	}

	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    if (res < 0) printf("hid_error: %ls\n", hid_error(handle));
	printf("Manufacturer String: %ls\n", wstr);

	// Read the Serial Number String
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
    if (res < 0) printf("hid_error: %ls\n", hid_error(handle));
	printf("Serial Number String: (%d) %ls\n", wstr[0], wstr);

    // print_hid_report_descriptor_from_device(handle);

	struct hid_device_info* info = hid_get_device_info(handle);
	if (info == NULL) {
		printf("Unable to get device info\n");
	} else {
		print_devices(info);
	}

    res = set_wheel_leds(handle, 0);
    if (res < 0) return NULL;

    curr_wheel = handle;
    return handle;
}

int set_wheel_leds(hid_device* wheel, unsigned char led_bits) {
    if (last_led_bits == led_bits) return 0;

    unsigned char buf[8] = {0x00, 0xf8, 0x12, led_bits, 0x00, 0x00, 0x00, 0x01};
    int res = hid_write(wheel, &buf[0], sizeof(buf));
    if (res < 0) {
        printf("hid_error: %ls\n", hid_error(curr_wheel));
    } else {
        last_led_bits = led_bits;
    }

    return res;
}

unsigned char map_rpm_percent_to_led_bits(float percent)
{
    unsigned char bits = 0;

    if (percent > 4.0)  bits |= DRT1_RPM_LED_GREEN_ONE;
    if (percent > 19.0) bits |= DRT1_RPM_LED_GREEN_TWO;
    if (percent > 39.0) bits |= DRT1_RPM_LED_ORANGE_ONE;
    if (percent > 69.0) bits |= DRT1_RPM_LED_ORANGE_TWO;
    if (percent > 84.0) bits |= DRT1_RPM_LED_RED;

    return bits;
}