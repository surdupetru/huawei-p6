

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <hsad/config_interface.h>
#include <linux/earlysuspend.h>
#include <asm/gpio.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/delay.h>



#define RED_PORT 0
#define GREEN_PORT 1
#define BLUE_PORT 2

#define CMD_RED_ON 1
#define CMD_GREEN_ON 2
#define CMD_BLUE_ON 3
#define CMD_ORANGE_ON 4
#define CMD_ALL_ON 5
#define CMD_RED_OFF 6
#define CMD_GREEN_OFF 7
#define CMD_BLUE_OFF 8
#define CMD_ORANGE_OFF 9
#define CMD_ALL_OFF 10

#define CMD_RED_BLINK_EN 11
#define CMD_GREEN_BLINK_EN 12
#define CMD_BLUE_BLINK_EN 13
#define CMD_ORANGE_BLINK_EN 14
#define CMD_ALL_BLINK_EN 15
#define CMD_RED_BLINK_DIS 16
#define CMD_GREEN_BLINK_DIS 17
#define CMD_BLUE_BLINK_DIS 18
#define CMD_ORANGE_BLINK_DIS 19
#define CMD_ALL_BLINK_DIS 20
#define CMD_RED_BLINK_TIME_SET 21
#define CMD_GREEN_BLINK_TIME_SET 22

#define CMD_BRIGHTNESS_SET_BEGIN 39
#define CMD_BRIGHTNESS_SET_END 56

#define SELECT0 0x00
#define SELECT1 0x01
#define SELECT2 0x02
#define FADE_ON_TIME 0x03
#define FULLY_ON_TIME 0x04
#define FADE_OFF_TIME 0x05
#define FIRST_FULLY_OFF_TIME 0x06
#define SECOND_FULLY_OFF_TIME 0x07
#define MAX_INTENSITY 0x08
#define ONE_SHOT_MASTER_INTENSITY 0x09
#define INITLIZATION 0x10

#define INIT_VALUE_OF_SELECT0 0x00
#define INIT_VALUE_OF_SELECT1 0x00
#define INIT_VALUE_OF_SELECT2 0x00
#define INIT_VALUE_OF_FADE_ON_TIME 0x00
#define INIT_VALUE_OF_FULLY_ON_TIME 0xaa
#define INIT_VALUE_OF_FADE_OFF_TIME 0x00
#define INIT_VALUE_OF_FIRST_FULLY_OFF_TIME 0xaa
#define INIT_VALUE_OF_SECOND_FULLY_OFF_TIME 0xaa

#define INIT_VALUE_OF_MAX_INTENSITY 0x57  //0x14

#define INIT_VALUE_OF_ONE_SHOT_MASTER_INTENSITY 0x00

#define MAX_BRIGHTNESS_WE_SET 0x00

#define TIME_PARAMETER_1_BEGIN 0
#define TIME_PARAMETER_1_END 96
#define TIME_PARAMETER_2_BEGIN 95
#define TIME_PARAMETER_2_END 160
#define TIME_PARAMETER_3_BEGIN 159
#define TIME_PARAMETER_3_END 224
#define TIME_PARAMETER_4_BEGIN 223
#define TIME_PARAMETER_4_END 320
#define TIME_PARAMETER_5_BEGIN 319
#define TIME_PARAMETER_5_END 448
#define TIME_PARAMETER_6_BEGIN 447
#define TIME_PARAMETER_6_END 640
#define TIME_PARAMETER_7_BEGIN 639
#define TIME_PARAMETER_7_END 896
#define TIME_PARAMETER_8_BEGIN 895
#define TIME_PARAMETER_8_END 1280
#define TIME_PARAMETER_9_BEGIN 1279
#define TIME_PARAMETER_9_END 1792
#define TIME_PARAMETER_10_BEGIN 1791
#define TIME_PARAMETER_10_END 2560
#define TIME_PARAMETER_11_BEGIN 2559
#define TIME_PARAMETER_11_END 3584
#define TIME_PARAMETER_12_BEGIN 3583
#define TIME_PARAMETER_12_END 4928
#define TIME_PARAMETER_13_BEGIN 4927
#define TIME_PARAMETER_13_END 6944
#define TIME_PARAMETER_14_BEGIN 6943
#define TIME_PARAMETER_14_END 12224
#define TIME_PARAMETER_15_BEGIN 12223



