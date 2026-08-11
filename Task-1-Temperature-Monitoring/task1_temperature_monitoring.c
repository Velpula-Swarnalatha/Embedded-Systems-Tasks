/*
 * File:   newmain.c
 * Author: SURYANARAYANA
 *
 * Created on 9 August, 2026, 11:00 AM
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

#define LCD_RS PORTBbits.RB0
#define LCD_EN PORTBbits.RB1
#define LCD_D4 PORTBbits.RB2
#define LCD_D5 PORTBbits.RB3
#define LCD_D6 PORTBbits.RB4
#define LCD_D7 PORTBbits.RB5

#define GREEN_LED  PORTDbits.RD2
#define YELLOW_LED PORTDbits.RD3
#define RED_LED    PORTDbits.RD4

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

void ADC_Init(void)
{
    ADCON0 = 0x81;
    ADCON1 = 0x8E;
    __delay_ms(2);
}

unsigned int ADC_Read(void)
{
    ADCON0bits.CHS = 0;

    __delay_us(20);

    ADCON0bits.GO = 1;

    while (ADCON0bits.GO);

    return ((unsigned int)ADRESH << 8) | ADRESL;
}

void UART_SendNumber(unsigned int number)
{
    char buffer[6];
    unsigned char i = 0;

    if (number == 0)
    {
        UART_SendChar('0');
        return;
    }

    while (number > 0)
    {
        buffer[i++] = (number % 10) + '0';
        number /= 10;
    }

    while (i > 0)
    {
        UART_SendChar(buffer[--i]);
    }
}

void LCD_SendNumber(unsigned int number)
{
    char buffer[6];
    unsigned char i = 0;

    if (number == 0)
    {
        LCD_Char('0');
        return;
    }

    while (number > 0)
    {
        buffer[i++] = (number % 10) + '0';
        number /= 10;
    }

    while (i > 0)
    {
        LCD_Char(buffer[--i]);
    }
}

void main(void)
{
    unsigned int adc_value;
    unsigned int temperature;

    TRISA = 0x01;
    TRISB = 0x00;
    TRISC = 0x80;
    TRISD = 0x00;

    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;

    LCD_Init();
    UART_Init();
    ADC_Init();

    LCD_SetCursor(1, 1);
    LCD_String("Temperature");
    LCD_SetCursor(2, 1);
    LCD_String("Monitoring...");
    __delay_ms(2000);

    while (1)
    {
        adc_value = ADC_Read();
        temperature = (adc_value * 500UL) / 1024UL;

        LCD_Command(0x01);
        __delay_ms(2);

        if (temperature > 50)
        {
            GREEN_LED = 0;
            YELLOW_LED = 0;
            RED_LED = 1;

            LCD_SetCursor(1, 1);
            LCD_String("ALERT! Temp:");

            LCD_SetCursor(2, 1);
            LCD_SendNumber(temperature);
            LCD_String(" C");

            UART_SendString("Alert: Temperature is ");
            UART_SendNumber(temperature);
            UART_SendString(" C\r\n");
        }
        else
        {
            GREEN_LED = 1;
            YELLOW_LED = 0;
            RED_LED = 0;

            LCD_SetCursor(1, 1);
            LCD_String("Temp:");
            LCD_SendNumber(temperature);
            LCD_String(" C");

            UART_SendString("Temperature is ");
            UART_SendNumber(temperature);
            UART_SendString(" C\r\n");
        }

        __delay_ms(1000);
    }
}