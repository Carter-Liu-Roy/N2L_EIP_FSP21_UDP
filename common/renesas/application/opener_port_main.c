/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2021 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
/** FSP module instances. */
#include "hal_data.h"
#include "common_data.h"

/** FreeRTOS related definitions. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/** User module instance APIs. */
#include "um_serial_io_api.h"
#include "um_opener_port.h"
#include "um_common_api.h"
#include "um_common_cfg.h"

/** Board IO definitions */
#include "board_io_define.h"

/** OpENer libraries */
#include "opener_api.h"
#include "cipqos.h"
#include "ciptcpipinterface.h"
#include "cipconnectionmanager.h"
#include "cipidentity.h"
#include "appcontype.h"
#if defined(OPENER_CIP_SAFETY)
#include "cipsafetysupervisor.h"
#include "cip_safety_sci.h"
#endif

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/**
 * Sample application version
 */
#define SAMPLE_APP_VERSION_MAJOR    (2)
#define SAMPLE_APP_VERSION_MINOR    (0)

/**
 * Settings for Module/Network statue Red/Green LED.
 */
#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZT2ME_RSK)   ///< Settings For RSK+RZ/T2M board and RSK+RZ/T2ME board
#define BOARD_PIN_MOD_GREEN     (BOARD_ETH_LED2_GREEN)
#define BOARD_PIN_MOD_RED       (BOARD_ETH_LED6_RED)
#define BOARD_PIN_NET_GREEN     (BOARD_ETH_LED5_GREEN)
#define BOARD_PIN_NET_RED       (BOARD_ETH_LED1_RED)

#elif defined(BOARD_RZT2H_EVB)  ///< Settings For RZ/T2H Evaluation Board
#define BOARD_PIN_MOD_GREEN     (BOARD_LED4_ESC_RUN_GREEN)
#define BOARD_PIN_MOD_RED       (BOARD_LED3_RED)
#define BOARD_PIN_NET_GREEN     (BOARD_LED6_ESC_LA0_GREEN)
#define BOARD_PIN_NET_RED       (BOARD_LED5_ESC_ERR_RED)

#elif defined(BOARD_RZN2L_RSK)  ///< Settings For RSK+RZ/N2L board
#define BOARD_PIN_MOD_GREEN     (BOARD_ETH_LED2_GREEN)
#define BOARD_PIN_MOD_RED       (BOARD_ETH_LED4_RED)
#define BOARD_PIN_NET_GREEN     (BOARD_ETH_LED5_GREEN)
#define BOARD_PIN_NET_RED       (BOARD_ETH_LED1_RED)

#elif defined(BOARD_RZN2H_EVB)  ///< Settings For RZ/T2H Evaluation Board
#define BOARD_PIN_MOD_GREEN     (BOARD_LED3_GREEN)
#define BOARD_PIN_MOD_RED       (BOARD_LED4_RED)
#define BOARD_PIN_NET_GREEN     (BOARD_LED9_GREEN)
#define BOARD_PIN_NET_RED       (BOARD_LED11_RED)

#elif defined(BOARD_RA6M3_EK)   ///< Settings For EK-RA6M3 board
#define BOARD_PIN_MOD_GREEN    (BOARD_PIN_DISABLED)     ///< If LED lacks, set BOARD_PIN_DISABLED
#define BOARD_PIN_MOD_RED      (BOARD_EKRA6M3_RED)
#define BOARD_PIN_NET_GREEN    (BOARD_EKRA6M3_GREEN)
#define BOARD_PIN_NET_RED      (BOARD_EKRA6M3_BLUE)

#else
#define BOARD_PIN_MOD_GREEN    (BOARD_PIN_DISABLED)
#define BOARD_PIN_MOD_RED      (BOARD_PIN_DISABLED)
#define BOARD_PIN_NET_GREEN    (BOARD_PIN_DISABLED)
#define BOARD_PIN_NET_RED      (BOARD_PIN_DISABLED)
#endif

/**
 * The number of pins of pin arrays which access to LED SW efficiently.
 */
#define DEMO_APP_PIN_ARRAY_LED_PIN_NUM           (4)
#define DEMO_APP_PIN_ARRAY_SW_PIN_NUM            (4)

/**
 * Definition of the number of assembly object instances.
 */
#define DEMO_ASSEMBLY_LED_INPUT                     (100) /// 0x64, Instance Type: Static Input */
#define DEMO_ASSEMBLY_SW_INPUT                      (101) /// 0x65, Instance Type: Static Input */
#define DEMO_ASSEMBLY_LED_OUTPUT                    (150) /// 0x96, Instance Type: Static Output */
#define DEMO_ASSEMBLY_HEARTBEAT_FOR_INPUT_ONLY      (238) /// 0xEE, Instance Type: Static Output */
#define DEMO_ASSEMBLY_HEARTBEAT_FOR_LISTEN_ONLY     (237) /// 0xED, Instance Type: Static Output */
#define DEMO_ASSEMBLY_CONFIG                        (151) /// 0x97, Instance Type: Static Configuration */
#define DEMO_ASSEMBLY_EXPLICT                       (154) /// 0x9A, Instance Type: Static I/O */
#if defined(OPENER_CIP_SAFETY)
#define SAFETY_ASSEMBLY_INPUT                       (385) /// 0x181, Instance Type: Static Safety Input */
#define SAFETY_ASSEMBLY_OUTPUT                      (433) /// 0x1B1, Instance Type: Static Safety Output */
#define SAFETY_ASSEMBLY_NULL                        (196) /// 0xC4, Instance Type: NULL */
#endif

/**
 * Definition of Assembly object instance attribute #4 Size of data
 */
#define DEMO_ASSEMBLY_LED_INPUT_BYTES    (1)    /// For instance ID #100
#define DEMO_ASSEMBLY_SW_INPUT_BYTES     (1)    /// For instance ID #101
#define DEMO_ASSEMBLY_LED_OUTPUT_BYTES   (1)    /// For instance ID #150
#define DEMO_ASSEMBLY_CONFIG_BYTES       (4)    /// For instance ID #151
#define DEMO_ASSEMBLY_EXPLICT_BYTES      (4)    /// For instance ID #154
#if defined(OPENER_CIP_SAFETY)
#define SAFETY_ASSEMBLY_INPUT_BYTES      (4)    /// For instance ID #385
#define SAFETY_ASSEMBLY_OUTPUT_BYTES     (4)    /// For instance ID #433
#endif

/**
 * Definition of connection object instance IDs
 */
#define DEMO_CONNECTION_LED_EXCLUSIVE_OWNER (0)    /// For input assembly #100 and output assembly #150
#define DEMO_CONNECTION_LED_LISTEN_ONLY     (0)    /// For input assembly #100 and output assembly #237 (heart beat)
#define DEMO_CONNECTION_SW_INPUT_ONLY       (0)    /// For input assembly #101 and output assembly #238 (heart beat)
#if defined(OPENER_CIP_SAFETY)
#define SAFETY_CONNECTION_INPUT_INPUT_ONLY       (1) /// For input assembly #385 and output assembly #196 (NULL)
#define SAFETY_CONNECTION_OUTPUT_EXCLUSIVE_OWNER (1) /// For input assembly #196 (NULL) and output assembly #433
#endif

/**
 * Definition of Flash Write/Read for NV Data
 */
#if defined(BSP_MCU_GROUP_RZT2M) || defined(BSP_MCU_GROUP_RZT2ME) || defined(BSP_MCU_GROUP_RZN2L)
    #define QSPI_DEVICE_START_ADDRESS_MIRROR (0x40000000)  /// xSPI0_CS0 memory (Mirror area)
    #define QSPI_DEVICE_START_ADDRESS        (0x60000000)  /// xSPI0_CS0 memory
#elif defined(BSP_MCU_GROUP_RZT2H) || defined(BSP_MCU_GROUP_RZN2H)
    #define QSPI_DEVICE_START_ADDRESS_MIRROR BSP_FEATURE_XSPI_DEVICE_1_START_ADDRESS  /// RZT2H doesn't have xSPI mirror area, so it is defined the same address.
    #define QSPI_DEVICE_START_ADDRESS        BSP_FEATURE_XSPI_DEVICE_1_START_ADDRESS  /// xSPI1 memory
#else
    #define QSPI_DEVICE_START_ADDRESS_MIRROR (0x40000000)  /// xSPI0_CS0 memory (Mirror area)
    #define QSPI_DEVICE_START_ADDRESS        (0x60000000)  /// xSPI0_CS0 memory
#endif
#define QSPI_DEVICE_SECTOR_SIZE          (4096)
#define QSPI_WRITE_SIZE_TEMP             (64)  //TODO @@ countermeasure against build error
#if defined(OPENER_CIP_SAFETY)
#define QSPI_ARRAY_SIZE_TEMP             (256)
#else
#define QSPI_ARRAY_SIZE_TEMP             (64)
#endif
#define WRITE_ADDRESS_FOR_QOS            (0x00200000)
#define WRITE_ADDRESS_FOR_TCPIP          (0x00201000)
#define WRITE_ADDRESS_FOR_CID            (0x00202000)
#if defined(OPENER_CIP_SAFETY)
#define WRITE_ADDRESS_FOR_SAFETYSUPERVISOR (0x00203000)
#endif
#define NVDATA_IS_CHANGED                (0xD1)        /// To avoid coincidence with old programs, define this to a value that is not a simple 1.
#define NVDATA_IS_DEFAULT                (0xD0)

/***********************************************************************************************************************
 * Private constants
 **********************************************************************************************************************/
/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/**
 * Structure of controller for example application utilizing the OpENer porting module.
 */
