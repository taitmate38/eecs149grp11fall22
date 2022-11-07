#include "wii_nunchuck_setup.h"

int wii_i2c_init(int i2c_port_num, int sda_pin, int scl_pin)
{
    if (wii_i2c_setup_i2c(i2c_port_num, sda_pin, scl_pin) != ESP_OK)
        return 1;
    if (wii_i2c_write(data_init1, sizeof(data_init1)) != ESP_OK)
        return 1;
    if (wii_i2c_write(data_init2, sizeof(data_init2)) != ESP_OK)
        return 1;
    return 0;
}

static esp_err_t wii_i2c_setup_i2c(i2c_port_t i2c_port_num, int sda_pin, int scl_pin)
{
    wii_i2c_port_num = i2c_port_num;
    i2c_config_t conf = {
        I2C_MODE_MASTER,
        sda_pin,
        scl_pin,
        GPIO_PULLUP_ENABLE,
        GPIO_PULLUP_ENABLE,
        {100000} // 100KHz
    };
    i2c_param_config(wii_i2c_port_num, &conf);
    return i2c_driver_install(wii_i2c_port_num, conf.mode, 0, 0, 0);
}

static esp_err_t wii_i2c_write(const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (WII_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, (uint8_t *)data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(wii_i2c_port_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t wii_i2c_read(uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (WII_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1)
    {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(wii_i2c_port_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

const unsigned char *wii_i2c_read_ident(void)
{
    if (wii_i2c_write(data_req_ident, sizeof(data_req_ident)) != ESP_OK)
        return NULL;
    if (wii_i2c_read(read_data, sizeof(read_data)) != ESP_OK)
        return NULL;
    return (const unsigned char *)read_data;
}

int wii_i2c_request_state(void)
{
    if (wii_i2c_write(data_req_data, sizeof(data_req_data)) != ESP_OK)
        return 1;
    return 0;
}

const unsigned char *wii_i2c_read_state(void)
{
    if (wii_i2c_read(read_data, sizeof(read_data)) != ESP_OK)
        return NULL;
    return (const unsigned char *)read_data;
}

unsigned int wii_i2c_decode_ident(const unsigned char *ident)
{
    if (!ident)
        return WII_I2C_IDENT_NONE;
    return (((uint32_t)ident[5] << 0) |
            ((uint32_t)ident[4] << 8) |
            ((uint32_t)ident[3] << 16) |
            ((uint32_t)ident[2] << 24));
}

void wii_i2c_decode_nunchuk(const unsigned char *data, struct wii_i2c_nunchuk_state *state)
{
    if (!data)
    {
        memset(state, 0, sizeof(*state));
        return;
    }

    state->x = data[0] - (1 << 7);
    state->y = data[1] - (1 << 7);
    state->acc_x = ((data[2] << 2) | ((data[5] & 0x0c) >> 2)) - (1 << 9);
    state->acc_y = ((data[3] << 2) | ((data[5] & 0x30) >> 4)) - (1 << 9);
    state->acc_z = ((data[4] << 2) | ((data[5] & 0xc0) >> 6)) - (1 << 9);
    state->c = (data[5] & 0x02) ? 0 : 1;
    state->z = (data[5] & 0x01) ? 0 : 1;
}