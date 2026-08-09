#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define _XTAL_FREQ 20000000

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

#define LIGHT PORTDbits.RD0
#define FAN PORTDbits.RD1
#define GREEN PORTDbits.RD2
#define RED PORTDbits.RD3
#define BUZZER PORTDbits.RD4

char command[30];
unsigned char command_index = 0;

unsigned char light_state = 0;
unsigned char fan_state = 0;

void LCD_Pulse(void)
{
    LCD_EN = 1;
    __delay_us(2);
    LCD_EN = 0;
    __delay_us(100);
}

void LCD_SendNibble(unsigned char data)
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

    LCD_SendNibble(cmd >> 4);
    LCD_SendNibble(cmd & 0x0F);

    __delay_ms(2);
}

void LCD_Char(char data)
{
    LCD_RS = 1;

    LCD_SendNibble(data >> 4);
    LCD_SendNibble(data & 0x0F);

    __delay_us(50);
}

void LCD_String(const char *str)
{
    while(*str)
    {
        LCD_Char(*str++);
    }
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    __delay_ms(2);
}

void LCD_SetCursor(unsigned char row, unsigned char column)
{
    unsigned char address;

    if(row == 1)
        address = 0x80 + column - 1;
    else
        address = 0xC0 + column - 1;

    LCD_Command(address);
}

void LCD_Init(void)
{
    LCD_RS = 0;
    LCD_EN = 0;

    __delay_ms(20);

    LCD_SendNibble(0x03);
    __delay_ms(5);

    LCD_SendNibble(0x03);
    __delay_us(150);

    LCD_SendNibble(0x03);
    LCD_SendNibble(0x02);

    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);

    __delay_ms(2);
}

void UART_Init(void)
{
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;

    SPBRG = 129;

    BRGH = 1;
    SYNC = 0;
    SPEN = 1;

    TXEN = 1;
    CREN = 1;
}

void UART_Write(char data)
{
    while(!TXIF);

    TXREG = data;
}

void UART_String(const char *str)
{
    while(*str)
    {
        UART_Write(*str++);
    }
}

char UART_Read(void)
{
    if(OERR)
    {
        CREN = 0;
        CREN = 1;
    }

    if(RCIF)
        return RCREG;

    return 0;
}

void ADC_Init(void)
{
    ADCON0 = 0x01;
    ADCON1 = 0x80;

    TRISAbits.TRISA0 = 1;
}

unsigned int ADC_Read(void)
{
    unsigned int result;

    ADCON0bits.CHS = 0;

    __delay_us(20);

    GO_nDONE = 1;

    while(GO_nDONE);

    result = ((unsigned int)ADRESH << 8) | ADRESL;

    return result;
}

unsigned int Get_Temperature(void)
{
    unsigned int adc_value;
    unsigned int temperature;

    adc_value = ADC_Read();

    temperature = (unsigned long)adc_value * 500 / 1024;

    return temperature;
}

void Display_Status(void)
{
    unsigned int temperature;
    char temp_string[8];

    temperature = Get_Temperature();

    LCD_Clear();

    LCD_SetCursor(1,1);

    LCD_String("Light:");

    if(light_state)
        LCD_String("ON ");
    else
        LCD_String("OFF");

    LCD_String(" Fan:");

    if(fan_state)
        LCD_String("ON");
    else
        LCD_String("OFF");

    LCD_SetCursor(2,1);

    LCD_String("Temp:");

    sprintf(temp_string, "%u", temperature);

    LCD_String(temp_string);
    LCD_String(" C");
}

void Light_On(void)
{
    LIGHT = 1;
    light_state = 1;

    GREEN = 1;
    RED = 0;

    UART_String("LIGHT turned ON\r\n");

    Display_Status();
}

void Light_Off(void)
{
    LIGHT = 0;
    light_state = 0;

    GREEN = 0;

    UART_String("LIGHT turned OFF\r\n");

    Display_Status();
}

void Fan_On(void)
{
    FAN = 1;
    fan_state = 1;

    GREEN = 1;
    RED = 0;

    UART_String("FAN turned ON\r\n");

    Display_Status();
}

