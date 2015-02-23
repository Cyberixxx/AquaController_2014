
#define F_CPU 4000000UL
#define F_OSC 4000000UL

#define BAUD 9600UL

//�����!
//	1.	������ ����������������� �����, � ����� ��������� �������, ������������ ��� ������!
//		��� ����������� ����� ���������������� �����, ����������� �� �����!
//		������������� ��������� ����������, ��� ��������� �������������� ���������� �����.

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include "delay.h"

#include "lcd.h"
#include "main.h"

//#include "uart.h"
#include "onewire.h"
#include "ds18x20.h"

#include "delay.h"



#include <util/delay.h>		// �������� <inttypes.h>, � ������� �������� <stdint.h>,
							//�������� ��� �-���: _delay_us(double __us) (� �������������) �

//#define s_div F_CPU / 256
#define WDR() __asm__ __volatile__ ("wdr");	// ����� ����������� �������
#define SWAP(r) 			asm volatile("swap %0" : "=r" (r) : "0" (r));
#define NOP() __asm__ __volatile__ ("nop");	// "������" �������


//				---- ����������� �-���: ----

unsigned up_pass(unsigned int n, unsigned char p);
unsigned down_pass(unsigned int n,		//�����
				   unsigned char p);	//�������, ������ ���� � ���� �� �����
void print_pass(unsigned char s,unsigned char p, unsigned int long n);
void print_time(unsigned char s,	//����� ������ 1 ��� 2
				unsigned char p,	//������� � ������ 1...16
				unsigned int  t);	//����� � �������
void print_work(void);
unsigned up_time(unsigned int n,	//�����
				 unsigned char p);	//�������, ������ ���� � ���� �� �����
unsigned down_time(unsigned int n,		//�����
				   unsigned char p);	//�������, ������ ���� � ���� �� �����
void EEPROM_write(unsigned char uiAddress, unsigned char ucData);
unsigned char EEPROM_read(unsigned char uiAddress);
void rele_state(void);
void reinit(void);


unsigned char check_data(void);
void debug (unsigned char a);
void resive(unsigned char index, unsigned char in_data);
//				==== ����� ������ �������: ====



//					++++ ���������: ++++

enum{key_down	= 1,
	 key_right	= 2,
	 key_ok		= 3,
	 key_left	= 4,
	 key_up		= 5,
	 min_adsorb = 5,		//����������� ����� ���������
	 min_regen	= 4,		//����������� ����� �����������
	 time_ret	= 10,		//����� �������� � ������� �����

	 a1r2	= 0b0110,	//��������� 1, ����������� 2
	 a1z2	= 0b0100,	//��������� 1, ���������� 2
	 a2r1	= 0b1001,	//��������� 2, ����������� 1
	 a2z1	= 0b1000,	//��������� 2, ���������� 1

	 adr_pass	= 10,	//����� ��� ������.
	 adr_adsorb	= 14,	//����� ��� ����� ���������.
	 adr_regen	= 18,	//����� ��� ����� �����������.
	 
	 adr_mode	= 22,
	 adr_c_regen= 26,
	 adr_c_adsorb=30,
//**********************************************
	 a_masck = 0b10000000,
	 w_masck = 0b01000000,
	 p_g = 1,
	 p_d = 2,
	 p_c = 4,
	 p_e = 8
//**********************************************
	};

//					==== ���������� ����������: ====

unsigned char 	current_screen,	//�������, ������������ �����
				time_out,		//������� ����-����, ��� �������� � ������� �����
				pos,			//������������� ������� �������
				mode,			//����� ��� ������� ���������, � ������� ��������
				time_outs,		//������ ����-����
				sec;			//
				
//*******************************************************************************
volatile unsigned char 	buff[6], 	//�������� �����, �� �� � ��������
						r_w;		//������ 0, ������ 1, ������ 2
volatile unsigned char adress = 7, tr_sys=0, t=0;
volatile unsigned int command = 0;
volatile unsigned int long data32i = 0, data32o = 0;


				

volatile unsigned int 	FastCount, 			//��������� �������
 						FlagsFastCount;		//����� ���������� ��������

//				**** ����������� ����������: ****