#define CMD_BRIGHTNESS_TO_REG(x) ((x>>4)+40)

#define LEDS_CONFIG_NAME_LENGTH 10

static unsigned long led_delay_on = 0;/* milliseconds on */
static unsigned long led_delay_off = 0;/* milliseconds off */


static int gpio_three_colors = 129;
static struct i2c_client *client_leds;


static int led_colors_read(struct i2c_client *client, u8 reg)
{
    int ret;

    ret = i2c_smbus_write_byte(client, reg);
    if (ret)
    {
        dev_err(&client->dev,
                    "couldn't send request. Returned %d\n", ret);
        return ret;
    }
    ret = i2c_smbus_read_byte(client);
    if (ret < 0)
    {
        dev_err(&client->dev,
                        "couldn't read register. Returned %d\n", ret);
        return ret;
    }
    return ret;
}

static int led_colors_write(struct i2c_client *client, u8 reg, u8 data)
{
    int error;

    error = i2c_smbus_write_byte_data(client, reg, data);
    if (error)
    {
        dev_err(&client->dev,
                    "couldn't send request. Returned %d\n", error);
        return error;
    }
    return error;
}

static void set_brightness_on(char select_led)
{
    int ret;
    char value_read;
    char reg_tca6507;

    reg_tca6507 = 0x01 << select_led;
    value_read = led_colors_read(client_leds, SELECT0);
    if(select_led == 0)
    {
        value_read &= ~reg_tca6507;
    }
    else
    {
        value_read |= reg_tca6507;
    }

    ret = led_colors_write(client_leds, SELECT0, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }

    value_read = led_colors_read(client_leds, SELECT1);
    value_read |= reg_tca6507;
    ret = led_colors_write(client_leds, SELECT1, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }

    value_read = led_colors_read(client_leds, SELECT2);
    value_read &=  (~reg_tca6507);
    ret = led_colors_write(client_leds, SELECT2, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }
}

static void set_brightness_off(char select_led)
{
    int ret;
    char value_read;
    char reg_tca6507;

    reg_tca6507 = 0x01 << select_led;
    value_read = led_colors_read(client_leds, SELECT0);
    if(select_led == 0)
    {
        value_read &= ~reg_tca6507;
    }
    else
    {
        value_read |= reg_tca6507;
    }

    ret = led_colors_write(client_leds, SELECT0, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }

    value_read = led_colors_read(client_leds, SELECT1);
    value_read &= ~reg_tca6507;
    ret = led_colors_write(client_leds, SELECT1, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }

    value_read = led_colors_read(client_leds, SELECT2);
    value_read &= ~reg_tca6507;

    ret = led_colors_write(client_leds, SELECT2, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }
}

static void set_blink_en(char select_led)
{
    int ret;
    char value_read;
    char reg_tca6507;

    reg_tca6507 = 0x01 << select_led;
    value_read = led_colors_read(client_leds, SELECT0);
    if(select_led == 0)
    {
        value_read &= ~reg_tca6507;
    }
    else
    {
        value_read |= reg_tca6507;
    }

    ret = led_colors_write(client_leds, SELECT0, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }

    value_read = led_colors_read(client_leds, SELECT1);
    value_read |= reg_tca6507;

    ret = led_colors_write(client_leds, SELECT1, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }

    value_read = led_colors_read(client_leds, SELECT2);
    value_read |= reg_tca6507;

    ret = led_colors_write(client_leds, SELECT2, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }
}

static void set_blink_dis(char select_led)
{
    int ret;
    char value_read;
    char reg_tca6507;

    reg_tca6507 = 0x01 << select_led;
    value_read = led_colors_read(client_leds, SELECT0);
    if(select_led == 0)
    {
        value_read &= ~reg_tca6507;
    }
    else
    {
        value_read |= reg_tca6507;
    }

    ret = led_colors_write(client_leds, SELECT0, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }

    value_read = led_colors_read(client_leds, SELECT1);
    value_read &= (~reg_tca6507);

    ret = led_colors_write(client_leds, SELECT1, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }

    value_read = led_colors_read(client_leds, SELECT2);
    value_read &= (~reg_tca6507);

    ret = led_colors_write(client_leds, SELECT2, value_read);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }
}

