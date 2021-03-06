#include "nfcm1833tinz.h"
#include "string.h"
#include "debug_uart.h"

#ifdef STM32L432xx
    #include "stm32l4xx_ll_spi.h"
#endif /*STM32L432xx*/

#ifdef STM32F401xC
    #include "stm32f4xx_ll_spi.h"
#endif
#ifdef STM32L052xx
    #include "stm32l0xx_ll_spi.h"
#endif




#define SPI_CTRL_BYTE_SEND 	0x00
#define SPI_CTRL_BYTE_RECV 	0x02
#define SPI_CTRL_BYTE_POLL 	0x03
#define SPI_STRL_BYTE_RESET     0x01
#define SPI_CMD_MAX_LENGTH 	0x0F //the command to cr95hf must smaller than 15 bytes per time
#define SPI_RECV_MAX_LENGTH 0x0F // the data recv from cf95hf must smaller than 15 bytes per time

#define GETID_CMD       {0x04,0x03,0x26,0x01,0x00}
#define GETID_CMD_LEN   5
#define SPI_POLLING_TIMEOUT 254

extern SPI_HandleTypeDef spi_to_nfcm1833tinz;
extern CRC_HandleTypeDef hcrc;

static uint16_t crcx25 (const uint8_t* data, uint8_t len);

