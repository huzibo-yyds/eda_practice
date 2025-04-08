#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include <ap_int.h>
#include <math.h>

#define CODE_LENGTH 31
#define SAMPLES_PER_CHIP 16
#define THRESHOLD 50000.0 // 调整后的门限值
#define TOTAL_SAMPLES 496
#define SAMPLE_RATE 496000
#define CARRIER_FREQ 124000
#define phi M_PI / 4

void sync_circuit(
    hls::stream<ap_axis<32,1,1,1>> &if_in,
    hls::stream<ap_axis<32,1,1,1>> &if_out
) {
    #pragma HLS INTERFACE axis port=if_in
    #pragma HLS INTERFACE axis port=if_out
    #pragma HLS INTERFACE ap_ctrl_none port=return

    // 本地码生成（1/-1）
    int local_code[CODE_LENGTH];
    uint32_t lfsr = 0x1F;
    for (int i=0; i<CODE_LENGTH; i++) {
        local_code[i] = (lfsr & 0x01) ? 1 : -1;
        uint32_t feedback = ((lfsr >> 4) ^ (lfsr >> 1)) & 1;
        lfsr = (lfsr >> 1) | (feedback << 4);
    }

    // 同步循环
    ap_axis<32,1,1,1> in_data;
    bool sync_success = false;

    for (int attempt=0; attempt<CODE_LENGTH; attempt++) {
        float sum_I = 0, sum_Q = 0, energy = 0;

        for (int i=0; i<TOTAL_SAMPLES; ) {
            if(!if_in.empty()) {

                i++;
                if_in.read(in_data);
                int quantized = in_data.data;
                
                // 补码转浮点
                int code = quantized & 0x03;
                float sample;
                switch(code) {
                    case 0b00: sample = 1.0; break;
                    case 0b01: sample = 0.5; break;
                    case 0b11: sample = -0.5; break;
                    case 0b10: sample = -1.0; break;
                    default: sample = 0.0;
                }
                
                // 正交下变频
                float t = i * (1.0 / SAMPLE_RATE);
                float phase = 2 * M_PI * CARRIER_FREQ * t + phi;
                float cos_wave = cos(phase);
                float sin_wave = sin(phase);
                float I = sample * cos_wave;
                float Q = sample * sin_wave;
                
                // 相关积分
                int code_idx = (i / SAMPLES_PER_CHIP) % CODE_LENGTH;
                sum_I += I * local_code[code_idx];
                sum_Q += Q * local_code[code_idx];
                 
            }
     }
        
        // 计算能量
        energy = sum_I * sum_I + sum_Q * sum_Q;
        // std::cout << "Attempt " << attempt <<", Energy = " << energy << std::endl;  
        

        if (energy > THRESHOLD) {
            sync_success = true;
            break;
        } else {
            // 循环右移本地码
            int last = local_code[CODE_LENGTH-1];
            for (int i=CODE_LENGTH-1; i>0; i--)
                local_code[i] = local_code[i-1];
            local_code[0] = last;
        }
    }

    // 输出同步结果
    ap_axis<32,1,1,1> out_pkt;
    out_pkt.data = sync_success ? 1 : 0;
    if_out.write(out_pkt);
}