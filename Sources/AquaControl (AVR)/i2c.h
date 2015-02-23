/*	�������������� 2014 �. 1.0.0.
	
	���������� ��� ������ � ����� ������ I2C.
	������������� � ������ � ������ ��������� ������� DS1307N
	
	���������� �.�. 2014�.
*/

#ifndef I2C_H
#define I2C_H
#endif
// ������������
#define SETSDA() I2C_DDR&=~SDA	 	// ���������� SDA
#define SETSCL() I2C_DDR&=~SCL		// ���������� SCL
#define RSTSDA() I2C_DDR|= SDA		// �������� SDA
#define RSTSCL() I2C_DDR|= SCL	    // �������� SCL
// ���������� ����������
unsigned char data_i2c_h=0;
unsigned char data_i2c_l=0;

void i2c_init(void) // ������� �������������
{
	// ��������� �������
	SETSDA(); 
	SETSCL();
	I2C_PORT&=~SDA;
	I2C_PORT&=~SCL;
}

void i2c_busy(void) // �������, ����������� ��������� ����
{
	while(I2C_DDR&(SDA|SCL)); // �������� 
}

void i2c_start(void) // ������� ������������ ������� ������ �������� ������
{
	SETSDA();		// ��������� ������� SDA
	SETSCL();		// ��������� ������� SCL
	delay_us(2);	// ��������
	RSTSDA();		// ����� ������� SDA
	delay_us(2);	// ��������
	RSTSCL();		// ����� ������� SCL
	delay_us(2);	// ��������
}

void i2c_stop(void) // ������� ������������ ������� ���������� ������ �������
{
	RSTSDA();		// ����� ������� SDA
	SETSCL();		// ��������� ������� SCL
	delay_us(2); 	// ��������
	SETSDA();		// ��������� ������� SDA
	delay_us(2); 	// ��������
}

unsigned char i2c_sendbyte(char data) // ������� ������� 1 �����; �������� � ������������ ����
{
	unsigned char i=8;	// ������� ���
	unsigned char acknowledge=0; // �������������
	do 
	{	
		// � ����� ����������� ������� 8 ���
		if((data&0x80)!=0) // ���� ������� ��� 1
		{
			SETSDA();	// ��������� ������� SDA ���� ������� ��� ����� 1
		}
		else RSTSDA();	// ����� ����� ������� SDA
		data=data << 1;	// ����� ������������� ����� �� 1 ������ �����
		delay_us(2);	// ������������ ��������
		// ������������ ��������� ��������
		SETSCL(); 		// ��������� ������� SCL
		delay_us(2);	// ��������
		RSTSCL();		// ����� ������� SCL
		delay_us(2);	// �������� 
	}
	while (--i);		// ���� ����������� 8 ���
	// ������������ ��������� �������� ��� ���� �������������
	RSTSDA();			// ����� ������ SDA
	SETSCL();			// ��������� ������� SCL
	delay_us(2);		// ��������
	// �������� ���� ������������� 
	if((I2C_PIN&SDA)==0) acknowledge=1; // ���� �� ����� SDA ������ �������, ��� ������������� ������
	RSTSCL(); 			// ����� ������� SCL
	delay_us(2);		// ��������
	return acknowledge; // ���� ��� ������������� ������, �����.1
}

unsigned char i2c_readbyte(unsigned char acknowledge) // ������� ������ ������ �����
{
	unsigned char i=8; // �������
	unsigned char i2c_data=0;	// ������ ����
	char data; // ��������� �����
	I2C_DDR&=~SDA;			// ������� ����� SDA � ������ ���������
	do 
	{
		i2c_data<<=1;		// ����� �� 1 ������ �����
		SETSCL(); 			// ��������� ������� SCL
		delay_us(2);		// ��������
		data=I2C_PIN;		// ���������� ��������� ����� 
		data=(data&SDA)>>SDA_N;// ��������� 0 ����
		i2c_data|=data;		// ����������� 0 ���� � ����������
		RSTSCL();			// ����� ������� SCL
		delay_us(2);		// ��������
	}
	while (--i);			// ���� ����������� 8 ���
	if (acknowledge)		// ���� ��������� ��� �������������
	{
		RSTSDA();			// ����� ������� SDA
	}
	// ������������ ��������� ��������
	SETSCL();			// ��������� ������� SCL
	delay_us(2); 		// ��������
	RSTSCL();			// ����� ������� SCL
	return i2c_data;	// ������� ���������� �������� ����
}

// ----- ������� ������ � ������ DS1307N ----- //
// ������� ������ ������
void DS_write(unsigned char addr, unsigned char data)
{
	i2c_start(); // ������ ��������
	i2c_sendbyte(0xD0); // �������
	i2c_sendbyte(addr); // �����
	i2c_sendbyte(data); // ������
	i2c_stop(); // ����� ��������
}

// ������� ������ ������
unsigned char DS_read(unsigned char addr)
{
	unsigned char temp;
	i2c_start(); // ������ �������� 
	i2c_sendbyte(0xD0); // �������
	i2c_sendbyte(addr); // �����
	i2c_stop(); // ����� ��������
	i2c_start(); // ������ ��������
	i2c_sendbyte(0xD1); // �������
	temp=i2c_readbyte(0); // ������
	i2c_stop(); // ����� ��������
	return temp; // ������� ��������
}

void DS1307_init(void) // ������������� DS1307N
{
	unsigned char i=0;
	i=DS_read(0x02);
	if((i&0x40)!=0)
	{
		DS_write(0x02,i&~0x40);
	}
	i=DS_read(0x00);
	if((i&0x80)!=0)
	{
		DS_write(0x00,i&~0x80);
	}
}

void DS1307(void) // ������� ������
{
	i2c_start();
	i2c_sendbyte(0xD0);
	i2c_sendbyte(0x00);
	i2c_start();
	i2c_sendbyte(0xD1);
	buffer_ds[0]=i2c_readbyte(1);
	buffer_ds[1]=i2c_readbyte(1);
	buffer_ds[2]=i2c_readbyte(0);
	i2c_stop();
}

unsigned char IntToDec(unsigned char data) // �������������� � ���������� �����
{
	data=data%100;
	return data/10*16+data%10;
}

void DS1307_set_time(void) // ������� ������
{
	DS_write(0x02,IntToDec(buffer_time[0])); // ����
	DS_write(0x01,IntToDec(buffer_time[1])); // ������
	DS_write(0x00,IntToDec(buffer_time[2])); // �������
}
