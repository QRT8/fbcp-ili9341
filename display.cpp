#include "config.h"
#include "display.h"
#include "spi.h"

#include <memory.h>

void ClearScreen(bool CS_BIT)
{
      for (int y = 0; y < DISPLAY_ACTUAL_HEIGHT; ++y)
      {
#ifdef DISPLAY_SPI_BUS_IS_16BITS_WIDE
          SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_X, 0, 0, 0, 0, 0, (DISPLAY_WIDTH - 1) >> 8, 0, (DISPLAY_WIDTH - 1) & 0xFF);
          SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_Y, 0, (uint8_t)(y >> 8), 0, (uint8_t)(y & 0xFF), 0, (DISPLAY_ACTUAL_HEIGHT - 1) >> 8, 0, (DISPLAY_ACTUAL_HEIGHT - 1) & 0xFF);
#elif defined(DISPLAY_SET_CURSOR_IS_8_BIT)
          SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_X, 0, DISPLAY_WIDTH - 1);
          SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_Y, (uint8_t)y, DISPLAY_ACTUAL_HEIGHT - 1);
#else
          SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_X, 0, 0, (DISPLAY_WIDTH - 1) >> 8, (DISPLAY_WIDTH - 1) & 0xFF);
          SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_Y, (uint8_t)(y >> 8), (uint8_t)(y & 0xFF), (DISPLAY_ACTUAL_HEIGHT - 1) >> 8, (DISPLAY_ACTUAL_HEIGHT - 1) & 0xFF);
#endif
          SPITask* clearLine = AllocTask(CS_BIT, DISPLAY_WIDTH * SPI_BYTESPERPIXEL);
          clearLine->cmd = DISPLAY_WRITE_PIXELS;
          memset(clearLine->data, 0, clearLine->size);
          CommitTask(clearLine);
          RunSPITask(clearLine);
          DoneTask(clearLine);
      }
#ifdef DISPLAY_SPI_BUS_IS_16BITS_WIDE
      SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_X, 0, 0, 0, 0, 0, (DISPLAY_WIDTH - 1) >> 8, 0, (DISPLAY_WIDTH - 1) & 0xFF);
      SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_Y, 0, 0, 0, 0, 0, (DISPLAY_ACTUAL_HEIGHT - 1) >> 8, 0, (DISPLAY_ACTUAL_HEIGHT - 1) & 0xFF);
#elif defined(DISPLAY_SET_CURSOR_IS_8_BIT)
      SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_X, 0, DISPLAY_WIDTH - 1);
      SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_Y, 0, DISPLAY_ACTUAL_HEIGHT - 1);
#else
      SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_X, 0, 0, (DISPLAY_WIDTH - 1) >> 8, (DISPLAY_WIDTH - 1) & 0xFF);
      SPI_TRANSFER(CS_BIT, DISPLAY_SET_CURSOR_Y, 0, 0, (DISPLAY_ACTUAL_HEIGHT - 1) >> 8, (DISPLAY_ACTUAL_HEIGHT - 1) & 0xFF);
#endif
}

