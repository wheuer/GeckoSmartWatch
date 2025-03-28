#define DT_DRV_COMPAT buydisplay_gc9a01

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <zephyr/logging/log.h>
#include <inttypes.h>

LOG_MODULE_REGISTER(gc9a01, CONFIG_DISPLAY_LOG_LEVEL_ERR);


// #define GC9A01_SPI_PROFILING

/**
 * gc9a01 display controller driver.
 *
 */

#define GC9A01A_SLPIN   0x10  ///< Enter Sleep Mode
#define GC9A01A_SLPOUT  0x11 ///< Sleep Out
#define GC9A01A_PTLON   0x12  ///< Partial Mode ON
#define GC9A01A_NORON   0x13  ///< Normal Display Mode ON

#define GC9A01A_INVOFF  0x20   ///< Display Inversion OFF
#define GC9A01A_INVON   0x21    ///< Display Inversion ON
#define GC9A01A_DISPOFF 0x28  ///< Display OFF
#define GC9A01A_DISPON  0x29   ///< Display ON

#define GC9A01A_CASET 0x2A ///< Column Address Set
#define GC9A01A_PASET 0x2B ///< Page Address Set
#define GC9A01A_RAMWR 0x2C ///< Memory Write

#define GC9A01A_PTLAR       0x30    ///< Partial Area
#define GC9A01A_VSCRDEF     0x33  ///< Vertical Scrolling Definition
#define GC9A01A_TEOFF       0x34    ///< Tearing effect line off
#define GC9A01A_TEON        0x35     ///< Tearing effect line on
#define GC9A01A_MADCTL      0x36   ///< Memory Access Control
#define GC9A01A_VSCRSADD    0x37 ///< Vertical Scrolling Start Address
#define GC9A01A_PIXFMT      0x3A   ///< COLMOD: Pixel Format Set

#define GC9A01A1_DFUNCTR 0xB6 ///< Display Function Control

#define GC9A01A1_VREG1A 0xC3 ///< Vreg1a voltage control
#define GC9A01A1_VREG1B 0xC4 ///< Vreg1b voltage control
#define GC9A01A1_VREG2A 0xC9 ///< Vreg2a voltage control

#define GC9A01A_RDID1 0xDA ///< Read ID 1
#define GC9A01A_RDID2 0xDB ///< Read ID 2
#define GC9A01A_RDID3 0xDC ///< Read ID 3

#define ILI9341_GMCTRP1     0xE0 ///< Positive Gamma Correction
#define ILI9341_GMCTRN1     0xE1 ///< Negative Gamma Correction
#define ILI9341_FRAMERATE   0xE8 ///< Frame rate control

#define GC9A01A_INREGEN2    0xEF ///< Inter register enable 2
#define GC9A01A_GAMMA1      0xF0 ///< Set gamma 1
#define GC9A01A_GAMMA2      0xF1 ///< Set gamma 2
#define GC9A01A_GAMMA3      0xF2 ///< Set gamma 3
#define GC9A01A_GAMMA4      0xF3 ///< Set gamma 4

#define GC9A01A_INREGEN1 0xFE ///< Inter register enable 1

#define MADCTL_MY   0x80  ///< Bottom to top
#define MADCTL_MX   0x40  ///< Right to left
#define MADCTL_MV   0x20  ///< Reverse Mode
#define MADCTL_ML   0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB  0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR  0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH   0x04  ///< LCD refresh right to left

#define DISPLAY_WIDTH         DT_INST_PROP(0, width)
#define DISPLAY_HEIGHT        DT_INST_PROP(0, height)

// Command codes:
#define COL_ADDR_SET        0x2A
#define ROW_ADDR_SET        0x2B
#define MEM_WR              0x2C
#define MEM_WR_CONT         0x3C
#define COLOR_MODE          0x3A
#define COLOR_MODE_12_BIT   0x03
#define COLOR_MODE_16_BIT   0x05
#define COLOR_MODE_18_BIT   0x06
#define SLPIN               0x10
#define SLPOUT              0x11

#define RGB565(r, g, b)         (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

typedef struct
{
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes;
  uint8_t delay_ms;
} GC9A01A_init_cmd_t;