ISR (TIMER0_OVF_vect){
	static uint16_t PrevState;
	FastCount++;
	//���������� ������ ����� �� 0 � 1 � ��������� FastCount
	//� ������������� ����� � ���������� ���������� FlagsFastCount, ���������� �����
	//������ ���������, ����� ������ �����������.
	FlagsFastCount |= ~PrevState & FastCount;
	PrevState = FastCount;

}
ISR	(USART_RXC_vect){	//USART, Rx Complete USART_RX_vect
//UDR=UDR;
	resive(1,UDR);
}
ISR	(USART_UDRE_vect){	//USART Data Register Empty
	UCSRB &= ~(1<<UDRIE);
}
ISR	(USART_TXC_vect){	//USART, Tx Complete
	transmit(1,0);
} 

//				**** ��� ���������: ****

void resive(unsigned char index, unsigned char in_data){
volatile static unsigned char i = 0, k=0, time_out = 0;
//UDR=in_data;	
	//	index - �������� ������:
	//	0 - ��������� ������ ~1.5 mC
	//	1 - RxD Complete
	
	if (index==1){	//���������:

//print_pass(1,in_data);
		if (i==0){	//��� ������ ���, ���������, � �� ������ �� ���:
			if (in_data & a_masck){	//������! �������, � �� ��� �� ��:
				if (in_data == (adress | a_masck)){	//
					buff[i++] = in_data;
					time_out=1;

				};
			
			};
		}
		else {		//����� �����, ����������:
			if (i == 1){	//������ ������, ������ ��������� ���� �������,
							//����� �����, ������ ��� ��� ������
				//buff[i++]=in_data;
				//time_out=1;
				if (in_data & w_masck) k=6;	//��� ������� ������!
				else k=4;					//��� ������� ������!
			}
			//else {				//��������� �����
			buff[i++] = in_data;
			time_out = 1;
			//};
		//	else {
		//		i = 0;
		//		time_out = 0;
		//	};
			if(i == k){	//������ ��������� ����!
				i = 0;
				time_out = 0;
				tr_sys |= p_g;
//t=111;
			};
		};
	};


//t=buff[3];
}
void transmit(unsigned char index, unsigned char len){
//	2 - TxD Complete
volatile static unsigned char i = 0, l=0;
	if (index==0){
		UDR=buff[i=0];
		l=len;
	};
	if (index==1){	//��������!
		if(i==0) i++;
		if(i>l)i=0;
		if(i>0 && i<l)UDR=buff[i++];
	};
	
}



void InitController(void){
			//��������� ��������� ������ �����������:
//���������� ������ �/�:

	//������:
//	DDRD = 0b01111100;
//	PORTD = 0;
//	DDRC = 0xff;
//	PORTC = 0;
//	DDRB = 0;
//	PORTB = 0xff;

	//�����:
	
	PORTC = 0;
	PORTB = 0;
	DDRD = 0b11111111;
	PORTD= 0b00000100;
	DDRC = 0b00111111;
	
	DDRB = 0b00011111;
	
	DDRD = 0b11111000;
	PORTD = 0b00001000;
	
	cli();	
	
	TCCR0 = _BV(CS01);// ������������� ������ 2 � ������������ 8:
	TIMSK = _BV(TOIE0);//��������� ��������� �������
	
	//TIMSK = _BV(TOIE0); //��������� ���������� �� ������������ �������
	//TCCR0 = _BV(CS00);	//������������� �����������
	
//���������� ����������:
	cli();	
	TIMSK = _BV(TOIE0); //��������� ���������� �� ������������ �������
	TCCR0 = _BV(CS01);	//������������� �����������
	
//���������� ����������:
//UBRRH = ((F_CPU / (16 * BAUD)) - 1)>>8;
//UBRRL = (F_CPU / (16 * BAUD)) - 1;

	unsigned int bauddiv;
	bauddiv = ((F_CPU+(BAUD*8L))/(BAUD*16L)-1);
	UBRRL= bauddiv;
	#ifdef UBRRH
	UBRRH = bauddiv>>8;
	#endif

	/* ���������� ������ ����������� � ��������� */
//UCSRB = (1<<RXEN)|(1<<TXEN)|_BV(RXCIE);

	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<TXCIE)|(1<<RXCIE);
	/* ��������� ������� �������: 8 ��� ������, 2 ����-���� */
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);



	sei();//��������� ����������

}


//������� ������������ ����:
void lcd_cls(void){
	//������� ������� �������:
	lcd_com(0x01); pause(10000);
}




