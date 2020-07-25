#include "main.h"
#include "nfcm1833_module.h"
#include "ndef.h"
#include "bcdencode.h"
#include <string.h>
#include "LIB_Config.h"

CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef spi_to_nfcm1833tinz;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_I2C1_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);

uint8_t idTag[ID_TAG_SIZE];
uint8_t idTagBCD[ID_TAG_SIZE * 2];

static void WakeUp_TinZ (void);
void display(char* data);
void deleteBuffer(char* buf);
char Sim_response[500] = {0};
char Sim_Rxdata[2] = {0};
char Sim_Rxdata1[500] = {0};
char IMEI_SIM[500] = {0};
char IMEI_SIM_REAL[16] = {0};
int8_t Sim_sendCommand(char*command ,char*response,uint32_t timeout);
int8_t Sim_sendCommand_IMEI(char*command);
int8_t Sim_Response(char*response,uint32_t timeout);
uint8_t permissReadTag = 0;
void DisplaySendText(uint8_t x, uint8_t y, char * TextSend, uint8_t sizeText);
uint8_t KhoiDongSim();

RTC_TimeTypeDef sTimes;
RTC_DateTypeDef sDates;

#define ONBUZZER  HAL_GPIO_WritePin (GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
#define OFFBUZZER  HAL_GPIO_WritePin (GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