typedef struct st_user_app_example_ctrl
{
    opener_port_instance_t const *    p_opener_port_instance;               ///< Pointer to controller of OpENer port module
    opener_port_callback_link_node_t  callback_link_node;                   ///< Callback link node.
    opener_port_cip_tcpip_nvdata_t    pseudo_nvstorage_for_tcpip_nvdata;    ///< Pseudo Non-volatile storage for TCP/IP interface non-volatile data.
    opener_port_cip_qos_nvdata_t      pseudo_nvstorage_for_qos_nvdata;      ///< Pseudo Non-volatile storage for QoS non-volatile data.
#if defined(OPENER_CIP_SAFETY)
    opener_port_cip_safetysupervisor_nvdata_t pseudo_nvstorage_for_safetysupervisor_nvdata;      ///< Pseudo Non-volatile storage for Safety Supervisor non-volatile data.
#endif
} user_app_example_ctrl_t;

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
/**
 * Callback function which OpENer porting module calls when some event occurs such as requesting user to access Module/Network status module.
 */
static void _user_callback_func( opener_port_callback_args_t * );

/**
 * Callback function which OpENer calls when non-volatile attribute is changed by Set_Attribute_Single service.
 */
static EipStatus _callback_qos_nvdata_set( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service );
static EipStatus _callback_tcpip_nvdata_set( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service );
#if defined(OPENER_CIP_SAFETY)
static EipStatus _callback_safetysupervisor_nvdata_set( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service );
#endif

/**
 * Functions for Flash Write/Read
 */
static void _restore_cip_nvdata (void);
static void _qspi_erase_nvdata_sector(spi_flash_ctrl_t    * p_ctrl,
                                      uint8_t * const       p_dest,
                                      uint32_t              byte_count);
static void _qspi_write_nvdata(spi_flash_ctrl_t    * p_ctrl,
                               void const * const    p_src,
                               uint8_t * const       p_dest,
                               uint32_t              byte_count);
static void _qspi_read_nvdata(spi_flash_ctrl_t    * p_ctrl,
                              void *                p_src,
                              uint8_t * const       p_dest,
                              uint8_t               byte_count);

/**
 * Utility function to access LED and SW pin on the target board.
 */
static usr_err_t _set_indicator_led( bsp_io_port_pin_t io_port_pin, opener_port_indicator_flash_state_t indicator_state );
static usr_err_t _set_pin_array( uint8_t const * const p_byte_array,
                                 size_t const byte_array_size,
                                 bsp_io_port_pin_t const * const p_pin_array,
                                 bsp_io_level_t const * const p_pin_inactive_levels,
                                 size_t const num_pins);
static usr_err_t _get_pin_array( uint8_t * const byte_array,
                                 size_t const byte_array_size,
                                 bsp_io_port_pin_t const * const p_pin_array,
                                 bsp_io_level_t const * const p_pin_inactive_levels,
                                 size_t const num_pins);
static bsp_io_level_t _pin_read (bsp_io_port_pin_t io_port_pin);
static void _pin_write (bsp_io_port_pin_t io_port_pin, uint8_t set_value);

/**
 * Utility function for Reset
 */
static void _reset_timer_set(void);
#if defined(EXECUTE_SYSTEM_RESET_PROCESS)
static void OsRebootService (void);
#endif
static void StoreNvData(void);
#if defined(UM_OPENER_PORT_FEATURE_DLR) && UM_OPENER_PORT_FEATURE_DLR
void dlr_beacon_enable(void);
#endif

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
/**
 * Instance of structure of controller for example application.
 */
static user_app_example_ctrl_t g_user_app_example0_ctrl;
static user_app_example_ctrl_t * const gp_user_app_example0_ctrl = &g_user_app_example0_ctrl;

/**
 * Callback data storages for opener port process
 */
static opener_port_callback_args_t g_user_callback_memory;

/**
 * Pseudo Non-volatile storages for CIP object which has non-volatile attribute.
 */
static CipQosObject   g_pseudo_nvstorage_for_qos_object;      ///< Pseudo Non-volatile storage for QoS Object.
static CipTcpIpObject g_pseudo_nvstorage_for_tcpip_object;    ///< Pseudo Non-volatile storage for TCP/IP Interface Object
#if defined(OPENER_CIP_SAFETY)
static CipSafetySupervisorObject g_pseudo_nvstorage_for_safetysupervisor_object; ///< Pseudo Non-volatile storage for Safety Supervisor Object.
#endif

/**
 * Buffers assembled to Assembly object instances.
 */
static EipUint8 g_assembly_data_led_input[DEMO_ASSEMBLY_LED_INPUT_BYTES]     = {0}; /** Input */
static EipUint8 g_assembly_data_sw_input[DEMO_ASSEMBLY_SW_INPUT_BYTES]       = {0}; /** Input */
static EipUint8 g_assembly_data_led_output[DEMO_ASSEMBLY_LED_OUTPUT_BYTES]   = {0}; /** Output */
static EipUint8 g_assembly_data_config[DEMO_ASSEMBLY_CONFIG_BYTES]           = {0}; /** Configuration */
static EipUint8 g_assembly_data_explicit[DEMO_ASSEMBLY_EXPLICT_BYTES]        = {0}; /** Explicit */
#if defined(OPENER_CIP_SAFETY)
static EipUint8 g_assembly_data_safety_input[SAFETY_ASSEMBLY_INPUT_BYTES]    = {0}; /** Safety Input */
static EipUint8 g_assembly_data_safety_output[SAFETY_ASSEMBLY_OUTPUT_BYTES]  = {0}; /** Safety Output */
#endif

/**
 * Arrays for efficiently accessing LED/SW pins.
 */
static const bsp_io_port_pin_t g_led_pin_array[DEMO_APP_PIN_ARRAY_LED_PIN_NUM] =
{
#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZT2ME_RSK)
     BOARD_LED0_GREEN,
     BOARD_LED1_YELLOW,
     BOARD_LED2_RED,
     BOARD_LED3_RED,
#elif defined(BOARD_RZT2H_EVB)
     BOARD_LED0_GREEN,
     BOARD_LED1_GREEN,
     BOARD_LED7_ESC_LA1_GREEN,
     BOARD_LED8_ESC_LA2_GREEN,
#elif defined(BOARD_RZN2H_EVB)
     BOARD_LED5_GREEN,
     BOARD_LED6_GREEN,
     BOARD_LED7_GREEN,
     BOARD_LED8_GREEN,
#elif defined(BOARD_RZN2L_RSK)
     BOARD_LED0_GREEN,
     BOARD_LED1_YELLOW,
     BOARD_LED2_RED,
     BOARD_LED3_RED,
#else
     BOARD_PIN_DISABLED,
     BOARD_PIN_DISABLED,
     BOARD_PIN_DISABLED,
     BOARD_PIN_DISABLED,
#endif
};

static const bsp_io_port_pin_t g_sw_pin_array[DEMO_APP_PIN_ARRAY_SW_PIN_NUM] =
{
#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZT2ME_RSK)
     BOARD_SW3_PIN0,
     BOARD_SW3_PIN1,
     BOARD_SW3_PIN2,
     BOARD_SW3_PIN3,
#elif defined(BOARD_RZT2H_EVB)
     BOARD_SW12_PIN1,
     BOARD_SW12_PIN2,
     BOARD_SW12_PIN3,
     BOARD_SW12_PIN3,
#elif defined(BOARD_RZN2H_EVB)
     BOARD_DSW1_PIN1,
     BOARD_DSW1_PIN2,
     BOARD_DSW1_PIN3,
     BOARD_DSW1_PIN4,
#elif defined(BOARD_RZN2L_RSK)
     BOARD_SW3_PIN0,
     BOARD_SW3_PIN1,
     BOARD_SW3_PIN2,
     BOARD_SW3_PIN3,
#else
     BOARD_PIN_DISABLED,
     BOARD_PIN_DISABLED,
     BOARD_PIN_DISABLED,
     BOARD_PIN_DISABLED,
#endif
};

static const bsp_io_level_t g_led_pin_array_incactive_levels[DEMO_APP_PIN_ARRAY_LED_PIN_NUM] =
{
#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZT2ME_RSK)
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
#elif defined(BOARD_RZT2H_EVB)
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
#elif defined(BOARD_RZN2H_EVB)
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
#elif defined(BOARD_RZN2L_RSK)
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
#else
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
     BOARD_IOLEVEL_LED_OFF,
#endif
};

static const bsp_io_level_t g_sw_pin_array_incactive_levels[DEMO_APP_PIN_ARRAY_SW_PIN_NUM] =
{
#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZT2ME_RSK)
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
#elif defined(BOARD_RZT2H_EVB)
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
#elif defined(BOARD_RZN2H_EVB)
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
#elif defined(BOARD_RZN2L_RSK)
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
#else
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
     BOARD_IOLEVEL_SW_OFF,
#endif
};

#if defined(OPENER_CIP_SAFETY)
static uint8_t toggle_cout[4] = {0};
#endif

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/**
 * Extern declaration of OpENer porting module from opener_port_intance.c
 */
extern opener_port_instance_t const * gp_opener_port0;

/**
 * Define the errno for lwIP BSD socket interface (EWARM only)
 */
#if defined(__ICCARM__)
int errno;
#endif

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Main function to launching OpENer and its porting module.
 **********************************************************************************************************************/