unsigned char scan_key(void){
	//����������
	unsigned char k,t,t1;
	t=0,t1=0;
	for(k=1;k<32;k<<=1){
		PORTC = k;
		t++;
		pause(10);
		if(PINB & _BV(5)) t1 += t;
		PORTC = 0;	
	};
	PORTC = 0;
	return(t1);
}

void print_pass(unsigned char s,
				unsigned char p,	//������� �� ������ ������
				unsigned int long n){	//��������� �����
	//unsigned char i
	lcd_cursor_set(s,p);
	if (n){
		lcd_dat(((n/1000000) % 10)+0x30);
		lcd_dat(((n/100000) % 10)+0x30);
		lcd_dat(((n/10000) % 10)+0x30);
		lcd_dat(((n/1000) % 10)+0x30);
		lcd_dat(((n/100) % 10)+0x30);
		lcd_dat(((n/10) % 10)+0x30);
		lcd_dat((n % 10) + 0x30);
	}
	else{
		lcd_dat(0x30);lcd_dat(0x30);lcd_dat(0x30);lcd_dat(0x30);
	};
}




void reinit(void){
	//����������������� ������ � ���:
	//(������, ������������ �����)
	lcd_init();
//	screen(1);
	time_out=1;
	time_outs = 3;
	current_screen = 1;
}


#define MAXSENSORS 5

uint8_t gSensorIDs[MAXSENSORS][OW_ROMCODE_SIZE];

uint8_t search_sensors(void)
{
	uint8_t i;
	uint8_t id[OW_ROMCODE_SIZE];
	uint8_t diff, nSensors;
	
//	uart_puts_P( "\rScanning Bus for DS18X20\r" );
	
	nSensors = 0;
	
	for( diff = OW_SEARCH_FIRST; 
		diff != OW_LAST_DEVICE && nSensors < MAXSENSORS ; )
	{
		DS18X20_find_sensor( &diff, &id[0] );
		
		if( diff == OW_PRESENCE_ERR ) {
//			uart_puts_P( "No Sensor found\r" );
			break;
		}
		
		if( diff == OW_DATA_ERR ) {
//			uart_puts_P( "Bus Error\r" );
			break;
		}
		
		for (i=0;i<OW_ROMCODE_SIZE;i++)
			gSensorIDs[nSensors][i]=id[i];
		
		nSensors++;
	}
	
	return nSensors;
}



//*******************************************************************************



//volatile unsigned int tttt;



unsigned char CRC8(unsigned char input, unsigned char seed)
{
    unsigned char i, feedback;

    for(i=0; i<8; i++){
        feedback = ((seed ^ input) & 0x01);
        if (!feedback) seed >>= 1;
        else{
            seed ^= 0x18;	//18
            seed >>= 1;
            seed |= 0x80;
        };
        input >>= 1;
    };
    return seed;    
}


unsigned char check_data(void){
	//����������� ����� ��������� ������ ��� ������ � ���������� ����� ������� CRC, 
	unsigned char crc, crc8=0, p_l, i;
	if (buff[1] & w_masck){	//������, �������� 6 ����
		p_l=5;	//Packep Len
		crc = (buff[5] & 0b01111111) | ((buff[4] & 0b01000000)<<1);
		buff[5] = 0;
		buff[4] &= 0b00111111;
		r_w=2;
	}
	else{					//������, �������� 4 ����
		p_l=3;
		crc = (buff[3] & 0b01111111) | ((buff[2] & 0b01000000)<<1);
		buff[3] = buff[4] = buff[5] = 0;
		buff[2] &= 0b00111111;
		r_w=1;
	};

	for (i=0;i<p_l;i++){
		crc8=CRC8(buff[i],crc8);
	};
	if (crc8==crc) return (1);
	else r_w=0; return (0);
}
void get_data(void){
//���������� �������� ������ � �������:
	unsigned int long  t1, t2;
//	unsigned int long temp;
	//adress = command = data = 0;
	adress = buff[0] & ~ a_masck;
	switch(r_w){
		case 1://�������� ������� ������!:
			t1=buff[1];
			t2=buff[2] & 0b00111111;
			command = t1 | (t2<<6);
			data32i=0;
//test();
get_packet();
transmit(0,4);

		break;
		case 2://�������� ������� ������!:
			command = buff[1] & ~ w_masck;
			t1=buff[3];
			t2=buff[4] & 0b00111111;
			data32i = buff[2] | (t1<<7) | (t2<<14);			
		break;
	};
//	return(rez);
}


