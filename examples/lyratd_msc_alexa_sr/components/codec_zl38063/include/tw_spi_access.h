#ifndef TW_SPI_ACCESS_H
#define TW_SPI_ACCESS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int zl38063_init_firmware(unsigned char mode);

int zl38063_init_vol(unsigned short cmd, unsigned char numwords, unsigned short* pData);

#endif /* TW_SPI_ACCESS_H */


