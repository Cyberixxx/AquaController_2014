/* 	�������������� 2014 �. 1.0.0.
	
	����������� ������������ ������
	
	���������� �.�. 2014�.
*/



#define InvBit(p,n) (p^=_BV(n))	//�������, ������ �������� ����, � �����., ��������, ����� ��.
#define WDR() __asm__ __volatile__ ("wdr");	// ����� ����������� �������
#define NOP() __asm__ __volatile__ ("nop");	// "������" �������
#define SWAP(r) 			asm volatile("swap %0" : "=r" (r) : "0" (r));
#define SBI(port,bit) asm volatile("sbi %0,%1"	:	: "I" (_SFR_IO_ADDR(port)),	"I" (bit));
#define CBI(port,bit) asm volatile("cbi %0,%1"	:	: "I" (_SFR_IO_ADDR(port)),	"I" (bit));