void get_packet(void){
//��������� ����� ������ � ��������:
	unsigned char i, crc8=0;
	buff[0]=data32o & ~a_masck;
	buff[1]=(data32o>>7) & ~a_masck;
	buff[2]=(data32o>>14) & ~a_masck;
	for(i=0;i<3;i++){
		crc8=CRC8(buff[i],crc8);
	};
	buff[3]=crc8 & ~a_masck;
	buff[2] |= (crc8 & a_masck)>>1 ;
}

void test(void){
	buff[0]=135;
	buff[1]=96;
	buff[2]=64;
	buff[3]=4;
	buff[4]=61;
	buff[5]=67;

	data32o = 0x8040f;
}
void put_t(int pt0, int pt1){
	data32o = (pt0 |= pt1<<8 );
	
}
//*************************************************************************
int main(void){



test();
//get_packet();
//if (check_data()) get_data();
//print_pass(1,1,data);
//command=adress;
//int temperature;


//	static unsigned char m, h;
	InitController();
 	lcd_init();
	
//	uint8_t nSensors, i;
	uint8_t subzero, cel, cel_frac_bits;
//int t,tt=0;

	//reinit();
//	screen(1);time_out=1;time_outs = 3; current_screen = 1;
	reinit();
		
	while(1){
		WDR();
		

/*		ow_command(0xCC,NULL);
		if ( DS18X20_start_meas( DS18X20_POWER_PARASITE, NULL ) == DS18X20_OK){
			delay_ms(DS18B20_TCONV_12BIT);
		};
		t=DS18X20_read_meas_single( DS18B20_ID, &subzero,
						&cel, &cel_frac_bits); //== DS18X20_OK )
*/
//		{
//cel=FlagsFastCount;
/*
			lcd_cursor_set(1,1);
			lcd_dat(((subzero)?'-':'+'));
			lcd_dat(((cel/100) % 10)+0x30);
			lcd_dat(((cel/10) % 10)+0x30);
			lcd_dat((cel % 10) + 0x30);
			lcd_dat('.');
			lcd_dat(((cel_frac_bits/10) % 10)+0x30);
			lcd_dat((cel_frac_bits % 10) + 0x30);
			lcd_dat(0b11101111);
			lcd_dat('C');
			//print_pass(1,subzero);
*/
//		};
//tt=UDR;
//if (tt) print_pass(1,tt);
//debug(FlagsFastCount);
//if(tr_sys){ print_pass(1,11);tr_sys=0;}

		if (FlagsFastCount & _BV(0)){	//
			resive(0,0);
			FlagsFastCount &= ~_BV(0);
		};
sei();
		if (FlagsFastCount & _BV(11)){	// ~1c
//debug(tt++);
//UDR=tt++;
/*
print_pass(1,1,buff[4]);
print_pass(1,8,buff[5]);
print_pass(2,1,buff[2]);
print_pass(2,8,buff[3]);
*/	
//print_pass(1,1,data32);
		ow_command(0xCC,NULL);
		
		if ( DS18X20_start_meas( DS18X20_POWER_PARASITE, NULL ) == DS18X20_OK){
			delay_ms(DS18B20_TCONV_12BIT);
		};
		
		t=DS18X20_read_meas_single( DS18B20_ID, &subzero,
						&cel, &cel_frac_bits); //== DS18X20_OK )		
						lcd_cursor_set(1,1);
			lcd_dat(((subzero)?'-':'+'));
			lcd_dat(((cel/100) % 10)+0x30);
			lcd_dat(((cel/10) % 10)+0x30);
			lcd_dat((cel % 10) + 0x30);
			lcd_dat('.');
			lcd_dat(((cel_frac_bits/10) % 10)+0x30);
			lcd_dat((cel_frac_bits % 10) + 0x30);
			lcd_dat(0b11101111);
			lcd_dat('C');
			print_pass(2,1,data32o);
			
			
			
			FlagsFastCount &= ~_BV(11);
		};
			
if(tr_sys & p_g){
//UDR=buff[0];
//	if (check_data){ 
//		get_data();
		data32i = check_data();
		get_data();
//		print_pass(1,1,command);
//		print_pass(2,1,data32i);
		tr_sys &= ~p_g;
//	};
};

		
		

	
	
	};
	return(0);
}
