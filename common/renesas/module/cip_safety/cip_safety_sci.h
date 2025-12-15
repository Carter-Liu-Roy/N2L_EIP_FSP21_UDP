
#ifndef CIP_SAFETY_SCI_H_
#define CIP_SAFETY_SCI_H_

#if defined(OPENER_CIP_SAFETY)

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/* Size of command header */
#define CIP_SAFETY_CMD_HEADER_SIZE    4 /* Header */

/* Size of command code */
#define CIP_SAFETY_CMD_CODE_SIZE    2 /* Command Code */

/* Size of command length */
#define CIP_SAFETY_CMD_LENGTH_SIZE    2 /* Length */

/* Size of Judge_data */
#define CIP_SAFETY_CMD_JUDGE_DATA_SIZE    3 /* Judge_data */

/* Size of Cip_Layer_Message in O to T Data */
#define CIP_SAFETY_CMD_CIP_MSG_O_TO_T_SIZE    397 /* Cip_Layer_Message in O to T Data */

/* Size of Connection_ID */
#define CIP_SAFETY_CMD_CONNECTION_ID_SIZE    4 /* Connection_ID */

/* Size of Message_size in T to O Data */
#define CIP_SAFETY_CMD_MSG_SIZE_T_TO_O_SIZE    1 /* Message_size in T to O Data */

/* Size of Cip_Layer_Message in O to T Data */
#define CIP_SAFETY_CMD_CIP_MSG_T_TO_O_SIZE    96 /* Cip_Layer_Message in T to O Data */

/* Command codes */
/* Start Up Message */
#define CIP_SAFETY_CMD_CODE_0x0101    0x0101 /* Start Up Message request */
#define CIP_SAFETY_CMD_CODE_0x0102    0x0102 /* Start Up Message response */
/* O to T data & T to O data */
#define CIP_SAFETY_CMD_CODE_0x0201    0x0201 /* O to T data */
#define CIP_SAFETY_CMD_CODE_0x0202    0x0202 /* T to O data */
/* Data Acquisition */
#define CIP_SAFETY_CMD_CODE_0x0301    0x0301 /* Data Acquisition request */
#define CIP_SAFETY_CMD_CODE_0x0302    0x0302 /* Data Acquisition response */

/* Length of Command */
/* Start Up Message */
#define CIP_SAFETY_CMD_CODE_LEN_0x0101    0   /* Start Up Message request */
#define CIP_SAFETY_CMD_CODE_LEN_0x0102    0   /* Start Up Message response */
/* O to T data & T to O data */
#define CIP_SAFETY_CMD_CODE_LEN_0x0201    400 /* O to T data */
#define CIP_SAFETY_CMD_CODE_LEN_0x0202    100 /* T to O data */
/* Data Acquisition */
#define CIP_SAFETY_CMD_CODE_LEN_0x0301    0   /* Data Acquisition request */
#define CIP_SAFETY_CMD_CODE_LEN_0x0302    12  /* Data Acquisition response */

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef struct {
    unsigned short cmd; /* Command Code */
    unsigned short len; /* Length */
    unsigned char data[512]; /* Data */
} cip_safety_sci_st_queue;

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global functions (to be accessed by other files)
 ***********************************************************************************************************************/
extern int cip_safety_sci_init(void);
/* Start Up Message */
extern int cip_safety_start_up_res_send(void);
extern int cip_safety_start_up_req_wait(void);
extern void cip_safety_start_up_finish(void);
/* O to T data & T to O data */
extern int cip_safety_exp_msg_send(unsigned short data_len, unsigned char * data_buf);
extern int cip_safety_exp_msg_wait(unsigned short rx_max_len, unsigned short * rx_len, unsigned char * rx_buf, CipMessageRouterResponse *const message_router_response);
extern int cip_safety_imp_msg_send(unsigned long connection_id, unsigned short data_len, unsigned char * data_buf);
extern int cip_safety_imp_msg_wait(unsigned long connection_id, unsigned short rx_max_len, unsigned short * rx_len, unsigned char * rx_buf);
extern int cip_safety_imp_msg_set_connection_id(unsigned long index, unsigned long connection_id);
/* Data Acquisition */
extern int cip_safety_data_acq_wait(void);
extern int cip_safety_data_acq_start_cyclic(void);
#endif

#endif /* CIP_SAFETY_SCI_H_ */