#ifndef _CREDIT_H_
#define _CREDIT_H_

class Credit {
public:
    int m_out_pc;
    int m_out_vc;
    int	m_num_credits;
    double m_clk_deposit;	// clock when credit is supposed to be deposited to the router

public:
    void init() {
        m_out_pc = INVALID_PC;
        m_out_vc = INVALID_VC;
        m_num_credits = INVALID_CREDIT;
        m_clk_deposit = INVALID_CLK;
    };
    void destroy() {};
};

#endif