void Fan_Off(void)
{
    FAN = 0;
    fan_state = 0;

    GREEN = 0;

    UART_String("FAN turned OFF\r\n");

    Display_Status();
}

void Send_Status(void)
{
    unsigned int temperature;
    char temp_string[10];

    temperature = Get_Temperature();

    UART_String("\r\n----- HOME STATUS -----\r\n");

    UART_String("LIGHT: ");

    if(light_state)
        UART_String("ON\r\n");
    else
        UART_String("OFF\r\n");

    UART_String("FAN: ");

    if(fan_state)
        UART_String("ON\r\n");
    else
        UART_String("OFF\r\n");

    UART_String("TEMP: ");

    sprintf(temp_string, "%u", temperature);

    UART_String(temp_string);
    UART_String(" C\r\n");

    UART_String("-----------------------\r\n");

    Display_Status();
}

void Send_Temperature(void)
{
    unsigned int temperature;
    char temp_string[10];

    temperature = Get_Temperature();

    UART_String("Temperature: ");

    sprintf(temp_string, "%u", temperature);

    UART_String(temp_string);
    UART_String(" C\r\n");

    Display_Status();
}

void Send_Help(void)
{
    UART_String("\r\nAvailable Commands:\r\n");

    UART_String("LIGHT ON\r\n");
    UART_String("LIGHT OFF\r\n");
    UART_String("FAN ON\r\n");
    UART_String("FAN OFF\r\n");
    UART_String("STATUS\r\n");
    UART_String("TEMP\r\n");
    UART_String("HELP\r\n");

    UART_String("----------------------\r\n");
}

void Process_Command(void)
{
    unsigned char i;

    for(i = 0; command[i] != '\0'; i++)
    {
        command[i] = toupper(command[i]);
    }

    if(strcmp(command, "LIGHT ON") == 0)
    {
        Light_On();
    }
    else if(strcmp(command, "LIGHT OFF") == 0)
    {
        Light_Off();
    }
    else if(strcmp(command, "FAN ON") == 0)
    {
        Fan_On();
    }
    else if(strcmp(command, "FAN OFF") == 0)
    {
        Fan_Off();
    }
    else if(strcmp(command, "STATUS") == 0)
    {
        Send_Status();
    }
    else if(strcmp(command, "TEMP") == 0)
    {
        Send_Temperature();
    }
    else if(strcmp(command, "HELP") == 0)
    {
        Send_Help();
    }
    else
    {
        UART_String("Unknown command\r\n");
        UART_String("Type HELP for commands\r\n");
    }

    command_index = 0;
    command[0] = '\0';
}

void UART_Command_StateMachine(void)
{
    static unsigned char state = 0;

    char received;

    received = UART_Read();

    if(received == 0)
        return;

    switch(state)
    {
        case 0:

            if(received == '\r' || received == '\n')
            {
                if(command_index > 0)
                {
                    state = 1;
                }
            }
            else
            {
                if(command_index < sizeof(command) - 1)
                {
                    command[command_index++] = received;
                    command[command_index] = '\0';
                }
            }

            break;

        case 1:

            Process_Command();

            state = 0;

            break;

        default:

            state = 0;
            command_index = 0;
            command[0] = '\0';

            break;
    }
}

void main(void)
{
    TRISB = 0x00;
    TRISD = 0x00;

    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;

    PORTB = 0x00;
    PORTD = 0x00;

    LCD_Init();
    UART_Init();
    ADC_Init();

    LIGHT = 0;
    FAN = 0;

    GREEN = 0;
    RED = 0;
    BUZZER = 0;

    UART_String("\r\n");
    UART_String("HOME AUTOMATION SYSTEM\r\n");
    UART_String("System Ready\r\n");
    UART_String("Type HELP for commands\r\n");
    UART_String("----------------------\r\n");

    LCD_Clear();

    LCD_SetCursor(1,1);
    LCD_String("HOME AUTOMATION");

    LCD_SetCursor(2,1);
    LCD_String("System Ready");

    __delay_ms(2000);

    Display_Status();

    while(1)
    {
        UART_Command_StateMachine();
    }
}