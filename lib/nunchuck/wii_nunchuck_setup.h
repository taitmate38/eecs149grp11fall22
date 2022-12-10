#include <driver/i2c.h>
#include <Arduino.h>

static i2c_port_t wii_i2c_port_num;
#define WII_I2C_ADDR 0x52
#define WII_I2C_IDENT_NONE 0
#define WII_I2C_IDENT_NUNCHUK 0xa4200000

static const uint8_t data_init1[] = {0xf0, 0x55};
static const uint8_t data_init2[] = {0xfb, 0x00};
static const uint8_t data_req_ident[] = {0xfa};
static const uint8_t data_req_data[] = {0x00};

struct wii_i2c_nunchuk_state
{
    // accelerometer
    short int acc_x;
    short int acc_y;
    short int acc_z;

    // analog stick:
    signed char x;
    signed char y;

    // buttons:
    char c;
    char z;
};

static uint8_t read_data[6];

int wii_i2c_init(int i2c_port_num, int sda_pin, int scl_pin);
static esp_err_t wii_i2c_setup_i2c(i2c_port_t i2c_port_num, int sda_pin, int scl_pin);
static esp_err_t wii_i2c_write(const uint8_t *data, size_t len);
static esp_err_t wii_i2c_read(uint8_t *data, size_t len);
const unsigned char *wii_i2c_read_ident(void);
int wii_i2c_request_state(void);
const unsigned char *wii_i2c_read_state(void);
unsigned int wii_i2c_decode_ident(const unsigned char *ident);
void wii_i2c_decode_nunchuk(const unsigned char *data, struct wii_i2c_nunchuk_state *state);