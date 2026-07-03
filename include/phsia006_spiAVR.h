#ifndef SPIAVR_H
#define SPIAVR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


//B5 should always be SCK(spi clock) and B3 should always be MOSI. If you are using an
//SPI peripheral that sends data back to the arduino, you will need to use B4 as the MISO pin.
//The SS pin can be any digital pin on the arduino. Right before sending an 8 bit value with
//the SPI_SEND() funtion, you will need to set your SS pin to low. If you have multiple SPI
//devices, they will share the SCK, MOSI and MISO pins but should have different SS pins.
//To send a value to a specific device, set it's SS pin to low and all other SS pins to high.

#define PIN_SCK   PORTB5
#define PIN_MOSI  PORTB3
#define PIN_SS    PORTB2
#define TFT_RST   PB0
#define TFT_DC    PB1

#define ST77_SWRESET  0x01
#define ST77_SLPOUT   0x11
#define ST77_COLMOD   0x3A
#define ST77_DISPON   0x29
#define ST77_CASET    0x2A
#define ST77_RASET    0x2B
#define ST77_RAMWR    0x2C


void SPI_INIT() {
    DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);
    SPCR |= (1 << SPE) | (1 << MSTR);
}

void SPI_SEND(char data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
}

void TFT_Select(void) {
    PORTB &= ~(1 << PIN_SS);
}

void TFT_Unselect(void) {
    PORTB |= (1 << PIN_SS);
}

void TFT_DC_Command(void) {
    PORTB &= ~(1 << TFT_DC);
}

void TFT_DC_Data(void) {
    PORTB |= (1 << TFT_DC);
}

void Send_Command(unsigned char cmd) {
    TFT_DC_Command();
    TFT_Select();
    SPI_SEND(cmd);
    TFT_Unselect();
}

void Send_Data(unsigned char data) {
    TFT_DC_Data();
    TFT_Select();
    SPI_SEND(data);
    TFT_Unselect();
}

void HardwareReset(void) {
    PORTB |=  (1 << TFT_RST);
    _delay_ms(10);
    PORTB &= ~(1 << TFT_RST);
    _delay_ms(200);
    PORTB |=  (1 << TFT_RST);
    _delay_ms(200);
}

void ST7735_init(void) {
    DDRB |= (1 << TFT_RST) | (1 << TFT_DC) | (1 << PIN_SS);
    PORTB |= (1 << PIN_SS) | (1 << TFT_DC) | (1 << TFT_RST);

    SPI_INIT();

    HardwareReset();

    Send_Command(ST77_SWRESET);
    _delay_ms(150);

    Send_Command(ST77_SLPOUT);
    _delay_ms(200);

    Send_Command(ST77_COLMOD);
    Send_Data(0x06);
    _delay_ms(10);

    Send_Command(ST77_DISPON);
    _delay_ms(200);
}

void ST7735_SetAddressWindow(unsigned char x0, unsigned char y0,
                             unsigned char x1, unsigned char y1) {
    Send_Command(ST77_CASET);
    Send_Data(0x00);
    Send_Data(x0);
    Send_Data(0x00);
    Send_Data(x1);

    Send_Command(ST77_RASET);
    Send_Data(0x00);
    Send_Data(y0);
    Send_Data(0x00);
    Send_Data(y1);
}

void ST7735_RAMWR_Start(void) {
    Send_Command(ST77_RAMWR);
}

void TFTColorMode(unsigned char r,
                        unsigned char g,
                        unsigned char b) {
    Send_Data(r);
    Send_Data(g);
    Send_Data(b);
}

void TFT_FillScreen(unsigned char r,
                       unsigned char g,
                       unsigned char b) {
    unsigned char x0 = 0;
    unsigned char y0 = 0;
    unsigned char x1 = 129;  
    unsigned char y1 = 128;   

    unsigned int width  = (unsigned int)(x1 - x0 + 1);
    unsigned int height = (unsigned int)(y1 - y0 + 1);
    unsigned long total = (unsigned long)width * (unsigned long)height;

    ST7735_SetAddressWindow(x0, y0, x1, y1);
    ST7735_RAMWR_Start();

    for (unsigned long i = 0; i < total; i++) {
        TFTColorMode(r, g, b);
    }
}
#endif /* SPIAVR_H */
