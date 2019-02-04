#include "stm32f0xx.h" 
#include "stdio.h"

uint64_t Freq;

#define BLUE GPIO_Pin_8
#define GREEN GPIO_Pin_9

void initLed(void);
void ledOn(uint32_t Which);
void ledOff(uint32_t Which);
void ledToggle(uint32_t Which);

void initUart(void); //pa9 - tx, pa10 - rx
uint8_t connectUart(void);

void initGenerator(void); //pa 4 - out

double setupTimerParams(double F);

char * uartRec(char * Buffer);
void sendMsg(char *Msg);


uint16_t Period;
uint16_t Prescaler;

int main(void)
{	
	initLed();
	initUart();
	initGenerator();
	
	while(connectUart()==0);	
	ledOn(GREEN);
	
	char Command[32];
	char Message[128];
	char Feedback[128];
	double DemandFreq;
	double Freq;
	while (1) 
	{		
		uartRec(Message);
		sscanf(Message, "%s %lf", Command, &DemandFreq);
		Freq = setupTimerParams(DemandFreq);
		sprintf(Feedback, "freq %lf\n", Freq);		
		sendMsg(Feedback);	
	}
}


void initLed(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitTypeDef Port;
	GPIO_StructInit(&Port);
	Port.GPIO_Mode = GPIO_Mode_OUT;
	Port.GPIO_OType = GPIO_OType_PP;
	Port.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOC, &Port);
	GPIO_WriteBit(GPIOC, GPIO_Pin_1, 0);
	GPIO_WriteBit(GPIOC, GPIO_Pin_1, 1);
	
	ledOff(BLUE);
	ledOff(GREEN);
}

void ledOn(uint32_t Which)
{
	GPIO_WriteBit(GPIOC, Which, 1);
}

void ledOff(uint32_t Which)
{
	GPIO_WriteBit(GPIOC, Which, 0);
}
void ledToggle(uint32_t Which)
{
	if(GPIO_ReadOutputDataBit(GPIOC, Which)==0)		
		GPIO_WriteBit(GPIOC, Which, 1);
	else
		GPIO_WriteBit(GPIOC, Which, 0);
}

void initUart(void) //pa9 - tx, pa10 - rx
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef UsartPin;
	UsartPin.GPIO_Mode = GPIO_Mode_AF;
	UsartPin.GPIO_OType = GPIO_OType_PP;
	UsartPin.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_Init(GPIOA, &UsartPin);	
	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);	
	
	USART_InitTypeDef UsartConf;
	USART_StructInit(&UsartConf);
	UsartConf.USART_BaudRate = 9600;
	UsartConf.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	UsartConf.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	UsartConf.USART_StopBits = USART_StopBits_1;
	UsartConf.USART_Parity = USART_Parity_No;
	UsartConf.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &UsartConf);
	
	USART_Cmd(USART1, ENABLE);

}

uint8_t connectUart(void)
{	
	char Rec;	
	char SynChar;	
	
	while( !(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==SET));
	SynChar = USART_ReceiveData(USART1);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET);	
	USART_SendData(USART1, SynChar+1);	
	while( !(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==SET) );
	Rec = USART_ReceiveData(USART1);	
	
	if(Rec==SynChar+2)
		return 1;
	else
		return 0;		
}


void sendMsg(char *Msg)
{		
	while(*Msg!=0)
	{	
		ledOn(BLUE);
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET);
		USART_SendData(USART1, *Msg);
		Msg++;
		ledOff(BLUE);
	}
}

char * uartRec(char * Buffer)
{
	char Rec = 0;
	uint8_t End = 0;
	char * Ptr = Buffer;

	while(End!=1)
	{
		while( !(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==SET));
		Rec = USART_ReceiveData(USART1);
		if(Rec=='\n')
		{
			End = 1;
			*Ptr=0;
		}
		else
		{
			*Ptr=Rec;
			Ptr++;
		}
	}
	return Buffer;
}
	
