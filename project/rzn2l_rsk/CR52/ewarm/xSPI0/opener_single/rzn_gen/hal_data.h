/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_xspi_qspi.h"
            #include "r_spi_flash_api.h"
FSP_HEADER
extern const spi_flash_instance_t g_qspi0;
            extern xspi_qspi_instance_ctrl_t g_qspi0_ctrl;
            extern const spi_flash_cfg_t g_qspi0_cfg;
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
