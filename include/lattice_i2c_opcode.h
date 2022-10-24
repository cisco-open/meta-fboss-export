#ifndef _OPCODE_H_
#define _OPCODE_H_

//*=====================================================
//*
//* I2C Opcode Table
//*
//* Version 1.0.0
//*		


// transmission related opcode def
#define	I2C_STARTTRAN		0x10
#define	I2C_RESTARTTRAN		0x11
#define I2C_ENDTRAN			0x12
#define	I2C_TRANSOUT		0x13
#define	I2C_TRANSIN			0x14
#define	I2C_RUNCLOCK		0x15
#define I2C_WAIT		 0x16
#define I2C_LOOP		 0x17  
#define I2C_ENDLOOP		 0x18
#define I2C_TDI		     0x19
#define I2C_CONTINUE	 0x1A
#define I2C_TDO		     0x1B
#define I2C_MASK		 0x1C
#define I2C_BEGIN_REPEAT 0x1D
#define I2C_END_REPEAT	 0x1E
#define I2C_END_FRAME	 0x1F
#define I2C_DATA		 0x20
#define I2C_PROGRAM		 0x21
#define I2C_VERIFY		 0x22
#define I2C_DTDI		 0x23
#define I2C_DTDO		 0x24
#define I2C_COMMENT		 0x25
#define I2C_ENDCOMMENT	 0x26
#define I2C_TRST		 0x27
#define I2C_ENDVME		 0x7F

/*************************************************************
*                                                            *
* ERROR DEFINITIONS                                          *
*                                                            *
*************************************************************/

#define ERR_VERIFY_FAIL				-1
#define ERR_FIND_ALGO_FILE			-2
#define ERR_FIND_DATA_FILE			-3
#define ERR_WRONG_VERSION			-4
#define ERR_ALGO_FILE_ERROR			-5
#define ERR_DATA_FILE_ERROR			-6
#define ERR_OUT_OF_MEMORY			-7
#define ERR_VERIFY_ACK_FAIL			-8

/*************************************************************
*                                                            *
* DATA TYPE REGISTER BIT DEFINITIONS                         *
*                                                            *
*************************************************************/

#define SDR_DATA		0x0001	/*** Current command is SDR ***/
#define TDI_DATA		0x0002	/*** Command contains TDI ***/
#define TDO_DATA		0x0004	/*** Command contains TDO ***/
#define MASK_DATA		0x0008	/*** Command contains MASK ***/
#define DTDI_DATA		0x0010	/*** Verification flow ***/
#define DTDO_DATA		0x0020	/*** Verification flow ***/
#define COMPRESS		0x0040	/*** Compressed data file ***/
#define COMPRESS_FRAME	0x0080	/*** Compressed data frame ***/




enum I2cTransactionState{
    I2C_ST_FIRST = 0xc2a,
    I2C_ST_S = I2C_ST_FIRST,
	I2C_ST_WAIT,
	I2C_ST_W_CMD,
	I2C_ST_RD,
	I2C_ST_WR,

};

enum I2cAction {
    I2C_A_OP_R_1 = 0x2ca,
	I2C_T_2,
	I2C_A_OP_W_1,
};

enum I2cOperation {
    I2C_OP_W = 0,
	I2C_OP_R,
};



#endif 

