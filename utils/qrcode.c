#include <stdint.h>
#include <string.h>
#include <display.h>
#include "utils/lzg/lzg.h"

// ecc::low ordianl=0, formatbits=1
static const int errorCorrectionLevel_formatBits = 1;
static const int size=33;

static const int PENALTY_N1 = 3;
static const int PENALTY_N2 = 3;
static const int PENALTY_N3 = 40;
static const int PENALTY_N4 = 10;

static unsigned char functions_bin_lzg[] = {
  0x4c, 0x5a, 0x47, 0x00, 0x00, 0x04, 0x41, 0x00, 0x00, 0x00, 0x46, 0x87,
  0x11, 0x05, 0x71, 0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x01, 0x05, 0x05,
  0x00, 0x05, 0x0d, 0x03, 0x06, 0x10, 0x05, 0x07, 0x03, 0x1f, 0x19, 0x03,
  0x1c, 0x19, 0x05, 0x1c, 0x03, 0x1d, 0x5b, 0x03, 0x07, 0x19, 0x03, 0x05,
  0x06, 0x03, 0x0e, 0x17, 0x05, 0x0e, 0x03, 0x1f, 0x19, 0x03, 0x1f, 0x19,
  0x03, 0x1f, 0x19, 0x03, 0x1e, 0x19, 0x03, 0x16, 0x19, 0x03, 0x47, 0x04,
  0x03, 0x56, 0x29, 0x03, 0x1f, 0x19, 0x03, 0x54, 0xd6, 0x03, 0x1e, 0x19,
  0x05, 0x10
};
static unsigned int functions_bin_lzg_len = 86;

static unsigned char modules_bin_lzg[] = {
  0x4c, 0x5a, 0x47, 0x00, 0x00, 0x04, 0x41, 0x00, 0x00, 0x00, 0x84, 0xfc,
  0xd8, 0x16, 0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00, 0x05, 0x03,
  0x01, 0x05, 0x10, 0x04, 0xd1, 0x03, 0x05, 0x13, 0x00, 0x05, 0x21, 0x05,
  0x0e, 0x04, 0xca, 0x04, 0x19, 0x04, 0x5d, 0x03, 0x13, 0x3a, 0x04, 0xd2,
  0x03, 0x1c, 0x19, 0x04, 0xd9, 0x03, 0x12, 0x5b, 0x03, 0x05, 0x19, 0x03,
  0x08, 0x68, 0x03, 0x14, 0x7c, 0x05, 0x04, 0x04, 0xbc, 0x05, 0x2d, 0x03,
  0x17, 0xd8, 0x03, 0x0f, 0x62, 0x03, 0x13, 0x7a, 0x04, 0xd4, 0x03, 0x15,
  0x11, 0x03, 0x17, 0x89, 0x05, 0x1d, 0x03, 0x1f, 0x3a, 0x03, 0x1f, 0x3a,
  0x03, 0x1f, 0x3a, 0x03, 0x1c, 0x3a, 0x03, 0x6a, 0x0e, 0x03, 0x1a, 0x1b,
  0x04, 0x48, 0x04, 0x8c, 0x03, 0x56, 0x72, 0x03, 0x47, 0x60, 0x03, 0x56,
  0xce, 0x03, 0x08, 0x3a, 0x03, 0x75, 0x10, 0x03, 0x07, 0x50, 0x03, 0x78,
  0x52, 0x03, 0x6c, 0x38, 0x03, 0x18, 0x1b, 0x03, 0x78, 0x52, 0x03, 0x6c,
  0xfe, 0x03, 0x18, 0x1b
};
static unsigned int modules_bin_lzg_len = 148;

static void print(uint8_t *modules) {
  memset(frame_buffer, 0x00, sizeof(frame_buffer));
  int y, x;
  for(y=1;y<size;y++) {
    for(x=0;x<size;x++) {
      if(modules[y*size+x]==0) {
        //disp_delpixel((128-33)/2+x, y+(DISPLAY_HEIGHT-33)/2);
#ifdef DISPLAY_NOKIA
        disp_setpixel((128-33)/2+x, y+(DISPLAY_HEIGHT-33)/2);
#else
        disp_setpixel((DISPLAY_WIDTH-66)/2+x*2, y*2);
        disp_setpixel((DISPLAY_WIDTH-66)/2+x*2+1, y*2);
        disp_setpixel((DISPLAY_WIDTH-66)/2+x*2, y*2+1);
        disp_setpixel((DISPLAY_WIDTH-66)/2+x*2+1, y*2+1);
#endif
      }
    }
  }
  disp_refresh();
}

