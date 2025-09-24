# STM32F103C8T6-UART-Bootloader
UART bootloader for stm32f103c8t6 blue pill clone board
## Configurations for the bootloader
<img width="1920" height="1080" alt="Screenshot (528)" src="https://github.com/user-attachments/assets/7bff37e7-3c98-4bcb-b207-43578305c393" />
<img width="1920" height="1080" alt="Screenshot (527)" src="https://github.com/user-attachments/assets/e2db9e2a-a867-4f9b-b279-d0cb0b617e27" />
<img width="1920" height="1080" alt="Screenshot (526)" src="https://github.com/user-attachments/assets/a95b38c3-ffeb-4fef-b9a7-b122750832db" />
The USART1 connection has been configured. Baud rate: 115200 Bits/s, Word length: 8 Bits(including parity), Stop bits: 1. The USART1 global interrupt has been enabled. The clock settings were configured to 8 MHz HSI the clock setting has to match the clock setting of the project which is going to be uploaded from the bootloader.

The bootloader memory address has a length of 16KB this configuration has been done with editing the linker file
'''

/* Memories definition */
MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 20K 
  FLASH    (rx)    : ORIGIN = 0x8000000,   LENGTH = 16K  /* 16KB for bootloader */
}

'''
## Configurations for the application
The app which is going to be uploaded using the bootloader needs to have following memory settings in the linker file:
'''

MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 20K
  FLASH    (rx)    : ORIGIN = 0x8004000,   LENGTH = 48K
}

'''
And the app needs an additional vector table set command in the main function:

'''
APP_ADDRESS2 = 0x08004000U
int main(void)
{

  /* USER CODE BEGIN 1 */
    SCB->VTOR = APP_ADDRESS2;

  /* USER CODE END 1 */
  //Rest of the code...
}

'''
## Configurations for the Python script
The python script needs to be set to the right port which is COM6 for me. And also the correct baud rate which is 115200 for this bootloader project.
'''

ser = serial.Serial('COM6', 115200, timeout=1)

'''
The desired apps .bin file must be copied to the python scripts location for script to work and the .bin file name must be put in the line:
'''

    if send_firmware('jump_to_app2.bin'):

'''