static void set_max_intensity(int value)
{
    char ret;
    ret = led_colors_write(client_leds, ONE_SHOT_MASTER_INTENSITY, MAX_BRIGHTNESS_WE_SET);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }
}
static void three_colors_led_set(struct led_classdev *led_cdev,
    enum led_brightness value)
{
    int ret;
    if (value == CMD_ALL_ON)
    {
        set_brightness_on(RED_PORT);
        set_brightness_on(GREEN_PORT);
        set_brightness_on(BLUE_PORT);
    }
    else if (value == CMD_RED_ON)
    {
        set_brightness_on(RED_PORT);
    }
    else if (value == CMD_GREEN_ON)
    {
        set_brightness_on(GREEN_PORT);
    }
    else if (value == CMD_BLUE_ON)
    {
        set_brightness_on(BLUE_PORT);
    }
    else if (value == CMD_ORANGE_ON)
    {
        set_brightness_on(RED_PORT);
        set_brightness_on(GREEN_PORT);
    }
    else if (value == CMD_RED_OFF)
    {
        set_brightness_off(RED_PORT);
    }
    else if (value == CMD_GREEN_OFF)
    {
        set_brightness_off(GREEN_PORT);
    }
    else if (value == CMD_BLUE_OFF)
    {
        set_brightness_off(BLUE_PORT);
    }
    else if (value == CMD_ORANGE_OFF)
    {
        set_brightness_off(RED_PORT);
        set_brightness_off(GREEN_PORT);
    }
    else if (value == CMD_ALL_OFF)
    {
        set_brightness_off(RED_PORT);
        set_brightness_off(GREEN_PORT);
        set_brightness_off(BLUE_PORT);
    }
    else if (value == CMD_RED_BLINK_EN)
    {
        set_blink_en(RED_PORT);
    }
    else if (value == CMD_GREEN_BLINK_EN)
    {
        set_blink_en(GREEN_PORT);
    }
    else if (value == CMD_BLUE_BLINK_EN)
    {
        set_blink_en(BLUE_PORT);
    }
    else if (value == CMD_ORANGE_BLINK_EN)
    {
        set_blink_en(RED_PORT);
        set_blink_en(GREEN_PORT);
    }
    else if (value == CMD_ALL_BLINK_EN)
    {
        set_blink_en(RED_PORT);
        set_blink_en(GREEN_PORT);
        set_blink_en(BLUE_PORT);
    }
    else if (value == CMD_RED_BLINK_DIS)
    {
        set_blink_dis(RED_PORT);
    }
    else if (value == CMD_GREEN_BLINK_DIS)
    {
        set_blink_dis(GREEN_PORT);
    }
    else if (value == CMD_BLUE_BLINK_DIS)
    {
        set_blink_dis(BLUE_PORT);
    }
    else if (value == CMD_ORANGE_BLINK_DIS)
    {
        set_blink_dis(RED_PORT);
        set_blink_dis(GREEN_PORT);
    }
    else if (value == CMD_ALL_BLINK_DIS)
    {
        set_blink_dis(RED_PORT);
        set_blink_dis(GREEN_PORT);
        set_blink_dis(BLUE_PORT);
    }
    else if (value == CMD_RED_BLINK_TIME_SET)
    {
        ret = led_colors_write(client_leds, FADE_ON_TIME, 0x00);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, FULLY_ON_TIME, 0x06);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, FADE_OFF_TIME, 0x00);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, FIRST_FULLY_OFF_TIME, 0x0a);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, SECOND_FULLY_OFF_TIME, 0x0a);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
    }
    else if (value == CMD_GREEN_BLINK_TIME_SET)
    {
        ret = led_colors_write(client_leds, FADE_ON_TIME, 0x00);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, FULLY_ON_TIME, 0x02);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, FADE_OFF_TIME, 0x00);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, FIRST_FULLY_OFF_TIME, 0x0b);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, SECOND_FULLY_OFF_TIME, 0x0b);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
    }
    else if (value > CMD_BRIGHTNESS_SET_BEGIN && value < CMD_BRIGHTNESS_SET_END)
    {
        ret = value-CMD_BRIGHTNESS_SET_BEGIN-1;
        ret |= 0x10;
        ret = led_colors_write(client_leds, ONE_SHOT_MASTER_INTENSITY, ret);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
    }
    else
    {
        ret = led_colors_write(client_leds, SELECT0, 0x00);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, SELECT1, 0x00);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
        ret = led_colors_write(client_leds, SELECT2, 0x00);
        if (ret)
        {
            dev_err(&client_leds->dev, "failed to set register!\n");
        }
    }
    return;
}
static struct led_classdev three_colors_led = {
    .name              = "three_colors_led",
    .brightness_set    = three_colors_led_set,
    .brightness        = LED_OFF,
};