void initGenerator(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);		
	
	GPIO_InitTypeDef PortTim14;
	GPIO_StructInit(&PortTim14);	
	PortTim14.GPIO_Mode = GPIO_Mode_AF;	 
	PortTim14.GPIO_Pin = GPIO_Pin_4;	
	PortTim14.GPIO_OType = GPIO_OType_PP;
	PortTim14.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &PortTim14);
	 
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_4);	
	
	TIM_TimeBaseInitTypeDef tim14;	 
	TIM_TimeBaseStructInit(&tim14);
	tim14.TIM_CounterMode = TIM_CounterMode_Up;
	tim14.TIM_Prescaler = 480-1;
	tim14.TIM_Period  = 10-1;
	tim14.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM14, &tim14);	 	 
	 	 
	TIM_InternalClockConfig(TIM14);	

	TIM_OCInitTypeDef Channel14;
	TIM_OCStructInit(&Channel14);
	Channel14.TIM_OCMode = TIM_OCMode_PWM1;
	Channel14.TIM_OutputState = TIM_OutputState_Enable;
	Channel14.TIM_OCPolarity = TIM_OCPolarity_High;

	Channel14.TIM_Pulse = 10/2-1;	 	 
	TIM_OC1Init(TIM14, &Channel14);	 
	
	TIM_ARRPreloadConfig(TIM14, ENABLE);
	TIM_OC1PreloadConfig(TIM14, TIM_OCPreload_Enable);
	
	TIM_CtrlPWMOutputs(TIM14, ENABLE);	
	
	TIM_Cmd(TIM14, ENABLE);	
	
}


double setupTimerParams(double F)
{
	float Product = SystemCoreClock/F;
	uint32_t IntProduct = (Product - (uint32_t)Product < 0.5) ? (uint32_t)Product : (uint32_t)(Product+1);
	
	const uint8_t Divers[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 31};
	const uint8_t Sizeof = sizeof(Divers);
	
	if(IntProduct < 2)
	{		
		Prescaler = 0;
		Period = 1;
		return -1;
	}
	
	uint32_t Multipler = 1;	
	uint32_t ProductCpy = IntProduct;
	uint32_t Rest = 0;
	uint32_t LastRest = 255;
	uint8_t LastRestIndex = 255;

	if(IntProduct > 65536)	
	{
		uint8_t X = 0;
		while(X < Sizeof)
		{						
			Rest = ProductCpy % Divers[X];
			if(Rest == 0)
			{
				ProductCpy/=Divers[X];
				Multipler*=Divers[X];
				
				if(ProductCpy <= 65536)	
					break;					
				else
					continue;
			}
			else
			{
				if(Rest <= LastRest)
				{
					LastRest = Rest;
					LastRestIndex = X;
				}
				X++;
			}
		}
		
		if(Multipler*ProductCpy == IntProduct && ProductCpy <= 65536 && Multipler <= 65536)
		{
			Prescaler = Multipler - 1;
			Period = ProductCpy - 1;
		}
		else
		{
			Multipler*=Divers[LastRestIndex];
			ProductCpy = IntProduct/Multipler;				
			if(ProductCpy<=65536)
			{
				Prescaler = Multipler - 1;
				Period = ProductCpy - 1;
			}		
			else
			{
				while(ProductCpy>65536)
				{
					ProductCpy/=2;
					Multipler*=2;
				}
				Prescaler = Multipler - 1;
				Period = ProductCpy - 1;
			}		
		}		
	}	
	else
	{
		Prescaler = 0;
		Period = IntProduct-1;
	}	
	
	if(Prescaler>Period)
	{
		Multipler = Period;
		Period = Prescaler;
		Prescaler = Multipler;
	}
	
	TIM_SetAutoreload(TIM14, Period);
	TIM_PrescalerConfig(TIM14, Prescaler, TIM_PSCReloadMode_Update);
	TIM_SetCompare1(TIM14, (Period+1)/2 - 1);
	
	return (double)SystemCoreClock/(double)((Prescaler+1)*(Period+1));
}


	