void opener_port_user_main( void );
void opener_port_user_main( void )
{
    /** Error code */
    usr_err_t usr_err;

    /** Load controller of example application. */
    user_app_example_ctrl_t * p_ctrl = gp_user_app_example0_ctrl;

    /** Initialize OpENer port module and related modules. */
    extern void opener_port_user_instance_init( void );
    (void) opener_port_user_instance_init();

    /** Enabled the printf() function via SCI UART. */
    USR_DEBUG_PRINT( "\n" );
    USR_DEBUG_PRINT( "/** Started sample application (v%d.%d) for OpENer Port. **/",
                     SAMPLE_APP_VERSION_MAJOR, SAMPLE_APP_VERSION_MINOR);
    USR_DEBUG_PRINT( "\n" );

    /** Load instance of OpENer porting module. */
    p_ctrl->p_opener_port_instance = gp_opener_port0;

    /** Set callback function to OpENer porting module  */
    p_ctrl->callback_link_node.p_context = p_ctrl;
    p_ctrl->callback_link_node.p_func = _user_callback_func;
    p_ctrl->callback_link_node.p_memory = &g_user_callback_memory;
    usr_err = p_ctrl->p_opener_port_instance->p_api->callbackAdd( p_ctrl->p_opener_port_instance->p_ctrl,
                                                                  &p_ctrl->callback_link_node );
    if( USR_SUCCESS != usr_err )
    {
        USR_DEBUG_PRINT( "Failed to add callback. \n" );
        USR_DEBUG_BLOCK_CPU();
    }

    /** Start OpENer porting module. */
    usr_err = p_ctrl->p_opener_port_instance->p_api->start( p_ctrl->p_opener_port_instance->p_ctrl );
    if( USR_SUCCESS != usr_err )
    {
        USR_DEBUG_PRINT( "Failed to start OpENer port. \n" );
        USR_DEBUG_BLOCK_CPU();
    }

    /** Apply Module Status to Operational. */
    usr_err = p_ctrl->p_opener_port_instance->p_api->moduleStateSet( p_ctrl->p_opener_port_instance->p_ctrl,
                                                                     OPENER_PORT_MODULE_STATE_DEVICE_OPERATIONAL );
    if( USR_SUCCESS != usr_err )
    {
        USR_DEBUG_PRINT( "Failed to set module state \n" );
        USR_DEBUG_BLOCK_CPU();
    }

#if defined(UM_OPENER_PORT_FEATURE_DLR) && UM_OPENER_PORT_FEATURE_DLR
    /**
     * The function is a provisional measure to pass official testing.
     * To avoid the problem, the Beacon frame transfer permission process is performed here.
     *   Problem: In DLR topology, Sign_On frame may not be sent from RZ after returning from Type 0 Reset.
     * We plan to update the measure and remove the provisional code in a future update.
     */
    dlr_beacon_enable();
#endif

    /** End of task. */
    while(1)
    {
        vTaskDelay( portMAX_DELAY );
    }
}

/***********************************************************************************************************************
 * Implementation of OpENer callback API (declared in opener_api.h)
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Callback for the application initialization
 *
 * Please see the detail in opener_api.h
 **********************************************************************************************************************/
EipStatus ApplicationInitialization (void)
{
    /** Create Assembly objects to be accessed by implicit connection. */
    (void) CreateAssemblyObject(DEMO_ASSEMBLY_LED_INPUT, g_assembly_data_led_input, sizeof(g_assembly_data_led_input));
    (void) CreateAssemblyObject(DEMO_ASSEMBLY_CONFIG, g_assembly_data_config, sizeof(g_assembly_data_config));
    (void) CreateAssemblyObject(DEMO_ASSEMBLY_SW_INPUT, g_assembly_data_sw_input, sizeof(g_assembly_data_sw_input));
    (void) CreateAssemblyObject(DEMO_ASSEMBLY_LED_OUTPUT, g_assembly_data_led_output, sizeof(g_assembly_data_led_output));
    (void) CreateAssemblyObject(DEMO_ASSEMBLY_HEARTBEAT_FOR_INPUT_ONLY, NULL, 0);
    (void) CreateAssemblyObject(DEMO_ASSEMBLY_HEARTBEAT_FOR_LISTEN_ONLY, NULL, 0);
#if defined(OPENER_CIP_SAFETY)
    (void) CreateAssemblyObject(SAFETY_ASSEMBLY_INPUT, g_assembly_data_safety_input, sizeof(g_assembly_data_safety_input));
    (void) CreateAssemblyObject(SAFETY_ASSEMBLY_OUTPUT, g_assembly_data_safety_output, sizeof(g_assembly_data_safety_output));
    (void) CreateAssemblyObject(OPENER_SAFETYSUPERVISOR_ASSEMBLY_CONFIG, g_safetysupervisor.config_data, sizeof(g_safetysupervisor.config_data));
    (void) CreateAssemblyObject(SAFETY_ASSEMBLY_NULL, NULL, 0);
#endif

    /** Create Assembly object to be accessed by explicit connection only. */
    (void) CreateAssemblyObject(DEMO_ASSEMBLY_EXPLICT, g_assembly_data_explicit, sizeof(g_assembly_data_explicit));

    /**
     * Exclusive Owner Connection for LED I/O.
     * Configure Connection Points as Exclusive Owner connection (Vol.1 3-6.5.3 Exclusive Owner)
     *  T->O: application data
     *  O->T: application data
     */
    ConfigureExclusiveOwnerConnectionPoint(DEMO_CONNECTION_LED_EXCLUSIVE_OWNER,
                                           DEMO_ASSEMBLY_LED_OUTPUT,
                                           DEMO_ASSEMBLY_LED_INPUT,
                                           DEMO_ASSEMBLY_CONFIG);

    /**
     * Input Only Connection for SW input
     * Configure Connection Points as Input Only Connection. (Vol.1 3-6.6 Input Only)
     *  T->O: application data
     *  O->T: heart beat
     */
    ConfigureInputOnlyConnectionPoint(DEMO_CONNECTION_SW_INPUT_ONLY,
                                      DEMO_ASSEMBLY_HEARTBEAT_FOR_INPUT_ONLY,
                                      DEMO_ASSEMBLY_SW_INPUT,
                                      DEMO_ASSEMBLY_CONFIG);

    /**
     * Listen Only Connection for LED input of Exclusive Owner Connection.
     * Configure Connection Points as Input Listen Only Connection. (Vol.1 3-6.5 Listen Only)
     *  T->O: application data
     *  O->T: heart beat
     */
    ConfigureListenOnlyConnectionPoint(DEMO_CONNECTION_LED_LISTEN_ONLY,
                                       DEMO_ASSEMBLY_HEARTBEAT_FOR_LISTEN_ONLY,
                                       DEMO_ASSEMBLY_LED_INPUT,
                                       DEMO_ASSEMBLY_CONFIG);

#if defined(OPENER_CIP_SAFETY)
    /**
     * Input Only Connection for Safety input
     * Configure Connection Points as Input Only Connection. (Vol.1 3-6.6 Input Only)
     *  T->O: application data
     *  O->T: NULL
     */
    ConfigureInputOnlyConnectionPoint(SAFETY_CONNECTION_INPUT_INPUT_ONLY,
                                      SAFETY_ASSEMBLY_NULL,
                                      SAFETY_ASSEMBLY_INPUT,
                                      OPENER_SAFETYSUPERVISOR_ASSEMBLY_CONFIG);

    /**
     * Exclusive Owner Connection for Safety Output.
     * Configure Connection Points as Exclusive Owner connection (Vol.1 3-6.5.3 Exclusive Owner)
     *  T->O: NULL
     *  O->T: application data
     */
    ConfigureExclusiveOwnerConnectionPoint(SAFETY_CONNECTION_OUTPUT_EXCLUSIVE_OWNER,
                                           SAFETY_ASSEMBLY_OUTPUT,
                                           SAFETY_ASSEMBLY_NULL,
                                           OPENER_SAFETYSUPERVISOR_ASSEMBLY_CONFIG);
#endif

    /** Get pointer to TCP/IP and QoS CIP class  */
    CipClass * p_tcpip_class = GetCipClass(kCipTcpIpInterfaceClassCode);
    CipClass * p_qos_class = GetCipClass(kCipQoSClassCode);
#if defined(OPENER_CIP_SAFETY)
    CipClass * p_safetysupervisor_class = GetCipClass(kCipSafetySupervisorClassCode);
#endif

    /** Set callback function of TCP/IP and QoS class to indicate that settable non-volatile data has been accessed by EtherNet/IP controller. */
    InsertGetSetCallback(p_tcpip_class, _callback_tcpip_nvdata_set, kPostSetFunc|kNvDataFunc);
    InsertGetSetCallback(p_qos_class, _callback_qos_nvdata_set, kPostSetFunc|kNvDataFunc);
#if defined(OPENER_CIP_SAFETY)
    InsertGetSetCallback(p_safetysupervisor_class, _callback_safetysupervisor_nvdata_set, kPostSetFunc|kNvDataFunc);
#endif

    /** Restore CIP NV data */
    _restore_cip_nvdata();  //There may be a more appropriate place for this process.

    return kEipStatusOk;
}

/*******************************************************************************************************************//**
 * @brief Allow the device specific application to perform its execution
 *
 * Please see the detail in opener_api.h
 **********************************************************************************************************************/
void HandleApplication (void)
{
    /**
     * This function is called cyclically every kOpenerTimerTickInMilliSeconds.
     */
}

/*******************************************************************************************************************//**
 * @brief Inform the application on changes occurred for a connection
 *
 * Please see the detail in opener_api.h
 **********************************************************************************************************************/
void CheckIoConnectionEvent (unsigned int      output_assembly_id,
                             unsigned int      input_assembly_id,
                             IoConnectionEvent io_connection_event)
{
    /** Unused parameter */
    (void) output_assembly_id;
    (void) input_assembly_id;

    /** Switch depending on event of I/O connection. */
    switch (io_connection_event)
    {
    case kIoConnectionEventOpened:
        break;
    case kIoConnectionEventTimedOut:
        break;
    case kIoConnectionEventClosed:
        break;
    }
}

/*******************************************************************************************************************//**
 * @brief Call back function to inform application on received data for an assembly object.
 *
 * Please see the detail in opener_api.h
 **********************************************************************************************************************/
