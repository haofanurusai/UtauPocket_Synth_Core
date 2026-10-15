#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#include "pyin.h"
#include "llsm_file_utils.h"
extern FP_TYPE* wavread_fp(FILE* fp, int* fs, int* nbit, int* wav_length);
#ifdef __cplusplus
};
#endif

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_TCHAR_WRAPPER_IMPLEMENTATION
#include "optparse_tchar_wrapper.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define INPUT_EXTENSION _T(".WAV")
#define OUTPUT_EXTENSION _T(".LS2")

typedef struct cfg
{
    /* from wav */
    int fs;
    int nbit;
    int nfrm;
    int nx;

    /* set by user */
    int maxnhar;
    int maxnhar_e;
    int nfft;
    int nhop;
    int npsd;
    int trange;
    double bias;
    double fmax;
    double fmin;
    uint32_t order_spec;
    uint32_t order_bap;
    const TCHAR *output_path;
} Cfg;

void cfg_init(Cfg *cfg)
{
    if (cfg == NULL) return;
    cfg->fs = 44100;
    cfg->nbit = 16;
    cfg->nfrm = 0;
    cfg->nx = 0;
    cfg->maxnhar = 400;
    cfg->maxnhar_e = 5;
    cfg->nfft = 2048;
    cfg->nhop = 128;
    cfg->npsd = 128;
    cfg->trange = 24;
    cfg->bias = 2.0;
    cfg->fmax = 800.0;
    cfg->fmin = 50.0;
    cfg->order_spec = 64;
    cfg->order_bap = 5;
    cfg->output_path = NULL;
}

void print_usage(const TCHAR *argv0)
{
    _ftprintf(stderr, _T("usage: %s file.wav [options]\n"), argv0);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "-b | --bias   <number>    Voicing bias\n");
    fprintf(stderr, "-B | --bap    <number>    Order for band aperiodicity compression\n");
    fprintf(stderr, "-e | --har_e  <number>    Maximum number of harmonics for noise envelopes\n");
    fprintf(stderr, "-f | --fmin   <number>    Minimum frequency for F0 extraction\n");
    fprintf(stderr, "-F | --fmax   <number>    Maximum frequency for F0 extraction\n");
    fprintf(stderr, "-h | --hop    <number>    Size of a hop window\n");
    fprintf(stderr, "-H | --har    <number>    Maximum number of harmonics\n");
    fprintf(stderr, "-n | --fft    <number>    FFT size for LLSM analysis\n");
    fprintf(stderr, "-o | --output <path>      Path of output file\n");
    fprintf(stderr, "-p | --psd    <number>    Size of the PSD vector\n");
    fprintf(stderr, "-s | --spec   <number>    Order for spectrum compression\n");
    fprintf(stderr, "-t | --trange <number>    Maximum allowed transition distance\n");
    fprintf(stderr, "On success, stdout contains f0,rd per frame, one frame per line (no header).\n");
}

