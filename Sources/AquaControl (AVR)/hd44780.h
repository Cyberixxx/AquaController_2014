/*	�������������� 2014 �. 1.0.0.

	���������� ��� ������ � ��� �� ����������� HD44780.
	������������ 4-� ������ ���� �������� ������.
	��������� WH2004L-TMI-CTW.
	
	���������� �.�. 2014�.
*/

#ifndef hd44780_H
#define hd44780_H
#endif
// ����������� ����������
#include <avr/io.h>
#include "Functions.h"
#include <util/delay.h>

// ������������
#define LCD_RS() LCD_SYGNAL_PORT|= RS // ���������� ������ ������/�������
#define LCD_NRS() LCD_SYGNAL_PORT&=~RS // �������� ������ ������/�������
#define LCD_E() LCD_SYGNAL_PORT|= E // ���������� ������ ������
#define LCD_NE() LCD_SYGNAL_PORT&=~E // �������� ������ ������
#define default1 0x38 // ����� ������������ ��������� (���� ������ 8-���, 4 ����, �����5�7)
#define default2 0x0C // �������� �������, �������� ������
#define delay 200 // �������� �������� ��� ����� ����� ���������
#define char_num 20 // ���-�� �������� � ������

// ���������� ����������
char string_1[char_num]="                    "; // ������ 1
char string_2[char_num]="<T1> 00.0  PH       "; // ������ 2
char string_3[char_num]="<T2> 00.0  Rdx      "; // ������ 3
char string_4[char_num]="<T3> 00.0  ---------"; // ������ 4

// ��������� �������
void write(void); // ������� ������ ������ ���
void init_LCD(void); // ������������� ���
void data_to_LCD(unsigned char data_in); // ��������� ������ ���
void comand_to_LCD(unsigned char data_in); // ���������� ������� ���
void string_to_LCD(unsigned char line, unsigned char pos, char str[]); // ������� ������ ���

void init_LCD() // ������� ������������� ���
{
	// ������������ ��������
	LCD_SYGNAL_DDR |= RS; 
	LCD_SYGNAL_DDR |= E; 
	
	// ������������� ���
	LCD_NRS();
	_delay_us(delay);
	LCD_DATA_PORT=0b00100000|(LCD_DATA_PIN&DATA_MASK); // ���������� ��������
	write(); // ������
	comand_to_LCD(0b00101000); _delay_us(40);
	comand_to_LCD(0b00001100); _delay_us(40);
	comand_to_LCD(0b00000001); _delay_ms(2);
	comand_to_LCD(0b00000110); _delay_us(40); 	
	_delay_ms(40); // ��������
}

void write() // ������� ������ ������ ���
{
	LCD_E(); // ��������� ������
	_delay_us(delay); 
	LCD_NE(); // ����� ������
	_delay_us(delay);
}

void comand_to_LCD(unsigned char data_in) // ������� �������� ������ ���
{
	LCD_NRS(); // �������� ������ ������/�������
	_delay_us(delay);
	// ��������� ������� �������
	LCD_DATA_PORT=(data_in&0xF0)|(LCD_DATA_PIN&DATA_MASK);
	write(); 
	// ��������� ������� �������
	LCD_DATA_PORT=(data_in<<4)|(LCD_DATA_PIN&DATA_MASK);
	write(); 
}

void data_to_LCD(unsigned char data_in) // ������� �������� ������ ���
{
	LCD_RS(); // ���������� ������ ������/�������
	_delay_us(delay);
	// ��������� ������� �������
	LCD_DATA_PORT=(data_in&0xF0)|(LCD_DATA_PIN&DATA_MASK);
	write();
	// ��������� ������� �������
	LCD_DATA_PORT=(data_in<<4)|(LCD_DATA_PIN&DATA_MASK);
	write(); 
}

void string_to_LCD(unsigned char line, unsigned char pos, char str[]) // ������� ������ ����� ���
{
	switch(line) // ����� ������������ ������
	{
		case 1: pos=pos+0x80; break;
		case 2: pos=pos+0xC0; break;
		case 3: pos=pos+0x94; break;
		case 4: pos=pos+0xD4; break;
		default:; break;
	}
	comand_to_LCD(pos); // ��������� ������� �������

	for(unsigned char t=0;t<char_num;t++) data_to_LCD(str[t]); // ����� ������
}