static void led_red_set(struct led_classdev *led_cdev,
    enum led_brightness value)
{
    if (value)
    {
        set_max_intensity(value);
        set_brightness_on(RED_PORT);
    }
    else
    {
         set_brightness_off(RED_PORT);
    }
    return;
}
static struct led_classdev led_red = {
        .name                = "tca6507_red",
        .brightness_set      = led_red_set,
        .brightness          = LED_OFF,
};

static void led_green_set(struct led_classdev *led_cdev,
    enum led_brightness value)
{
    if (value)
    {
        set_max_intensity(value);
        set_brightness_on(GREEN_PORT);
    }
    else
    {
         set_brightness_off(GREEN_PORT);
    }
    return;
}
static struct led_classdev led_green = {
    .name           = "tca6507_green",
    .brightness_set = led_green_set,
    .brightness     = LED_OFF,
};

static void led_blue_set(struct led_classdev *led_cdev,
    enum led_brightness value)
{
    if (value)
    {
        set_max_intensity(value);
        set_brightness_on(BLUE_PORT);
    }
    else
    {
         set_brightness_off(BLUE_PORT);
    }
    return;
}
static struct led_classdev led_blue = {
    .name                = "tca6507_blue",
    .brightness_set      = led_blue_set,
    .brightness          = LED_OFF,
};

static int transform_delay_to_reg(unsigned long delay_time)
{
    int value_to_reg;
    unsigned long state = delay_time;
    if (state == 0)
    {
        value_to_reg = 0;
    }
    else if (state > TIME_PARAMETER_1_BEGIN && state < TIME_PARAMETER_1_END)
    {
        value_to_reg = 1;
    }
    else if (state > TIME_PARAMETER_2_BEGIN && state < TIME_PARAMETER_2_END)
    {
        value_to_reg = 2;
    }
    else if (state > TIME_PARAMETER_3_BEGIN && state < TIME_PARAMETER_3_END)
    {
        value_to_reg = 3;
    }
    else if (state > TIME_PARAMETER_4_BEGIN && state < TIME_PARAMETER_4_END)
    {
        value_to_reg = 4;
    }
    else if (state > TIME_PARAMETER_5_BEGIN && state < TIME_PARAMETER_5_END)
    {
        value_to_reg = 5;
    }
    else if (state > TIME_PARAMETER_6_BEGIN && state < TIME_PARAMETER_6_END)
    {
        value_to_reg = 6;
    }
    else if (state > TIME_PARAMETER_7_BEGIN && state < TIME_PARAMETER_7_END)
    {
        value_to_reg = 7;
    }
    else if (state > TIME_PARAMETER_8_BEGIN && state < TIME_PARAMETER_8_END)
    {
        value_to_reg = 8;
    }
    else if (state > TIME_PARAMETER_9_BEGIN && state < TIME_PARAMETER_9_END)
    {
        value_to_reg = 9;
    }
    else if (state > TIME_PARAMETER_10_BEGIN && state < TIME_PARAMETER_10_END)
    {
        value_to_reg = 10;
    }
    else if (state > TIME_PARAMETER_11_BEGIN && state < TIME_PARAMETER_11_END)
    {
        value_to_reg = 11;
    }
    else if (state > TIME_PARAMETER_12_BEGIN && state < TIME_PARAMETER_12_END)
    {
        value_to_reg = 12;
    }
    else if (state > TIME_PARAMETER_13_BEGIN && state < TIME_PARAMETER_13_END)
    {
        value_to_reg = 13;
    }
    else if (state > TIME_PARAMETER_14_BEGIN && state < TIME_PARAMETER_14_END)
    {
        value_to_reg = 14;
    }
    else if (state > TIME_PARAMETER_15_BEGIN)
    {
        value_to_reg = 15;
    }
    else
    {
        value_to_reg = 4;
    }
    value_to_reg |= (value_to_reg << 4);

    return value_to_reg;
}
static ssize_t led_delay_on_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%lu\n", led_delay_on);
}
static ssize_t led_delay_on_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    char *after;
    unsigned long state = simple_strtoul(buf, &after, 10);
    int ret;
    int value_to_reg;

    led_delay_on = state;
    if((led_delay_on > 0) && (led_delay_off > 0))
    {
        if(!strcmp(led_cdev->name, "red"))
        {
            set_blink_en(RED_PORT);
        }
        else if(!strcmp(led_cdev->name, "green"))
        {
            set_blink_en(GREEN_PORT);
        }
        else if(!strcmp(led_cdev->name, "blue"))
        {
            set_blink_en(BLUE_PORT);
        }
    }
    else
    {
        set_blink_dis(RED_PORT);
        set_blink_dis(GREEN_PORT);
        set_blink_dis(BLUE_PORT);
    }
    value_to_reg = transform_delay_to_reg(state);
    ret = led_colors_write(client_leds, FULLY_ON_TIME, value_to_reg);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }
    return size;
}

