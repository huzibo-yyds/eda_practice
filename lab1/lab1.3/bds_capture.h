#include <ap_int.h>
#include <ap_fixed.h>

#define CODE_LEN 2046
#define SATELLITE_ID 1 // 示例卫星ID

typedef ap_int<2> sample_t; // 2位补码采样
typedef ap_uint<11> lfsr_state_t; // LFSR状态

// Gold码生成器
void gold_code_gen(
    ap_uint<11> g2_phase, 
    ap_uint<1> &gold_code
);

// 下变频与相关积分
void bds_capture(
    sample_t if_in,       // 输入中频信号
    ap_uint<1> &synced,   // 同步标志
    ap_uint<12> &phase_out// 捕获相位
);