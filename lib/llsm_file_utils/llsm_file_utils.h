#ifndef __LLSM_FILE_UTILS_H__
#define __LLSM_FILE_UTILS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../libllsm2/llsm.h"

#include "llsm_errors.h"
#include "utau_pocket_tchar.h"

// 缓存文件相关的魔数与限制
#define LLSM_MAGIC_DOUBLE 0x4432534Cul      // 标识使用 64-bit double 存储的 LS2D
#define LLSM_MAGIC_FLOAT 0x4632534Cul       // 标识使用 32-bit float 存储的 LS2F
#define LLSM_RLE_MAX_RUN_15BIT 32767        // RLE 压缩的最大连续计数 (15bit)
#define LLSM_RLE_RUN_SHIFT 49               // 行程长度在uint64_t中的左移位数
#define LLSM_RLE_RUN_MASK 0x7FFF            // 行程长度的掩码(15bit)
#define LLSM_RLE_COMPRESSED_BIT 48          // 标识压缩块存在性的 bit 位
#define LLSM_RLE_BMP_MASK 0xFFFFFFFFFFFFull // 48bit 容器存在性位图掩码
#define LLSM_FILE_READ_LEN_MAX 0x200000ul   // 读取 LS2 文件时最大读取长度

// 数据类型（占用3bit：0~7）
#define LLSM_TYPE_NULL 0
#define LLSM_TYPE_FPARRAY 1
#define LLSM_TYPE_FP 2
#define LLSM_TYPE_HMFRAME 3
#define LLSM_TYPE_INT 4
#define LLSM_TYPE_NMFRAME 5

typedef struct
{
    uint32_t order_spec;
    uint32_t order_bap;
} llsm_coptions;

typedef struct
{
    FILE *fp;
    int nchannel;
    int npsd;
    llsm_coptions *copt;
} llsm_file_context;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 释放 llsm_coptions 结构体
     * @param opt_c 需要释放的压缩参数
     */
    void llsm_delete_coptions(llsm_coptions *opt_c);

    /**
     * @brief 读取并解析 LLSM 缓存文件
     * @param file_path 主缓存文件路径
     * @param opt_a 输出参数：声学特征选项
     * @param opt_s 输出参数：合成选项
     * @param opt_c 输出参数：压缩参数
     * @param chunk 输出参数：解析出的音频数据块
     * @param nfrm 输出参数：解析出的帧数
     * @return 返回错误代码，LLSM_ERR_OK (0) 表示成功
     */
    int llsm_read_cache_file(const TCHAR *file_path, llsm_aoptions **opt_a, llsm_soptions **opt_s, llsm_coptions **opt_c, llsm_chunk **chunk, int *nfrm);

    /**
     * @brief 将 LLSM 数据写入缓存文件
     * @param file_path 主缓存文件路径
     * @param opt_a 声学特征选项
     * @param opt_s 合成选项
     * @param opt_c 压缩参数
     * @param chunk 需要写入的音频数据块
     * @param nfrm 帧数
     * @return 返回错误代码，LLSM_ERR_OK (0) 表示成功
     */
    int llsm_write_cache_file(const TCHAR *file_path, llsm_aoptions *opt_a, llsm_soptions *opt_s, llsm_coptions *opt_c, llsm_chunk *chunk, int nfrm);

#ifdef __cplusplus
};
#endif

#endif // __LLSM_FILE_UTILS_H__
