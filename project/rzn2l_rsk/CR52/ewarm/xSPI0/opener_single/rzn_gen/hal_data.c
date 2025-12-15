/* generated HAL source file - do not edit */
#include "hal_data.h"
xspi_qspi_instance_ctrl_t g_qspi0_ctrl;

static const spi_flash_erase_command_t g_qspi0_erase_command_list[] =
{
#if 4096 > 0
    {.command = 0x20,     .size = 4096 },
#endif
#if 32768 > 0
    {.command = 0x52, .size = 32768 },
#endif
#if 65536 > 0
    {.command = 0xD8,      .size = 65536 },
#endif
#if 0xC7 > 0
    {.command = 0xC7,       .size  = SPI_FLASH_ERASE_SIZE_CHIP_ERASE         },
#endif
};

static xspi_qspi_timing_setting_t g_qspi0_timing_settings =
{
    .command_to_command_interval = XSPI_QSPI_COMMAND_INTERVAL_CLOCKS_7,
    .cs_pullup_lag               = XSPI_QSPI_CS_PULLUP_CLOCKS_NO_EXTENSION,
    .cs_pulldown_lead            = XSPI_QSPI_CS_PULLDOWN_CLOCKS_NO_EXTENSION
};

static xspi_qspi_address_space_t g_qspi0_address_space_settings =
{
    .unit0_cs0_end_address   = XSPI_QSPI_CFG_UNIT_0_CS_0_END_ADDRESS,
    .unit0_cs1_start_address = XSPI_QSPI_CFG_UNIT_0_CS_1_START_ADDRESS,
    .unit0_cs1_end_address   = XSPI_QSPI_CFG_UNIT_0_CS_1_END_ADDRESS,
    .unit1_cs0_end_address   = XSPI_QSPI_CFG_UNIT_1_CS_0_END_ADDRESS,
    .unit1_cs1_start_address = XSPI_QSPI_CFG_UNIT_1_CS_1_START_ADDRESS,
    .unit1_cs1_end_address   = XSPI_QSPI_CFG_UNIT_1_CS_1_END_ADDRESS,
};
static const xspi_qspi_extended_cfg_t g_qspi0_extended_cfg =
{
    .unit              = 0,
    .chip_select       = XSPI_QSPI_CHIP_SELECT_0,
    .memory_size       = XSPI_QSPI_MEMORY_SIZE_64MB,
    .p_timing_settings = &g_qspi0_timing_settings,
#if 0 == 0
    .prefetch_en       = (xspi_qspi_prefetch_function_t) XSPI_QSPI_CFG_UNIT_0_PREFETCH_FUNCTION,
#else
    .prefetch_en       = (xspi_qspi_prefetch_function_t) XSPI_QSPI_CFG_UNIT_1_PREFETCH_FUNCTION,
#endif
#if BSP_FEATURE_XSPI_VOLTAGE_SETTING_SUPPORTED
 #if 0 == 0
    .io_voltage        = (xspi_qspi_io_voltage_t) XSPI_QSPI_CFG_UNIT_0_IOVOLTAGE,
 #else
    .io_voltage        = (xspi_qspi_io_voltage_t) XSPI_QSPI_CFG_UNIT_1_IOVOLTAGE,
 #endif
#endif
    .p_address_space   = &g_qspi0_address_space_settings,
};
const spi_flash_cfg_t g_qspi0_cfg =
{
    .spi_protocol        = SPI_FLASH_PROTOCOL_1S_1S_1S,
    .address_bytes       = SPI_FLASH_ADDRESS_BYTES_3,
    .dummy_clocks        = SPI_FLASH_DUMMY_CLOCKS_0,
    .read_command        = 0x03,
    .page_program_command = 0x02,
    .write_enable_command = 0x06,
    .status_command = 0x05,
    .write_status_bit    = 0,
    .xip_enter_command   = 0x20,
    .xip_exit_command    = 0xFF,
    .p_erase_command_list = &g_qspi0_erase_command_list[0],
    .erase_command_list_length = sizeof(g_qspi0_erase_command_list) / sizeof(g_qspi0_erase_command_list[0]),
    .p_extend            = &g_qspi0_extended_cfg,
};
/** This structure encompasses everything that is needed to use an instance of this interface. */
const spi_flash_instance_t g_qspi0 =
{
    .p_ctrl = &g_qspi0_ctrl,
    .p_cfg =  &g_qspi0_cfg,
    .p_api =  &g_spi_flash_on_xspi_qspi,
};
void g_hal_init(void) {
g_common_init();
}