static uint8_t multiply(uint8_t x, uint8_t y) {
  // Russian peasant multiplication
  int z = 0, i;

  for(i = 7;i >= 0;i--) {
    z = (z << 1) ^ ((z >> 7) * 0x11D);
    z ^= ((y >> i) & 1) * x;
  }
  return z;
}

static void getRemainder(const uint8_t *data, uint8_t *res) {
  const uint8_t coefficients[] = {152, 185, 240, 5, 111, 99, 6, 220, 112, 150, 69, 36, 187, 22, 228, 198, 121, 121, 165, 174};
  // Compute the remainder by performing polynomial division
  memset(res,0,20);
  int b=0,e=19;
  int i;
  for(i = 0; i < 80; i++) {
    uint8_t factor = data[i] ^ res[b];
    b=(b+1)%20; e=(e+1)%20;
    res[e]=0;
    int j;
    for(j = 0; j < 20; j++)
      res[(b+j)%20] ^= multiply(coefficients[j], factor);
  }
}

static void drawCodewords(const uint8_t *data, uint8_t *modules, uint8_t *fns) {
  int i = 0;  // Bit index into the data
  // Do the funny zigzag scan
  int right;
  for (right = size - 1; right >= 1; right -= 2) {  // Index of right column in each column pair
    if(right == 6)
      right = 5;
    int vert;
    for(vert = 0;vert < size; vert++) {  // Vertical counter
      int j;
      for(j = 0;j < 2;j++) {
        int x = right - j;  // Actual x coordinate
        uint8_t upwards = ((right & 2) == 0) ^ (x < 6);
        int y = upwards ? size - 1 - vert : vert;  // Actual y coordinate
        if (!fns[y*size+x] && i < 100 * 8) {
          modules[y*size+x] = ((data[i >> 3] >> (7 - (i & 7))) & 1) == 0;
          i++;
        }
        // If there are any remainder bits (0 to 7), they are already
        // set to 0/false/white when the grid of modules was initialized
      }
    }
  }
}

static void setFunctionModule(int x, int y, char isBlack, uint8_t *modules, uint8_t *fns) {
  modules[y*33+x] = isBlack == 0;
  fns[y*33+x]=1;
}

static void drawFormatBits(int mask, uint8_t *modules, uint8_t* fns) {
	// Calculate error correction code and pack bits
	int data = errorCorrectionLevel_formatBits << 3 | mask;  // errCorrLvl is uint2, mask is uint3
	int rem = data, i;
	for(i = 0; i < 10; i++)
     rem = (rem << 1) ^ ((rem >> 9) * 0x537);
	data = data << 10 | rem;
	data ^= 0x5412;  // uint15

	// Draw first copy
	for(i = 0;i <= 5;i++)
     setFunctionModule(8, i, ((data >> i) & 1) != 0, modules, fns);
	setFunctionModule(8, 7, ((data >> 6) & 1) != 0, modules, fns);
	setFunctionModule(8, 8, ((data >> 7) & 1) != 0, modules, fns);
	setFunctionModule(7, 8, ((data >> 8) & 1) != 0, modules, fns);
	for (i = 9; i < 15; i++)
     setFunctionModule(14 - i, 8, ((data >> i) & 1) != 0, modules, fns);

	// Draw second copy
	for(i = 0;i <= 7; i++)
     setFunctionModule(size - 1 - i, 8, ((data >> i) & 1) != 0, modules, fns);
	for(i = 8;i < 15; i++)
     setFunctionModule(8, size - 15 + i, ((data >> i) & 1) != 0, modules, fns);
	setFunctionModule(8, size - 8, 1, modules, fns);
}

static void applyMask(int mask, uint8_t *modules, uint8_t *fns) {
  int y, x;
  for(y = 0;y < size;y++) {
    for(x = 0;x < size;x++) {
      char invert;
      switch (mask) {
      case 0:  invert = (x + y) % 2 == 0;                    break;
      case 1:  invert = y % 2 == 0;                          break;
      case 2:  invert = x % 3 == 0;                          break;
      case 3:  invert = (x + y) % 3 == 0;                    break;
      case 4:  invert = (x / 3 + y / 2) % 2 == 0;            break;
      case 5:  invert = x * y % 2 + x * y % 3 == 0;          break;
      case 6:  invert = (x * y % 2 + x * y % 3) % 2 == 0;    break;
      case 7:  invert = ((x + y) % 2 + x * y % 3) % 2 == 0;  break;
      default:  while(1); // todo fail somehow?
      }
      modules[y*33+x] = modules[y*33+x] ^ (invert & !fns[y*33+x]);
    }
  }
}

