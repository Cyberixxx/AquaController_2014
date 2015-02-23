/* 	�������������� 2014 �. 1.0.0.

	����������� ����������� �����������, ����������� �������� ��� ����������.
	����� ������������� ��������, ���������� PH, Redox, ������ ���� � ��������� ������������� ������� ����������.
	������ ��������� ���������� �������� �������� � ���������� ���������������� ��������.
	��������� � �������� ������� ������, ������ �������� � ����������������� ������.
	���������� ���������� �����, ����������� �� �������� ������.
	
	���������� �.�. 2014�.
*/

// ����������� ����������
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h> 
#include <string.h> 
#include <math.h>

#include "ds18b20.c" // ������ � 1-Wire
 
#define baudrate 9600UL // �������� �������� USART
#define FREQ_CPU 11059200UL // ������� ��
// ������������ ���
#define LCD_DATA_PORT PORTA 
#define LCD_DATA_DDR DDRA 
#define LCD_DATA_PIN PINA 
#define LCD_SYGNAL_PORT PORTC 
#define LCD_SYGNAL_DDR DDRC 
#define DATA_MASK 0x0F
#define RS _BV(PC7) 
#define E _BV(PC6) 
// ������������ I2C
#define I2C_PORT PORTD 
#define I2C_DDR DDRD 
#define I2C_PIN PIND 
#define SDA_N 2 
#define SDA _BV(PD2) 
#define SCL _BV(PD3) 
// ������������ ADC
#define ADC_DDR DDRA
#define ADC_chanel 0x07
// ������������ RELE
#define RELE_PORT_1 PORTD 
#define RELE_DDR_1 DDRD 
#define RELE_PORT_2 PORTC 
#define RELE_DDR_2 DDRC 
#define RELE_1 _BV(PD4) 
#define RELE_2 _BV(PD5) 
#define RELE_3 _BV(PD6) 
#define RELE_4 _BV(PD7) 
#define RELE_5 _BV(PC0) 
#define RELE_6 _BV(PC1) 
#define RELE_7 _BV(PC2) 
#define RELE_8 _BV(PC3) 
// ������������ LED
#define LCD_LED _BV(PC5) 
#define LED _BV(PC4) 
#define LED_PORT PORTC
#define LED_DDR DDRC 
#define LCD_LED_PORT PORTC
#define LCD_LED_DDR DDRC
// ������������ ADM690A
#define WDI_DDR DDRA
#define WDI_PORT PORTA
#define WDI _BV(PA3)

// ���������� ����������
unsigned char buffer_ds[3]; // ����� DS1307N (�������, ������, ����)
unsigned char buffer_time[3]; // ��������� ������� DS1307N - ������������� � �� (�������, ������, ����)
unsigned char temp_control=0; // ���� �������������� ������
unsigned char produv=0; // ���� ������ �������� ������
int set_temp=260; // ���������� ����������� ������ 
unsigned int cooler_set=64; // �������� ������� (0-100)
unsigned char set_temp_dec=3; // ������������� ����� �������� (������� �������)
int temp_now=0; // ������� �����������
unsigned char heater=0; // ���� - ������ ����
unsigned char cooler=0; // ���� - ���������� ����
unsigned char co2=0; // ���� ������ CO2/PH
unsigned char co2_set=0; // ���� - ������ ��2
unsigned char rele_set[8]={0,0,0,0,0,0,0,0}; // ��������� RELE � ������ ������
unsigned char hand_mode_set=0; // ��������� RELE (������ �����)
unsigned char time_one_sec=0; // ������ ���������� ���������
unsigned char doliv=0; // ���� ������ ������ ����
unsigned char pompa=0; // ���� - ������ ����
int doliv_porog1=0; // ����� ���������� ������ ����
int doliv_porog2=0; // ����� ������ ������ ����
unsigned char WATER_ERROR=0; // ������������� ������� ������ ����
unsigned char WATER_ERROR_POMPA=0; // ������������� ������� ������ ����, �������� ��� ������ �����
unsigned char pompa_time_count=0; // ������ ������� ������ �����
unsigned char water_lvl_status=0; // ��������� ����� (0x30-���������, 0x31-������, 0x32-������ �����, 0x33-����� ����, 0x34-������������� ������� ������ ����)
unsigned char co2_status=0; // �������� �������������� (0x30-���������, 0x31-������, 0x32-������ �����, 0x33-����� PH/CO2)
unsigned char cooler_hand_start=0; // ��������� ������� ��� ������� ���������� (������ �����)
unsigned int cooler_hand_power=0; // �������� ������� (��������)

#include "i2c.h" // ������ � I2C
#include "USART.h" // ������ � USART