EipStatus AfterAssemblyDataReceived (CipInstance * instance)
{
    /**
     * Check the instance number of Assembly object isntance.
     */
    switch (instance->instance_number)
    {
    case DEMO_ASSEMBLY_LED_OUTPUT:
        /** Write received assembly data into LED. */
        _set_pin_array(g_assembly_data_led_output,
                       DEMO_ASSEMBLY_LED_OUTPUT_BYTES,
                       g_led_pin_array,
                       g_led_pin_array_incactive_levels,
                       DEMO_APP_PIN_ARRAY_LED_PIN_NUM);
        break;
    default:
        break;
    }

    /** Return success code. */
    return kEipStatusOk;
}

/*******************************************************************************************************************//**
 * @brief Inform the application that the data of an assembly object will be sent.
 *
 * Please see the detail in opener_api.h
 **********************************************************************************************************************/
EipBool8 BeforeAssemblyDataSend (CipInstance * pa_pstInstance)
{
    EipBool8 has_changed = false;

    /** Check instance number */
    switch( pa_pstInstance->instance_number )
    {
    case DEMO_ASSEMBLY_LED_INPUT:

        /** Read LED array data to assembly data */
        _get_pin_array(g_assembly_data_led_input,
                       DEMO_ASSEMBLY_LED_INPUT_BYTES,
                       g_led_pin_array,
                       g_led_pin_array_incactive_levels,
                       DEMO_APP_PIN_ARRAY_LED_PIN_NUM);
        has_changed = true;
        break;

    case DEMO_ASSEMBLY_SW_INPUT:

        /** Ready SW array data to assembly data */
        _get_pin_array(g_assembly_data_sw_input,
                       DEMO_ASSEMBLY_SW_INPUT_BYTES,
                       g_sw_pin_array,
                       g_sw_pin_array_incactive_levels,
                       DEMO_APP_PIN_ARRAY_SW_PIN_NUM);
        has_changed = true;
        break;

    default:
        break;
    }

    return has_changed;
}

/*******************************************************************************************************************//**
 * @brief Emulate as close a possible a power cycle of the device
 *
 * Please see the detail in opener_api.h
 **********************************************************************************************************************/
EipStatus ResetDevice (void)
{
    /** Enable Object parameters which should be enabled when Reset Service is required. */
    CipQosUpdateUsedSetQosValues(); ///< QoS DSCP values (#2 ~ #8) of QoS object instance.

    /** Close All connections */
    CloseAllConnections();

    /** Reset Applications */
    memset(g_assembly_data_led_input,  0, DEMO_ASSEMBLY_LED_INPUT_BYTES);
    memset(g_assembly_data_sw_input,   0, DEMO_ASSEMBLY_SW_INPUT_BYTES);
    memset(g_assembly_data_led_output, 0, DEMO_ASSEMBLY_LED_OUTPUT_BYTES);
    memset(g_assembly_data_config,     0, DEMO_ASSEMBLY_CONFIG_BYTES);
    memset(g_assembly_data_explicit,   0, DEMO_ASSEMBLY_EXPLICT_BYTES);
#if defined(OPENER_CIP_SAFETY)
    memset(g_assembly_data_safety_input,  0, SAFETY_ASSEMBLY_INPUT_BYTES);
    memset(g_assembly_data_safety_output, 0, SAFETY_ASSEMBLY_OUTPUT_BYTES);
#endif

    /** Reset Hardware and CPU */
    _reset_timer_set();

    /** Return success code */
    return kEipStatusOk;
}

/*******************************************************************************************************************//**
 * @brief Reset the device to the initial configuration and emulate as close as possible a power cycle of the device
 *
 * Please see the detail in opener_api.h
 **********************************************************************************************************************/
#if defined(OPENER_CIP_SAFETY)
EipStatus ResetDeviceToInitialConfiguration (CipUsint attribute_bit_map)
#else
EipStatus ResetDeviceToInitialConfiguration (void)
#endif
{
    uint8_t * dest;

    /** Reset QoS DSCP value (#2 ~ #8) of QoS object instance to default value. */
    CipQosResetAttributesToDefaultValues();
    opener_port_cip_qos_nvdata_t temp_qos_values = {
        .dscp_change_flag = NVDATA_IS_DEFAULT,
        .dscp_urgent      = g_qos.dscp.urgent,
        .dscp_scheduled   = g_qos.dscp.scheduled,
        .dscp_high        = g_qos.dscp.high,
        .dscp_low         = g_qos.dscp.low,
        .dscp_explict     = g_qos.dscp.explicit_msg
    };

    /** Write NV Data to Flash */
    _qspi_erase_nvdata_sector(&g_qspi0_ctrl, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + WRITE_ADDRESS_FOR_QOS), QSPI_DEVICE_SECTOR_SIZE);
    dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_QOS);
    _qspi_write_nvdata(&g_qspi0_ctrl, &temp_qos_values, dest, sizeof(opener_port_cip_qos_nvdata_t));

    /** Reset encapsulation inactivity timeout (#13) of TCP/IP interface object instance to default value. */
    opener_port_cip_tcpip_nvdata_t tmp_tcpip_values = {
        .tcp_change_flag = NVDATA_IS_DEFAULT,
        .encapsulation_inactivity_timeout_sec = 120,
        .arp_pdu = {0},
        .remote_mac = {0},
        .acd_activity = 0,
        .select_acd = true,
        .config_method = LWIP_PORT_DHCP_DISABLE,
    };

    /** Write NV Data to Flash */
    _qspi_erase_nvdata_sector(&g_qspi0_ctrl, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + WRITE_ADDRESS_FOR_TCPIP), QSPI_DEVICE_SECTOR_SIZE);
    dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_TCPIP);
    _qspi_write_nvdata(&g_qspi0_ctrl, &tmp_tcpip_values, dest, sizeof(opener_port_cip_tcpip_nvdata_t));

#if defined(OPENER_CIP_SAFETY)
    opener_port_cip_safetysupervisor_nvdata_t temp_safetysupervisor_values = {
        .safetysupervisor_change_flag = NVDATA_IS_DEFAULT,
        /* Password */
        .password = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        /* Alarm Enable */
        .alarm_enable = 0x01,
        /* Warning Enable */
        .warning_enable = 0x01,
        /* Configuration Lock */
        .config_lock = 0x00,
        /* Configuration UNID */
        /* SRS134 */
        .cfunid.snn.time_of_day = 0x00000000,
        .cfunid.snn.date = 0x0000,
        .cfunid.nodeid = 0x00000000,
        /* Safety Configuration Identifier */
        /* SRS184 */
        .scid.sccrc = 0x00000000,
        .scid.scts.time_of_day = 0x00000000,
        .scid.scts.date = 0x0000,
        /* Target UNID */
        /* SRS117 */
        .tunid.snn.time_of_day = 0xFFFFFFFF,
        .tunid.snn.date = 0xFFFF,
        .tunid.nodeid = 0xFFFFFFFF,
        /* Output Connection Point Owners */
        /* SRS201 */
        .output_connection_owners.num = 1,
        .output_connection_owners.output_owners[0].ocpunid.snn.time_of_day = 0x00000000,
        .output_connection_owners.output_owners[0].ocpunid.snn.date = 0x0000,
        .output_connection_owners.output_owners[0].ocpunid.nodeid = 0x00000000,
        /* Config Data */
        .config_data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    };

