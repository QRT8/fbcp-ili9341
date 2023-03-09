#include "config.h"

#if defined(ST7735R) || defined(ST7735S) || defined(ST7789)

#include "spi.h"

#include <memory.h>
#include <stdio.h>

void InitST7735R()
{
  // If a Reset pin is defined, toggle it briefly high->low->high to enable the device. Some devices do not have a reset pin, in which case compile with GPIO_TFT_RESET_PIN left undefined.
#if defined(GPIO_TFT_RESET_PIN) && GPIO_TFT_RESET_PIN >= 0
  printf("Resetting display at reset GPIO pin %d\n", GPIO_TFT_RESET_PIN);
  SET_GPIO_MODE(GPIO_TFT_RESET_PIN, 1);
  SET_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
  CLEAR_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
  SET_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
#endif

  // Do the initialization with a very low SPI bus speed, so that it will succeed even if the bus speed chosen by the user is too high.
  spi->clk = 34;
  __sync_synchronize();

  bool SPI_CS_BIT = 0;
#if defined(DISPLAY_USES_CE1) && NUM_DISPLAY_LOOPS < 2
  SPI_CS_BIT = 1;
#endif

  for (uint8_t DISPLAY_LOOP = 0; DISPLAY_LOOP < NUM_DISPLAY_LOOPS; DISPLAY_LOOP++, SPI_CS_BIT = !SPI_CS_BIT)
  {
  BEGIN_SPI_COMMUNICATION(SPI_CS_BIT);
  {
      #ifndef ST7789VW // For some reason, ST7789VW does not want to accept the Software Reset command, but screen stays black if SWRESET is sent to it.
        SPI_TRANSFER(SPI_CS_BIT, 0x01/*Software Reset*/);
      #endif
        usleep(120*1000);
        SPI_TRANSFER(SPI_CS_BIT, 0x11/*Sleep Out*/);
        usleep(120 * 1000);
      #ifndef ST7789VW // This is disabled on ST7789VW because it was observed to look visually bad, makes colors a bit too contrasty/deep
         SPI_TRANSFER(SPI_CS_BIT, 0x26/*Gamma Curve Select*/, 0x04/*Gamma curve 3 (2.5x if GS=1, 2.2x otherwise)*/);
      #endif
          SPI_TRANSFER(SPI_CS_BIT, 0x3A/*COLMOD: Pixel Format Set*/, 0x05/*16bpp*/);
      usleep(20 * 1000);

      #define MADCTL_BGR_PIXEL_ORDER (1<<3)
      #define MADCTL_ROW_COLUMN_EXCHANGE (1<<5)
      #define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1<<6)
      #define MADCTL_ROW_ADDRESS_ORDER_SWAP (1<<7)
      #define MADCTL_ROTATE_180_DEGREES (MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_ADDRESS_ORDER_SWAP)

          uint8_t madctl = 0;
      #if defined(ST7735R) || defined(ST7735S)
          madctl |= MADCTL_BGR_PIXEL_ORDER;
      #endif
      #ifdef DISPLAY_SWAP_BGR
          madctl ^= MADCTL_BGR_PIXEL_ORDER;
      #endif
      #if defined(DISPLAY_FLIP_ORIENTATION_IN_HARDWARE)
          madctl |= MADCTL_ROW_COLUMN_EXCHANGE;
      #endif

          madctl |= MADCTL_ROW_ADDRESS_ORDER_SWAP;

      #if defined(WAVESHARE_ST7789VW_HAT) || defined(WAVESHARE_ST7735S_HAT)
          madctl ^= MADCTL_ROTATE_180_DEGREES;
      #endif

      #ifdef DISPLAY_ROTATE_180_DEGREES
          madctl ^= MADCTL_ROTATE_180_DEGREES;
      #endif

          SPI_TRANSFER(SPI_CS_BIT, 0x36/*MADCTL: Memory Access Control*/, madctl);
          usleep(10*1000);

      #ifdef ST7789
          SPI_TRANSFER(SPI_CS_BIT, 0xBA/*DGMEN: Enable Gamma*/, 0x04);
          bool invertColors = true;
      #else
          bool invertColors = false;
      #endif
      #ifdef DISPLAY_INVERT_COLORS
          invertColors = !invertColors;
      #endif
          if (invertColors)
            SPI_TRANSFER(SPI_CS_BIT, 0x21/*Display Inversion On*/);
          else
            SPI_TRANSFER(SPI_CS_BIT, 0x20/*Display Inversion Off*/);

          SPI_TRANSFER(SPI_CS_BIT, 0x13/*NORON: Partial off (normal)*/);
          usleep(10*1000);

      #ifdef ST7789
          // The ST7789 controller is actually a unit with 320x240 graphics memory area, but only 240x240 portion
          // of it is displayed. Therefore if we wanted to swap row address mode above, writes to Y=0...239 range will actually land in
          // memory in row addresses Y = 319-(0...239) = 319...80 range. To view this range, we must scroll the view by +80 units in Y
          // direction so that contents of Y=80...319 is displayed instead of Y=0...239.
          if ((madctl & MADCTL_ROW_ADDRESS_ORDER_SWAP))
            SPI_TRANSFER(SPI_CS_BIT, 0x37/*VSCSAD: Vertical Scroll Start Address of RAM*/, 0, 320 - DISPLAY_WIDTH);
      #endif

          // TODO: The 0xB1 command is not Frame Rate Control for ST7789VW, 0xB3 is (add support to it)
      #ifndef ST7789VW
          // Frame rate = 850000 / [ (2*RTNA+40) * (162 + FPA+BPA)]
          SPI_TRANSFER(SPI_CS_BIT, 0xB1/*FRMCTR1:Frame Rate Control*/, /*RTNA=*/6, /*FPA=*/1, /*BPA=*/1); // This should set frame rate = 99.67 Hz
      #endif

          SPI_TRANSFER(SPI_CS_BIT, /*Display ON*/0x29);
          usleep(100 * 1000);


#if defined(GPIO_TFT_BACKLIGHT) && defined(BACKLIGHT_CONTROL)
    printf("Setting TFT backlight on at pin %d\n", GPIO_TFT_BACKLIGHT);
    SET_GPIO_MODE(GPIO_TFT_BACKLIGHT, 0x01); // Set backlight pin to digital 0/1 output mode (0x01) in case it had been PWM controlled
    SET_GPIO(GPIO_TFT_BACKLIGHT); // And turn the backlight on.
#endif

    ClearScreen(SPI_CS_BIT);

    printf("Initialized ST7789 on CS %d\n", SPI_CS_BIT);
  }
#ifndef USE_DMA_TRANSFERS // For DMA transfers, keep SPI CS & TA active.
  END_SPI_COMMUNICATION(SPI_CS_BIT);
#endif

  } //end DISPLAY_LOOP

  // And speed up to the desired operation speed finally after init is done.
  usleep(10 * 1000); // Delay a bit before restoring CLK, or otherwise this has been observed to cause the display not init if done back to back after the clear operation above.
  spi->clk = SPI_BUS_CLOCK_DIVISOR;
}