volatile unsigned int FlagsFastCount=0; // ����� ���������� ��������
unsigned char temp_req=0; // ����� ������������ �������������� �������
unsigned long adc_data=0; // �������� ���
unsigned char dh,dl; // �������� ��������� ���
unsigned long ph=0; // PH (��������� � ADC)
unsigned long redox=0; // redox-��������� (��������� � ADC)
int water_lvl=0; // ������� ������� ���� (��������� � ADC)
int water_lvl_last=0; // ���������� �������� ������ ����
int water_lvl_pompa=0; // ���������� �������� ������ ����, ��� ���������� �����
unsigned char water_lvl_ovf_time=0; // ������ ������� � ������� ������� ��������� ������ ���� (����������� �������)
long k1=1000; // ����������� 1 ��� ����������� PH-���������
long k2=1000; // ����������� 2 ��� ����������� PH-���������
long k=1000; // ����������� ��� ����������� redox-���������
unsigned int adc_ph_1=0; // �������� PH ��� ������� ��������
unsigned int adc_ph_2=0; // �������� PH ��� ������� ��������
unsigned int adc_redox=0; // �������� redox ��� ��������������� ��������
unsigned int ph_high=700; // ��������  PH, ��� ������� �������� ������ CO2 (7*100)
unsigned int ph_low=500; // ��������  PH, ��� ������� ��������� ������ CO2 (5*100)
unsigned long adc_ph_1_calibr=0; // ���������� �������������� �������� �������� 1
unsigned long adc_ph_2_calibr=0; // ���������� �������������� �������� �������� 2
unsigned long adc_redox_calibr=0; // ���������� �������������� �������� redox
unsigned char adc_count=0; // ������ ������ ������� ADC
float cooler_data[4]={0,0,0,0}; // �������� ��� ��� �������, ������������ ������
float cooler_min=64; // ����������� �������� ������� (~25%)
unsigned char cooler_start_pulse[4]={0,0,0,0}; // ������ ���������� ������� ��� ������������ ��������� ��������� �������
unsigned char temp_start_pulse=0; // ������ ��������� ������� ������ ������������� ��������
// ���������� ���������� ��������
volatile unsigned long timer1_start[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // ������ �1
volatile unsigned long timer1_stop[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // ����� �1
volatile unsigned long timer2_start[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // ������ �2
volatile unsigned long timer2_stop[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // ����� �2
volatile unsigned long timer3_start[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // ������ �3
volatile unsigned long timer3_stop[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // ����� �3 
volatile unsigned long time_min_and_sec=0; // �������� ����� � ������ � �������� ������������� (0-5959) - ��� �������� ������ ���������� ����
volatile unsigned long timer_produv_time=500; // ����� �������� �������� (5*100) 
volatile unsigned char timer_produv_period=1; // ������������� ��������� �������� ������
volatile unsigned char timer_produv_period_count=0; // ������ ������� ��� �������� ������
volatile unsigned long time_now=0; // �������� �������� ������� � �������� ������������� (07h30m14s*10000=73014)
volatile unsigned long lcd_led_start=70000; // ����� ��������� ��������� ��� (7*10000)
volatile unsigned long lcd_led_stop=230000; // ����� ���������� ��������� ��� (23*10000)
volatile unsigned long cooler_start=70000; // ����� ��������� ������� (7*10000)
volatile unsigned long cooler_stop=230000; // ����� ���������� ������� (23*10000)
volatile unsigned long co2_start=70000; // ����� ��������� ��2 (7*10000)
volatile unsigned long co2_stop=230000; // ����� ���������� ��2 (23*10000)
unsigned long data_ph=0; // �������� PH � ������ �������������� �������������
unsigned long data_redox=0; //�������� redox � ������ ��������������� ������������
unsigned char rele_ports_status=0; // ��������� ����� RELE 
unsigned char rele_ports_status_lcd=0; // ����������� ���������� ������� �� ���
unsigned char heater_timer_status=0; // ���� - ����������� � ������ ��������
unsigned char co2_timer_status=0; // ���� - ��2 � ������ ��������
unsigned char doliv_timer_status=0; // ���� - ����� � ������ ��������
unsigned char string_to_print=0; // ������ ������������ ����� ���
unsigned char set_temp_sensor=1; // ������������� ������������� ������ (������������� �����)
unsigned char rele_hand_status=0; // ��������� RELE (������ �����)
unsigned char cooler_chanel_status=0; // ��������� ������� (0x30-���������, 0x31-������, 0x32-������ �����, 0x33-������������� �����, 0x34-������)
unsigned char heater_chanel_status=0; // ��������� ����������� (0x30-���������, 0x31-������, 0x32-������ �����, 0x33-������������� �����)
unsigned char mode_status=0; // ��������� �������
unsigned char eeprom_count=0; // ������� EEPROM
unsigned char EEPROM_WRITE=0; // ���� ������ EEPROM

// ��������� �������
void scale_t (uint16_t t, uint16_t *c, uint16_t *d, uint8_t *s); // �������������� �����������
void set_temp_mode_and_produv(void); // ������������� ����� / ����� �������� ������
void save_option_eeprom(void); // ��������� ��������� � EEPROM
void load_option_eeprom(void); // ��������� ��������� �� EEPROM
void timers_data_to_uart(void); // �������� ������ ��������
void events_data_to_uart(void); // �������� ������� ������ ������� �� ��
void water_doliv(void); // ����� ��������� ����

#include "hd44780.h" // ����������� ���������� ��� ������ � HD44780

// ���������� ���������� - ��������� ������ (������������)
ISR (TIMER2_OVF_vect)
{
	TCNT2 = 31; // ���������� ��� ��������� ��������� 1 ������� (0.333...*3=1)
    static uint16_t PrevState=0, FastCount=0;
	
	FastCount++;
	// ���������� ������� ����� �� 0 � 1 � ��������� FastCount
	// � ������������� ����� � ���������� ���������� FlagsFastCount, ���������� �����
	// ������ ���������, ����� ������ �����������.
	FlagsFastCount|=~PrevState & FastCount;
	// ���������� ���������� ���������
	PrevState = FastCount;
} 

static inline void InitController(void) // ������������� �����������
{	
	// ��������� ����������
	cli();	
	// ������������ �������	
	LCD_DATA_DDR|=0xF0;	
	LCD_SYGNAL_DDR|=0xC0;
	// ADC
	ADC_DDR&=~ADC_chanel;
	ADMUX=0b01000000;
	// ��������� ������, ����� ������ Free Running, �������� 8
	ADCSRA=(1<<ADEN)|(1<<ADATE)|(1<<ADPS0)|(1<<ADPS1);
	ADCSRA|=(1<<ADSC); // ������� ������ �������������� ���
	// RELE
	RELE_DDR_1 |=RELE_1|RELE_2|RELE_3|RELE_4;
	RELE_DDR_2 |=RELE_5|RELE_6|RELE_7|RELE_8;
	RELE_PORT_1&=~(RELE_1|RELE_2|RELE_3|RELE_4);
	RELE_PORT_2&=~(RELE_5|RELE_6|RELE_7|RELE_8);
	// LED
	LCD_LED_DDR |=LCD_LED;
	LED_DDR |=LED;
	LCD_LED_PORT|=LCD_LED;
	LED_PORT&=~LED;
	// WDI
	WDI_DDR|=WDI;
	WDI_PORT&=~WDI;
	// WDI ��������� �������
	WDI_PORT|=WDI;
	_delay_us(100);
	WDI_PORT&=~WDI;
	// ����� PWM
	DDRB|= _BV(PB3);
	// ������������� ���
	_delay_ms(100);
	init_LCD();
	// ��������� ������
	TIMSK = _BV(TOIE2); // ��������� ���������� �� ������������ �������
	TCCR2 = _BV(CS21);	// ������������� �����������
	// Fast PWM, �������� OC0 ��� ����������, ����������� 8
	TCCR0 = (1<<CS01)|(1<<WGM01)|(1<<WGM00)|(1<<COM01)|(0<<COM00);
	OCR0  = 0;
	TCNT0 = 0;
    TCNT2 = 31; // ���������� ��� ��������� ��������� 0.333 �	
	// �������� ����������
	sei();
}

// �������������� �����������
void scale_t (uint16_t t, uint16_t *c, uint16_t *d, uint8_t *s)
{
	uint16_t cel=0,dec=0; // ���������� ����� � ���������� �������� �����
	uint8_t sig=0; // ���������� ����� �����
	
	if(t&0x8000) // ������������� �����������
	{
		cel=((t^0xffff)+1)>>4 ;
		dec=16-(t&0xf);
		sig='-';
	} 
	else // ������������� �����������
	{
		cel=t>>4;
		dec=t&0xf;
		sig='+';
	};

	if(t==0) sig=' '; // 0 ��������
	dec*=625;
	
	*c=cel;	// �����
	*d=dec;	// ����������
	*s=sig;	// ����
}

int main(void)
{
	struct // ������ ������������� ��������
	{
		uint8_t		s_p;	//	Sensor Present - ��������������� ����� �������
		uint8_t		s_m;	//	���������
		uint16_t	m_t;	//	�����������
	} ST[3]; // 3 �������
	
	// ��������������� ������ �����������
	uint16_t sb=0,mb=0;
	uint8_t s=0;

	// �������������
	InitController(); // ��
	usart_init(); // USART
	DS1307_init(); // ���� DS1307N

	temp_req=0; // ����� ������������ �������������� �������

	// ���� � EEPROM �������� ��������� - �������
	if(eeprom_read_word((unsigned int*)1000)==0x1d) 
	{
		load_option_eeprom();
	}
	// ���� ����� ����������� ����� ��� ������ � ������ ������
	if(cooler_start_pulse[0]>0) cooler_hand_start=1; // ����������� ��������� �������
	else cooler_hand_start=0;
	// ��� ������ ������� - �������� ��������� �������
	cooler_start_pulse[1]=0;
	cooler_start_pulse[2]=0;
	cooler_start_pulse[3]=0;
	
	while(1)
	{
		if (FlagsFastCount & _BV(7)) // ~ 41 ms
		{   
			// ----- ����������� ����������� ����� ��� ----- //
			string_to_print++;
			switch(string_to_print)
			{
				case 1: string_to_LCD(1,0,string_1); break;
				case 2:	string_to_LCD(2,0,string_2); break;
				case 3:	string_to_LCD(3,0,string_3); break;
				case 4:	{string_to_LCD(4,0,string_4); string_to_print=0;} break;
				default:; break;
			}
			FlagsFastCount &= ~_BV(7); // �������� ����
		};

		if (FlagsFastCount & _BV(9)) // ~ 167 ms
		{	
			// ----- ����� ������������� �������� ----- //
			switch(temp_req)
			{
				case 0: // ������ ��������������
					ST[0].s_p = DS18B20_start_meas( DS18X20_POWER_EXTERN, 'b', 4);
					ST[1].s_p = DS18B20_start_meas( DS18X20_POWER_EXTERN, 'b', 2);
					ST[2].s_p = DS18B20_start_meas( DS18X20_POWER_EXTERN, 'b', 1);
					temp_req =1;
				break;
				case 1:
					temp_start_pulse++;
					if(temp_start_pulse==20) // ������ ������� ����� �����������
					{
						ST[0].s_m = DS18B20_read_meas_single(&ST[0].m_t, 'b', 4);
						ST[1].s_m = DS18B20_read_meas_single(&ST[1].m_t, 'b', 2);
						ST[2].s_m = DS18B20_read_meas_single(&ST[2].m_t, 'b', 1);
						temp_req = 0;
						temp_start_pulse=0;
					}	
				break;
				default:; break;
			}
			// ----- �������������� ����������� ----- //
			// 1-� ������
			if(ST[0].s_m == DS18X20_OK) // ������ �������
			{
				sb=0;mb=0;s=' ';							
				scale_t(ST[0].m_t, &sb, &mb, &s); // ��������������					
				// ������ � ���������� ��� �����������
				string_2[5] = (((sb/10) % 10)+0x30);
				string_2[6] = ((sb % 10) + 0x30);			
				string_2[7] = ('.');						
				string_2[8] = (((mb/1000) % 10)+0x30);
			};
			// 2-� ������
			if(ST[1].s_m == DS18X20_OK) // ������ �������
			{
				sb=0;mb=0;s=' ';							
				scale_t(ST[1].m_t, &sb, &mb, &s); // ��������������					
				// ������ � ���������� ��� �����������
				string_3[5] = (((sb/10) % 10)+0x30);
				string_3[6] = ((sb % 10) + 0x30);		
				string_3[7] = ('.');										
				string_3[8] = (((mb/1000) % 10)+0x30);					
			};
			// 3-� ������
			if(ST[2].s_m == DS18X20_OK) // ������ �������
			{
				sb=0;mb=0;s=' ';							
				scale_t(ST[2].m_t, &sb, &mb, &s); // ��������������				
				// ������ � ���������� ��� �����������
				string_4[5] = (((sb/10) % 10)+0x30);
				string_4[6] = ((sb % 10) + 0x30);		
				string_4[7] = ('.');										
				string_4[8] = (((mb/1000) % 10)+0x30);					
			};
            
			// ----- ������� ����� ----- //
            DS1307(); // ����� DS1307N
			// �������������� � ������ � ���������� ��� �����������
			// ����
			string_1[0]=((((buffer_ds[2]&0xF0)>>4)*10)+(buffer_ds[2]&0x0F))/10+0x30; // HEX to BIN, ����� �� LCD
			string_1[1]=((((buffer_ds[2]&0xF0)>>4)*10)+(buffer_ds[2]&0x0F))%10+0x30;
			string_1[2]=0x3A; 
			// ������
			string_1[3]=((((buffer_ds[1]&0xF0)>>4)*10)+(buffer_ds[1]&0x0F))/10+0x30; // HEX to BIN, ����� �� LCD
			string_1[4]=((((buffer_ds[1]&0xF0)>>4)*10)+(buffer_ds[1]&0x0F))%10+0x30;
			string_1[5]=0x3A; 
			// �������
			string_1[6]=((((buffer_ds[0]&0xF0)>>4)*10)+(buffer_ds[0]&0x0F))/10+0x30; // HEX to BIN, ����� �� LCD
			string_1[7]=((((buffer_ds[0]&0xF0)>>4)*10)+(buffer_ds[0]&0x0F))%10+0x30;
				
			// ----- ����� ����� ADC ----- //
			adc_count++;
			// ������ ���������, �������� ������������ ��������
			dl=ADCL;
			dh=ADCH;
			adc_data =(dh<<8)+dl; // ����� ������������ ��������
			// ������������ �������
			if(adc_count==1)	
			{
				ph=adc_data; // �������� PH
				ADMUX=0b01000001;
				ADCSRA|=(1<<ADSC); // ������� ������ �������������� ���
			}
			if(adc_count==2)	
			{
				redox=adc_data; // �������� redox
				ADMUX=0b01000010;
				ADCSRA|=(1<<ADSC); // ������� ������ �������������� ���
			}
			if(adc_count==3)	
			{
				water_lvl=adc_data; // �������� ������ ����
				ADMUX=0b01000000;
				ADCSRA|=(1<<ADSC); // ������� ������ �������������� ���
				adc_count=0;
			}
			
			// ----- �������������� � ������ � ���������� ��� ����������� ----- //
			// PH, � ������ �������������� ������������
			data_ph=(ph*k1+k2)/1000;
			string_2[15]=data_ph/1000+0x30; 
			string_2[16]=(data_ph%1000)/100+0x30; 
			string_2[17]='.';
			string_2[18]=((data_ph%1000)%100)/10+0x30; 
			string_2[19]=((data_ph%1000)%100)%10+0x30; 
			// redox, � ������ ��������������� �����������
			data_redox=redox*k/1000;
			string_3[15]=data_redox/1000+0x30; 
			string_3[16]='.';
			string_3[17]=(data_redox%1000)/100+0x30; 
			string_3[18]=((data_redox%1000)%100)/10+0x30; 
			string_3[19]=((data_redox%1000)%100)%10+0x30; 
            
			FlagsFastCount &= ~_BV(9); // �������� ����
		};	
		
		if (FlagsFastCount & _BV(10)) // ~ 333 ms
		{
			// ----- ��������� �������� ������ ----- //
			// ----- ������ ���������� � ���� ������ ----- //
			if(usart_data_in[0]==0x3B) // ������� ������ ������ ������
			{
				switch(usart_data_in[1]) // ����������� �������
				{
					// ----- ������ ������� <����������� ����������> AquaControl ----- //
					
					case 0x01: // ������ �����, ��������� �����������
					{
						// ������ ���������� ��������
						if(usart_data_in[22]==0xB3) // ������ ����� ������ ������
						{
							if(usart_data_in[2]==0x31) rele_set[0]=1; // ��������� 1
							else rele_set[0]=0;
							if(usart_data_in[3]==0x31) rele_set[1]=1; // ��������� 2
							else rele_set[1]=0;
							if(usart_data_in[4]==0x31) rele_set[2]=1; // ��������� 3
							else rele_set[2]=0;
							if(usart_data_in[5]==0x31) rele_set[3]=1; // ��������� 4
							else rele_set[3]=0;
							if(usart_data_in[6]==0x31) rele_set[4]=1; // ����������
							else rele_set[4]=0;
							if(usart_data_in[7]==0x31) // --- �����������
							{
								rele_set[5]=1; // ����������� ��������
								heater_chanel_status=0x32; // ���� - ����������� � ������ ������
							}
							else 
							{
								rele_set[5]=0; // ����������� ���������
								heater_chanel_status=0; // �������� ����
							}
							if(usart_data_in[8]==0x31) // --- ������ ��2
							{
								rele_set[6]=1; // ������ ��2 ��������
								co2_status=0x32; // ���� - ������������� � ������ ������
							}
							else 
							{
								rele_set[6]=0; // ������ ��2 ���������
								co2_status=0; // �������� ����
							}
							if(usart_data_in[9]==0x31) // --- �����
							{
								rele_set[7]=1; // ����� ��������
								water_lvl_status=0x32; // ���� - ����� � ������ ������
							}
							else 
							{
								rele_set[7]=0; // ����� ���������
								water_lvl_status=0; // �������� ����
							}
							usart_data_in[22]=0; // ����� ������ ���������
						}
							
						// ������ ���������� ��������
						if(usart_data_in[10]==0x31)
						{
							cooler_start_pulse[0]++; // ������ ������� ���������� ��������
							// ~3.33 � - ������������� �������� ��������
							if(cooler_start_pulse[0]==10) 
							{
								// ��������� �������� �������� �������
								cooler_data[0]=(usart_data_in[11]*100+usart_data_in[12]*10+usart_data_in[13])*2.55;
								cooler_start_pulse[0]=9; // ���������� ��������� �������
								// ���� �������� ������� ������ �����������
								if(cooler_data[0]<cooler_min) cooler_data[0]=cooler_min;
								// ������������ ������� �������� �������� ��� ������ ��������
								cooler_data[2]=cooler_data[0];
							}
							// �������� ������� �� ��������
							else cooler_data[0]=255;
							OCR0=cooler_data[0]; // ������ PWM
							cooler_chanel_status=0x32; // ���� - ������ � ������ ������
						}
						else 
						{
							// �������� PWM
							OCR0=0; 
							cooler_start_pulse[0]=0;
							cooler_chanel_status=0;
							cooler_hand_start=0;
						}
						
						// ���������� ��������� ��������� ��� ��������� ��� � ������� (�������� ������������� ������)
						lcd_led_stop=(((unsigned long)usart_data_in[14]-0x30)*10+((unsigned long)usart_data_in[15]-0x30))*10000;
						lcd_led_start=(((unsigned long)usart_data_in[16]-0x30)*10+((unsigned long)usart_data_in[17]-0x30))*10000;
						cooler_stop=(((unsigned long)usart_data_in[18]-0x30)*10+((unsigned long)usart_data_in[19]-0x30))*10000;
						cooler_start=(((unsigned long)usart_data_in[20]-0x30)*10+((unsigned long)usart_data_in[21]-0x30))*10000;	
					}break;
					
					case 0x02: // ������������� ����� DS1307N � ��
					{
						// ��������� ������� � ��
						buffer_time[0]=(usart_data_in[2]-0x30)*10+(usart_data_in[3]-0x30); // ����
						buffer_time[1]=(usart_data_in[4]-0x30)*10+(usart_data_in[5]-0x30); // ������
						buffer_time[2]=(usart_data_in[6]-0x30)*10+(usart_data_in[7]-0x30); // �������
						
						DS1307_set_time(); // ������ � ����
						if(usart_data_in[8]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[8]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x13: // ��������� ��������� � EEPROM
					{
						eeprom_count=0;
						EEPROM_WRITE=1;
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					// ----- ������ ������� <������/�������> AquaControl ----- //
					
					case 0x03: // ���������� ������������� �����
					{
						temp_control=1; // ���� - ���������� ������������� �����
						// ��������� ������ ������ 
						set_temp_sensor=usart_data_in[2]-0x30; // ����� ������� 
						set_temp=((usart_data_in[3]-0x30)*10+(usart_data_in[4]-0x30))*10; // �������� �����������
						set_temp_dec=usart_data_in[5]-0x30; // ����� ������������ 
						cooler_set=((usart_data_in[6]-0x30)*100+(usart_data_in[7]-0x30)*10+(usart_data_in[8]-0x30))*2.55; // �������� �������
						if(usart_data_in[9]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[9]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x04: // ��������� ������������� ����� 
					{
						temp_control=2; // ���� - ��������� ������������� �����
						// �������� ����������
						cooler=0; // ������
						heater=0; // �����������
						cooler_start_pulse[1]=0; // ��������� �������
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x06: // ���������� ����� �������� ������
					{
						produv=1; // ���� - ��������� ������
						// ��������� ������� ��������
						timer_produv_time=(((unsigned long)usart_data_in[2]-0x30)*10+((unsigned long)usart_data_in[3]-0x30))*100
										+((unsigned long)usart_data_in[4]-0x30)*10+((unsigned long)usart_data_in[5]-0x30);
						// ��������� ������� ��������� ��������
						timer_produv_period=usart_data_in[6]-0x30;
						// ��������� ������������ �������� �������� �������
						cooler_min=((usart_data_in[7]-0x30)*100+(usart_data_in[8]-0x30)*10+(usart_data_in[9]-0x30))*2.55;
						if(usart_data_in[10]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[10]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x07: // ��������� ����� �������� ������
					{
						produv=2; // ���� - ��������� �������� ������
						cooler_start_pulse[3]=0; // �������� ��������� �������
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x11: // ���������� ����� ��2/��
					{
						co2=1; // ���� - ���������� ����� CO2/PH
						// �������� ������ ������
						// ����� ��������� ��2
						ph_high=(usart_data_in[2]-0x30)*100+(usart_data_in[3]-0x30)*10+(usart_data_in[4]-0x30);
						// ����� ���������� ��2
						ph_low=(usart_data_in[5]-0x30)*100+(usart_data_in[6]-0x30)*10+(usart_data_in[7]-0x30);
						// ��������� ����������� ��2
						co2_stop=(((unsigned long)usart_data_in[8]-0x30)*10+((unsigned long)usart_data_in[9]-0x30))*10000;
						co2_start=(((unsigned long)usart_data_in[10]-0x30)*10+((unsigned long)usart_data_in[11]-0x30))*10000;
						if(usart_data_in[12]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[12]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x12: // ��������� ����� ��2/��
					{
						co2=0; // ���� - ��������� ����� ��2/��
						// ���� ����� �� ����� ��������� - ��������� �������������
						if(co2_timer_status==0) RELE_PORT_2&=~RELE_7;
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x15: // ���������� ����� ��������������� ������ ����
					{
						doliv=1; // ���� - ���������� ���������
						water_lvl_last=water_lvl; // �������� ���������� �������� ������ ����
						WATER_ERROR_POMPA=0;
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x16: // ��������� ����� ��������������� ������ ����
					{
						pompa=0;
						doliv=2; // ���� - ��������� ���������
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x17: // ���������� ����� 1 ������� ����������
					{
						doliv_porog1=water_lvl; // ������� ��������
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x18: // ���������� ����� 2 ������� ����������
					{
						doliv_porog2=water_lvl; // ������� ��������
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x19: // �������� ������� ������ ����������� �������
					{
						events_data_to_uart(); // ��������� ��������
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					// ----- ������ ������� <���������� �������> AquaControl ----- //
					
					case 0x05: // ���������� �������� ��������
					{
						for(int i=0;i<9;i++) // ��� 9 ������� (������� PWM-�����)
						{
							// �������� ������� ������������� � �������� ��������
							timer1_start[i]=(((unsigned long)usart_data_in[2+36*i]-0x30)*10+((unsigned long)usart_data_in[3+36*i]-0x30))*10000
										+(((unsigned long)usart_data_in[4+36*i]-0x30)*10+((unsigned long)usart_data_in[5+36*i]-0x30))*100
										+(((unsigned long)usart_data_in[6+36*i]-0x30)*10+((unsigned long)usart_data_in[7+36*i]-0x30));
							timer1_stop[i]=(((unsigned long)usart_data_in[8+36*i]-0x30)*10+((unsigned long)usart_data_in[9+36*i]-0x30))*10000
										+(((unsigned long)usart_data_in[10+36*i]-0x30)*10+((unsigned long)usart_data_in[11+36*i]-0x30))*100
										+(((unsigned long)usart_data_in[12+36*i]-0x30)*10+((unsigned long)usart_data_in[13+36*i]-0x30));
							timer2_start[i]=(((unsigned long)usart_data_in[14+36*i]-0x30)*10+((unsigned long)usart_data_in[15+36*i]-0x30))*10000
										+(((unsigned long)usart_data_in[16+36*i]-0x30)*10+((unsigned long)usart_data_in[17+36*i]-0x30))*100
										+(((unsigned long)usart_data_in[18+36*i]-0x30)*10+((unsigned long)usart_data_in[19+36*i]-0x30));
							timer2_stop[i]=(((unsigned long)usart_data_in[20+36*i]-0x30)*10+((unsigned long)usart_data_in[21+36*i]-0x30))*10000
										+(((unsigned long)usart_data_in[22+36*i]-0x30)*10+((unsigned long)usart_data_in[23+36*i]-0x30))*100
										+(((unsigned long)usart_data_in[24+36*i]-0x30)*10+((unsigned long)usart_data_in[25+36*i]-0x30));
							timer3_start[i]=(((unsigned long)usart_data_in[26+36*i]-0x30)*10+((unsigned long)usart_data_in[27+36*i]-0x30))*10000
										+(((unsigned long)usart_data_in[28+36*i]-0x30)*10+((unsigned long)usart_data_in[29+36*i]-0x30))*100
										+(((unsigned long)usart_data_in[30+36*i]-0x30)*10+((unsigned long)usart_data_in[31+36*i]-0x30));
							timer3_stop[i]=(((unsigned long)usart_data_in[32+36*i]-0x30)*10+((unsigned long)usart_data_in[33+36*i]-0x30))*10000
										+(((unsigned long)usart_data_in[34+36*i]-0x30)*10+((unsigned long)usart_data_in[35+36*i]-0x30))*100
										+(((unsigned long)usart_data_in[36+36*i]-0x30)*10+((unsigned long)usart_data_in[37+36*i]-0x30));
						}
						if(usart_data_in[326]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[326]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x14: // ��������� ������ �������� �� ��
					{
						timers_data_to_uart();
						if(usart_data_in[2]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[2]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					// ----- ������ ������� <����������� ����������> AquaControl ----- //
					
					case 0x08: // ������� �������� 1-�� ������������ ��������
					{
						adc_ph_1=ph; // �������� �������� ADC 1-�� ��������
						// �������� ��������������� �������� PH �������������� ���������
						adc_ph_1_calibr=((unsigned long)usart_data_in[2]-0x30)*1000+((unsigned long)usart_data_in[3]-0x30)*100
										+((unsigned long)usart_data_in[4]-0x30)*10+((unsigned long)usart_data_in[5]-0x30);
						adc_ph_2_calibr=((unsigned long)usart_data_in[6]-0x30)*1000+((unsigned long)usart_data_in[7]-0x30)*100
										+((unsigned long)usart_data_in[8]-0x30)*10+((unsigned long)usart_data_in[9]-0x30);
						if(usart_data_in[13]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[13]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x09: // ������� �������� 2-�� ������������ ��������
					{
						adc_ph_2=ph; // �������� �������� ADC 2-�� ��������
						// ���������� ������������ PH (��������� ������, ���������� ����� 2 �����)
						// ��� �������� �������� ������������ ���������� �������� � ������ ���� (7010 = 7.01 * 1000 PH)
						// � �����������, ��� ������� ������ � ����������� ������������ �������� ���������� ��������
						
						// ���������������� �����������:
						k1=((long)adc_ph_1_calibr-(long)adc_ph_2_calibr)*1000/((long)adc_ph_1-(long)adc_ph_2);
						// ����������� ��������:
						k2=(long)adc_ph_1_calibr*1000-k1*adc_ph_1;
						if(usart_data_in[13]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[13]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					
					case 0x10: // ������� �������� ������������ �������� redox
					{
						adc_redox=redox; // �������� �������� ADC ��������
						// �������� ��������������� ������� ������������� ��������
						adc_redox_calibr=(usart_data_in[10]-0x30)*100+(usart_data_in[11]-0x30)*10+(usart_data_in[12]-0x30);
						// ���������� �����������
						k=(unsigned long)adc_redox_calibr*1000/adc_redox;
						if(usart_data_in[13]==0xb3) 
						{
							// ����� �� ���������
							usart_data_in[13]=0x00; 
							usart_data_in[1]=0x00; 
						}
					}break;
					default:; break;
				}
			}
			
			// �������������� �������� ������� � �������� �������� (����, ������, �������)
			time_now=(((unsigned long)string_1[0]-0x30)*10+((unsigned long)string_1[1]-0x30))*10000
					+(((unsigned long)string_1[3]-0x30)*10+((unsigned long)string_1[4]-0x30))*100
					+(((unsigned long)string_1[6]-0x30)*10+((unsigned long)string_1[7]-0x30));	
			// �������������� �������� ������� � �������� �������� (������, �������) - ��� ������ �������� �������
			time_min_and_sec=(((unsigned long)string_1[3]-0x30)*10+((unsigned long)string_1[4]-0x30))*100
							 +(((unsigned long)string_1[6]-0x30)*10+((unsigned long)string_1[7]-0x30));
				
			// ----- ���������� �������� RELE ----- //
			LED_PORT&=~LED; // ��������� ���������	
			time_one_sec++; // ������ ���������� ���������
			if(time_one_sec==3) // 1 c - ���������� ��������
			{
				// ���� ������� ����� ��������� � ���������� �������� ���������� ��������
				// ��� ����� ������ ����� ������ ������
				// ������������ �����
				// ����� - ��������� �����
				
				if (((time_now>=timer1_start[0])&&(time_now<timer1_stop[0])&&(timer1_start[0]!=timer1_stop[0]))||
					((time_now>=timer2_start[0])&&(time_now<timer2_stop[0])&&(timer2_start[0]!=timer2_stop[0]))||
					((time_now>=timer3_start[0])&&(time_now<timer3_stop[0])&&(timer3_start[0]!=timer3_stop[0]))||
					(rele_set[0]==1)) RELE_PORT_1|=RELE_1;
				else {RELE_PORT_1&=~RELE_1; rele_set[0]=0;}
				
				if (((time_now>=timer1_start[1])&&(time_now<timer1_stop[1])&&(timer1_start[1]!=timer1_stop[1]))||
					((time_now>=timer2_start[1])&&(time_now<timer2_stop[1])&&(timer2_start[1]!=timer2_stop[1]))||
					((time_now>=timer3_start[1])&&(time_now<timer3_stop[1])&&(timer3_start[1]!=timer3_stop[1]))||
					(rele_set[1]==1)) RELE_PORT_1|=RELE_2;
				else {RELE_PORT_1&=~RELE_2; rele_set[1]=0;}
				
				if (((time_now>=timer1_start[2])&&(time_now<timer1_stop[2])&&(timer1_start[2]!=timer1_stop[2]))||
					((time_now>=timer2_start[2])&&(time_now<timer2_stop[2])&&(timer2_start[2]!=timer2_stop[2]))||
					((time_now>=timer3_start[2])&&(time_now<timer3_stop[2])&&(timer3_start[2]!=timer3_stop[2]))||
					(rele_set[2]==1)) RELE_PORT_1|=RELE_3;
				else {RELE_PORT_1&=~RELE_3; rele_set[2]=0;}
				
				if (((time_now>=timer1_start[3])&&(time_now<timer1_stop[3])&&(timer1_start[3]!=timer1_stop[3]))||
					((time_now>=timer2_start[3])&&(time_now<timer2_stop[3])&&(timer2_start[3]!=timer2_stop[3]))||
					((time_now>=timer3_start[3])&&(time_now<timer3_stop[3])&&(timer3_start[3]!=timer3_stop[3]))||
					(rele_set[3]==1)) RELE_PORT_1|=RELE_4;
				else {RELE_PORT_1&=~RELE_4; rele_set[3]=0;}
				
				if (((time_now>=timer1_start[4])&&(time_now<timer1_stop[4])&&(timer1_start[4]!=timer1_stop[4]))||
					((time_now>=timer2_start[4])&&(time_now<timer2_stop[4])&&(timer2_start[4]!=timer2_stop[4]))||
					((time_now>=timer3_start[4])&&(time_now<timer3_stop[4])&&(timer3_start[4]!=timer3_stop[4]))||
					(rele_set[4]==1)) RELE_PORT_2|=RELE_5;
				else {RELE_PORT_2&=~RELE_5; rele_set[4]=0;}
				
				if (((time_now>=timer1_start[5])&&(time_now<timer1_stop[5])&&(timer1_start[5]!=timer1_stop[5]))||
					((time_now>=timer2_start[5])&&(time_now<timer2_stop[5])&&(timer2_start[5]!=timer2_stop[5]))||
					((time_now>=timer3_start[5])&&(time_now<timer3_stop[5])&&(timer3_start[5]!=timer3_stop[5]))||
					(rele_set[5]==1)) 
					{
						if(!temp_control) RELE_PORT_2|=RELE_6; 
						heater_timer_status=1; // ���� - ����� ��������
						// ���� �������� ������������� � ������ ������
						if((heater_chanel_status!=0x32)&&(heater_chanel_status!=0x33)) heater_chanel_status=0x31; // ���� - ����� ��������
					} 
				else {RELE_PORT_2&=~RELE_6; rele_set[5]=0; heater_timer_status=0;}
				
				if (((time_now>=timer1_start[6])&&(time_now<timer1_stop[6])&&(timer1_start[6]!=timer1_stop[6]))||
					((time_now>=timer2_start[6])&&(time_now<timer2_stop[6])&&(timer2_start[6]!=timer2_stop[6]))||
					((time_now>=timer3_start[6])&&(time_now<timer3_stop[6])&&(timer3_start[6]!=timer3_stop[6]))||
					(rele_set[6]==1)) 
					{
						if(!co2) RELE_PORT_2|=RELE_7; 
						co2_timer_status=1; // ���� - ����� ��������
						// ���� ��������� ������ PH/CO2 � ������ �����
						if((co2_status!=0x32)&&(co2_status!=0x33)) co2_status=0x31; // ���� - ����� ��������
					} 
				else {RELE_PORT_2&=~RELE_7; rele_set[6]=0; co2_timer_status=0;}
				
				if (((time_now>=timer1_start[7])&&(time_now<timer1_stop[7])&&(timer1_start[7]!=timer1_stop[7]))||
					((time_now>=timer2_start[7])&&(time_now<timer2_stop[7])&&(timer2_start[7]!=timer2_stop[7]))||
					((time_now>=timer3_start[7])&&(time_now<timer3_stop[7])&&(timer3_start[7]!=timer3_stop[7]))||
					(rele_set[7]==1)) 
					{
						if(!doliv) RELE_PORT_2|=RELE_8; 
						doliv_timer_status=1; // ���� - ����� ��������
						// ���� ��������� ������ ��������� � ������ �����
						if((water_lvl_status!=0x32)&&(water_lvl_status!=0x33)) water_lvl_status=0x31; // ���� - ����� ��������
					}
				else {RELE_PORT_2&=~RELE_8; rele_set[7]=0; doliv_timer_status=0;}
				
				time_one_sec=0; // �������� ������
                LED_PORT|=LED; // �������� ���������	
			}
			
			// ----- ���������� ������� PWM ----- //		
			// ���� ������� ����� ��������� � ���������� ��������� ���������� ��������� ������ �������
			// ���� ������� ����� ��������� � ���������� �������� ���������� ��������
			// ���� ����� ������� ��� ����� ������ �����
			// ������������ �����
			// ����� - ��������� �����
			if((time_now>cooler_start)&&(time_now<cooler_stop))
			{
				if (((time_now>=timer1_start[8])&&(time_now<timer1_stop[8])&&(timer1_start[8]!=timer1_stop[8]))||
					((time_now>=timer2_start[8])&&(time_now<timer2_stop[8])&&(timer2_start[8]!=timer2_stop[8]))||
					((time_now>=timer3_start[8])&&(time_now<timer3_stop[8])&&(timer3_start[8]!=timer3_stop[8]))||
					(cooler_hand_start))
					{
						cooler_start_pulse[2]++; // ������ ������� ���������� ��������
						// ~3.33 � - ������������� �������� ��������
						if(cooler_start_pulse[2]==10) 
						{
							// ������������ �������� �������� ������� ������ ��� ������ ��������
							cooler_data[2]=cooler_data[0];
							cooler_start_pulse[2]=9; // ���������� ��������� �������
							// ���� �������� ������� ������ �����������
							if(cooler_data[2]<cooler_min) cooler_data[2]=cooler_min;
						}
						// �������� ������� �� ��������
						else cooler_data[2]=255;
						OCR0=cooler_data[2];
						// ���� ��������� ������ �������� ������ � ������ �����
						if((cooler_chanel_status!=0x32)&&(cooler_chanel_status!=0x34)) cooler_chanel_status=0x31; // ���� - ����� ��������
					}	
				else 
				{	
					// �������� PWM
					cooler_start_pulse[2]=0;
					// ��������� �������� PWM ������������ ���������� �������
					if(cooler_start_pulse[0]) OCR0=cooler_data[0]; // 1-������ ����� ����������
					if(cooler_start_pulse[3]) OCR0=cooler_data[3]; // 2-����� �������� ������
					if(cooler_start_pulse[1]) OCR0=cooler_data[1]; // 3-������������� �����
					// ���� �� ���� �� ������� �� ������� - ��������� PWM
					if((cooler_start_pulse[0]==0)&(cooler_start_pulse[1]==0)&(cooler_start_pulse[2]==0)&(cooler_start_pulse[3]==0)) OCR0=0;
				}
			}
			
			// ----- ����� ��2/�� ----- //
			// ���� ������� ����� ��������� � ���������� ��������� ���������� ��������� ������
			if((time_now>co2_start)&&(time_now<co2_stop))
			{
				if(co2) // ����� �����
				{
					if(data_ph>=ph_high) co2_set=1; // �������� ���������
					if(data_ph<=ph_low) co2_set=0; // �������� ����������
					if(co2_set) 
					{
						co2_status=0x33;
						RELE_PORT_2|=RELE_7; // �������� �������������
					}
					else 
					{
						RELE_PORT_2&=~RELE_7; // ��������� �������������
						if(co2_timer_status) co2_status=0x31;
						if(rele_set[6]) co2_status=0x32;
						// ���� �� ����� ����� ������� � ������ ����� - ��������� �����
						if((co2_timer_status==0)&(rele_set[6]==0)) co2_status=0x30;
					}
				}
				else 
				{
					if(rele_set[6]==1) co2_status=0x32;
					// ���� �� ����� ����� ������� � ������ ����� - ��������� �����
					if((co2_timer_status==0)&(rele_set[6]==0)) {RELE_PORT_2&=~RELE_7; co2_status=0x30;}
				}
			}
			
			// ----- ��������� �������� ������ ��������� ��� ----- //
			// ���� ������� ����� ��������� � ���������� ��������� ���������� ��������� ������
			if((time_now>lcd_led_start)&&(time_now<lcd_led_stop)) LCD_LED_PORT&=~LCD_LED; // �������� ���������
			else LCD_LED_PORT|=LCD_LED; // ��������� ��������� 
			
			// ----- ��������� �������������� ������ � ������ �������� ������ ----- //
			set_temp_mode_and_produv();
			
			// ----- ��������� ������ ��������������� ������ ���� ----- //
			water_doliv();
			
			// ----- ������ ���������� � EEPROM ----- //
			if(EEPROM_WRITE) save_option_eeprom();
			
			// ----- ��������� �������, ������� ----- //
			rele_ports_status=(PIND>>4)+((PINC&0x01)<<4); // ��������� ����� RELE
			rele_ports_status_lcd=(PIND>>4)+(PINC<<4); // ����������� ���������� ������� �� ���
			rele_hand_status=rele_set[0]|(rele_set[1]<<1)|(rele_set[2]<<2)|(rele_set[3]<<3)|(rele_set[4]<<4);
			// ��������� PWM
			if((cooler_start_pulse[0]==0)&(cooler_start_pulse[1]==0)&(cooler_start_pulse[2]==0)&(cooler_start_pulse[3]==0)) cooler_chanel_status=0x30;
			// ��������� ��������������
			if((rele_set[6]==0)&(co2_timer_status==0)&(co2==0)) co2_status=0x30;
			// ��������� �����
			if((rele_set[7]==0)&(doliv_timer_status==0)&(pompa==0)&(WATER_ERROR==0)) water_lvl_status=0x30;
			// ��������� �����������
			if((rele_set[5]==0)&(heater_timer_status==0)&(heater==0)) heater_chanel_status=0x30;
			// ��������� �������
			if(temp_control==1) mode_status|=_BV(0);
			else mode_status&=~_BV(0);
			if(produv==1) mode_status|=_BV(1);
			else mode_status&=~_BV(1);
			if(co2==1) mode_status|=_BV(2);
			else mode_status&=~_BV(2);
			if(doliv==1) mode_status|=_BV(3);
			else mode_status&=~_BV(3);
			
			// ���� ���� �� ������� � ������ ������
			hand_mode_set=0;
			for(unsigned int i=0;i<8;i++)
			{
				if((rele_set[i]!=0)|(cooler_start_pulse[0]!=0)) hand_mode_set=1;
			}
			if(WATER_ERROR) // ���� ������ ������ ���� �� �������� - ���������� ����������
			{
				string_1[11]='!';
				string_1[12]='E';
				string_1[13]='R';
				string_1[14]='R';
				string_1[15]='O';
				string_1[16]='R';
				string_1[17]='!';
				string_1[18]=0x20;
			}
			else
			{
				if(hand_mode_set) // ���� ������ ������ ����� ������ �� ������� - ���������� ����������
				{
					string_1[11]='!';
					string_1[12]='!';
					string_1[13]='H';
					string_1[14]='A';
					string_1[15]='N';
					string_1[16]='D';
					string_1[17]='!';
					string_1[18]='!';
				}
				else // ����� - ��������� ��������� ����������� ������� - ����� ��������
				{
					string_1[11]='T';
					string_1[12]='I';
					string_1[13]='M';
					string_1[14]='E';
					string_1[15]='R';
					string_1[16]='S';
					string_1[17]=0x20;
					string_1[18]=0x20;
				}
			}
			
			// ����������� �������� ������� � ������� ������
			if((rele_ports_status_lcd&0x01)==0x01) string_4[11]=0x31;
			else string_4[11]='-';
			if((rele_ports_status_lcd&0x02)==0x02) string_4[12]=0x32;
			else string_4[12]='-';
			if((rele_ports_status_lcd&0x04)==0x04) string_4[13]=0x33;
			else string_4[13]='-';
			if((rele_ports_status_lcd&0x08)==0x08) string_4[14]=0x34;
			else string_4[14]='-';
			if((rele_ports_status_lcd&0x10)==0x10) string_4[15]=0x35;
			else string_4[15]='-';
			if((rele_ports_status_lcd&0x20)==0x20) string_4[16]=0x36;
			else string_4[16]='-';
			if((rele_ports_status_lcd&0x40)==0x40) string_4[17]=0x37;
			else string_4[17]='-';
			if((rele_ports_status_lcd&0x80)==0x80) string_4[18]=0x38;
			else string_4[18]='-';
			if(OCR0>0) string_4[19]='C';
			else string_4[19]='-';
			
			FlagsFastCount &= ~_BV(10); // �������� ����
		};
		
		if (FlagsFastCount & _BV(12)) // ~ 1,34 s
		{
			// ������������ ������� ��� ADM690A
			WDI_PORT|=WDI;
			_delay_us(100);
			WDI_PORT&=~WDI;
			
			FlagsFastCount &= ~_BV(12); // �������� ����
		};
		
		if (FlagsFastCount & _BV(13)) // ~ 2,67 s
		{
			if(TRANSMIT==0x00) // �������� ���������
			{
				TRANSMIT=0x01; // ������ ��������
				usart_data_in_num=67; // ���-�� ������������ ����
				cooler_hand_power=cooler_data[0]/2.55; // �������������� ��������
				// START
				buffer[0]=0x3B; 
				buffer[1]=0xa1; // ������� - ������ ���������
				// time
				buffer[2]=string_1[0];
				buffer[3]=string_1[1];
				buffer[4]=string_1[3];
				buffer[5]=string_1[4];
				buffer[6]=string_1[6];
				buffer[7]=string_1[7];
				// T1
				buffer[8]=string_2[5];
				buffer[9]=string_2[6];
				buffer[10]=string_2[8];
				// T2
				buffer[11]=string_3[5];
				buffer[12]=string_3[6];
				buffer[13]=string_3[8];
				// T3
				buffer[14]=string_4[5];
				buffer[15]=string_4[6];
				buffer[16]=string_4[8];
				// PH
				buffer[17]=string_2[15];
				buffer[18]=string_2[16];
				buffer[19]=string_2[18];
				buffer[20]=string_2[19];
				// Redox
				buffer[21]=string_3[15];
				buffer[22]=string_3[17];
				buffer[23]=string_3[18];
				buffer[24]=string_3[19];
				// status
				buffer[25]=rele_ports_status;
				buffer[26]=rele_hand_status;
				buffer[27]=cooler_chanel_status;
				// k1
				buffer[28]=k1/1000+0x30;
				buffer[29]=(k1%1000)/100+0x30;
				buffer[30]=((k1%1000)%100)/10+0x30;
				buffer[31]=((k1%1000)%100)%10+0x30;
				// k2
				buffer[32]=(k2/1000)/100+0x30;
				buffer[33]=((k2/1000)%100)/10+0x30;
				buffer[34]=((k2/1000)%100)%10+0x30;
				// k
				buffer[35]=k/1000+0x30;
				buffer[36]=(k%1000)/100+0x30;
				buffer[37]=((k%1000)%100)/10+0x30;
				buffer[38]=((k%1000)%100)%10+0x30;
				// ��������� �������
				buffer[39]=mode_status;
				// doliv_porog1
				buffer[40]=doliv_porog1/1000+0x30;
				buffer[41]=(doliv_porog1%1000)/100+0x30;
				buffer[42]=((doliv_porog1%1000)%100)/10+0x30;
				buffer[43]=((doliv_porog1%1000)%100)%10+0x30;
				// doliv_porog2
				buffer[44]=doliv_porog2/1000+0x30;
				buffer[45]=(doliv_porog2%1000)/100+0x30;
				buffer[46]=((doliv_porog2%1000)%100)/10+0x30;
				buffer[47]=((doliv_porog2%1000)%100)%10+0x30;
				// water_lvl
				buffer[48]=water_lvl/1000+0x30;
				buffer[49]=(water_lvl%1000)/100+0x30;
				buffer[50]=((water_lvl%1000)%100)/10+0x30;
				buffer[51]=((water_lvl%1000)%100)%10+0x30;
				buffer[52]=water_lvl_status;
				// LED time
				buffer[53]=lcd_led_stop/100000+0x30;
				buffer[54]=(lcd_led_stop%100000)/10000+0x30;
				buffer[55]=lcd_led_start/100000+0x30;
				buffer[56]=(lcd_led_start%100000)/10000+0x30;
				// COOLER time
				buffer[57]=cooler_stop/100000+0x30;
				buffer[58]=(cooler_stop%100000)/10000+0x30;
				buffer[59]=cooler_start/100000+0x30;
				buffer[60]=(cooler_start%100000)/10000+0x30;
				// COOLER hand power
				buffer[61]=cooler_hand_power/100+0x30;
				buffer[62]=(cooler_hand_power%100)/10+0x30;
				buffer[63]=(cooler_hand_power%100)%10+0x30;
				// CO2 status
				buffer[64]=co2_status;
				// temp. mode status
				buffer[65]=heater_chanel_status;
				// END
				buffer[66]=0xb3;
				
				buffer_start(); // �������� ������ ������
			}
			FlagsFastCount &= ~_BV(13); // �������� ����
		};
		
		if (FlagsFastCount & _BV(15)) // ~ 10.6 s
		{
			if(pompa) // ����� ��������
			{
				pompa_time_count++; // ������ �������
				if(pompa_time_count==2) // ~ 21.2 s
				{
					pompa_time_count=0; // ��������
					if(water_lvl_pompa==water_lvl) WATER_ERROR_POMPA=1; // ���� ���������� ��������� ������� � ������� ����� - ������
				}
			}
			water_lvl_pompa=water_lvl; // ������ ����������� ��������� �������
			FlagsFastCount &= ~_BV(15); // �������� ����
		};
	};
	return(0);
}

// ������� �������������� ������ � ������ �������� ������
void set_temp_mode_and_produv(void)
{
	// ������������� �����
	switch(temp_control)
	{
		case 1: // ���������� �����
		{
			switch(set_temp_sensor) // ����� �������������� �������
			{
				case 1: temp_now=(string_2[5]-0x30)*100+(string_2[6]-0x30)*10+(string_2[8]-0x30); break;
				case 2: temp_now=(string_3[5]-0x30)*100+(string_3[6]-0x30)*10+(string_3[8]-0x30); break;
				case 3: temp_now=(string_4[5]-0x30)*100+(string_4[6]-0x30)*10+(string_4[8]-0x30); break;
				default:; break;
			}
			if(temp_now!=0)
			{
				// ���� ������� ����������� ������ ������������� �� �������� ������ ������������
				if((set_temp-temp_now)>set_temp_dec) heater=1; // ���� - �������� ������
				if(heater) // ���� ������ �����
				{
					RELE_PORT_2|=RELE_6; // �������� �����������
					OCR0=0; // ��������� ������
					if(temp_now>=set_temp) heater=0; // ���� ������� ����������� ��������� �������� - �������� ���� 
					heater_chanel_status=0x33; // ��������� - ������������� �����
				}
				else RELE_PORT_2&=~RELE_6; // ��������� �����������
				// ���� ������� ����������� ���� ������������� �� �������� ������ ������������
				if((temp_now-set_temp)>set_temp_dec) cooler=1; // ���� - �������� ������
				// ���� ������ ������ ������� 
				// � ���� ������� ����� ��������� � ���������� ��������� ���������� ��������� ������ �������
				if(cooler&&((time_now>cooler_start)&&(time_now<cooler_stop)))
				{
					cooler_start_pulse[1]++; // ������ ������� ���������� ��������
					// ~3.33 � - ������������� �������� ��������
					if(cooler_start_pulse[1]==10) 
					{
						cooler_start_pulse[1]=9; // ���������� ��������� �������
						cooler_data[1]=cooler_set; // ������������� �������� �������� �������
						// ���� �������� ������� ������ �����������
						if(cooler_data[1]<cooler_min) cooler_data[1]=cooler_min;
					}
					// �������� ������� �� ��������
					else cooler_data[1]=255;
					OCR0=cooler_data[1];
					// ���� ������� ����������� ���� ��������
					if(temp_now<=set_temp) {cooler=0; cooler_start_pulse[1]=0;} // �������� ����, �������� �������� ���������� ��������
					cooler_chanel_status=0x33; // ��������� ������� - ������������� �����
				}
				// ��������� ������
				else {OCR0=0;cooler_start_pulse[1]=0;}		
			}
		} break;
		
		case 2: // �������� �����
		{
			if(rele_set[5]==1) heater_chanel_status=0x32; 
			// ���� ������ ����� � ����� �������� �� �������
			if((rele_set[5]==0)&&(heater_timer_status==0)) RELE_PORT_2&=~RELE_6; // ��������� �����������	
			// ��������� �������� PWM ������������ ���������� �������
			if(cooler_start_pulse[0]) cooler_start_pulse[0]=0; /*OCR0=cooler_data[0];*/ // 1-������ ����� ����������
			if(cooler_start_pulse[3]) OCR0=cooler_data[3]; // 2-����� �������� ������
			if(cooler_start_pulse[2]) cooler_start_pulse[2]=0; // 3-����� ��������
			if(cooler_start_pulse[0]!=0) cooler_chanel_status=0x32;
			// ���� �� ���� �� ������� �� ������� - ��������� PWM
			if((cooler_start_pulse[0]==0)&(cooler_start_pulse[1]==0)&(cooler_start_pulse[2]==0)&(cooler_start_pulse[3]==0)) OCR0=0;
			temp_control=0;
		}break;
		default:; break;
	}
	
	// ����� �������� ������
	// ��������� ������� �� �������� �������� ������� ���������� � ������ ����
	switch(produv)
	{
		case 1: // ���������� �����
		{
			// ���� ������� ����� ��������� � ���������� ��������� ���������� ��������� ������ �������
			if((time_now>=cooler_start)&&(time_now<cooler_stop))
			{
				if(cooler==0) // ���� ������ �� �������
				{
					// ������ ���������� ���� - ������ �������
					if(time_min_and_sec==0) timer_produv_period_count++;
					// ������������� ����� �������� ������ ���������� � �������� ��������� �������
					if((time_min_and_sec>=0)&&(time_min_and_sec<timer_produv_time))
					{
						// ������������� ������ ������� ��������� �������� ��������� � ��������
						if(timer_produv_period_count==timer_produv_period*3) // *3 - ������� � �������� ������ (~0.333 �)
						{
							cooler_start_pulse[3]++; // ������ ������� ���������� ��������
							// ~3.33 � - ������������� �������� ��������
							if(cooler_start_pulse[3]==10) 
							{
								cooler_start_pulse[3]=9; // ���������� ��������� �������
								// ���� �������� ������� ������ �����������
								cooler_data[3]=cooler_min;
							}
							// �������� ������� �� ��������
							else cooler_data[3]=255;
							OCR0=cooler_data[3];
							cooler_chanel_status=0x34; // ��������� ������� - ����� �������� ������
						}
					}
					else // ��������� ������
					{
						// ���� ����� �������� ����� - �������� ����������
						if(cooler_start_pulse[3]) {cooler_start_pulse[3]=0; timer_produv_period_count=0;}
						// ��������� �������� PWM ������������ ���������� �������
						if(cooler_start_pulse[0]) OCR0=cooler_data[0]; // 1-������ ����� ����������
						if(cooler_start_pulse[2]) OCR0=cooler_data[2]; // 2-����� ��������
						if(cooler_start_pulse[1]) OCR0=cooler_data[1]; // 3-������������� �����
						// ���� �� ���� �� ������� �� ������� - ��������� PWM
						if((cooler_start_pulse[0]==0)&(cooler_start_pulse[1]==0)&(cooler_start_pulse[2]==0)&(cooler_start_pulse[3]==0)) OCR0=0;
					}
				}
			}
			else {OCR0=0;cooler_start_pulse[3]=0;} // ��������� ������, �������� �������� ���������� ��������
		}break;
		
		case 2: // �������� �����
		{
			timer_produv_period_count=0; // �������� �������� ������� ���������
			// ��������� �������� PWM ������������ ���������� �������
			if(cooler_start_pulse[0]) OCR0=cooler_data[0]; // 1-������ ����� ����������
			if(cooler_start_pulse[2]) OCR0=cooler_data[2]; // 2-����� ��������
			if(cooler_start_pulse[1]) OCR0=cooler_data[1]; // 3-������������� �����
			// ���� �� ���� �� ������� �� ������� - ��������� PWM
			if((cooler_start_pulse[0]==0)&(cooler_start_pulse[1]==0)&(cooler_start_pulse[2]==0)&(cooler_start_pulse[3]==0)) OCR0=0;
			produv=0; // ���� - ����� �������� ��������
		}break;
		default:; break;
	}	
}

// ������� ������ �������� � EEPROM
// ����� ���������� ������ ������ � EEPROM ������� ��������� �� �����
void save_option_eeprom(void)
{
	unsigned char count_i=0;
	eeprom_count++; // ������ ����������
	switch(eeprom_count)
	{
		case 1:
		{
			// �������� �������� - "������ �����������"
			eeprom_write_word((unsigned int*)1000,0x1d);
			// ���������, ������� ���������� �������, ������������
			eeprom_write_dword((unsigned long*)10,k1);
			eeprom_write_dword((unsigned long*)14,k2);
			eeprom_write_dword((unsigned long*)18,k);
			eeprom_write_byte((unsigned char*)22,rele_set[0]);
			eeprom_write_byte((unsigned char*)23,rele_set[1]);
			eeprom_write_byte((unsigned char*)24,rele_set[2]);
			eeprom_write_byte((unsigned char*)25,rele_set[3]);
			eeprom_write_byte((unsigned char*)26,rele_set[4]);
			eeprom_write_byte((unsigned char*)27,rele_set[5]);
		}break;
		case 2:
		{
			eeprom_write_byte((unsigned char*)28,rele_set[6]);
			eeprom_write_byte((unsigned char*)29,rele_set[7]);
			eeprom_write_byte((unsigned char*)30,cooler_start_pulse[0]);
			eeprom_write_byte((unsigned char*)31,cooler_start_pulse[1]);
			eeprom_write_byte((unsigned char*)32,cooler_start_pulse[2]);
			eeprom_write_byte((unsigned char*)33,cooler_start_pulse[3]);
			eeprom_write_byte((unsigned char*)34,cooler_data[0]);
			eeprom_write_byte((unsigned char*)35,cooler_data[1]);
			eeprom_write_byte((unsigned char*)36,cooler_data[2]);
			eeprom_write_byte((unsigned char*)37,cooler_data[3]);
			eeprom_write_byte((unsigned char*)38,cooler_min);
			eeprom_write_byte((unsigned char*)39,cooler_chanel_status);
			eeprom_write_byte((unsigned char*)40,OCR0);
			eeprom_write_dword((unsigned long*)41,lcd_led_stop);
			eeprom_write_dword((unsigned long*)45,lcd_led_start);
		}break;
		case 3:
		{
			eeprom_write_dword((unsigned long*)49,cooler_stop);
			eeprom_write_dword((unsigned long*)53,cooler_start);
			eeprom_write_byte((unsigned char*)57,temp_control);
			eeprom_write_byte((unsigned char*)58,set_temp_sensor);
			eeprom_write_word((unsigned int*)59,set_temp);
			eeprom_write_byte((unsigned char*)61,set_temp_dec);
			eeprom_write_word((unsigned int*)62,cooler_set);
			eeprom_write_byte((unsigned char*)64,cooler);
			eeprom_write_byte((unsigned char*)65,heater);
		}break;
		case 4:
		{
			count_i=0;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 5:
		{
			count_i=1;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 6:
		{
			count_i=2;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 7:
		{
			count_i=3;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 8:
		{
			count_i=4;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 9:
		{
			count_i=5;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 10:
		{
			count_i=6;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 11:
		{
			count_i=7;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 12:
		{
			count_i=8;
			eeprom_write_dword((unsigned long*)(66+(count_i*4)), (unsigned long)timer1_start[count_i]);
			eeprom_write_dword((unsigned long*)(102+(count_i*4)),(unsigned long)timer2_start[count_i]);
			eeprom_write_dword((unsigned long*)(138+(count_i*4)),(unsigned long)timer3_start[count_i]);
			eeprom_write_dword((unsigned long*)(174+(count_i*4)),(unsigned long)timer1_stop[count_i]);
			eeprom_write_dword((unsigned long*)(210+(count_i*4)),(unsigned long)timer2_stop[count_i]);
			eeprom_write_dword((unsigned long*)(246+(count_i*4)),(unsigned long)timer3_stop[count_i]);
		}break;
		case 13:
		{
			eeprom_write_byte((unsigned char*)282,produv);
			eeprom_write_dword((unsigned long*)283,timer_produv_time);
			eeprom_write_byte((unsigned char*)287,timer_produv_period);
			eeprom_write_byte((unsigned char*)288,co2);
			eeprom_write_word((unsigned int*)289,ph_high);
			eeprom_write_word((unsigned int*)291,ph_low);
			eeprom_write_dword((unsigned long*)293,co2_stop);
			eeprom_write_dword((unsigned long*)297,co2_start);
			eeprom_write_byte((unsigned char*)301,heater_timer_status);
		}
		case 14:
		{
			eeprom_write_byte((unsigned char*)302,co2_timer_status);
			eeprom_write_byte((unsigned char*)303,co2_set);
			eeprom_write_word((unsigned int*)304,water_lvl);
			eeprom_write_word((unsigned int*)306,doliv_porog1);
			eeprom_write_word((unsigned int*)308,doliv_porog2);
			eeprom_write_byte((unsigned char*)310,water_lvl_status);
			eeprom_write_byte((unsigned char*)311,co2_status);
			eeprom_write_byte((unsigned char*)312,doliv);
			eeprom_write_byte((unsigned char*)313,heater_chanel_status);
			EEPROM_WRITE=0;
		}
		default:;break;
	}
}

// ������� ������ �������� �� EEPROM 
void load_option_eeprom(void) // ������� ������ �� EEPROM
{
	k1=eeprom_read_dword((unsigned long*)10);
	k2=eeprom_read_dword((unsigned long*)14);
	k=eeprom_read_dword((unsigned long*)18);
	rele_set[0]=eeprom_read_byte((unsigned char*)22);
	rele_set[1]=eeprom_read_byte((unsigned char*)23);
	rele_set[2]=eeprom_read_byte((unsigned char*)24);
	rele_set[3]=eeprom_read_byte((unsigned char*)25);
	rele_set[4]=eeprom_read_byte((unsigned char*)26);
	rele_set[5]=eeprom_read_byte((unsigned char*)27);
	rele_set[6]=eeprom_read_byte((unsigned char*)28);
	rele_set[7]=eeprom_read_byte((unsigned char*)29); 
	cooler_start_pulse[0]=eeprom_read_byte((unsigned char*)30);
	cooler_start_pulse[1]=eeprom_read_byte((unsigned char*)31);
	cooler_start_pulse[2]=eeprom_read_byte((unsigned char*)32);
	cooler_start_pulse[3]=eeprom_read_byte((unsigned char*)33);
	cooler_data[0]=eeprom_read_byte((unsigned char*)34);
	cooler_data[1]=eeprom_read_byte((unsigned char*)35);
	cooler_data[2]=eeprom_read_byte((unsigned char*)36);
	cooler_data[3]=eeprom_read_byte((unsigned char*)37);
	cooler_min=eeprom_read_byte((unsigned char*)38);
	cooler_chanel_status=eeprom_read_byte((unsigned char*)39);
	OCR0=eeprom_read_byte((unsigned char*)40);
	lcd_led_stop=eeprom_read_dword((unsigned long*)41);
	lcd_led_start=eeprom_read_dword((unsigned long*)45);
	cooler_stop=eeprom_read_dword((unsigned long*)49);
	cooler_start=eeprom_read_dword((unsigned long*)53);
	temp_control=eeprom_read_byte((unsigned char*)57);
	set_temp_sensor=eeprom_read_byte((unsigned char*)58);
	set_temp=eeprom_read_word((unsigned int*)59);
	set_temp_dec=eeprom_read_byte((unsigned char*)61);
	cooler_set=eeprom_read_word((unsigned int*)62);
	cooler=eeprom_read_byte((unsigned char*)64);
	heater=eeprom_read_byte((unsigned char*)65);
	for(unsigned char i=0;i<9;i++)
	{
		timer1_start[i]=eeprom_read_dword((unsigned long*)(66+(i*4)));
		timer2_start[i]=eeprom_read_dword((unsigned long*)(102+(i*4)));
		timer3_start[i]=eeprom_read_dword((unsigned long*)(138+(i*4)));
		timer1_stop[i]=eeprom_read_dword((unsigned long*)(174+(i*4)));
		timer2_stop[i]=eeprom_read_dword((unsigned long*)(210+(i*4)));
		timer3_stop[i]=eeprom_read_dword((unsigned long*)(246+(i*4)));
	}
	produv=eeprom_read_byte((unsigned char*)282);
	timer_produv_time=eeprom_read_dword((unsigned long*)283);
	timer_produv_period=eeprom_read_byte((unsigned char*)287);
	co2=eeprom_read_byte((unsigned char*)288);
	ph_high=eeprom_read_word((unsigned int*)289);
	ph_low=eeprom_read_word((unsigned int*)291);
	co2_stop=eeprom_read_dword((unsigned long*)293);
	co2_start=eeprom_read_dword((unsigned long*)297);
	heater_timer_status=eeprom_read_byte((unsigned char*)301);
	co2_timer_status=eeprom_read_byte((unsigned char*)302);
	co2_set=eeprom_read_byte((unsigned char*)303);
	water_lvl=eeprom_read_word((unsigned int*)304);
	doliv_porog1=eeprom_read_word((unsigned int*)306);
	doliv_porog2=eeprom_read_word((unsigned int*)308);
	water_lvl_status=eeprom_read_byte((unsigned char*)310);
	co2_status=eeprom_read_byte((unsigned char*)311);
	doliv=eeprom_read_byte((unsigned char*)312);
	heater_chanel_status=eeprom_read_byte((unsigned char*)313);
}

// ������� �������� ������ �������� �� ��
void timers_data_to_uart(void)
{
	if(TRANSMIT==0x00) // �������� ���������
	{
		TRANSMIT=0x01; // ������ ��������
		usart_data_in_num=327; // ���-�� ������������ ����
		
		buffer[0]=0x3B; // ������ ������
		buffer[1]=0xa2; // ������� - ������ ��������
		// ������ ��������
		for(unsigned char i=0;i<9;i++)
		{
			buffer[2+36*i]=timer1_start[i]/100000+0x30;
			buffer[3+36*i]=(timer1_start[i]%100000)/10000+0x30;
			buffer[4+36*i]=((timer1_start[i]%100000)%10000)/1000+0x30;
			buffer[5+36*i]=(((timer1_start[i]%100000)%10000)%1000)/100+0x30;
			buffer[6+36*i]=((((timer1_start[i]%100000)%10000)%1000)%100)/10+0x30;
			buffer[7+36*i]=((((timer1_start[i]%100000)%10000)%1000)%100)%10+0x30;
			
			buffer[8+36*i]=timer2_start[i]/100000+0x30;
			buffer[9+36*i]=(timer2_start[i]%100000)/10000+0x30;
			buffer[10+36*i]=((timer2_start[i]%100000)%10000)/1000+0x30;
			buffer[11+36*i]=(((timer2_start[i]%100000)%10000)%1000)/100+0x30;
			buffer[12+36*i]=((((timer2_start[i]%100000)%10000)%1000)%100)/10+0x30;
			buffer[13+36*i]=((((timer2_start[i]%100000)%10000)%1000)%100)%10+0x30;
			
			buffer[14+36*i]=timer3_start[i]/100000+0x30;
			buffer[15+36*i]=(timer3_start[i]%100000)/10000+0x30;
			buffer[16+36*i]=((timer3_start[i]%100000)%10000)/1000+0x30;
			buffer[17+36*i]=(((timer3_start[i]%100000)%10000)%1000)/100+0x30;
			buffer[18+36*i]=((((timer3_start[i]%100000)%10000)%1000)%100)/10+0x30;
			buffer[19+36*i]=((((timer3_start[i]%100000)%10000)%1000)%100)%10+0x30;
			
			buffer[20+36*i]=timer1_stop[i]/100000+0x30;
			buffer[21+36*i]=(timer1_stop[i]%100000)/10000+0x30;
			buffer[22+36*i]=((timer1_stop[i]%100000)%10000)/1000+0x30;
			buffer[23+36*i]=(((timer1_stop[i]%100000)%10000)%1000)/100+0x30;
			buffer[24+36*i]=((((timer1_stop[i]%100000)%10000)%1000)%100)/10+0x30;
			buffer[25+36*i]=((((timer1_stop[i]%100000)%10000)%1000)%100)%10+0x30;
			
			buffer[26+36*i]=timer2_stop[i]/100000+0x30;
			buffer[27+36*i]=(timer2_stop[i]%100000)/10000+0x30;
			buffer[28+36*i]=((timer2_stop[i]%100000)%10000)/1000+0x30;
			buffer[29+36*i]=(((timer2_stop[i]%100000)%10000)%1000)/100+0x30;
			buffer[30+36*i]=((((timer2_stop[i]%100000)%10000)%1000)%100)/10+0x30;
			buffer[31+36*i]=((((timer2_stop[i]%100000)%10000)%1000)%100)%10+0x30;
			
			buffer[32+36*i]=timer3_stop[i]/100000+0x30;
			buffer[33+36*i]=(timer3_stop[i]%100000)/10000+0x30;
			buffer[34+36*i]=((timer3_stop[i]%100000)%10000)/1000+0x30;
			buffer[35+36*i]=(((timer3_stop[i]%100000)%10000)%1000)/100+0x30;
			buffer[36+36*i]=((((timer3_stop[i]%100000)%10000)%1000)%100)/10+0x30;
			buffer[37+36*i]=((((timer3_stop[i]%100000)%10000)%1000)%100)%10+0x30;
		}
		buffer[326]=0xb3; // ����� ������
		
		buffer_start(); // �������� ������
	}
}

// ������� �������� ������ ������� �� ��
void events_data_to_uart(void)
{	
	TRANSMIT=0x01; // ������ ��������
	usart_data_in_num=28; // ���-�� ������������ ����
	
	buffer[0]=0x3B; // ������ ������
	buffer[1]=0xa3; // ������� - ������ �������
	// ������ �������
	// ������������� ������ �������������� ������
	buffer[2]=set_temp_sensor+0x30;
	// ������������� �����������
	buffer[3]=set_temp/100+0x30;
	buffer[4]=(set_temp%100)/10+0x30;
	// ������������� ����� ������������
	buffer[5]=set_temp_dec+0x30;
	// �������� ������� �������������� ������
	buffer[6]=cooler_set/100+0x30;
	buffer[7]=(cooler_set%100)/10+0x30;
	buffer[8]=(cooler_set%100)%10+0x30;
	// ����� ������ ������� � ������ �������� 
	buffer[9]=timer_produv_time/1000+0x30;
	buffer[10]=(timer_produv_time%1000)/100+0x30;
	buffer[11]=((timer_produv_time%1000)%100)/10+0x30;
	buffer[12]=((timer_produv_time%1000)%100)%10+0x30;
	// ������ ��������� ������� � ������ ��������
	buffer[13]=timer_produv_period+0x30;
	// ����������� �������� �������
	buffer[14]=cooler_min/100+0x30;
	buffer[15]=((unsigned int)cooler_min%100)/10+0x30;
	buffer[16]=((unsigned int)cooler_min%100)%10+0x30;
	// �������� �� - ��������� ��2
	buffer[17]=ph_high/100+0x30;
	buffer[18]=(ph_high%100)/10+0x30;
	buffer[19]=(ph_high%100)%10+0x30;
	// �������� �� - ���������� ��2
	buffer[20]=ph_low/100+0x30;
	buffer[21]=(ph_low%100)/10+0x30;
	buffer[22]=(ph_low%100)%10+0x30;
	// ��������� ��������� ������ ��2 - ���������
	buffer[23]=(co2_stop/10000)/10+0x30;
	buffer[24]=(co2_stop/10000)%10+0x30;
	// ��������� ��������� ������ ��2 - ��������
	buffer[25]=(co2_start/10000)/10+0x30;
	buffer[26]=(co2_start/10000)%10+0x30;
	
	buffer[27]=0xb3; // ����� ������
		
	buffer_start(); // �������� ������
}

// ������� ��������������� ������ ����
void water_doliv(void)
{
	switch(doliv) 
	{
		case 1: // ���������� �����
		{
			// ���� ��������� ������ ������ ������ ���� - ����������� 10 �
			// ���� �� ��� ����� ��������� ������� �� ������������� � �������� �������� - ������ �� ��������
			// ����� ����������� � ������������ ��������� ������
			if((abs(water_lvl-water_lvl_last)>=abs((doliv_porog2-doliv_porog1)/2))|(WATER_ERROR_POMPA)) 
			{
				WATER_ERROR=1; // ���� - ������ �� ��������
				water_lvl_status=0x34; // ��������� ������ 
				water_lvl_ovf_time++; // ������ �������
			}
			else 
			{
				// ���� �� ~10 � �������� ������� ��������������
				if(water_lvl_ovf_time<30) 
				{
					WATER_ERROR=0; // ���� - ������ ��������
					water_lvl_ovf_time=0; // �������� ������ �������
				}
			}
			// ���� ������ ��������
			if(!WATER_ERROR)
			{
				// ������� ���� ������ ������� ������ (2)
				if(water_lvl<=doliv_porog2) pompa=1; // ���� - �������� �����
				// ������� ���� ������ �������� ������ (1)
				if(water_lvl>=doliv_porog1) pompa=0; // ���� - ��������� �����
				// ������ ����� ������
				if(pompa)
				{
					RELE_PORT_2|=RELE_8; // �������� �����
					water_lvl_status=0x33; // ��������� - ����� ��������������� ������
				}
				else RELE_PORT_2&=~RELE_8; // ��������� �����
				// �������� ������� �������� ������� ��� ������������ ��������� ������ ����
				water_lvl_last=water_lvl; 
			}
		}break;
		
		case 2: // �������� �����
		{
			if(rele_set[7]==1) water_lvl_status=0x32;
			// ���� �� ����� ������ ����� � ����� �������� - ��������� �����
			if((rele_set[7]==0)&&(doliv_timer_status==0)) {RELE_PORT_2&=~RELE_8; water_lvl_status=0x30;}
			// �������� �����
			doliv=0; 
			water_lvl_ovf_time=0;
			WATER_ERROR=0;
		}break;
		default:; break;
	}
}