static GC9A01A_init_cmd_t GC9A01A_init_cmds[] = {
    {0xEF, {0}, 0, 0},
    {0xEB, {0x14}, 1, 0},
    {0xFE, {0}, 0, 0},
    {0xEF, {0}, 0, 0},
    {0xEB, {0x14}, 1, 0},
    {0x84, {0x40}, 1, 0},
    {0x85, {0xFF}, 1, 0},
    {0x86, {0xFF}, 1, 0},
    {0x87, {0xFF}, 1, 0},
    {0x88, {0x0A}, 1, 0},
    {0x89, {0x21}, 1, 0},
    {0x8A, {0x00}, 1, 0},
    {0x8B, {0x80}, 1, 0},
    {0x8C, {0x01}, 1, 0},
    {0x8D, {0x01}, 1, 0},
    {0x8E, {0xFF}, 1, 0},
    {0x8F, {0xFF}, 1, 0},
    {0xB6, {0x00, 0x00}, 2, 0},
    {0x36, {0x48}, 1, 0}, // This one might need to be changed if it's being weird
    {0x3A, {0x05}, 1, 0},
    {0x90, {0x08, 0x08, 0x08, 0x08}, 4, 0},
    {0xBD, {0x06}, 1, 0},
    {0xBC, {0x00}, 1, 0},
    {0xFF, {0x60, 0x01, 0x04}, 3, 0},
    {0xC3, {0x13}, 1, 0},
    {0xC4, {0x13}, 1, 0},
    {0xC9, {0x22}, 1, 0},
    {0xBE, {0x11}, 1, 0},
    {0xE1, {0x10, 0x0E}, 2, 0},
    {0xDF, {0x21, 0x0C, 0x02}, 3, 0},
    {0xF0, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}, 6, 0},
    {0xF1, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}, 6, 0},
    {0xF2, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}, 6, 0},
    {0xF3, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}, 6, 0},
    {0xED, {0x1B, 0x0B}, 2, 0},
    {0xAE, {0x77}, 1, 0},
    {0xCD, {0x63}, 1, 0},
    {0x70, {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03}, 9, 0},
    {0xE8, {0x34}, 1, 0},
    {0x62, {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70}, 12, 0},
    {0x63, {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70}, 12, 0},
    {0x64, {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07}, 7, 0},
    {0x66, {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00}, 10, 0},
    {0x67, {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98}, 10, 0},
    {0x74, {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00}, 7, 0},
    {0x98, {0x3E, 0x07}, 2, 0},
    {0x35, {0}, 0, 0},
    {0x21, {0}, 0, 0},
    {0x11, {0}, 0, 120},
    {0x29, {0}, 0, 20},
    {0x00, {0}, 0xFF, 20} // End of sequence command
};

struct gc9a01_config {
    struct spi_dt_spec  bus;
    struct gpio_dt_spec dc_gpio;
    struct pwm_dt_spec  bl_pwm;
    struct gpio_dt_spec reset_gpio;
};

struct gc9a01_point {
    uint16_t X, Y;
};

struct gc9a01_frame {
    struct gc9a01_point start, end;
};

static struct gc9a01_frame frame = {{0, 0}, {DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1}};

static inline int gc9a01_write_cmd(const struct device *dev, uint8_t cmd,
                                   const uint8_t *data, size_t len)
{
    const struct gc9a01_config *config = dev->config;
    struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
    struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
    gpio_pin_set_dt(&config->dc_gpio, 0);
    if (spi_write_dt(&config->bus, &buf_set) != 0) {
        LOG_ERR("Failed sending data");
        return -EIO;
    }

    if (data != NULL && len != 0) {
        buf.buf = (void *)data;
        buf.len = len;
        gpio_pin_set_dt(&config->dc_gpio, 1);
        if (spi_write_dt(&config->bus, &buf_set) != 0) {
            LOG_ERR("Failed sending data");
            return -EIO;
        }
    }

    return 0;
}