static HAL_StatusTypeDef spi_send_data_to_cr95hf (uint8_t* data_send, uint8_t data_len)
{
    assert_param (data_len < SPI_CMD_MAX_LENGTH);
    uint8_t data_control_send[] = {SPI_CTRL_BYTE_SEND};
    uint8_t data_temp[SPI_CMD_MAX_LENGTH] = {0x00};
    HAL_GPIO_WritePin (SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive (&spi_to_nfcm1833tinz, data_control_send, data_temp, 1, SPI_TIMEOUT);
    status |= HAL_SPI_TransmitReceive (&spi_to_nfcm1833tinz, data_send, data_temp, data_len & SPI_CMD_MAX_LENGTH, SPI_TIMEOUT);
    HAL_GPIO_WritePin (SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_SET);
    HAL_Delay (8);
#ifdef DEBUG_UART
    if (status == HAL_OK)
    {
      spi_debug_result.spi_send_succ_count ++;
    }
    else
    {
      spi_debug_result.spi_send_fail_count ++;
    }
#endif    
    return status;
}

static HAL_StatusTypeDef spi_polling_data_cr95hf (void)
{
    uint8_t data_control_polling[] = {SPI_CTRL_BYTE_POLL};
    uint8_t data_temp[] = {0x00};
    HAL_StatusTypeDef status = HAL_OK;
    HAL_GPIO_WritePin (SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_RESET);
    uint8_t counter = 0;
    
    while ( ( (data_temp[0]) != 0x08) && (counter < SPI_POLLING_TIMEOUT))
        {
            counter++;
            status |= HAL_SPI_TransmitReceive (&spi_to_nfcm1833tinz, data_control_polling, data_temp, 1, SPI_TIMEOUT);
            data_temp[0] = (data_temp[0] & 0x08);
        }
        
    HAL_GPIO_WritePin (SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_SET);
    HAL_Delay (8);
    
    if (counter >= SPI_POLLING_TIMEOUT)
        status = HAL_ERROR;
#ifdef DEBUG_UART
    if (status == HAL_OK)
    {
      spi_debug_result.spi_poll_succ_count ++;
    }
    else
    {
      spi_debug_result.spi_poll_fail_count ++;
    }
#endif   
    return status;
}

static HAL_StatusTypeDef spi_recv_data_from_cr95hf (uint8_t* data_recv)
{
    uint8_t data_control_recv[] = {SPI_CTRL_BYTE_RECV};
    uint8_t data_temp[SPI_RECV_MAX_LENGTH] = {0x00, 0x00, 0x00};
    HAL_GPIO_WritePin (SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive (&spi_to_nfcm1833tinz, data_control_recv, data_temp, 1, SPI_TIMEOUT);
    status |= HAL_SPI_TransmitReceive (&spi_to_nfcm1833tinz, data_temp, data_recv, 2, SPI_TIMEOUT);
    
    if (data_recv[0] == 0x80 && data_recv[1] > 0)
        {
            status |= HAL_SPI_TransmitReceive (&spi_to_nfcm1833tinz, data_temp, data_recv + 2, data_recv[1] & SPI_RECV_MAX_LENGTH, SPI_TIMEOUT);
        }
        
    HAL_GPIO_WritePin (SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_SET);
    HAL_Delay (10);
#ifdef DEBUG_UART
    if (status == HAL_OK)
    {
      spi_debug_result.spi_recv_succ_count ++;
    }
    else
    {
      spi_debug_result.spi_send_fail_count ++;
    }
#endif 
    return status;
}

ping_status ping_module_spi (void)
{
    uint8_t ping_cmd[] = {0x55};
    uint8_t data_result[SPI_RECV_MAX_LENGTH * 2] = {0x00};
    ping_status status = PING_ERR;
    
    if (spi_send_data_to_cr95hf (ping_cmd, sizeof (ping_cmd)) == HAL_OK)
        if (spi_polling_data_cr95hf() == HAL_OK)
            if (spi_recv_data_from_cr95hf (data_result) == HAL_OK)
                {
                    if (data_result[0] == 0x55)
                        {
                            status = PING_OK;
                        }
                }
                
    return status;
}

static protocol_status_t select_tag_type_spi_tag_type_5 (void)
{
    uint8_t spi_select_tag_type5[] = {0x02, 0x02, 0x01, 0x0D};
    uint8_t data_result[255] = {0xFF, 0xFF};
    protocol_status_t status = PROTOCOL_ERR;
    
    if (spi_send_data_to_cr95hf (spi_select_tag_type5, sizeof (spi_select_tag_type5)) == HAL_OK)
        if (spi_polling_data_cr95hf() == HAL_OK)
            if (spi_recv_data_from_cr95hf (data_result) == HAL_OK)
                if (data_result[0] == 0x00 && data_result[1] == 0x00)
                    {
                        status = PROTOCOL_OK;
                    }
                    
    return status;
}

protocol_status_t select_tag_type_spi (tag_type_t tag_type)
{
    switch (tag_type)
        {
            case TYPE_5:
                return select_tag_type_spi_tag_type_5();
                break;
                
            default:
                return PROTOCOL_ERR;
                break;
        }
}

send_recv_status_t sendrecv_spi (uint8_t* command, uint8_t command_len, cr95hf_recv_format_t* data_result)
{
    uint8_t data_temp[SPI_RECV_MAX_LENGTH * 2] = {0x00};
    send_recv_status_t status = RECV_ERR;
    
    if (spi_send_data_to_cr95hf (command, command_len) == HAL_OK)
        if (spi_polling_data_cr95hf() == HAL_OK)
            if (spi_recv_data_from_cr95hf (data_temp) == HAL_OK)
                {
                    data_result->result_code = (send_recv_status_t) data_temp[0] ;
                    
                    if (data_result->result_code == SEND_RECV_OK)
                        {
                            if (data_temp[1] > 3)
                                {
                                    data_result->data_len = data_temp[1] - 3;
                                    memcpy (data_result->data_recv, data_temp + 2, data_result->data_len);
                                    data_result->crc[0] = data_temp[data_result->data_len + 2];
                                    data_result->crc[1] = data_temp[data_result->data_len + 3];
                                    data_result->rfu    = data_temp[data_result->data_len + 4];
                                    int16_t crc = data_result->crc[1] * 0x0100 + data_result->crc[0];
                                    int16_t crc_cal = crcx25 (data_result->data_recv, data_result->data_len);
                                    
                                    if (crc == crc_cal)
                                        {
                                            status = data_result->result_code;
                                        }
                                }
                        }
                    else
                        {
                            status = data_result->result_code;
                        }
                }
    return status;
}

send_recv_status_t getDeviceID_spi (uint8_t* deviceID)
{
    uint8_t get_ID_cmd[] = GETID_CMD;
    cr95hf_recv_format_t data_recv;
    send_recv_status_t status = sendrecv_spi (get_ID_cmd, GETID_CMD_LEN, &data_recv);
    memcpy (deviceID, data_recv.data_recv + data_recv.data_len - ID_TAG_SIZE, ID_TAG_SIZE);
    return status;
}

uint16_t crcx25 (const uint8_t* data, uint8_t len)
{
    return HAL_CRC_Calculate (&hcrc, (uint32_t*) data, len) ^ 0xFFFF;
}