static int getPenaltyScore(uint8_t *modules) {
  int result = 0;
	
  int y, x;
  // Adjacent modules in row having same color
  for(y = 0; y < size; y++) {
    char colorX = modules[y*33];
    int runX;
    for(x=1,runX=1; x < size; x++) {
      if (modules[y*33+x] != colorX) {
        colorX = modules[y*33+x];
        runX = 1;
      } else {
        runX++;
        if (runX == 5)
          result += PENALTY_N1;
        else if (runX > 5)
          result++;
      }
    }
  }
  // Adjacent modules in column having same color
  for (x = 0; x < size; x++) {
    char colorY = modules[x]; //modules.at(0).at(x);
    int runY;
    for(y=1, runY=1;y<size;y++) {
      if (modules[y*33+x] != colorY) {
        colorY = modules[y*33+x];
        runY = 1;
      } else {
        runY++;
        if (runY == 5)
          result += PENALTY_N1;
        else if (runY > 5)
          result++;
      }
    }
  }
	
  // 2*2 blocks of modules having same color
  for(y=0;y<size-1;y++) {
    for(x=0;x<size-1;x++) {
      char  color = modules[y*33+x];
      if (  color == modules[y*33+x+1] &&
            color == modules[(y + 1)*33+x] &&
            color == modules[(y + 1)*33+x+1])
        result += PENALTY_N2;
    }
  }
	
  // Finder-like pattern in rows
  for(y=0;y<size;y++) {
    int bits;
    for(x=0,bits=0;x<size;x++) {
      bits = ((bits << 1) & 0x7FF) | (modules[y*33+x] ? 1 : 0);
      if (x >= 10 && (bits == 0x05D || bits == 0x5D0))  // Needs 11 bits accumulated
        result += PENALTY_N3;
    }
  }
  // Finder-like pattern in columns
  for(x=0;x<size;x++) {
    int bits;
    for(y=0,bits=0;y<size;y++) {
      bits = ((bits << 1) & 0x7FF) | (modules[y*33+x] ? 1 : 0);
      if (y >= 10 && (bits == 0x05D || bits == 0x5D0))  // Needs 11 bits accumulated
        result += PENALTY_N3;
    }
  }
	
  // Balance of black and white modules
  int black = 0;
  for(y=0;y<size;y++) {
    for(x=0;x<size;x++) {
      if (modules[y*33+x])
        black++;
    }
  }
  int total = size * size;
  // Find smallest k such that (45-5k)% <= dark/total <= (55+5k)%
  int k;
  for(k = 0; black*20 < (9-k)*total || black*20 > (11+k)*total; k++)
    result += PENALTY_N4;
  return result;
}


static int handleConstructorMasking(uint8_t *modules, uint8_t *fns) {
  int mask=-1;
  int32_t minPenalty = INT32_MAX;
  for (int i = 0; i < 8; i++) {
    drawFormatBits(i, modules, fns);
    applyMask(i, modules, fns);
    int penalty = getPenaltyScore(modules);
    if (penalty < minPenalty) {
      mask = i;
      minPenalty = penalty;
    }
    applyMask(i, modules, fns);  // Undoes the mask due to XOR
  }
  drawFormatBits(mask, modules, fns);  // Overwrite old format bits
  applyMask(mask, modules, fns);  // Apply the final choice of mask
  return mask;  // The caller shall assign this value to the final-declared field
}

void qrcode(uint8_t *data, int len) {
  // Create the data bit string by concatenating all segments
  int i;
  uint8_t payload[100];
  payload[0]=(4<<4) | (64 >> 4); // 4(4)64(8)<data>
  payload[1]=(64 << 4) | (data[0] >> 4);
  for(i=0;i<len-1;i++) {
    payload[i+2]=(data[i]<<4) | (data[i+1] >> 4);
  }
  payload[2+len-1]=(data[len-1] << 4); // implicit 0(4) terminator

  // Pad with alternate bytes until data capacity is reached
  uint8_t padByte = 0xEC;
  for(i=0;i<80-len-2; padByte^=0xEC^0x11, i++)
    payload[2+len+i]=padByte;

  // Draw function patterns, draw all codewords, do masking
  // used QR-Code-generator/python/dump.py to generate these (and the reedsolomon coeffs)
  uint8_t modules[size*size];
  LZG_Decode(modules_bin_lzg, modules_bin_lzg_len, modules, size*size);
  uint8_t fns[size*size];
  LZG_Decode(functions_bin_lzg, functions_bin_lzg_len, fns, size*size);

  getRemainder(payload,&payload[80]);

  drawCodewords(payload, modules, fns);
  handleConstructorMasking(modules, fns);
  print(modules);
}
