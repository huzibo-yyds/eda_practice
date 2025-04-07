#include "bds_capture.h"

// G1 LFSR初始化
lfsr_state_t g1_lfsr = 0b01010101010; // 初始相位

// G1生成函数
ap_uint<1> generate_g1() {
    ap_uint<1> bit = g1_lfsr[10] ^ g1_lfsr[7] ^ g1_lfsr[5] ^ g1_lfsr[0];
    g1_lfsr = (g1_lfsr << 1) | bit;
    return g1_lfsr[10];
}

// Gold码生成
void gold_code_gen(ap_uint<11> g2_phase, ap_uint<1> &gold_code) {
    static lfsr_state_t g2_lfsr = 0b01010101010; // G2初始相位
    static ap_uint<11> phase_counter = 0;
    
    // 根据卫星ID选择G2相位（示例简化）
    ap_uint<1> g2_bit = (g2_lfsr >> g2_phase) & 0x1;
    
    // 更新G2 LFSR
    ap_uint<1> feedback = g2_lfsr[10] ^ g2_lfsr[8] ^ g2_lfsr[7] ^ g2_lfsr[5] ^ g2_lfsr[1] ^ g2_lfsr[0];
    g2_lfsr = (g2_lfsr << 1) | feedback;
    
    gold_code = generate_g1() ^ g2_bit;
}

// 主捕获模块
void bds_capture(sample_t if_in, ap_uint<1> &synced, ap_uint<12> &phase_out) {
    static ap_int<16> i_acc = 0, q_acc = 0;
    static ap_uint<12> code_phase = 0;
    static ap_uint<1> local_code;
    
    // 生成本地码
    gold_code_gen(SATELLITE_ID, local_code); // 根据卫星ID生成
    
    // 下变频（简化：直接乘符号）
    ap_int<2> i_mult = if_in * (local_code ? 1 : -1);
    ap_int<2> q_mult = if_in * (local_code ? 1 : -1);
    
    // 积分1ms（2046码片）
    i_acc += i_mult;
    q_acc += q_mult;
    
    // 积分完成
    if (code_phase == CODE_LEN - 1) {
        ap_int<32> s = i_acc * i_acc + q_acc * q_acc;
        if (s > 1000) { // 假设门限值1000
            synced = 1;
            phase_out = code_phase;
        } else {
            code_phase = (code_phase + 1) % CODE_LEN; // 相位滑动
        }
        i_acc = q_acc = 0;
    } else {
        code_phase++;
    }
}