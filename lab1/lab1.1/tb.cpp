#include <stdio.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <math.h>

#define CODE_LENGTH 31
#define SAMPLES_PER_CHIP 16
#define CARRIER_FREQ 124000
#define SAMPLE_RATE 496000
#define TOTAL_SAMPLES 496 // 1ms数据
#define phi M_PI / 4

void synchronization_circuit(
    hls::stream<ap_axis<32,1,1,1>> &input_stream,
    hls::stream<ap_axis<32,1,1,1>> &output_stream
);

//------------------------ 测试信号生成 ------------------------
void generate_test_signal(
    hls::stream<ap_axis<32,1,1,1>> &signal_stream,
    int phase_shift,
    const int* D,
    int d_length
) {
    int M[CODE_LENGTH];
    uint32_t lfsr = 0x1F; // m序列种子
    
    // 生成0/1的M序列
    for (int i=0; i<CODE_LENGTH; i++) {
        M[i] = (lfsr & 0x01) ? 1 : 0;
        uint32_t feedback = ((lfsr >> 4) ^ (lfsr >> 1)) & 1;
        lfsr = (lfsr >> 1) | (feedback << 4);
 
    
    // 生成IFin信号
    for (int i=0; i<TOTAL_SAMPLES; i++) {
        int chip_idx = i / SAMPLES_PER_CHIP;
        int code_idx = (chip_idx + phase_shift) % CODE_LENGTH;
        int m_bit = M[code_idx];
        int d_bit = D[chip_idx / CODE_LENGTH % d_length]; 
        int xor_result = d_bit ^ m_bit;
        float polarized = (xor_result) ? 1.0 : -1.0;
        
        // 载波生成与调制
        float t = (float)i / SAMPLE_RATE;
        float carrier = cos(2 * M_PI * CARRIER_FREQ * t + phi);
        float modulated = polarized * carrier;
        
        // 2位补码量化
        int quantized;
        if (modulated >= 0.75)       quantized = 0b00; // +1
        else if (modulated >= 0.25)  quantized = 0b01; // +0.5
        else if (modulated >= -0.25) quantized = 0b11; // -0.5
        else                         quantized = 0b10; // -1
        
        ap_axis<32,1,1,1> data_pkt;
        data_pkt.data = quantized;
        data_pkt.last = (i == TOTAL_SAMPLES-1) ? 1 : 0;
        signal_stream.write(data_pkt);
    }  
}
} 

//------------------------ 主测试逻辑 ------------------------
int main() {
    hls::stream<ap_axis<32,1,1,1>> input_stream;
    hls::stream<ap_axis<32,1,1,1>> output_stream;

    // 自定义D序列（1个数据位）
    int D[] = {1};
    int d_length = sizeof(D)/sizeof(D[0]);

    // 测试用例1：相位对齐
    printf("==== Test Case 1: Phase Shift=0 ====\n");
    generate_test_signal(input_stream, 0, D, d_length);
    synchronization_circuit(input_stream, output_stream);

    ap_axis<32,1,1,1> result;
    int success_flags[]  = {0, 0};
    if (!output_stream.empty()) {
        output_stream.read(result);
        success_flags[0] = (int)result.data;
        printf("Status: %s\n", result.data ? "Success" : "Fail");
    
    }

    // 测试用例2：相位偏移5
    // printf("\n==== Test Case 2: Phase Shift=5 ====\n");
    // generate_test_signal(input_stream, 5, D, d_length);
    // synchronization_circuit(input_stream, output_stream);
    // if (!output_stream.empty()) {
    //     output_stream.read(result);
    //     success_flags[1] = (int)result.data;
    //     printf("Status: %s\n", result.data ? "Success" : "Fail");
        
    // }

    printf("\n==== Test Summary ====\n");
    printf("Errors: %d\n", (result.data != 1));
    return 0;
}