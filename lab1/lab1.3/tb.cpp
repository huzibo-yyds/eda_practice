#include <iostream>
#include "bds_capture.h"

int main() {
    sample_t if_input;
    ap_uint<1> synced;
    ap_uint<12> phase;
    int total_cycles = 2046 * 10; // 仿真10ms
    
    // 生成测试信号（含同步相位）
    for (int i = 0; i < total_cycles; i++) {
        // 模拟北斗BII信号：Gold码 + BPSK
        ap_uint<1> true_code = (i % CODE_LEN) < 100 ? 1 : 0; // 示例码
        if_input = true_code ? 1 : -1; // BPSK调制
        
        bds_capture(if_input, synced, phase);
        
        if (synced) {
            std::cout << "Synced at phase: " << phase << std::endl;
            break;
        }
    }
    
    return 0;
}