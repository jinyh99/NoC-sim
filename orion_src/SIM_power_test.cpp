#include "SIM_power.h"
#include "SIM_power_router.h"

// This parameter was previously specified as a compile-time parameter
namespace orion {
  double PARM_Freq = 3000000000.0;
}


// Originally from SIM_power_test.h
// Modified July 2006 by Noel Eisley
namespace orion {
  double SCALE_T = 1.0;
  double SCALE_M = 1.0;
  double SCALE_S = 1.0;
}


// Originally from SIM_port.h
// Modified July 2006 by Noel Eisley
/* router module parameters */
/* general parameters */
namespace orion {
  int PARM_in_port	  = 9;	/* # of network input ports */
  int PARM_cache_in_port  = 0;	/* # of cache input ports */
  int PARM_mc_in_port	  = 0;	/* # of memory controller input ports */
  int PARM_io_in_port	  = 0;	/* # of I/O device input ports */
  int PARM_out_port	  = 9;  /* # of network output ports */
  int PARM_cache_out_port = 0;	/* # of cache output ports */
  int PARM_mc_out_port	  = 0;	/* # of memory controller output ports */
  int PARM_io_out_port	  = 0;	/* # of I/O device output ports */
  int PARM_flit_width	  = 64;	/* flit width in bits */

/* virtual channel parameters */
  int PARM_v_channel	  = 8;	/* # of network port virtual channels */
  int PARM_v_class	  = 0;	/* # of total virtual classes */
  int PARM_cache_class	  = 0;	/* # of cache port virtual classes */
  int PARM_mc_class	  = 0;	/* # of memory controller port virtual classes */
  int PARM_io_class	  = 0;	/* # of I/O device port virtual classes */
/* ?? */
  int PARM_in_share_buf	  = 0;	/* do input virtual channels physically share buffers? */
  int PARM_out_share_buf  = 0;	/* do output virtual channels physically share buffers? */
/* ?? */
  int PARM_in_share_switch  = 1;        /* do input virtual channels share crossbar input ports? */
  int PARM_out_share_switch = 1;	/* do output virtual channels share crossbar output ports? */

/* crossbar parameters */
  SIM_power_crossbar_model_t PARM_crossbar_model = MATRIX_CROSSBAR; /* crossbar model type */
  int PARM_crsbar_degree    = 4;	/* crossbar mux degree */
  SIM_power_connect_t PARM_connect_type          = TRANS_GATE;	 /* crossbar connector type */
  SIM_power_trans_t PARM_trans_type	         = NP_GATE;	 /* crossbar transmission gate type */
  int PARM_crossbar_in_len  = 0;	/* crossbar input line length, if known */
  int PARM_crossbar_out_len = 0;	/* crossbar output line length, if known */
  int PARM_xb_in_seg        = 0;
  int PARM_xb_out_seg       = 0;
/* HACK HACK HACK */
  SIM_power_crossbar_model_t PARM_exp_xb_model   = MATRIX_CROSSBAR;
  int PARM_exp_in_seg       = 0;
  int PARM_exp_out_seg      = 0;

/* input buffer parameters */
  int PARM_in_buf	  = 1;	/* have input buffer? */
  int PARM_in_buf_set	  = 64;	/* # of rows */
  int PARM_in_buf_rport	  = 1;	/* # of read ports */
}
