/*	�������������� 2014 �. 1.0.0.

	���������� ��� ������ � USART.
	��� ��������/������ ������ ������������ ���������� �� ����������� �������,
	�� ���������� ������/�������� ��������.
	
	���������� �.�. 2014�.
*/

#ifndef USART_H
#define USART_H
#endif
// ������������
#define USART_DDR DDRD // ����, ������������� ������� 
#define RX _BV(PD0) // ����� RX
#define TX _BV(PD1) // ����� TX
#define bauddivider (FREQ_CPU/(16*baudrate)-1) // ������ �������� ��������
#define HI(x) ((x)>>8) // ������ � ������� �������
#define LO(x) ((x)& 0xFF) // ������ � ������� �������
#define buffer_MAX 512 // ����� ���������� �������
#define buffer_MAX_IN 512 // ����� ���������� �������
// ���������� ����������
volatile unsigned char buffer[buffer_MAX]=""; // ������ �������
volatile unsigned int buffer_index=0; // ������ ��������
volatile unsigned char usart_data_in[buffer_MAX_IN]=""; // �������� ������
volatile unsigned char TRANSMIT=0x00;
volatile unsigned int usart_count=0;
volatile unsigned int usart_data_in_num=0;
// ��������� �������
void buffer_out(void);
void buffer_start(void);

// ���������� �� ����������� �������
ISR (USART_UDRE_vect)		
{
	buffer_out();
}
// ���������� �� ���������� ������
ISR (USART_RXC_vect)		
{
	usart_data_in[usart_count]=UDR;
	if(usart_data_in[usart_count]==0x3B) 
	{
		usart_count=0;
		usart_data_in[usart_count]=0x3B;
	}
	if(usart_data_in[usart_count]==0xB3) 
	{
		TRANSMIT=0x01; // ���������� ��������
		for(int i=0;i<=usart_count;i++)
		{
			buffer[i]=usart_data_in[i];
		}
		usart_data_in_num=usart_count+1;
		buffer_start();
		usart_count=0;
	}
	usart_count++;
}

// ���������� �� ���������� ��������
ISR (USART_TXC_vect)
{
    if(TRANSMIT) buffer_start(); // ������ ��������
}
// ������������� USART
void usart_init(void)
{
	// ������������� �������
	USART_DDR&=~RX;
	USART_DDR|=TX;
	// ������ �������� �������� ��������
	UBRRL = LO(bauddivider);
	UBRRH = HI(bauddivider);
	// ������������� ��������� ����������
	UCSRA = 0;
	UCSRB = 1<<RXEN| // �������� �����
			1<<TXEN| // �������� ��������
			1<<RXCIE| // �������� ���������� �� ������
			1<<TXCIE; // �������� ���������� �� ��������
	UCSRC = 1<<URSEL| // ������ � ������� UCSRC
			1<<UCSZ0| // ������ ������ 8 ���
			1<<UCSZ1;
}
// ������� ������ �������
void buffer_out(void)
{
	buffer_index++;	// ���������� �������
	if(buffer_index==usart_data_in_num) // ���� ������ ���� ������
	{
		TRANSMIT=0x00;
		UCSRB &=~(1<<TXEN); // ��������� ��������
		UCSRB &=~(1<<UDRIE); // ��������� ���������� �� ����������� - �������� ���������
		UCSRA &=~(1<<TXC);	 // ������� ���������� ���� TXC - ������� �� ���������� �� ���������� ��������
	}
	else // ���������� �������� ������
	{
		UDR = buffer[buffer_index];	// ����� ������ �� �������. 
	}
}
// ������� ������ �������� �������
void buffer_start(void)
{
	buffer_index=0;	   // ���������� ������
	UCSRB|=(1<<TXEN); // �������� ��������
	UDR = buffer[0];   // ���������� ������ ����
	UCSRB|=(1<<UDRIE); // ��������� ���������� UDRE	
}