static ssize_t led_delay_off_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%lu\n", led_delay_off);
}
static ssize_t led_delay_off_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    char *after;
    unsigned long state = simple_strtoul(buf, &after, 10);
    int ret;
    int value_to_reg;

    led_delay_off = state;
    if((led_delay_on > 0) && (led_delay_off > 0))
    {
        if(!strcmp(led_cdev->name, "red"))
        {
            set_blink_en(RED_PORT);
        }
        else if(!strcmp(led_cdev->name, "green"))
        {
            set_blink_en(GREEN_PORT);
        }
        else if(!strcmp(led_cdev->name, "blue"))
        {
            set_blink_en(BLUE_PORT);
        }
    }
    else
    {
        set_blink_dis(RED_PORT);
        set_blink_dis(GREEN_PORT);
        set_blink_dis(BLUE_PORT);
    }
    value_to_reg = transform_delay_to_reg(state);

    ret = led_colors_write(client_leds, FIRST_FULLY_OFF_TIME, value_to_reg);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }
    ret = led_colors_write(client_leds, SECOND_FULLY_OFF_TIME, value_to_reg);
    if (ret)
    {
        dev_err(&client_leds->dev, "failed to set register!\n");
    }
    return size;
}


static DEVICE_ATTR(delay_on, 0666, led_delay_on_show, led_delay_on_store);
static DEVICE_ATTR(delay_off, 0666, led_delay_off_show, led_delay_off_store);


//static char config_name[LEDS_CONFIG_NAME_LENGTH] = "unkown";

