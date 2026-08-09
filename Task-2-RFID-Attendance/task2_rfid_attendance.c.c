/*
 * File:   newmain.c
 * Author: SURYANARAYANA
 *
 * Created on 9 August, 2026, 3:18 PM
 */
#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 20000000UL

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define LCD_RS PORTBbits.RB2
#define LCD_EN PORTBbits.RB3
#define LCD_D4 PORTBbits.RB4
#define LCD_D5 PORTBbits.RB5
#define LCD_D6 PORTBbits.RB6
#define LCD_D7 PORTBbits.RB7

#define GREEN_LED PORTDbits.RD0
#define RED_LED PORTDbits.RD1
#define BUZZER PORTDbits.RD2

void LCD_Pulse(void)
{
    LCD_EN = 1;
    __delay_us(5);
    LCD_EN = 0;
    __delay_us(100);
}

void LCD_Send4Bit(unsigned char data)
{
    LCD_D4 = (data >> 0) & 1;
    LCD_D5 = (data >> 1) & 1;
    LCD_D6 = (data >> 2) & 1;
    LCD_D7 = (data >> 3) & 1;
    LCD_Pulse();
}

void LCD_Command(unsigned char cmd)
{
    LCD_RS = 0;
    LCD_Send4Bit(cmd >> 4);
    LCD_Send4Bit(cmd & 0x0F);
    __delay_ms(2);
}

void LCD_Char(unsigned char data)
{
    LCD_RS = 1;
    LCD_Send4Bit(data >> 4);
    LCD_Send4Bit(data & 0x0F);
    __delay_us(100);
}

void LCD_String(const char *str)
{
    while (*str)
    {
        LCD_Char(*str++);
    }
}

void LCD_Init(void)
{
    LCD_RS = 0;
    LCD_EN = 0;

    __delay_ms(20);

    LCD_Send4Bit(0x03);
    __delay_ms(5);
    LCD_Send4Bit(0x03);
    __delay_us(150);
    LCD_Send4Bit(0x03);
    LCD_Send4Bit(0x02);

    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);

    __delay_ms(2);
}

void LCD_SetCursor(unsigned char row, unsigned char column)
{
    unsigned char address;

    if (row == 1)
        address = 0x80 + (column - 1);
    else
        address = 0xC0 + (column - 1);

    LCD_Command(address);
}

void UART_Init(void)
{
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;

    TXSTA = 0x24;
    RCSTA = 0x90;
    SPBRG = 129;
}

void UART_SendChar(char data)
{
    while (!TXIF);
    TXREG = data;
}

void UART_SendString(const char *str)
{
    while (*str)
    {
        UART_SendChar(*str++);
    }
}

char UART_ReceiveChar(void)
{
    while (!RCIF);
    return RCREG;
}

void UART_Clear(void)
{
    if (RCSTAbits.OERR)
    {
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
    }
}

unsigned char RFID_Compare(char *id, const char *authorized)
{
    unsigned char i;

    for (i = 0; i < 12; i++)
    {
        if (id[i] != authorized[i])
            return 0;
    }

    return 1;
}

void RFID_Process(char *id)
{
    if (RFID_Compare(id, "123456789012") ||
        RFID_Compare(id, "234567890123"))
    {
        GREEN_LED = 1;
        RED_LED = 0;
        BUZZER = 0;

        LCD_Command(0x01);
        __delay_ms(2);

        LCD_SetCursor(1, 1);
        LCD_String("Access Granted");

        LCD_SetCursor(2, 1);
        LCD_String("John Doe");

        UART_SendString("Employee John Doe logged in\r\n");

        __delay_ms(10000);
    }
    else
    {
        GREEN_LED = 0;
        RED_LED = 1;
        BUZZER = 1;
        __delay_ms(300);
        BUZZER = 0;

        LCD_Command(0x01);
        __delay_ms(500);

        LCD_SetCursor(1, 1);
        LCD_String("Access Denied!");

        LCD_SetCursor(2, 1);
        LCD_String("Unknown RFID");

        UART_SendString("Unknown RFID: ");
        UART_SendString(id);
        UART_SendString("\r\n");

        __delay_ms(10000);

        BUZZER = 0;
    }
}

void main(void)
{
    char rfid_id[13];
    unsigned char count;
    char received;

    TRISA = 0x00;
    TRISB = 0x00;
    TRISC = 0x80;
    TRISD = 0x00;

    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;

    LCD_Init();
    UART_Init();

    UART_SendString("RFID System Ready\r\n");
    
    GREEN_LED = 0;
    RED_LED = 0;
    BUZZER = 0;

    LCD_SetCursor(1, 1);
    LCD_String("RFID Attendance");
    LCD_SetCursor(2, 1);
    LCD_String("Scan Card...");

    __delay_ms(2000);

    UART_Clear();

    while (1)
    {
        count = 0;

        while (count < 12)
        {
            received = UART_ReceiveChar();

            if (received >= '0' && received <= '9')
            {
                rfid_id[count] = received;
                count++;
            }
        }

        rfid_id[12] = '\0';

        RFID_Process(rfid_id);

        GREEN_LED = 0;
        RED_LED = 0;
        BUZZER = 0;

        __delay_ms(1000);

        LCD_Command(0x01);
        __delay_ms(2);

        LCD_SetCursor(1, 1);
        LCD_String("RFID Attendance");

        LCD_SetCursor(2, 1);
        LCD_String("Scan Card...");
    }
}