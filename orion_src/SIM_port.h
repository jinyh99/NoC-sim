/* fake SIM_port.h, for testing module functions without the module itself */

/* this file lists the parameters of IBM InfiniBand 8x8 12X router */
#ifndef _SIM_PORT_H
#define _SIM_PORT_H

#ifndef POWER_TEST
#define POWER_TEST
#endif

#define PARM_POWER_STATS	1

#include "SIM_power.h"
#include "SIM_power_array.h"
#include "SIM_power_router.h"
#include "SIM_power_misc.h"

/* RF module parameters */
#define PARM_read_port	1
#define PARM_write_port	1
#define PARM_n_regs	64
#define PARM_reg_width	32

#define PARM_ndwl	1
#define PARM_ndbl	1
#define PARM_nspd	1

//#define PARM_wordline_model	CACHE_RW_WORDLINE
//#define PARM_bitline_model	RW_BITLINE
//#define PARM_mem_model		NORMAL_MEM
//#define PARM_row_dec_model	SIM_NO_MODEL
//#define PARM_row_dec_pre_model	SIM_NO_MODEL
//#define PARM_col_dec_model	SIM_NO_MODEL
//#define PARM_col_dec_pre_model	SIM_NO_MODEL
//#define PARM_mux_model		SIM_NO_MODEL
//#define PARM_outdrv_model	SIM_NO_MODEL

/* these 3 should be changed together */
//#define PARM_data_end		2
//#define PARM_amp_model		GENERIC_AMP
//#define PARM_bitline_pre_model	EQU_BITLINE
//#define PARM_data_end		1
//#define PARM_amp_model		SIM_NO_MODEL
//#define PARM_bitline_pre_model	SINGLE_OTHER


/* router module parameters */
/* general parameters */
// Defines were changed to global parameters
namespace orion {
  extern int PARM_in_port;	  /* # of network input ports */
  extern int PARM_cache_in_port;  /* # of cache input ports */
  extern int PARM_mc_in_port;	  /* # of memory controller input ports */
  extern int PARM_io_in_port;	  /* # of I/O device input ports */
  extern int PARM_out_port;	  /* # of network output ports */
  extern int PARM_cache_out_port; /* # of cache output ports */
  extern int PARM_mc_out_port;	  /* # of memory controller output ports */
  extern int PARM_io_out_port;	  /* # of I/O device output ports */
  extern int PARM_flit_width;	  /* flit width in bits */

/* virtual channel parameters */
  extern int PARM_v_channel;	    /* # of network port virtual channels */
  extern int PARM_v_class;	    /* # of total virtual classes */
  extern int PARM_cache_class;	    /* # of cache port virtual classes */
  extern int PARM_mc_class;	    /* # of memory controller port virtual classes */
  extern int PARM_io_class;	    /* # of I/O device port virtual classes */
/* ?? */
  extern int PARM_in_share_buf;	    /* do input virtual channels physically share buffers? */
  extern int PARM_out_share_buf;    /* do output virtual channels physically share buffers? */
/* ?? */
  extern int PARM_in_share_switch;  /* do input virtual channels share crossbar input ports? */
  extern int PARM_out_share_switch; /* do output virtual channels share crossbar output ports? */

/* crossbar parameters */
  extern SIM_power_crossbar_model_t PARM_crossbar_model; /* crossbar model type */
  extern int PARM_crsbar_degree;    /* crossbar mux degree */
  extern SIM_power_connect_t PARM_connect_type; /* crossbar connector type */
  extern SIM_power_trans_t PARM_trans_type; /* crossbar transmission gate type */
  extern int PARM_crossbar_in_len;  /* crossbar input line length, if known */
  extern int PARM_crossbar_out_len; /* crossbar output line length, if known */
  extern int PARM_xb_in_seg;          
  extern int PARM_xb_out_seg;        
/* HACK HACK HACK */
  extern SIM_power_crossbar_model_t PARM_exp_xb_model;       
  extern int PARM_exp_in_seg;         
  extern int PARM_exp_out_seg;        

/* input buffer parameters */
  extern int PARM_in_buf;	  /* have input buffer? */
  extern int PARM_in_buf_set;	  /* # of rows */
  extern int PARM_in_buf_rport;   /* # of read ports */
}

#define PARM_cache_in_buf	0
#define PARM_cache_in_buf_set	0
#define PARM_cache_in_buf_rport	0
#define PARM_mc_in_buf		0
#define PARM_mc_in_buf_set	0
#define PARM_mc_in_buf_rport	0

#define PARM_io_in_buf		0
#define PARM_io_in_buf_set	0
#define PARM_io_in_buf_rport	0

/* output buffer parameters */
#define PARM_out_buf		0
#define PARM_out_buf_set	64
#define PARM_out_buf_wport	1

/* central buffer parameters */
#define PARM_central_buf	0	/* have central buffer? */
#define PARM_cbuf_set		2560	/* # of rows */
#define PARM_cbuf_rport		2	/* # of read ports */
#define PARM_cbuf_wport		2	/* # of write ports */
#define PARM_cbuf_width		4	/* # of flits in one row */
#define PARM_pipe_depth		4	/* # of banks */

/* array parameters shared by various buffers */
#define PARM_wordline_model	CACHE_RW_WORDLINE
#define PARM_bitline_model	RW_BITLINE
#define PARM_mem_model		NORMAL_MEM
#define PARM_row_dec_model	GENERIC_DEC
#define PARM_row_dec_pre_model	SINGLE_OTHER
#define PARM_col_dec_model	SIM_NO_MODEL
#define PARM_col_dec_pre_model	SIM_NO_MODEL
#define PARM_mux_model		SIM_NO_MODEL
#define PARM_outdrv_model	REG_OUTDRV

/* these 3 should be changed together */
/* use double-ended bitline because the array is too large */
#define PARM_data_end		2
#define PARM_amp_model		GENERIC_AMP
#define PARM_bitline_pre_model	EQU_BITLINE
//#define PARM_data_end		1
//#define PARM_amp_model		SIM_NO_MODEL
//#define PARM_bitline_pre_model	SINGLE_OTHER

/* arbiter parameters */
#define PARM_in_arb_model	MATRIX_ARBITER	/* input side arbiter model type */
#define PARM_in_arb_ff_model	NEG_DFF		/* input side arbiter flip-flop model type */
#define PARM_out_arb_model	MATRIX_ARBITER	/* output side arbiter model type */
#define PARM_out_arb_ff_model	NEG_DFF		/* output side arbiter flip-flop model type */

#endif	/* _SIM_PORT_H */