/*static bool get_leds_config_name(void)
{
    if(get_hw_config("leds/leds_type", config_name, LEDS_CONFIG_NAME_LENGTH, NULL))
    {
        return true;
    }
    else
    {
        return false;
    }
}*/
static int __devinit led_colors_probe(struct i2c_client *client,
                                        const struct i2c_device_id *id)
{
    int ret, error;
    dev_err(&client->dev, "Three colors leds probe begin!\n");
   /* ret = get_leds_config_name();
    if(!ret)
    {
        return -EIO;
    }
    if(strcmp(config_name, "tca6507"))
    {
        return -EIO;
    }*/

    client_leds = client;
  //  gpio_three_colors = get_gpio_num_by_name("GPIO_LED_EN");
    ret = gpio_request(gpio_three_colors, "gpio_leds_en");
    if (ret < 0)
    {
        dev_err(&client->dev, "unable to get EN GPIO 129!\n");
        ret = -ENODEV;
        goto err_free_mem;
    }

    gpio_direction_output(gpio_three_colors, 1);
    error = i2c_check_functionality(client->adapter,
                    I2C_FUNC_SMBUS_BYTE);
    if (!error)
    {
        dev_err(&client->dev, "%s adapter not supported\n",
                    dev_driver_string(&client->adapter->dev));
        ret = -ENODEV;
        goto err_i2c_check;

    }

    ret = led_colors_write(client, SELECT0, INIT_VALUE_OF_SELECT0);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, SELECT1, INIT_VALUE_OF_SELECT1);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, SELECT2, INIT_VALUE_OF_SELECT2);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, FADE_ON_TIME, INIT_VALUE_OF_FADE_ON_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, FULLY_ON_TIME, INIT_VALUE_OF_FULLY_ON_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, FADE_OFF_TIME, INIT_VALUE_OF_FADE_OFF_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, FIRST_FULLY_OFF_TIME, INIT_VALUE_OF_FIRST_FULLY_OFF_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, SECOND_FULLY_OFF_TIME, INIT_VALUE_OF_SECOND_FULLY_OFF_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, MAX_INTENSITY, INIT_VALUE_OF_MAX_INTENSITY);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_colors_write(client, ONE_SHOT_MASTER_INTENSITY, INIT_VALUE_OF_ONE_SHOT_MASTER_INTENSITY);
    if (ret)
    {
        dev_err(&client->dev, "failed to mode device\n");
        goto err_free_mem;
    }

    ret = led_classdev_register(&client->dev, &three_colors_led);
    if (ret)
    {
		gpio_free(gpio_three_colors);
		goto err_free_mem;
    }

    ret = device_create_file(three_colors_led.dev, &dev_attr_delay_on);
    if (ret)
    {
        gpio_free(gpio_three_colors);
		goto err_free_mem;
    }

    ret = device_create_file(three_colors_led.dev, &dev_attr_delay_off);
    if (ret)
    {
        gpio_free(gpio_three_colors);
		goto err_free_mem;
    }

    ret = led_classdev_register(&client->dev, &led_red);
    if (ret)
    {
        gpio_free(gpio_three_colors);
		goto err_free_mem;
    }

    ret = led_classdev_register(&client->dev, &led_green);
    if (ret)
    {
        gpio_free(gpio_three_colors);
		goto err_free_mem;
    }

    ret = led_classdev_register(&client->dev, &led_blue);
    if (ret)
    {
        gpio_free(gpio_three_colors);
		goto err_free_mem;
    }

    ret = device_create_file(led_red.dev, &dev_attr_delay_on);
    if (ret)
    {
        gpio_free(gpio_three_colors);
		goto err_free_mem;
    }

    ret = device_create_file(led_red.dev, &dev_attr_delay_off);
    if (ret)
    {
        gpio_free(gpio_three_colors);
    }

    ret = device_create_file(led_green.dev, &dev_attr_delay_on);
    if (ret)
    {
        gpio_free(gpio_three_colors);
    }

    ret = device_create_file(led_green.dev, &dev_attr_delay_off);
    if (ret)
    {
        gpio_free(gpio_three_colors);
    }

    ret = device_create_file(led_blue.dev, &dev_attr_delay_on);
    if (ret)
    {
        gpio_free(gpio_three_colors);
    }
    ret = device_create_file(led_blue.dev, &dev_attr_delay_off);
    if (ret)
    {
        gpio_free(gpio_three_colors);
    }
dev_err(&client->dev, "Three colors leds probe end!\n");
err_i2c_check:
	gpio_free(gpio_three_colors);
err_free_mem:

    return ret;
}

static int __devexit led_colors_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id led_colors_idtable[] = {
    { "led_colors", 0, },
    { }
};

MODULE_DEVICE_TABLE(i2c, led_colors_idtable);

static struct i2c_driver led_colors_driver = {
    .driver = {
            .name   = "led_colors",
            .owner  = THIS_MODULE,
    },

    .id_table      = led_colors_idtable,
    .probe         = led_colors_probe,
    .remove        = __devexit_p(led_colors_remove),
};
static int __init led_colors_init(void)
{
    return i2c_add_driver(&led_colors_driver);
}

static void __exit led_colors_exit(void)
{
    i2c_del_driver(&led_colors_driver);
}

module_init(led_colors_init);
module_exit(led_colors_exit);

MODULE_AUTHOR("wangpeng");
MODULE_DESCRIPTION("TCA6507 LED driver");
MODULE_LICENSE("GPL");