void TurnDisplayOff()
{
#if defined(GPIO_TFT_BACKLIGHT) && defined(BACKLIGHT_CONTROL)
  SET_GPIO_MODE(GPIO_TFT_BACKLIGHT, 0x01); // Set backlight pin to digital 0/1 output mode (0x01) in case it had been PWM controlled
  CLEAR_GPIO(GPIO_TFT_BACKLIGHT); // And turn the backlight off.
#endif
#if 0
  bool SPI_CS_BIT = 0
#if defined(DISPLAY_USES_CE1) && NUM_DISPLAY_LOOPS < 2
  SPI_CS_BIT = 1;
#endif
  for (uint8_t DISPLAY_LOOP = 0; DISPLAY_LOOP < NUM_DISPLAY_LOOPS; DISPLAY_LOOP++, SPI_CS_BIT = !SPI_CS_BIT)
    QUEUE_SPI_TRANSFER(SPI_CS_BIT, 0x28/*Display OFF*/);
    QUEUE_SPI_TRANSFER(SPI_CS_BIT, 0x10/*Enter Sleep Mode*/);
  } //end DISPLAY_LOOP
  usleep(120*1000); // Sleep off can be sent 120msecs after entering sleep mode the earliest, so synchronously sleep here for that duration to be safe.
#endif
//  printf("Turned display OFF\n");
}

void TurnDisplayOn()
{
#if 0
  bool SPI_CS_BIT = 0
#if defined(DISPLAY_USES_CE1) && NUM_DISPLAY_LOOPS < 2
  SPI_CS_BIT = 1;
#endif
  for (uint8_t DISPLAY_LOOP = 0; DISPLAY_LOOP < NUM_DISPLAY_LOOPS; DISPLAY_LOOP++, SPI_CS_BIT = !SPI_CS_BIT)
    QUEUE_SPI_TRANSFER(SPI_CS_BIT, 0x11/*Sleep Out*/);
    usleep(120 * 1000);
    QUEUE_SPI_TRANSFER(SPI_CS_BIT, 0x29/*Display ON*/);
  } //end DISPLAY_LOOP
#endif
#if defined(GPIO_TFT_BACKLIGHT) && defined(BACKLIGHT_CONTROL)
  SET_GPIO_MODE(GPIO_TFT_BACKLIGHT, 0x01); // Set backlight pin to digital 0/1 output mode (0x01) in case it had been PWM controlled
  SET_GPIO(GPIO_TFT_BACKLIGHT); // And turn the backlight on.
#endif
//  printf("Turned display ON\n");
}

void DeinitSPIDisplay()
{
  bool SPI_CS_BIT = 0;
#if defined(DISPLAY_USES_CE1) && NUM_DISPLAY_LOOPS < 2
  SPI_CS_BIT = 1;
#endif
  for (uint8_t DISPLAY_LOOP = 0; DISPLAY_LOOP < NUM_DISPLAY_LOOPS; DISPLAY_LOOP++, SPI_CS_BIT = !SPI_CS_BIT)
  {
    ClearScreen(SPI_CS_BIT);
  } //end DISPLAY_LOOP
}

#endif