#if 1 /* Type 2 Safety Reset bit inverted */
    if ((attribute_bit_map & OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_2) == 0) {
#else
    if ((attribute_bit_map & OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_2) == OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_2) {
#endif
        /* TUNID */
        temp_safetysupervisor_values.safetysupervisor_change_flag = NVDATA_IS_CHANGED,
        temp_safetysupervisor_values.tunid.snn.time_of_day = g_safetysupervisor.tunid.snn.time_of_day;
        temp_safetysupervisor_values.tunid.snn.date = g_safetysupervisor.tunid.snn.date;
        temp_safetysupervisor_values.tunid.nodeid = g_safetysupervisor.tunid.nodeid;
    }
#if 1 /* Type 2 Safety Reset bit inverted */
    if ((attribute_bit_map & OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_3) == 0) {
#else
    if ((attribute_bit_map & OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_3) == OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_3) {
#endif
        /* Password */
        temp_safetysupervisor_values.safetysupervisor_change_flag = NVDATA_IS_CHANGED,
        memcpy(temp_safetysupervisor_values.password, g_safetysupervisor.password, sizeof(g_safetysupervisor.password));
    }
#if 1 /* Type 2 Safety Reset bit inverted */
    if ((attribute_bit_map & OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_4) == 0) {
#else
    if ((attribute_bit_map & OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_4) == OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_4) {
#endif
        /* CFUNID */
        temp_safetysupervisor_values.safetysupervisor_change_flag = NVDATA_IS_CHANGED,
        temp_safetysupervisor_values.cfunid.snn.time_of_day = g_safetysupervisor.cfunid.snn.time_of_day;
        temp_safetysupervisor_values.cfunid.snn.date = g_safetysupervisor.cfunid.snn.date;
        temp_safetysupervisor_values.cfunid.nodeid = g_safetysupervisor.cfunid.nodeid;
    }
#if 1 /* Type 2 Safety Reset bit inverted */
    if ((attribute_bit_map & OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_5) == 0) {
#else
    if ((attribute_bit_map & OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_5) == OPENER_SAFETYSUPERVISOR_RESET_ATTR_BIT_5) {
#endif
        /* OCPUNID */
        temp_safetysupervisor_values.safetysupervisor_change_flag = NVDATA_IS_CHANGED,
        temp_safetysupervisor_values.output_connection_owners.num = g_safetysupervisor.output_connection_owners.num;
        temp_safetysupervisor_values.output_connection_owners.output_owners[0].ocpunid.snn.time_of_day = g_safetysupervisor.output_connection_owners.output_owners[0].ocpunid.snn.time_of_day;
        temp_safetysupervisor_values.output_connection_owners.output_owners[0].ocpunid.snn.date = g_safetysupervisor.output_connection_owners.output_owners[0].ocpunid.snn.date;
        temp_safetysupervisor_values.output_connection_owners.output_owners[0].ocpunid.nodeid = g_safetysupervisor.output_connection_owners.output_owners[0].ocpunid.nodeid;
    }

    /** Write NV Data to Flash */
    _qspi_erase_nvdata_sector(&g_qspi0_ctrl, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + WRITE_ADDRESS_FOR_SAFETYSUPERVISOR), QSPI_DEVICE_SECTOR_SIZE);
    dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_SAFETYSUPERVISOR);
    _qspi_write_nvdata(&g_qspi0_ctrl, &temp_safetysupervisor_values, dest, sizeof(opener_port_cip_safetysupervisor_nvdata_t));
#endif

    /** Perform Type 0: Reset */
    ResetDevice();

    /** Return success code */
    return kEipStatusOk;
}

/*******************************************************************************************************************//**
 * @brief Inform the application that the Run/Idle State has been changed
 *
 * Please see the detail in opener_api.h
 **********************************************************************************************************************/
void RunIdleChanged (EipUint32 run_idle_value)
{
    /** Check run/idle state */
    if ((0x0001 & run_idle_value) == 1)
    {
        /** Set device status to run mode */
        CipIdentitySetExtendedDeviceStatus(kAtLeastOneIoConnectionInRunMode);
    }
    else
    {
        /** Set device status to idle mode */
        CipIdentitySetExtendedDeviceStatus(kAtLeastOneIoConnectionEstablishedAllInIdleMode);
    }
}

/***********************************************************************************************************************
 * Private functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Callback function invoked when the CIP TCP/IP interface object non-volatile attribute is accessed by Set_Attribute_Single service.
 **********************************************************************************************************************/
static EipStatus _callback_tcpip_nvdata_set( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service )
{
    /** Unused parameter */
    (void) p_instance;
    (void) p_attribute;
    (void) service;

    /** Resolve example application context. */
    user_app_example_ctrl_t * p_ctrl = gp_user_app_example0_ctrl;

    /** For example, save the object instance directly. */
    g_pseudo_nvstorage_for_tcpip_object = g_tcpip;

    /** For example, save the settable nvdata from OpENer port module. */
    opener_port_cip_settable_nvdata_t settable_nvdata;
    settable_nvdata.p_qos_nvdata   = &p_ctrl->pseudo_nvstorage_for_qos_nvdata;
    settable_nvdata.p_tcpip_nvdata = &p_ctrl->pseudo_nvstorage_for_tcpip_nvdata;
#if defined(OPENER_CIP_SAFETY)
    settable_nvdata.p_safetysupervisor_nvdata = &p_ctrl->pseudo_nvstorage_for_safetysupervisor_nvdata;
#endif
    (void) p_ctrl->p_opener_port_instance->p_api->settableNvdataGet( p_ctrl->p_opener_port_instance->p_ctrl, &settable_nvdata );

    /** Write NV Data to Flash */
    settable_nvdata.p_tcpip_nvdata->tcp_change_flag = NVDATA_IS_CHANGED;
    _qspi_erase_nvdata_sector(&g_qspi0_ctrl, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + WRITE_ADDRESS_FOR_TCPIP), QSPI_DEVICE_SECTOR_SIZE);
    uint8_t * dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_TCPIP);
    _qspi_write_nvdata(&g_qspi0_ctrl, settable_nvdata.p_tcpip_nvdata, dest, sizeof(opener_port_cip_tcpip_nvdata_t));

    /** Return success code */
    return kEipStatusOk;
}

/*******************************************************************************************************************//**
 * @brief Callback function invoked when the CIP QoS object non-volatile attribute is accessed by Set_Attribute_Single service.
 **********************************************************************************************************************/
static EipStatus _callback_qos_nvdata_set( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service )
{
    /** Unused parameter */
    (void) p_instance;
    (void) p_attribute;
    (void) service;

    /** Resolve example application context. */
    user_app_example_ctrl_t * p_ctrl = gp_user_app_example0_ctrl;

    /** For example, save the object instance directly. */
    g_pseudo_nvstorage_for_qos_object = g_qos;

    /** For example, save the settable nvdata from OpENer port module. */
    opener_port_cip_settable_nvdata_t settable_nvdata;
    settable_nvdata.p_qos_nvdata   = &p_ctrl->pseudo_nvstorage_for_qos_nvdata;
    settable_nvdata.p_tcpip_nvdata = &p_ctrl->pseudo_nvstorage_for_tcpip_nvdata;
#if defined(OPENER_CIP_SAFETY)
    settable_nvdata.p_safetysupervisor_nvdata = &p_ctrl->pseudo_nvstorage_for_safetysupervisor_nvdata;
#endif
    (void) p_ctrl->p_opener_port_instance->p_api->settableNvdataGet( p_ctrl->p_opener_port_instance->p_ctrl, &settable_nvdata );

    /** Write NV Data to Flash */
    settable_nvdata.p_qos_nvdata->dscp_change_flag = NVDATA_IS_CHANGED;
    _qspi_erase_nvdata_sector(&g_qspi0_ctrl, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + WRITE_ADDRESS_FOR_QOS), QSPI_DEVICE_SECTOR_SIZE);
    uint8_t * dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_QOS);
    _qspi_write_nvdata(&g_qspi0_ctrl, settable_nvdata.p_qos_nvdata, dest, sizeof(opener_port_cip_qos_nvdata_t));

    /** Return success code */
    return kEipStatusOk;
}

#if defined(OPENER_CIP_SAFETY)
/*******************************************************************************************************************//**
 * @brief Callback function invoked when the CIP Safety Supervisor object non-volatile attribute is accessed by each service.
 **********************************************************************************************************************/
static EipStatus _callback_safetysupervisor_nvdata_set( CipInstance *const p_instance, CipAttributeStruct *const p_attribute, CipByte service )
{
    /** Unused parameter */
    (void) p_instance;
    (void) p_attribute;
    (void) service;

    /** Resolve example application context. */
    user_app_example_ctrl_t * p_ctrl = gp_user_app_example0_ctrl;

    /** For example, save the object instance directly. */
    g_pseudo_nvstorage_for_safetysupervisor_object = g_safetysupervisor;

    /** For example, save the settable nvdata from OpENer port module. */
    opener_port_cip_settable_nvdata_t settable_nvdata;
    settable_nvdata.p_qos_nvdata   = &p_ctrl->pseudo_nvstorage_for_qos_nvdata;
    settable_nvdata.p_tcpip_nvdata = &p_ctrl->pseudo_nvstorage_for_tcpip_nvdata;
    settable_nvdata.p_safetysupervisor_nvdata = &p_ctrl->pseudo_nvstorage_for_safetysupervisor_nvdata;
    (void) p_ctrl->p_opener_port_instance->p_api->settableNvdataGet( p_ctrl->p_opener_port_instance->p_ctrl, &settable_nvdata );

    /** Write NV Data to Flash */
    settable_nvdata.p_safetysupervisor_nvdata->safetysupervisor_change_flag = NVDATA_IS_CHANGED;
    _qspi_erase_nvdata_sector(&g_qspi0_ctrl, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + WRITE_ADDRESS_FOR_SAFETYSUPERVISOR), QSPI_DEVICE_SECTOR_SIZE);
    uint8_t * dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_SAFETYSUPERVISOR);
    _qspi_write_nvdata(&g_qspi0_ctrl, settable_nvdata.p_safetysupervisor_nvdata, dest, sizeof(opener_port_cip_safetysupervisor_nvdata_t));

    /** Return success code */
    return kEipStatusOk;
}
#endif

/*******************************************************************************************************************//**
 * @brief Example callback function to handle the notification from OpENer port module.
 * Please don't block in callback function context.
 **********************************************************************************************************************/
static void _user_callback_func( opener_port_callback_args_t * p_callback_args )
{
    /** Check the callback event. */
    switch( p_callback_args->event )
    {
        /** Require user application to change module indication LED on user board. */
        case OPENER_PORT_CALLBACK_CHANGE_MODULE_STATUS_LED:
            R_BSP_PinAccessEnable();
            _set_indicator_led( BOARD_PIN_MOD_GREEN, p_callback_args->indicator_status.green );
            _set_indicator_led( BOARD_PIN_MOD_RED,   p_callback_args->indicator_status.red );
            R_BSP_PinAccessDisable();
            break;

        /** Require user application to change network indication LED on user board. */
        case OPENER_PORT_CALLBACK_CHANGE_NETWORK_STATUS_LED:
            R_BSP_PinAccessEnable();
            _set_indicator_led( BOARD_PIN_NET_GREEN, p_callback_args->indicator_status.green );
            _set_indicator_led( BOARD_PIN_NET_RED,   p_callback_args->indicator_status.red);
            R_BSP_PinAccessDisable();
            break;

        /** Require user application to store TCP/IP settable non-volatile data. */
        case OPENER_PORT_CALLBACK_STORE_NVDATA_FOR_TCP:
            _callback_tcpip_nvdata_set(NULL, NULL, 0);
            break;
    }
}

/*******************************************************************************************************************//**
 * @brief Restore CIP NV Data at power cycle
 **********************************************************************************************************************/
