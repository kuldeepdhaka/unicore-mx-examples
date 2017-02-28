/* Includes ------------------------------------------------------------------*/
#include <unicore-mx/stm32/rcc.h>
#include <unicore-mx/stm32/gpio.h>
#include <unicore-mx/lcd_tft/lcd_tft.h>
#include "RGB_565_480_272.h"
#include "rk043fn48h.h"

/* Private function prototypes -----------------------------------------------*/
static void lcd_config(void);
static void lcd_pinmux(void);
static void lcd_clock(void);
static void lcd_config_layer(lcd_tft *lt);

/* Private functions ---------------------------------------------------------*/

void lcd_pinmux(void)
{
    /* Enable the LTDC Clock */
    rcc_periph_clock_enable(RCC_LTDC);

    /* Enable GPIOs clock */
    rcc_periph_clock_enable(RCC_GPIOE);
    rcc_periph_clock_enable(RCC_GPIOG);
    rcc_periph_clock_enable(RCC_GPIOI);
    rcc_periph_clock_enable(RCC_GPIOJ);
    rcc_periph_clock_enable(RCC_GPIOK);

    /*** LTDC Pins configuration ***/
    gpio_mode_setup(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO4);
    gpio_set_output_options(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO4);
    gpio_set_af(GPIOE, GPIO_AF14, GPIO4);

    /* GPIOG configuration */
    gpio_mode_setup(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO12);
    gpio_set_output_options(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO12);
    gpio_set_af(GPIOG, GPIO_AF9, GPIO12);

    /* GPIOI LTDC alternate configuration */
    gpio_mode_setup(GPIOI, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);
    gpio_set_output_options(GPIOI, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);
    gpio_set_af(GPIOI, GPIO_AF14, GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);

    /* GPIOJ configuration */
    gpio_mode_setup(GPIOJ, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
                                                         GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 |
                                                         GPIO10 | GPIO11 | GPIO13 | GPIO14 | GPIO15);
    gpio_set_output_options(GPIOJ, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
                                                         GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 |
                                                         GPIO10 | GPIO11 | GPIO13 | GPIO14 | GPIO15);
    gpio_set_af(GPIOJ, GPIO_AF14, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
                                                         GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 |
                                                         GPIO10 | GPIO11 | GPIO13 | GPIO14 | GPIO15);

    /* GPIOK configuration */
    gpio_mode_setup(GPIOK, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO2 | GPIO4 | GPIO5 | GPIO6 | GPIO7);
    gpio_set_output_options(GPIOK, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO0 | GPIO1 | GPIO2 | GPIO4 | GPIO5 | GPIO6 | GPIO7);
    gpio_set_af(GPIOK, GPIO_AF14, GPIO0 | GPIO1 | GPIO2 | GPIO4 | GPIO5 | GPIO6 | GPIO7);

    /* LCD_DISP GPIO configuration */
    gpio_mode_setup(GPIOI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
    gpio_set_output_options(GPIOI, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO12);

    /* LCD_BL_CTRL GPIO configuration */
    gpio_mode_setup(GPIOK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
    gpio_set_output_options(GPIOK, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO3);

    /* Assert display enable LCD_DISP pin */
    gpio_set(GPIOI, GPIO12);

    /* Assert backlight LCD_BL_CTRL pin */
    gpio_set(GPIOK, GPIO3);
}

static void lcd_config_layer(lcd_tft *lt)
{
	const struct lcd_tft_layer layer = {
		.framebuffer = {
			.x = 0,
			.y = 0,
			.width = RGB_565_480_272.width,
			.height = RGB_565_480_272.height,
			.format = LCD_TFT_RGB565,
			.data = RGB_565_480_272.pixel_data
		},

		.palette = {.data = NULL, .count = 0},
		.transparent = {.data = NULL, .count = 0}
	};

	lcd_tft_layer_set(lt, 1, &layer);
}

/**
  * @brief LCD Configuration.
  * @note  This function Configure tha LTDC peripheral :
  *        1) Configure the Pixel Clock for the LCD
  *        2) Configure the LTDC Timing and Polarity
  *        3) Configure the LTDC Layer 1 :
  *           - The frame buffer is located at FLASH memory
  *           - The Layer size configuration : 480x272
  * @retval
  *  None
  */
static void lcd_config(void)
{
	const struct lcd_tft_config config = {
		.timing = {
			.hsync = RK043FN48H_HSYNC,
			.vsync = RK043FN48H_VSYNC,
			.hbp = RK043FN48H_HBP,
			.vbp = RK043FN48H_VBP,
			.height = RK043FN48H_HEIGHT,
			.width = RK043FN48H_WIDTH,
			.vfp = RK043FN48H_VFP,
			.hfp = RK043FN48H_HFP
		},
		.features = LCD_TFT_HSYNC_ACTIVE_LOW |
					LCD_TFT_VSYNC_ACTIVE_LOW |
					LCD_TFT_DE_ACTIVE_LOW |
					LCD_TFT_CLK_ACTIVE_LOW,
		.output = LCD_TFT_OUTPUT_RGB888,
		.background = 0x00000000
	};

	lcd_tft *lt = lcd_tft_init(LCD_TFT_STM32_LTDC, &config);

	/* Configure the Layer*/
	lcd_config_layer(lt);
}


static void lcd_clock(void)
{
  /* LCD clock configuration */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */
  /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */

  /* Disable PLLSAI */
  RCC_CR &= ~RCC_CR_PLLSAION;
  while((RCC_CR & (RCC_CR_PLLSAIRDY))) {};

  /* N and R are needed,
   * P and Q are not needed for LTDC */
  RCC_PLLSAICFGR &= ~RCC_PLLSAICFGR_PLLSAIN_MASK;
  RCC_PLLSAICFGR |= 192 << RCC_PLLSAICFGR_PLLSAIN_SHIFT;
  RCC_PLLSAICFGR &= ~RCC_PLLSAICFGR_PLLSAIR_MASK;
  RCC_PLLSAICFGR |= 5 << RCC_PLLSAICFGR_PLLSAIR_SHIFT;
  RCC_DCKCFGR1 &= ~RCC_DCKCFGR1_PLLSAIDIVR_MASK;
  RCC_DCKCFGR1 |= RCC_DCKCFGR1_PLLSAIDIVR_DIVR_4;

  /* Enable PLLSAI */
  RCC_CR |= RCC_CR_PLLSAION;
  while(!(RCC_CR & (RCC_CR_PLLSAIRDY))) {};
}

int main(void)
{
  lcd_pinmux();
  lcd_clock();
  lcd_config(); /* Configure LCD : Only one layer is used */

  while (1) {}
}

