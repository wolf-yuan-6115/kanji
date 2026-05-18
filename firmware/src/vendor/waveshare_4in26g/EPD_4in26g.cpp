/*****************************************************************************
* | File      	:   EPD_4in26g.c
* | Author      :   Waveshare team
* | Function    :   4in26 e-paper (G)
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-12-22
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_4in26g.h"
#include "Debug.h"
#include "time.h"


/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_4IN26G_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_4IN26G_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_4IN26G_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_4IN26G_ReadBusy(void)
{
    Debug("e-Paper busy\r\n");
    DEV_Delay_ms(100);
    while(DEV_Digital_Read(EPD_BUSY_PIN)) {      //LOW: idle, HIGH: busy
        DEV_Delay_ms(100);
    }
    Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static void EPD_4IN26G_ReadBusyH(void)
{
    Debug("e-Paper busy H\r\n");
    DEV_Delay_ms(100);
    while(!DEV_Digital_Read(EPD_BUSY_PIN)) {      //LOW: idle, HIGH: busy
        DEV_Delay_ms(5);
    }
    Debug("e-Paper busy H release\r\n");
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_4IN26G_TurnOnDisplay(void)
{
    EPD_4IN26G_SendCommand(0x12); // DISPLAY_REFRESH
    EPD_4IN26G_SendData(0x00);
    EPD_4IN26G_ReadBusyH();
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_4IN26G_Init(void)
{
    EPD_4IN26G_Reset();
    EPD_4IN26G_ReadBusyH();	

    EPD_4IN26G_SendCommand(0x00);	//0x00
    EPD_4IN26G_SendData(0x2B);	
    EPD_4IN26G_SendData(0x29);	

    EPD_4IN26G_SendCommand(0x06);	//0x06
    EPD_4IN26G_SendData(0x0F);	
    EPD_4IN26G_SendData(0x8B);	
    EPD_4IN26G_SendData(0x93);	
    EPD_4IN26G_SendData(0xA1);  //0xA4   0xA1

    EPD_4IN26G_SendCommand(0x61);//0x61	
    EPD_4IN26G_SendData(EPD_4IN26G_WIDTH/256);	
    EPD_4IN26G_SendData(EPD_4IN26G_WIDTH%256);	
    EPD_4IN26G_SendData(EPD_4IN26G_HEIGHT_INIT/256);	
    EPD_4IN26G_SendData(EPD_4IN26G_HEIGHT_INIT%256);	
    EPD_4IN26G_ReadBusyH();	


    EPD_4IN26G_SendCommand(0x50);	//0x50
    EPD_4IN26G_SendData(0x37);	

    EPD_4IN26G_SendCommand(0x30);	//0x30
    EPD_4IN26G_SendData(0x08);	

    EPD_4IN26G_SendCommand(0x62);
    EPD_4IN26G_SendData(0x76); 
    EPD_4IN26G_SendData(0x76);
    EPD_4IN26G_SendData(0x76); 
    EPD_4IN26G_SendData(0x5A);
    EPD_4IN26G_SendData(0x9D); 
    EPD_4IN26G_SendData(0x8A);	
    EPD_4IN26G_SendData(0x76); 
    EPD_4IN26G_SendData(0x62); 

    EPD_4IN26G_SendCommand(0xE3);	//0xE3
    EPD_4IN26G_SendData(0x22);	
    
    EPD_4IN26G_SendCommand(0xE0);	//0xE3
    EPD_4IN26G_SendData(0x10);	

    EPD_4IN26G_SendCommand(0xE7);	//0xE7
    EPD_4IN26G_SendData(0xA1);	     //0xA4   0xA1

    EPD_4IN26G_SendCommand(0xE9);	
    EPD_4IN26G_SendData(0x01);

    EPD_4IN26G_SendCommand(0x04); //Power on
    EPD_4IN26G_ReadBusyH();          //waiting for the electronic paper IC to release the idle signal
}

void EPD_4IN26G_Init_Fast(void)
{
    EPD_4IN26G_Reset();
    EPD_4IN26G_ReadBusyH();	

    EPD_4IN26G_SendCommand(0x00);	//0x00
    EPD_4IN26G_SendData(0x2B);	
    EPD_4IN26G_SendData(0x29);	

    EPD_4IN26G_SendCommand(0x06);	//0x06
    EPD_4IN26G_SendData(0x0F);	
    EPD_4IN26G_SendData(0x8B);	
    EPD_4IN26G_SendData(0x93);	
    EPD_4IN26G_SendData(0xA4);

    EPD_4IN26G_SendCommand(0x61);//0x61	
    EPD_4IN26G_SendData(EPD_4IN26G_WIDTH/256);	
    EPD_4IN26G_SendData(EPD_4IN26G_WIDTH%256);	
    EPD_4IN26G_SendData(EPD_4IN26G_HEIGHT_INIT/256);	
    EPD_4IN26G_SendData(EPD_4IN26G_HEIGHT_INIT%256);
    EPD_4IN26G_ReadBusyH();	

    EPD_4IN26G_SendCommand(0x50);	//0x50
    EPD_4IN26G_SendData(0x37);	

    EPD_4IN26G_SendCommand(0x30);	//0x30
    EPD_4IN26G_SendData(0x08);	

    EPD_4IN26G_SendCommand(0x62);
    EPD_4IN26G_SendData(0x76); 
    EPD_4IN26G_SendData(0x76);
    EPD_4IN26G_SendData(0x76); 
    EPD_4IN26G_SendData(0x5A);
    EPD_4IN26G_SendData(0x9D); 
    EPD_4IN26G_SendData(0x8A);	
    EPD_4IN26G_SendData(0x76); 
    EPD_4IN26G_SendData(0x62); 

    EPD_4IN26G_SendCommand(0x65);	//0x65
    EPD_4IN26G_SendData(0x00);	
    EPD_4IN26G_SendData(0x00);	
    EPD_4IN26G_SendData(0x00);	
    EPD_4IN26G_SendData(0x00);	

    EPD_4IN26G_SendCommand(0xE3);	//0xE3
    EPD_4IN26G_SendData(0x22);	

    EPD_4IN26G_SendCommand(0xE0);	//0xE3
    EPD_4IN26G_SendData(0x10);	

    EPD_4IN26G_SendCommand(0xE7);	//0xE7
    EPD_4IN26G_SendData(0xA4);	

    EPD_4IN26G_SendCommand(0xE9);	
    EPD_4IN26G_SendData(0x01);

    EPD_4IN26G_SendCommand(0xEF);	
    EPD_4IN26G_SendData(0x01);
    EPD_4IN26G_SendCommand(0xF6);	
    EPD_4IN26G_SendData(0x20);

    EPD_4IN26G_SendCommand(0xEF);	
    EPD_4IN26G_SendData(0x00);

    EPD_4IN26G_SendCommand(0xE0);	
    EPD_4IN26G_SendData(0x12);

    EPD_4IN26G_SendCommand(0xE6);	
    EPD_4IN26G_SendData(92);

    EPD_4IN26G_SendCommand(0xA5);	
    EPD_4IN26G_SendData(0x00);
    EPD_4IN26G_ReadBusyH();

    EPD_4IN26G_SendCommand(0x04); //Power on
    EPD_4IN26G_ReadBusyH();          //waiting for the electronic paper IC to release the idle signal
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_4IN26G_Clear(const UBYTE color)
{
    UWORD Width, Height;
    Width = (EPD_4IN26G_WIDTH % 4 == 0)? (EPD_4IN26G_WIDTH / 4 ): (EPD_4IN26G_WIDTH / 4 + 1);
    Height = EPD_4IN26G_HEIGHT;

    EPD_4IN26G_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN26G_SendData((color << 6) | (color << 4) | (color << 2) | color);
        }
    }

    EPD_4IN26G_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_4IN26G_Display(const UBYTE *Image)
{
    UWORD Width, Height;
    Width = (EPD_4IN26G_WIDTH % 4 == 0)? (EPD_4IN26G_WIDTH / 4 ): (EPD_4IN26G_WIDTH / 4 + 1);
    Height = EPD_4IN26G_HEIGHT;

    EPD_4IN26G_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN26G_SendData(Image[i + j * Width]);
        }
    }

    EPD_4IN26G_TurnOnDisplay();
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_4IN26G_Sleep(void)
{
    EPD_4IN26G_SendCommand(0x02); // POWER_OFF
    EPD_4IN26G_SendData(0X00);
    EPD_4IN26G_SendCommand(0x07); // DEEP_SLEEP
    EPD_4IN26G_SendData(0XA5);
}