static void _restore_cip_nvdata (void)
{
    /** Resolve example application context. */
    user_app_example_ctrl_t * p_ctrl = gp_user_app_example0_ctrl;

    /** For example, save the settable nvdata from OpENer port module. */
    opener_port_cip_settable_nvdata_t settable_nvdata;
    settable_nvdata.p_qos_nvdata   = &p_ctrl->pseudo_nvstorage_for_qos_nvdata;
    settable_nvdata.p_tcpip_nvdata = &p_ctrl->pseudo_nvstorage_for_tcpip_nvdata;
#if defined(OPENER_CIP_SAFETY)
    settable_nvdata.p_safetysupervisor_nvdata = &p_ctrl->pseudo_nvstorage_for_safetysupervisor_nvdata;
#endif

    /** Get QoS used values on QSPI Flash */
    fsp_err_t err = R_XSPI_QSPI_Open (&g_qspi0_ctrl, &g_qspi0_cfg);
    while (err);
    uint8_t * dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_QOS);
    _qspi_read_nvdata(&g_qspi0_ctrl, settable_nvdata.p_qos_nvdata, dest, sizeof(opener_port_cip_qos_nvdata_t));
    if (settable_nvdata.p_qos_nvdata->dscp_change_flag == NVDATA_IS_CHANGED)
    {
        g_qos.dscp.urgent = settable_nvdata.p_qos_nvdata->dscp_urgent;
        g_qos.dscp.scheduled = settable_nvdata.p_qos_nvdata->dscp_scheduled;
        g_qos.dscp.high = settable_nvdata.p_qos_nvdata->dscp_high;
        g_qos.dscp.low = settable_nvdata.p_qos_nvdata->dscp_low;
        g_qos.dscp.explicit_msg = settable_nvdata.p_qos_nvdata->dscp_explict;
    }

    /** Get TCP/IP interface object used values on QSPI Flash */
    dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_TCPIP);
    _qspi_read_nvdata(&g_qspi0_ctrl, settable_nvdata.p_tcpip_nvdata, dest, sizeof(opener_port_cip_tcpip_nvdata_t));
    if (settable_nvdata.p_tcpip_nvdata->tcp_change_flag == NVDATA_IS_CHANGED)
    {
        g_tcpip.encapsulation_inactivity_timeout = settable_nvdata.p_tcpip_nvdata->encapsulation_inactivity_timeout_sec;
        g_tcpip.select_acd = settable_nvdata.p_tcpip_nvdata->select_acd;
        g_tcpip.last_conflict.acd_activity = settable_nvdata.p_tcpip_nvdata->acd_activity;
        memcpy(g_tcpip.last_conflict.remote_mac, settable_nvdata.p_tcpip_nvdata->remote_mac, sizeof(g_tcpip.last_conflict.remote_mac));
        memcpy(g_tcpip.last_conflict.arp_pdu, settable_nvdata.p_tcpip_nvdata->arp_pdu, sizeof(g_tcpip.last_conflict.arp_pdu));
        if( settable_nvdata.p_tcpip_nvdata->config_method == LWIP_PORT_DHCP_ENABLE)
        {
            g_tcpip.config_control |= kTcpipCfgCtrlDhcp;
        }
        else
        {
            g_tcpip.config_control &= ~kTcpipCfgCtrlDhcp;
        }
    }

#if defined(OPENER_CIP_SAFETY)
    dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_SAFETYSUPERVISOR);
    _qspi_read_nvdata(&g_qspi0_ctrl, settable_nvdata.p_safetysupervisor_nvdata, dest, sizeof(opener_port_cip_safetysupervisor_nvdata_t));
    if (settable_nvdata.p_safetysupervisor_nvdata->safetysupervisor_change_flag == NVDATA_IS_CHANGED)
    {
        /* Password */
        memcpy(g_safetysupervisor.password, settable_nvdata.p_safetysupervisor_nvdata->password, sizeof(g_safetysupervisor.password));
        /* Alarm Enable */
        g_safetysupervisor.alarm_enable = settable_nvdata.p_safetysupervisor_nvdata->alarm_enable;
        /* Warning Enable */
        g_safetysupervisor.warning_enable = settable_nvdata.p_safetysupervisor_nvdata->warning_enable;
        /* Configuration Lock */
        g_safetysupervisor.config_lock = settable_nvdata.p_safetysupervisor_nvdata->config_lock;
        /* Configuration UNID */
        g_safetysupervisor.cfunid.snn.time_of_day = settable_nvdata.p_safetysupervisor_nvdata->cfunid.snn.time_of_day;
        g_safetysupervisor.cfunid.snn.date = settable_nvdata.p_safetysupervisor_nvdata->cfunid.snn.date;
        g_safetysupervisor.cfunid.nodeid = settable_nvdata.p_safetysupervisor_nvdata->cfunid.nodeid;
        /* Safety Configuration Identifier */
        g_safetysupervisor.scid.sccrc = settable_nvdata.p_safetysupervisor_nvdata->scid.sccrc;
        g_safetysupervisor.scid.scts.time_of_day = settable_nvdata.p_safetysupervisor_nvdata->scid.scts.time_of_day;
        g_safetysupervisor.scid.scts.date = settable_nvdata.p_safetysupervisor_nvdata->scid.scts.date;
        /* Target UNID */
        g_safetysupervisor.tunid.snn.time_of_day = settable_nvdata.p_safetysupervisor_nvdata->tunid.snn.time_of_day;
        g_safetysupervisor.tunid.snn.date = settable_nvdata.p_safetysupervisor_nvdata->tunid.snn.date;
        g_safetysupervisor.tunid.nodeid = settable_nvdata.p_safetysupervisor_nvdata->tunid.nodeid;
        /* Output Connection Point Owners */
        g_safetysupervisor.output_connection_owners.num = settable_nvdata.p_safetysupervisor_nvdata->output_connection_owners.num;
        if (g_safetysupervisor.output_connection_owners.num > OPENER_SAFETYSUPERVISOR_MAX_OUTPUT_OWNER) {
            g_safetysupervisor.output_connection_owners.num = OPENER_SAFETYSUPERVISOR_MAX_OUTPUT_OWNER;
        }
        g_safetysupervisor.output_connection_owners.output_owners[0].ocpunid.snn.time_of_day = settable_nvdata.p_safetysupervisor_nvdata->output_connection_owners.output_owners[0].ocpunid.snn.time_of_day;
        g_safetysupervisor.output_connection_owners.output_owners[0].ocpunid.snn.date = settable_nvdata.p_safetysupervisor_nvdata->output_connection_owners.output_owners[0].ocpunid.snn.date;
        g_safetysupervisor.output_connection_owners.output_owners[0].ocpunid.nodeid = settable_nvdata.p_safetysupervisor_nvdata->output_connection_owners.output_owners[0].ocpunid.nodeid;
        /* Config Data */
        memcpy(g_safetysupervisor.config_data, settable_nvdata.p_safetysupervisor_nvdata->config_data, sizeof(g_safetysupervisor.config_data));
    }
#endif

    /** Get Connection ID offset on Flash */
    /*
     * This code is a temporary measure to avoid Connection ID duplication error.
     * It may be changed in future updates.
     */
    dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_CID);
    _qspi_read_nvdata(&g_qspi0_ctrl, &g_connection_id_initial_offset, dest, sizeof(EipUint32));
    g_connection_id_initial_offset++;

    /*
     * Stores the current Connection ID offset value in Flash.
     * This code is a temporary measure to avoid Connection ID duplication error.
     * It may be changed in future updates.
     */
    _qspi_erase_nvdata_sector(&g_qspi0_ctrl, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + WRITE_ADDRESS_FOR_CID), QSPI_DEVICE_SECTOR_SIZE);
    dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_CID);
    _qspi_write_nvdata(&g_qspi0_ctrl, &g_connection_id_initial_offset, dest, sizeof(EipUint32));
}

/*******************************************************************************************************************//**
 * @brief QSPI Flash Erase
 **********************************************************************************************************************/
static void _qspi_erase_nvdata_sector(spi_flash_ctrl_t    * p_ctrl,
                                      uint8_t * const       p_dest,
                                      uint32_t              byte_count)
{
    /*---------- QSPI Flash Erase ----------*/
    fsp_err_t err = R_XSPI_QSPI_Erase(p_ctrl, p_dest, byte_count);
    while(err);

    vTaskDelay( (10 / portTICK_PERIOD_MS) );
}

/*******************************************************************************************************************//**
 * @brief QSPI Flash Write
 **********************************************************************************************************************/
static void _qspi_write_nvdata(spi_flash_ctrl_t    * p_ctrl,
                               void const * const    p_src,
                               uint8_t * const       p_dest,
                               uint32_t              byte_count)
{

#if defined(BOARD_RZT2ME_RSK)
    uint8_t temp_array[QSPI_ARRAY_SIZE_TEMP];
    memcpy(temp_array, p_src, byte_count);
    spi_flash_status_t status_erase;
    uint32_t i;

    /* Write data every 4 bytes */
    for ( i = 0; i < sizeof(temp_array) / sizeof(uint32_t) ; i++)
    {
        do
        {
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
        } while (true == status_erase.write_in_progress);

        R_XSPI_QSPI_Write(p_ctrl,
                          &temp_array[0 + (i * sizeof(uint32_t))],
                          (p_dest + (i * sizeof(uint32_t))),
                          sizeof(uint32_t));
    }
#else
    uint32_t dummy;
    uint8_t temp_array[QSPI_WRITE_SIZE_TEMP];
    uint32_t offset = 0;
    uint32_t remain = byte_count;

    while (remain >= QSPI_WRITE_SIZE_TEMP) {
        memcpy(&dummy, p_dest + offset, sizeof(dummy));
        memcpy(temp_array, (uint32_t*)p_src + offset, QSPI_WRITE_SIZE_TEMP);

        /* Wait for status register to update. */
        spi_flash_status_t status_erase;
        do
        {
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
            vTaskDelay( (1 / portTICK_PERIOD_MS) );
        } while (true == status_erase.write_in_progress);

        /*---------- Write Data via write API ----------*/
        fsp_err_t err = R_XSPI_QSPI_Write(p_ctrl, temp_array, p_dest + offset, QSPI_WRITE_SIZE_TEMP);
        while(err);

        vTaskDelay( (10 / portTICK_PERIOD_MS) );

        offset += QSPI_WRITE_SIZE_TEMP;
        remain -= QSPI_WRITE_SIZE_TEMP;
    }

    if (remain != 0) {
        memcpy(&dummy, p_dest + offset, sizeof(dummy));
        memcpy(temp_array, (uint32_t*)p_src + offset, remain);

        /* Wait for status register to update. */
        spi_flash_status_t status_erase;
        do
        {
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
            vTaskDelay( (1 / portTICK_PERIOD_MS) );
        } while (true == status_erase.write_in_progress);

        /*---------- Write Data via write API ----------*/
        fsp_err_t err = R_XSPI_QSPI_Write(p_ctrl, temp_array, p_dest + offset, remain);
        while(err);

        vTaskDelay( (10 / portTICK_PERIOD_MS) );
    }
#endif

}

