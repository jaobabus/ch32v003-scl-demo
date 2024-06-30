
#include "debug.h"

#include "cli.hpp"
#include "commands.hpp"

#include <stdlib.h>



static
int my_getchar();
bool last_state = false;
uint8_t args_buffer[64];


void* opaques1[] = { new StringArg{} };
void* opaques2[] = { new IntArg{}, new IntArg{} };
ConsoleExecutor::CommandHandle handlers[] =
{
    default_handle(new EchoCommand(), opaques1),
    default_handle(new AddCommand(), opaques2),
};

SCLAllocator alloc =
{
    malloc,
    +[](void* ptr, size_t) { free(ptr); }
};

int main(void)
{
    ConsoleExecutor buffer(handlers, sizeof(handlers) / sizeof(handlers[0]), &alloc);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    Delay_Ms(100);

    GPIO_InitTypeDef  GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_SetBits(GPIOD, GPIO_Pin_4), last_state = true;

    while(1)
    {
        int ch = my_getchar();
        if (ch != -1)
        {
            buffer.on_char((uint8_t)ch);
        }
    }
}


uint16_t input_buf_int_pos = 0;
uint16_t input_buf_main_pos = 0;
uint8_t uart_input_buffer[24];

extern "C"
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
extern "C"
void USART1_IRQHandler(void)
{
    if (not last_state)
        GPIO_SetBits(GPIOD, GPIO_Pin_4), last_state = true;
    else
        GPIO_ResetBits(GPIOD, GPIO_Pin_4), last_state = false;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint16_t ch = USART_ReceiveData(USART1);
        if (uart_input_buffer[input_buf_int_pos] != '\0')
            return;
        uart_input_buffer[input_buf_int_pos++] = ch;
        if (input_buf_int_pos == sizeof(uart_input_buffer))
            input_buf_int_pos = 0;
    }
}

int my_getchar()
{
    if (uart_input_buffer[input_buf_main_pos] == '\0')
        return -1;
    uint8_t ch = uart_input_buffer[input_buf_main_pos];
    uart_input_buffer[input_buf_main_pos++] = '\0';
    if (input_buf_main_pos == sizeof(uart_input_buffer))
        input_buf_main_pos = 0;
    return ch;
}

void uwrite(const char* str, size_t size)
{
    for(size_t i = 0; i < size; i++){
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
        USART_SendData(USART1, *str++);
    }
}