#define ONLED HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
#define OFFLED HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
int main(void)
{
    uint8_t count = 0;
    uint8_t Second = 0;
    uint8_t Minute = 0;
    char urlDiemDanh[100] = "AT+HTTPPARA=\"URL\",\"http://nfcapi.conek.net/conek/dulieudiemdanh?uidTag=";
    char urlActive[100] = "AT+HTTPPARA=\"URL\",\"http://nfcapi.conek.net/conek/activesim\"";
    HAL_Init(); 
    SystemClock_Config();

    MX_GPIO_Init();
    HAL_Delay(7000);
    MX_CRC_Init();
    MX_I2C1_Init();
    MX_RTC_Init();
    
    MX_USART1_UART_Init();
    //MX_USART2_UART_Init();
    OLED_init();
    ssd1306_clear_screen(0x00);
    ssd1306_draw_bitmap(0, 12, &ConekLogo[0], 40, 40);
    ssd1306_display_string(50, 8, "Conek", 16, 1);
    ssd1306_refresh_gram();
    
    ssd1306_display_string(40, 23, "Connecting", 12, 1);
    ssd1306_display_string(55, 35, ".", 16, 1);
    ssd1306_refresh_gram();
    if(Sim_sendCommand_IMEI("AT+CGSN")){
      
    }
    HAL_Delay(10);
    uint8_t a = KhoiDongSim();
    while(a < 1){
        DisplaySendText(25,42,"Start Failed",12);
    }
    
    for(uint8_t i = 0; i < 15; i++){
      IMEI_SIM_REAL[i] = IMEI_SIM[i + 11] - 0x30;
    }
    DisplaySendText(25,50,"Welcome",16);
    MX_SPI1_Init();
    permissReadTag=0;
    HAL_Delay(100);
    WakeUp_TinZ();
    HAL_Delay(100);
    while (1)
    {
            uint8_t countSend = 0;
            switch(permissReadTag)
            {
            case 0:
              if((ping_module() == PING_OK) && (select_tag_type (TYPE_5) == PROTOCOL_OK) && (getDeviceID (idTag) == SEND_RECV_OK)){
                idTag[7] = 0x00;
                if (encode8byte_little_edian (idTag, idTagBCD) == 0)
                {
                  for (count = 0; count < ID_TAG_SIZE * 2; count++)
                  {
                    idTagBCD[count] += 0x30;
                  }
                }
                permissReadTag = 1;  
                //__HAL_SPI_DISABLE(&spi_to_nfcm1833tinz);
                DisplaySendText(25,45,"Sending...",16);    
              }
                break;
            case 1:
              while(countSend < 5)
              {
                HAL_Delay(10);                
                display((char *)urlDiemDanh);   
                for(uint8_t i = 0; i < 16; i++){
                  HAL_UART_Transmit(&huart1, &idTagBCD[i], 1, 1000);
                }          
                if(Sim_sendCommand("\"","OK",3000)){
                  ssd1306_display_string(60, 40, ".", 16, 1);
                  ssd1306_refresh_gram();
                }
                HAL_Delay(10);
                if(Sim_sendCommand("AT+HTTPACTION=0","OK",3000)){
                  HAL_Delay(10);
                  if(Sim_Response("200",5000)){
                    DisplaySendText(25,50,"Success",16);
                    countSend = 0;
                    break;
                  }
                }                                                    
                countSend++;
              }
              if(countSend > 0){
                DisplaySendText(25,47,"Re-Check",16); 
                HAL_Delay(1500);
              }
              DisplaySendText(25,50,"Welcome",16); 
              ONBUZZER;
              HAL_Delay(200);
              OFFBUZZER;
              permissReadTag = 0;
              //__HAL_SPI_DISABLE(&spi_to_nfcm1833tinz);
              break;
            default:
              HAL_Delay(10);
              DisplaySendText(25,45,"Sending...",16);
              if(Sim_sendCommand(urlActive,"OK",3000)){
              }
              HAL_Delay(10);
              if(Sim_sendCommand("AT+HTTPACTION=0","OK",3000)){
                HAL_Delay(10);
                if(Sim_Response("200",5000)){
                  
                }
              }             
              permissReadTag = 0;
              DisplaySendText(25,50,"Welcome",16);
              break;
            }
            HAL_RTC_GetTime(&hrtc,&sTimes,RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc,&sDates,RTC_FORMAT_BIN);
            Second = sTimes.Seconds;
            Minute = sTimes.Minutes;
            if((Minute == 1 && Second <= 3) || (Minute == 35 && Second <= 3))
            {
              permissReadTag = 4;
            } 
      }
}
uint8_t KhoiDongSim(){
      ssd1306_clear_screen(0x00);
      ssd1306_draw_bitmap(0, 12, &ConekLogo[0], 40, 40);
      ssd1306_display_string(50, 8, "Conek", 16, 1);
      ssd1306_display_string(40, 23, "Connecting", 12, 1);
      ssd1306_display_string(55, 35, ".", 16, 1);
      ssd1306_refresh_gram();
      if(Sim_sendCommand("AT","OK",10000)){
        ssd1306_display_string(60, 35, ".", 16, 1);
        ssd1306_refresh_gram();
        HAL_Delay(10);
        if(Sim_sendCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"","OK",10000)){
          ssd1306_display_string(65, 35, ".", 16, 1);
          ssd1306_refresh_gram();
          HAL_Delay(10);        
          if(Sim_sendCommand("AT+SAPBR=3,1,\"APN\",\"e-connect\"","OK",10000)){
            ssd1306_display_string(70, 35, ".", 16, 1);
            ssd1306_refresh_gram();
            HAL_Delay(10);           
            if(Sim_sendCommand("AT+SAPBR=1,1","OK",10000)){
              ssd1306_display_string(75, 35, ".", 16, 1);
              ssd1306_refresh_gram();
              HAL_Delay(10);             
              if(Sim_sendCommand("AT+HTTPINIT","OK",10000)){
                ssd1306_display_string(80, 35, ".", 16, 1);
                ssd1306_refresh_gram();
                HAL_Delay(10);
                if(Sim_sendCommand("AT+HTTPPARA=\"CID\",1","OK",10000)){
                  ssd1306_display_string(80, 35, ".", 16, 1);
                  ssd1306_refresh_gram();
                  HAL_Delay(10);                
                  return 1;
                }else{
                  return 0;
                }
                
              }else{
                return 0;
              }
            }else{
              return 0;
            }
          }else{
            return 0;
          }
        }else{
          return 0;
        }
      }else{
        return 0;
      }
      
}
void DisplaySendText(uint8_t x, uint8_t y, char * TextSend, uint8_t sizeText)
{
  /*
    Size text is 12 - 14 - 16 - 18
*/
    ssd1306_clear_screen(0x00);
    ssd1306_draw_bitmap(0, 12, &ConekLogo[0], 40, 40);
    ssd1306_display_string(62, 2, "Conek", 16, 1);
    ssd1306_display_string(y, x, (uint8_t *)TextSend, sizeText, 1);
    ssd1306_refresh_gram();  
}
void display(char* data)																								
{
	HAL_UART_Transmit(&huart1,(uint8_t *)data,(uint16_t)strlen(data),1000);
}

void deleteBuffer(char* buf)
{
	int len = strlen(buf);
	for(int i = 0; i < len; i++)
	{
		buf[i] = 0;
	}
}
int8_t Sim_sendCommand(char*command ,char*response,uint32_t timeout)
{
  uint8_t answer = 0, count  = 0;
  deleteBuffer(Sim_response);
  uint32_t time = HAL_GetTick();
  uint32_t time1 = HAL_GetTick();
  HAL_UART_Transmit(&huart1, (uint8_t *)command, (uint16_t)strlen(command), 1000);
  HAL_UART_Transmit(&huart1,(uint8_t *)"\r\n",(uint16_t)strlen("\r\n"),1000);
  do
  {
    while(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata, 1, 1000) != HAL_OK)
    {
      if(HAL_GetTick() - time > timeout) 
      {
        return 0;
      }
    }
    time = HAL_GetTick();
    Sim_response[count++] = Sim_Rxdata[0];
    while((HAL_GetTick() - time < timeout))
    {
      if(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata, 1, 1000) == HAL_OK)
      {
        Sim_response[count++] = Sim_Rxdata[0];
        time1 = HAL_GetTick();
      }
      else
      {
        if(HAL_GetTick() - time1 > 100)
        {
          if(strstr(Sim_response,response) != NULL) 
          {
            answer = 1;
          }
          else
          {
            answer = 0;
          }
          break;
        }
      }
    }
  }
  while(answer == 0); 
  return answer;
}