static void gc9a01_set_frame(const struct device *dev, struct gc9a01_frame frame)
{
    uint8_t data[4];

    data[0] = (frame.start.X >> 8) & 0xFF;
    data[1] = frame.start.X & 0xFF;
    data[2] = (frame.end.X >> 8) & 0xFF;
    data[3] = frame.end.X & 0xFF;
    gc9a01_write_cmd(dev, COL_ADDR_SET, data, sizeof(data));

    data[0] = (frame.start.Y >> 8) & 0xFF;
    data[1] = frame.start.Y & 0xFF;
    data[2] = (frame.end.Y >> 8) & 0xFF;
    data[3] = frame.end.Y & 0xFF;
    gc9a01_write_cmd(dev, ROW_ADDR_SET, data, sizeof(data));
}

// EXIT SLEEP/WAKE UP
static int gc9a01_blanking_off(const struct device *dev)
{
    // return gc9a01_write_cmd(dev, GC9A01A_DISPON, NULL, 0);
    //
    // Intention for the interface is to just have a un-clear the display but 
    //      we will have this function wake the display up
    //
    gc9a01_write_cmd(dev, GC9A01A_SLPOUT, NULL, 0);

    // Must wait at least 5 ms before next command, 120 ms before sending sleep out
    // Fix this in the future, we need a mutex and timer here, for now just wait it out
    k_msleep(150);
    
    return 0;
}

// ENTER SLEEP
static int gc9a01_blanking_on(const struct device *dev)
{
    // return gc9a01_write_cmd(dev, GC9A01A_DISPOFF, NULL, 0);
    //
    // Intention for the interface is to just have a clear display but 
    //      we will have this function put the display into sleep mode
    //
    gc9a01_write_cmd(dev, GC9A01A_SLPIN, NULL, 0);

    // Must wait at least 5 ms before next command, 120 ms before sending sleep out
    // Fix this in the future, we need a mutex and timer here, for now just wait it out
    k_msleep(150);

    return 0;
}

static int gc9a01_write(const struct device *dev, const uint16_t x, const uint16_t y,
                        const struct display_buffer_descriptor *desc,
                        const void *buf)
{
#ifdef GC9A01_SPI_PROFILING
    uint32_t start_time;
    uint32_t stop_time;
    uint32_t cycles_spent;
    uint32_t nanoseconds_spent;
#endif
    uint16_t x_end_idx = x + desc->width - 1;
    uint16_t y_end_idx = y + desc->height - 1;

    frame.start.X = x;
    frame.end.X = x_end_idx;
    frame.start.Y = y;
    frame.end.Y = y_end_idx;
    gc9a01_set_frame(dev, frame);

    size_t len = (x_end_idx + 1 - x) * (y_end_idx + 1 - y) * 16 / 8;
    //printk("x_start: %d, y_start: %d, x_end: %d, y_end: %d, buf_size: %d, pitch: %d len: %d\n", x, y, x_end_idx, y_end_idx, desc->buf_size, desc->pitch, len);

#ifdef GC9A01_SPI_PROFILING
    start_time = k_cycle_get_32();
#endif
    gc9a01_write_cmd(dev, GC9A01A_RAMWR, buf, len);
#ifdef GC9A01_SPI_PROFILING
    stop_time = k_cycle_get_32();
    cycles_spent = stop_time - start_time;
    nanoseconds_spent = k_cyc_to_ns_ceil32(cycles_spent);
    printf("%d =>: %dns\n", len, nanoseconds_spent);
    LOG_DBG("%d =>: %dns", len, nanoseconds_spent);
#endif

    return 0;
}

static int gc9a01_read(const struct device *dev, const uint16_t x, const uint16_t y,
                       const struct display_buffer_descriptor *desc, void *buf)
{
    LOG_ERR("not supported");
    return -ENOTSUP;
}

static void* gc9a01_get_framebuffer(const struct device *dev)
{
    LOG_ERR("not supported");
    return NULL;
}

static int gc9a01_set_brightness(const struct device *dev,
                                 const uint8_t brightness)
{
    const struct gc9a01_config *config = dev->config;
    pwm_set_dt(&config->bl_pwm, config->bl_pwm.period, (uint32_t) ((brightness / 255.0) * config->bl_pwm.period));
    return 0;
}