// ==============================================================================
// 经过宏隔离后，真正的执行入口。该宏依据平台会自动展开为 int wmain 或 int main
// ==============================================================================
int MAIN(argc, argv)
{
    int c;
    const TCHAR *filename = NULL;
    TCHAR *temp_path = NULL;
    Cfg cfg;
    int err = LLSM_ERR_OK;

    if (argc < 2) {
        print_usage(argv[0]);
        return LLSM_ERR_INVALID_DATA;
    }

    struct optparse_long long_options[] = {
        {"bias",   'b', OPTPARSE_REQUIRED},
        {"bap",    'B', OPTPARSE_REQUIRED},
        {"har_e",  'e', OPTPARSE_REQUIRED},
        {"fmin",   'f', OPTPARSE_REQUIRED},
        {"fmax",   'F', OPTPARSE_REQUIRED},
        {"hop",    'h', OPTPARSE_REQUIRED},
        {"har",    'H', OPTPARSE_REQUIRED},
        {"fft",    'n', OPTPARSE_REQUIRED},
        {"output", 'o', OPTPARSE_REQUIRED},
        {"psd",    'p', OPTPARSE_REQUIRED},
        {"spec",   's', OPTPARSE_REQUIRED},
        {"trange", 't', OPTPARSE_REQUIRED},
        {"help",   '?', OPTPARSE_NONE},
        {0, 0, OPTPARSE_NONE}
    };

    cfg_init(&cfg);

    optparse_tchar_t opts;
    optparse_tchar_init(&opts, argc, argv);

    while ((c = optparse_tchar_long(&opts, long_options, NULL)) != -1)
    {
        switch (c)
        {
        case 'b': cfg.bias       = atof(opts.opt.optarg); break;
        case 'B': cfg.order_bap  = atoi(opts.opt.optarg); break;
        case 'e': cfg.maxnhar_e  = atoi(opts.opt.optarg); break;
        case 'f': cfg.fmin       = atof(opts.opt.optarg); break;
        case 'F': cfg.fmax       = atof(opts.opt.optarg); break;
        case 'h': cfg.nhop       = atoi(opts.opt.optarg); break;
        case 'H': cfg.maxnhar    = atoi(opts.opt.optarg); break;
        case 'n': cfg.nfft       = atoi(opts.opt.optarg); break;
        case 'o': cfg.output_path = opts.optarg_tchar;    break; // 直接取解析好的 TCHAR 指针
        case 'p': cfg.npsd       = atoi(opts.opt.optarg); break;
        case 's': cfg.order_spec = atoi(opts.opt.optarg); break;
        case 't': cfg.trange     = atoi(opts.opt.optarg); break;
        case '?':
            if (opts.optopt == '?') {
                print_usage(argv[0]);
                optparse_tchar_free(&opts);
                return LLSM_ERR_OK;
            }
            fprintf(stderr, "%s\n", opts.errmsg);
            optparse_tchar_free(&opts);
            return LLSM_ERR_INVALID_DATA;
        }
    }

    filename = optparse_tchar_arg(&opts);
    if (filename == NULL)
    {
        fprintf(stderr, "Filename not specified\n");
        optparse_tchar_free(&opts);
        return LLSM_ERR_INVALID_DATA;
    }

    temp_path = (TCHAR *)malloc((_tcslen(filename) + _tcslen(OUTPUT_EXTENSION) + 1) * sizeof(TCHAR));
    if (temp_path == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        optparse_tchar_free(&opts);
        return LLSM_ERR_MEM_ALLOC;
    }

    _tcscpy(temp_path, filename);
    TCHAR *ext_ptr = _tcsrchr(temp_path, _T('.'));
    // 跨平台封装的 _tcsicmp 会在不同系统安全处理大小写判断
    if (ext_ptr && _tcsicmp(ext_ptr, INPUT_EXTENSION) == 0)
    {
        *ext_ptr = _T('\0');
    }
    else
    {
        _ftprintf(stderr, _T("Input file must be %s\n"), INPUT_EXTENSION);
        free(temp_path);
        optparse_tchar_free(&opts);
        return LLSM_ERR_INVALID_DATA;
    }

    if (cfg.output_path == NULL)
    {
        _tcscat(temp_path, OUTPUT_EXTENSION);
        cfg.output_path = temp_path;
    }

    // 核心业务流定义
    FP_TYPE *x = NULL;
    FP_TYPE *f0 = NULL;
    llsm_aoptions *opt_a = NULL;
    llsm_soptions *opt_s = NULL;
    llsm_coptions *opt_c = NULL;
    llsm_chunk *chunk = NULL;

    do
    {
        FILE *fp = _tfopen(filename, _T("rb"));
        if (!fp)
        {
            _ftprintf(stderr, _T("Failed to open WAV file\n"));
            err = LLSM_ERR_FILE_OPEN;
            break;
        }

        x = wavread_fp(fp, &cfg.fs, &cfg.nbit, &cfg.nx);
        fclose(fp);

        if (!x)
        {
            _ftprintf(stderr, _T("Failed to decode WAV file '%s' or file is empty\n"), filename);
            err = LLSM_ERR_INVALID_DATA;
            break;
        }

        pyin_config param = pyin_init(cfg.nhop);
        param.fmin = cfg.fmin;
        param.fmax = cfg.fmax;
        param.trange = cfg.trange;
        param.bias = cfg.bias;
        param.nf = (int)ceil(cfg.fs * 0.025);

        f0 = pyin_analyze(param, x, cfg.nx, cfg.fs, &cfg.nfrm);
        if (f0 == NULL)
        {
            fprintf(stderr, "F0 extraction failed\n");
            err = LLSM_ERR_MEM_ALLOC;
            break;
        }

        opt_a = llsm_create_aoptions();
        if (!opt_a)
        {
            err = LLSM_ERR_MEM_ALLOC;
            break;
        }
        opt_a->thop = (FP_TYPE)cfg.nhop / cfg.fs;
        opt_a->npsd = cfg.npsd;
        opt_a->maxnhar = cfg.maxnhar;
        opt_a->maxnhar_e = cfg.maxnhar_e;

        opt_s = llsm_create_soptions(cfg.fs);
        if (!opt_s)
        {
            err = LLSM_ERR_MEM_ALLOC;
            break;
        }

        opt_c = (llsm_coptions*)calloc(1, sizeof(llsm_coptions));
        if (!opt_c)
        {
            err = LLSM_ERR_MEM_ALLOC;
            break;
        }
        opt_c->order_bap = cfg.order_bap;
        opt_c->order_spec = cfg.order_spec;

        chunk = llsm_analyze(opt_a, x, cfg.nx, cfg.fs, f0, cfg.nfrm, NULL);
        if (!chunk)
        {
            fprintf(stderr, "LLSM analysis failed\n");
            err = LLSM_ERR_MEM_ALLOC;
            break;
        }

        llsm_chunk_tolayer1(chunk, cfg.nfft);
        llsm_chunk_phasesync_rps(chunk, 1);
        opt_s->use_l1 = 1;
        for (int i = 0; i < cfg.nfrm; i++)
        {
            llsm_container_attach(chunk->frames[i], LLSM_FRAME_HM, NULL, NULL, NULL);
        }

        err = llsm_write_cache_file(cfg.output_path, opt_a, opt_s, opt_c, chunk, cfg.nfrm);
        if (err != LLSM_ERR_OK)
        {
            _ftprintf(stderr, _T("Write %s failed with error code %d\n"), cfg.output_path, err);
            break;
        }

        // Export F0 and Rd from the final analyzed frames.
        // Emit only after the cache file has been written successfully.
        for (int i = 0; i < cfg.nfrm; ++i)
        {
            const FP_TYPE *frame_f0 = chunk->frames[i]
                ? (const FP_TYPE *)llsm_container_get(chunk->frames[i], LLSM_FRAME_F0)
                : NULL;
            const FP_TYPE *frame_rd = chunk->frames[i]
                ? (const FP_TYPE *)llsm_container_get(chunk->frames[i], LLSM_FRAME_RD)
                : NULL;
            if (fprintf(stdout, "%g,%g\n",
                        frame_f0 ? (double)*frame_f0 : 0.0,
                        frame_rd ? (double)*frame_rd : 0.0) < 0)
            {
                err = LLSM_ERR_FILE_WRITE;
                break;
            }
        }
        if (fflush(stdout) == EOF)
            err = LLSM_ERR_FILE_WRITE;
        if (err != LLSM_ERR_OK)
            fprintf(stderr, "Failed to write F0/Rd frames to stdout\n");

    } while (0);

    // 统一下沉清理
    if (chunk) llsm_delete_chunk(chunk);
    if (opt_a) llsm_delete_aoptions(opt_a);
    if (opt_s) llsm_delete_soptions(opt_s);
    if (opt_c) llsm_delete_coptions(opt_c);
    if (f0) free(f0);
    if (x) free(x);
    if (temp_path) free(temp_path);
    
    // 安全卸载 optparse_tchar_t 包装器内存
    optparse_tchar_free(&opts);

    return err;
}