int8_t Sim_Response(char*response,uint32_t timeout)
{
  uint8_t answer = 0, count  = 0;
  deleteBuffer(Sim_response);
  uint32_t time = HAL_GetTick();
  uint32_t time1 = HAL_GetTick();
  do
  {
    while(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata, 1, 1000) != HAL_OK)
    {
      if(HAL_GetTick() - time > timeout) 
      {
        return 0;
      }
    }
    time = HAL_GetTick();
    Sim_response[count++] = Sim_Rxdata[0];
    while((HAL_GetTick() - time < timeout))
    {
      if(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata, 1, 1000) == HAL_OK)
      {
        Sim_response[count++] = Sim_Rxdata[0];
        time1 = HAL_GetTick();
      }
      else
      {
        if(HAL_GetTick() - time1 > 100)
        {
          if(strstr(Sim_response,response) != NULL) 
          {
            answer = 1;
          }
          else
          {
            answer = 0;
          }
          break;
        }
      }
    }
  }
  while(answer == 0);
  while(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata1, 1, 2000) == HAL_OK){}
  return answer;
}
int8_t Sim_sendCommand_IMEI(char*command)
{
  uint8_t answer = 0, count  = 0;
  deleteBuffer(IMEI_SIM);
  HAL_UART_Transmit(&huart1, (uint8_t *)command, (uint16_t)strlen(command), 1000);
  HAL_UART_Transmit(&huart1,(uint8_t *)"\r\n",(uint16_t)strlen("\r\n"),1000);
  do
  {
      if(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata1, 1, 2000) == HAL_OK)
      {
        IMEI_SIM[count] = Sim_Rxdata1[0] + 0x30;
        count++;
      }    
    
  }
  while(count <= 30);
  while(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata1, 1, 2000) == HAL_OK){}
  //ssd1306_display_string(40, 35,(uint8_t *)IMEI_SIM, 12, 1);
  //ssd1306_refresh_gram();    
  return answer;
}

static void WakeUp_TinZ (void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    HAL_GPIO_WritePin (GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init (GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin (GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_Delay (150);
    HAL_GPIO_WritePin (GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_Delay (150);
    HAL_GPIO_WritePin (GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_Delay (150);
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;//|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  //RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1//|RCC_PERIPHCLK_USART2
                              |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  //PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{
    /* USER CODE BEGIN CRC_Init 0 */
    /* USER CODE END CRC_Init 0 */
    /* USER CODE BEGIN CRC_Init 1 */
    /* USER CODE END CRC_Init 1 */
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
    hcrc.Init.GeneratingPolynomial    = 0x1021;
    hcrc.Init.CRCLength               = CRC_POLYLENGTH_16B;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_DISABLE;
    hcrc.Init.InitValue           = 0xFFFF;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_BYTE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_ENABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    
    if (HAL_CRC_Init (&hcrc) != HAL_OK)
        {
            Error_Handler();
        }
        
    /* USER CODE BEGIN CRC_Init 2 */
    /* USER CODE END CRC_Init 2 */
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00707CBB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only 
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
    
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date 
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */
  //HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP1R,0x32F2);
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  spi_to_nfcm1833tinz.Instance = SPI1;
  spi_to_nfcm1833tinz.Init.Mode = SPI_MODE_MASTER;
  spi_to_nfcm1833tinz.Init.Direction = SPI_DIRECTION_2LINES;
  spi_to_nfcm1833tinz.Init.DataSize = SPI_DATASIZE_8BIT;
  spi_to_nfcm1833tinz.Init.CLKPolarity = SPI_POLARITY_LOW;
  spi_to_nfcm1833tinz.Init.CLKPhase = SPI_PHASE_1EDGE;
  spi_to_nfcm1833tinz.Init.NSS = SPI_NSS_SOFT;
  spi_to_nfcm1833tinz.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  spi_to_nfcm1833tinz.Init.FirstBit = SPI_FIRSTBIT_MSB;
  spi_to_nfcm1833tinz.Init.TIMode = SPI_TIMODE_DISABLE;
  spi_to_nfcm1833tinz.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  spi_to_nfcm1833tinz.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&spi_to_nfcm1833tinz) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    HAL_GPIO_WritePin (GPIOA, GPIO_PIN_8|GPIO_PIN_1, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init (GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin (GPIOA, GPIO_PIN_8|GPIO_PIN_1, GPIO_PIN_RESET);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