/*******************************************************************************************************************//**
 * @brief QSPI Flash Read
 **********************************************************************************************************************/
static void _qspi_read_nvdata(spi_flash_ctrl_t    * p_ctrl,
                              void *                p_src,
                              uint8_t * const       p_dest,
                              uint8_t               byte_count)
{
    (void) p_ctrl;

    /* Wait for status register to update. */
    spi_flash_status_t status_erase;
    do
    {
        (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
        vTaskDelay( (1 / portTICK_PERIOD_MS) );
    } while (true == status_erase.write_in_progress);

    /*---------- Read data using memory-mapping mode ----------*/
    uint8_t g_src[QSPI_ARRAY_SIZE_TEMP];
    for(uint32_t i = 0; i < byte_count; i++)
    {
        uint8_t dest_data = *(uint8_t *)(p_dest + i);
        g_src[i] = dest_data;
    }

    memcpy(p_src, g_src, byte_count);
}

/*******************************************************************************************************************//**
 * @brief Set the indicator LED pin
 **********************************************************************************************************************/
static usr_err_t _set_indicator_led( bsp_io_port_pin_t io_port_pin,
                                     opener_port_indicator_flash_state_t indicator_state )
{
    /** For checking current I/O level of indicator */
    bsp_io_level_t current_io_level;

    /** If the pin is disabled, */
    if( BOARD_PIN_DISABLED == (int8_t) io_port_pin )
    {
        /** Return error code. */
        return USR_ERR_ABORTED;
    }

    /** Read current I/O level */
    current_io_level = (bsp_io_level_t) _pin_read(io_port_pin);

    switch(indicator_state)
    {
    /** Set ON to LED pin. */
    case OPENER_PORT_INDICATOR_STATE_STEADY_ON:
        if( current_io_level != BOARD_IOLEVEL_LED_ON )
        {
            _pin_write( io_port_pin, BOARD_IOLEVEL_LED_ON);
        }
        break;

    /** Set OFF to LED pin. */
    case OPENER_PORT_INDICATOR_STATE_STEADY_OFF:
        if( current_io_level != BOARD_IOLEVEL_LED_OFF )
        {
            _pin_write( io_port_pin, BOARD_IOLEVEL_LED_OFF);
        }
        break;

    /** Toggle the LED pin. */
    case OPENER_PORT_INDICATOR_STATE_FLASHING:
#if defined(OPENER_CIP_SAFETY)
        if( BOARD_PIN_MOD_GREEN == io_port_pin )
        {
            if (toggle_cout[0] & 0x01) {
                _pin_write( io_port_pin, (bsp_io_level_t)!current_io_level);
            }
            toggle_cout[0]++;
        }
        else if( BOARD_PIN_MOD_RED == io_port_pin )
        {
            if (toggle_cout[1] & 0x01) {
                _pin_write( io_port_pin, (bsp_io_level_t)!current_io_level);
            }
            toggle_cout[1]++;
        }
        else if( BOARD_PIN_NET_GREEN == io_port_pin )
        {
            if (toggle_cout[2] & 0x01) {
                _pin_write( io_port_pin, (bsp_io_level_t)!current_io_level);
            }
            toggle_cout[2]++;
        }
        else if( BOARD_PIN_NET_RED == io_port_pin )
        {
            if (toggle_cout[3] & 0x01) {
                _pin_write( io_port_pin, (bsp_io_level_t)!current_io_level);
            }
            toggle_cout[3]++;
        }
        else
        {
            /** Return error code. */
            return USR_ERR_ABORTED;
        }
#else
        _pin_write( io_port_pin, (bsp_io_level_t)!current_io_level);
#endif
        break;

#if defined(OPENER_CIP_SAFETY)
    /** Toggle the LED pin fast. */
    case OPENER_PORT_INDICATOR_STATE_FAST_FLASHING:
        _pin_write( io_port_pin, (bsp_io_level_t)!current_io_level);
        break;
#endif

    /** Unused State */
    default:
        break;
    }

#if defined(OPENER_CIP_SAFETY)
    if( indicator_state != OPENER_PORT_INDICATOR_STATE_FLASHING )
    {
        if( BOARD_PIN_MOD_GREEN == io_port_pin )
        {
            toggle_cout[0] = 0;
        }
        else if( BOARD_PIN_MOD_RED == io_port_pin )
        {
            toggle_cout[1] = 0;
        }
        else if( BOARD_PIN_NET_GREEN == io_port_pin )
        {
            toggle_cout[2] = 0;
        }
        else if( BOARD_PIN_NET_RED == io_port_pin )
        {
            toggle_cout[3] = 0;
        }
    }
#endif

    /** Return success code */
    return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief set array pins to the specified value
 **********************************************************************************************************************/
static usr_err_t _set_pin_array( uint8_t const * const p_byte_array,
                                 size_t const byte_array_size,
                                 bsp_io_port_pin_t const * const p_pin_array,
                                 bsp_io_level_t const * const p_pin_inactive_levels,
                                 size_t const num_pins)
{
    /** For scanning pins and bytes. */
    size_t pin_idx = 0;
    size_t byte_idx = 0;

    /** For bit data to be written to pin. */
    bsp_io_level_t bit_data = BSP_IO_LEVEL_LOW;

    R_BSP_PinAccessEnable();
    for ( pin_idx = 0; pin_idx < num_pins; pin_idx++ )
    {
        /** If the pin is disabled, */
        if( BOARD_PIN_DISABLED == (int8_t) p_pin_array[pin_idx] )
        {
            /** Skip and continue */
            continue;
        }

        /** If byte index is over, */
        byte_idx = pin_idx / 8;
        if ( byte_idx >= byte_array_size )
        {
            /** Return with error */
            return USR_ERR_ABORTED;
        }

        /** Write data */
        bit_data = (bsp_io_level_t) ((p_byte_array[byte_idx] >> (pin_idx % 8)) & BSP_IO_LEVEL_HIGH);
        _pin_write(p_pin_array[pin_idx], (bsp_io_level_t) ((bit_data ^ p_pin_inactive_levels[pin_idx]) & BSP_IO_LEVEL_HIGH));
    }
    R_BSP_PinAccessDisable();

    /** Return success code */
    return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Get the values of array pins.
 **********************************************************************************************************************/
static usr_err_t _get_pin_array( uint8_t * const byte_array,
                                 size_t const byte_array_size,
                                 bsp_io_port_pin_t const * const p_pin_array,
                                 bsp_io_level_t const * const p_pin_inactive_levels,
                                 size_t const num_pins)
{
    /** For scanning pins and bytes. */
    size_t pin_idx = 0;
    size_t byte_idx = 0;

    /** For bit data to be written to pin. */
    bsp_io_level_t bit_data = BSP_IO_LEVEL_LOW;

    R_BSP_PinAccessEnable();
    for (pin_idx = 0; pin_idx < num_pins; pin_idx++ )
    {
        /** If the pin is disabled, */
        if( BOARD_PIN_DISABLED == (int8_t) p_pin_array[pin_idx] )
        {
            /** Skip and continue */
            continue;
        }

        /** If byte index is over, */
        byte_idx = pin_idx / 8;
        if ( byte_idx >= byte_array_size )
        {
            /** Return with error */
            return USR_ERR_ABORTED;
        }

        /** Read data */
        bit_data = (bsp_io_level_t)( (_pin_read(p_pin_array[pin_idx]) ^ p_pin_inactive_levels[pin_idx]) & BSP_IO_LEVEL_HIGH );

        /** Apply to byte array */
        if (bit_data)
        {
            byte_array[byte_idx] = (byte_array[byte_idx] | (uint8_t)(bit_data << (pin_idx % 8)));
        }
        else
        {
            byte_array[byte_idx] = (byte_array[byte_idx] & (uint8_t)~(!bit_data << (pin_idx % 8)));
        }
    }
    R_BSP_PinAccessDisable();

    /** Return success code */
    return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * Read the input value of the port in the specified region.
 *
 * @param[in]  port             The port
 *
 * @retval     Current input level
 **********************************************************************************************************************/
static bsp_io_level_t _pin_read (bsp_io_port_pin_t io_port_pin)
{
    bsp_io_level_t current_level = BSP_IO_LEVEL_LOW;

    uint8_t pin_array_num = 0;

    /** Search pin region */
    while ( USR_SUCCESS != memcmp( &io_port_pin, &g_pin_region.p_pin_region[pin_array_num][0], sizeof(bsp_io_port_pin_t)) )
    {
        pin_array_num++;
        if (pin_array_num > g_pin_region.pin_count)
        {
            /** No matching pin */
            USR_DEBUG_PRINT( "Failed to search pin \n" );
            USR_DEBUG_BLOCK_CPU();
        }
    }

    /** Read pin level */
    current_level = (bsp_io_level_t) R_BSP_FastPinRead((bsp_io_region_t) g_pin_region.p_pin_region[pin_array_num][1], (bsp_io_port_pin_t) g_pin_region.p_pin_region[pin_array_num][0]);

    /** Return current pin level */
    return current_level;
}

/*******************************************************************************************************************//**
 * Set the output level of the port in the specified region.
 *
 * @param[in]  port             The port
 * @param[in]  set_value        The setting value
 **********************************************************************************************************************/
static void _pin_write (bsp_io_port_pin_t io_port_pin, uint8_t set_value)
{
    uint32_t port_val = 0;
    uint8_t pin_array_num = 0;
    uint32_t port_num = 0;
    uint32_t pin_num = 0;
    uint32_t port_val_mask = 0;

    /** Search pin region */
    while ( USR_SUCCESS != memcmp( &io_port_pin, &g_pin_region.p_pin_region[pin_array_num][0], sizeof(bsp_io_port_pin_t)) )
    {
        pin_array_num++;
        if (pin_array_num > g_pin_region.pin_count)
        {
            /** No matching pin */
            USR_DEBUG_PRINT( "Failed to search pin \n" );
            USR_DEBUG_BLOCK_CPU();
        }
    }

    /** Read-Modify-Write port value */
    port_num = io_port_pin & ((uint32_t)BSP_IO_PRV_8BIT_MASK << 8);
    port_val = R_BSP_PortRead((bsp_io_region_t) g_pin_region.p_pin_region[pin_array_num][1], (bsp_io_port_t) port_num);

    pin_num = (io_port_pin & BSP_IO_PRV_8BIT_MASK);
    port_val_mask = (uint32_t) 1 << pin_num;
    port_val = (port_val & ~port_val_mask) | ((uint32_t) set_value << pin_num);

    R_BSP_PortWrite((bsp_io_region_t) g_pin_region.p_pin_region[pin_array_num][1], (bsp_io_port_t) port_num, (uint8_t) port_val);

}

/*******************************************************************************************************************//**
 * @brief Set timer for Reset
 **********************************************************************************************************************/
static void _reset_timer_set(void)
{
    /** Store Non-Volatile data before reset */
    StoreNvData();

    /**
     * Create timer for Reboot.
     * When this process executes, debugger connection will be disconnected.
     * Please create a ROM execution project if you need check the operation of this function.
     */
#if defined(EXECUTE_SYSTEM_RESET_PROCESS)

    TimerHandle_t p_reset_timer = xTimerCreate("Reset Timer",
                 1000,
                 pdFALSE,
                 NULL,
                 (TimerCallbackFunction_t) OsRebootService);

    /** Start timer */
    xTimerStart( p_reset_timer, portMAX_DELAY );
#endif

}

#if defined(EXECUTE_SYSTEM_RESET_PROCESS)
static void OsRebootService (void)
{
    portENTER_CRITICAL();
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
    R_BSP_SystemReset();

    while(1);
}
#endif

/*******************************************************************************************************************//**
 * @brief Store Non-Volatile data via QSPI
 **********************************************************************************************************************/
static void StoreNvData(void)
{
    /** Resolve example application context. */
    user_app_example_ctrl_t * p_ctrl = gp_user_app_example0_ctrl;

    /** For example, save the settable nvdata from OpENer port module. */
    opener_port_cip_settable_nvdata_t settable_nvdata;
    settable_nvdata.p_qos_nvdata   = &p_ctrl->pseudo_nvstorage_for_qos_nvdata;
    settable_nvdata.p_tcpip_nvdata = &p_ctrl->pseudo_nvstorage_for_tcpip_nvdata;
#if defined(OPENER_CIP_SAFETY)
    settable_nvdata.p_safetysupervisor_nvdata = &p_ctrl->pseudo_nvstorage_for_safetysupervisor_nvdata;
#endif
    (void) p_ctrl->p_opener_port_instance->p_api->settableNvdataGet( p_ctrl->p_opener_port_instance->p_ctrl, &settable_nvdata );

    /** Write NV Data to Flash */
    /* QoS NV Data */
    settable_nvdata.p_qos_nvdata->dscp_change_flag = NVDATA_IS_CHANGED;
    _qspi_erase_nvdata_sector(&g_qspi0_ctrl, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + WRITE_ADDRESS_FOR_QOS), QSPI_DEVICE_SECTOR_SIZE);
    uint8_t * dest = (uint8_t *)(QSPI_DEVICE_START_ADDRESS_MIRROR + WRITE_ADDRESS_FOR_QOS);
    _qspi_write_nvdata(&g_qspi0_ctrl, settable_nvdata.p_qos_nvdata, dest, sizeof(opener_port_cip_qos_nvdata_t));
}


#if defined(UM_OPENER_PORT_FEATURE_DLR) && UM_OPENER_PORT_FEATURE_DLR
/**
 * The following code is a provisional measure to pass official testing.
 * To avoid the problem, the Beacon frame transfer permission process is
 * performed at the last stage of user processing.
 *   Problem: In DLR topology, Sign_On frame may not be sent from RZ after returning from Type 0 Reset.
 * We plan to update the measure and remove the provisional code in a future update.
 */
#include "r_ethsw.h"
/* Initial value of down counter for timeout detection */
#define ETHSW_TIMEOUT_COUNT                   (1000000000U)
/* LK_CTRL Register Bit Definitions */
#define ETHSW_LK_MASK                         (0x9E)
#define ETHSW_LK_PERVLAN                      (7)
#define ETHSW_LK_DISCUNKN                     (4)
#define ETHSW_LK_MIGRAT                       (3)
#define ETHSW_LK_AGING                        (2)
#define ETHSW_LK_LEARN                        (1)
#define ETHSW_LK_ADDR_CLEAR                   (1 << 29)
#define ETHSW_LK_ADDR_LOOKUP                  (1 << 28)
#define ETHSW_LK_ADDR_VALID                   (1)
#define ETHSW_LK_ADDR_WRITE                   (1 << 25)
#define ETHSW_LK_ADDR_REG_VALID               (1 << 16)
#define ETHSW_LK_ADDR_REG_TYPE                (1 << 17)
#define ETHSW_LK_ADDR_PORTMASK                (0xF << 21)
#define ETHSW_LK_ADDR_PRIOMASK                (0x7 << 18)
#define ETHSW_LK_ADDR_PORT_OFFSET             (5)
#define ETHSW_LK_ADDR_PORT_REG_OFFSET         (21)
#define ETHSW_LK_ADDR_PRIO_REG_OFFSET         (18)
#define ETHSW_LK_ADDR_MASKANDFLAGS_MASK       (0xFFFFU << 16)
typedef union
{
    ethsw_mac_addr_t mac_addr;         /* MAC address                                  */
    uint32_t         reg_data[2];      /* data for LK_DATA_LO and LK_DATA_HI register */
} ethsw_mac_addr_reg_t;
void dlr_beacon_enable(void)
{
    volatile ethsw_mac_table_entry_addr_t mac_entry_addr = {0};
    volatile ethsw_mac_table_entry_info_t mac_entry_info = {0};
    volatile ethsw_mac_table_entry_addr_t * p_mac_tab_addr;
    volatile ethsw_mac_table_entry_info_t * p_mac_tab_info;

    ethsw_mac_addr_reg_t entry = {0};
    uint64_t             timeout = ETHSW_TIMEOUT_COUNT;
    uint32_t             port_mask;
    volatile uint32_t    dammy_read;

    R_ETHSW_Type volatile * p_switch_reg;
    p_switch_reg = (void *) R_ETHSW;

    /* Multicast MAC for Beacon Frames (01:21:6c:00:00:01) */
    uint8_t dlr_mac[ETHSW_MAC_ADDR_LENGTH] = {0x01, 0x21, 0x6c, 0x00, 0x00, 0x01};
    mac_entry_addr.mac_addr = (ethsw_mac_addr_t *) &dlr_mac;

    /* Enable forwarding from port 0, 1 */
    mac_entry_info.port_mask.mask = ( ETHSW_PORT_BIT(0) | ETHSW_PORT_BIT(1) );
    p_mac_tab_addr = &mac_entry_addr;
    p_mac_tab_info = &mac_entry_info;

    port_mask = p_mac_tab_info->port_mask.mask;

    /* copy mac address to get valid alignment */
    memcpy(entry.mac_addr, *p_mac_tab_addr->mac_addr, ETHSW_MAC_ADDR_LENGTH);

    p_switch_reg->LK_DATA_LO = entry.reg_data[0];
    p_switch_reg->LK_DATA_HI = entry.reg_data[1];

    p_switch_reg->LK_DATA_HI2_b.MEMDATA = p_mac_tab_addr->vlan_id &
                                          (R_ETHSW_LK_DATA_HI2_MEMDATA_Msk >> R_ETHSW_LK_DATA_HI2_MEMDATA_Pos);

    /* damy read */
    dammy_read = p_switch_reg->LK_DATA_LO;
    dammy_read = p_switch_reg->LK_DATA_HI;
    dammy_read = dammy_read;

    /* Perform lookup to get address entry */
    p_switch_reg->LK_ADDR_CTRL |= ETHSW_LK_ADDR_LOOKUP;
    while (true == p_switch_reg->LK_ADDR_CTRL_b.BUSY)
    {
        timeout--;
    }

    /* Address is already there. Just update the port mask and priority */
    /* update port bit mask  */
    p_switch_reg->LK_DATA_HI &= ~(uint32_t) ETHSW_LK_ADDR_PORTMASK;
    p_switch_reg->LK_DATA_HI |= (port_mask << ETHSW_LK_ADDR_PORT_REG_OFFSET);

    /* updated priority */
    p_switch_reg->LK_DATA_HI &= ~(uint32_t) ETHSW_LK_ADDR_PRIOMASK;
    p_switch_reg->LK_DATA_HI |= (p_mac_tab_info->priority << ETHSW_LK_ADDR_PRIO_REG_OFFSET) &
    		                    ETHSW_LK_ADDR_PRIOMASK;

    /* trigger MAC table write */
    p_switch_reg->LK_ADDR_CTRL |= ETHSW_LK_ADDR_WRITE;
}
#endif