static int gc9a01_set_contrast(const struct device *dev, uint8_t contrast)
{
    LOG_WRN("not supported");
    return -ENOTSUP;
}

static void gc9a01_get_capabilities(const struct device *dev,
                                    struct display_capabilities *caps)
{
    memset(caps, 0, sizeof(struct display_capabilities));
    caps->x_resolution = DISPLAY_WIDTH;
    caps->y_resolution = DISPLAY_HEIGHT;
    caps->supported_pixel_formats = PIXEL_FORMAT_BGR_565;
    caps->current_pixel_format = PIXEL_FORMAT_BGR_565;
    caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST;
}

static int gc9a01_set_orientation(const struct device *dev,
                                  const enum display_orientation
                                  orientation) {
    LOG_ERR("Unsupported");
    return -ENOTSUP;
}

static int gc9a01_set_pixel_format(const struct device *dev,
                                   const enum display_pixel_format pf)
{
    LOG_ERR("not supported");
    return -ENOTSUP;
}

static int gc9a01_controller_init(const struct device *dev)
{
    const struct gc9a01_config *config = dev->config;

    LOG_DBG("Initialize GC9A01 controller");

    gpio_pin_set_dt(&config->reset_gpio, 0);
    k_msleep(5);
    gpio_pin_set_dt(&config->reset_gpio, 1);
    k_msleep(150);

    int cmd = 0;
    while (GC9A01A_init_cmds[cmd].databytes != 0xff)
    {
        gc9a01_write_cmd(dev, GC9A01A_init_cmds[cmd].cmd, GC9A01A_init_cmds[cmd].data, GC9A01A_init_cmds[cmd].databytes);
        k_msleep(GC9A01A_init_cmds[cmd].delay_ms);
        cmd++;
    }

    return 0;
}

static int gc9a01_init(const struct device *dev)
{
    const struct gc9a01_config *config = dev->config;

    LOG_DBG("");

    if (!spi_is_ready_dt(&config->bus)) {
        LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
        return -ENODEV;
    }

    if (!device_is_ready(config->reset_gpio.port)) {
        LOG_ERR("Reset GPIO device not ready");
        return -ENODEV;
    }

    if (!device_is_ready(config->dc_gpio.port)) {
        LOG_ERR("DC GPIO device not ready");
        return -ENODEV;
    }

    gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&config->dc_gpio, GPIO_OUTPUT_INACTIVE);
    k_msleep(500);

	if (!device_is_ready(config->bl_pwm.dev)) {
		LOG_ERR("Backlight PWM device not ready");
		return -ENODEV;
	}

    // printf("PWM: Channel: %u     Period: %u     Flags: %u\n", config->bl_pwm.channel, 
    //                                                         config->bl_pwm.period,
    //                                                         config->bl_pwm.flags);

    pwm_set_dt(&config->bl_pwm, config->bl_pwm.period, config->bl_pwm.period / 2); // 50%

    gc9a01_controller_init(dev);

    //printf("Finish init\n");

    return 0;
}

static const struct gc9a01_config gc9a01_config = {
    .bus = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),
    .reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
    .dc_gpio = GPIO_DT_SPEC_INST_GET(0, dc_gpios),
    .bl_pwm = PWM_DT_SPEC_GET(DT_NODELABEL(gc9a01)),
};

static struct display_driver_api gc9a01_driver_api = {
    .blanking_on = gc9a01_blanking_on,
    .blanking_off = gc9a01_blanking_off,
    .write = gc9a01_write,
    .read = gc9a01_read,
    .get_framebuffer = gc9a01_get_framebuffer,
    .set_brightness = gc9a01_set_brightness,
    .set_contrast = gc9a01_set_contrast,
    .get_capabilities = gc9a01_get_capabilities,
    .set_pixel_format = gc9a01_set_pixel_format,
    .set_orientation = gc9a01_set_orientation,
};

DEVICE_DT_INST_DEFINE(0, gc9a01_init, NULL, NULL, &gc9a01_config, POST_KERNEL,
                      CONFIG_DISPLAY_INIT_PRIORITY, &gc9a01_driver_api);



