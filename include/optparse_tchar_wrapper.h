#ifndef OPTPARSE_TCHAR_WRAPPER_H
#define OPTPARSE_TCHAR_WRAPPER_H

#include "optparse.h"
#include <stdlib.h>
#include <string.h>

#include "utau_pocket_tchar.h"
#ifdef _WIN32
#include <Windows.h>
#endif

/* ---------------- 判定是否需要寬字符轉換層 ---------------- */
#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
#define OPTPARSE_USE_WCHAR 1
#define MAIN(argc, argv) wmain(int argc, wchar_t *argv[])
#else
#define OPTPARSE_USE_WCHAR 0
#define MAIN(argc, argv) main(int argc, char *argv[])
#endif

/* ---------------- 結構體定義 (根據平台分支) ---------------- */
#if OPTPARSE_USE_WCHAR

typedef struct
{
	char *ansi_ptr;
	TCHAR *tchar_ptr;
} optparse_tchar_map_t;

typedef struct
{
	struct optparse opt;
	int argc;
	char **argv_ansi;
	optparse_tchar_map_t *map;

	TCHAR *optarg_tchar;
	char errmsg[64];
	int optopt;
} optparse_tchar_t;

#else

// 非 wmain 分支：結構體退化，實現零內存拷貝
typedef struct
{
	struct optparse opt;
	TCHAR *optarg_tchar;
	char errmsg[64];
	int optopt;
} optparse_tchar_t;

#endif

/* ---------------- 接口聲明 ---------------- */
#ifdef __cplusplus
extern "C"
{
#endif
	void optparse_tchar_init(optparse_tchar_t *ctx, int argc, TCHAR **argv);
	int optparse_tchar_long(optparse_tchar_t *ctx, const struct optparse_long *longopts, int *longindex);
	TCHAR *optparse_tchar_arg(optparse_tchar_t *ctx);
	void optparse_tchar_free(optparse_tchar_t *ctx);

/* ===================================================================== */
/* ---------------- 內部實現 (STB-Style) ------------------------------- */
/* ===================================================================== */
#ifdef OPTPARSE_TCHAR_WRAPPER_IMPLEMENTATION

#if OPTPARSE_USE_WCHAR
	/* --------- Windows Unicode 分支 (UTF-8 映射與反查) --------- */
	static TCHAR *optparse_tchar_recover(optparse_tchar_t *ctx, char *ansi_ptr)
	{
		if (!ansi_ptr)
			return NULL;
		for (int i = 0; i < ctx->argc; i++)
		{
			char *base = ctx->map[i].ansi_ptr;
			size_t len = strlen(base);
			if (ansi_ptr >= base && ansi_ptr <= base + len)
			{
				size_t byte_offset = ansi_ptr - base;
				int wchar_offset = 0;
				if (byte_offset > 0)
				{
					wchar_offset = MultiByteToWideChar(CP_UTF8, 0, base, (int)byte_offset, NULL, 0);
				}
				return ctx->map[i].tchar_ptr + wchar_offset;
			}
		}
		return NULL;
	}

	void optparse_tchar_init(optparse_tchar_t *ctx, int argc, TCHAR **argv)
	{
		ctx->argc = argc;
		// 分配內存以存儲 UTF-8 轉換結果與映射表
		ctx->argv_ansi = (char **)calloc(argc + 1, sizeof(char *));
		ctx->map = (optparse_tchar_map_t *)malloc(argc * sizeof(optparse_tchar_map_t));
		ctx->optarg_tchar = NULL;

		for (int i = 0; i < argc; i++)
		{
			ctx->map[i].tchar_ptr = argv[i];
			int len = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
			ctx->argv_ansi[i] = (char *)malloc(len);
			WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, ctx->argv_ansi[i], len, NULL, NULL);
			ctx->map[i].ansi_ptr = ctx->argv_ansi[i];
		}
		optparse_init(&ctx->opt, ctx->argv_ansi);
	}

	int optparse_tchar_long(optparse_tchar_t *ctx, const struct optparse_long *longopts, int *longindex)
	{
		int ret = optparse_long(&ctx->opt, longopts, longindex);
		ctx->optopt = ctx->opt.optopt;
		strcpy(ctx->errmsg, ctx->opt.errmsg);
		ctx->optarg_tchar = optparse_tchar_recover(ctx, ctx->opt.optarg);
		return ret;
	}

	TCHAR *optparse_tchar_arg(optparse_tchar_t *ctx)
	{
		char *ansi_arg = optparse_arg(&ctx->opt);
		return optparse_tchar_recover(ctx, ansi_arg);
	}

	void optparse_tchar_free(optparse_tchar_t *ctx)
	{
		for (int i = 0; i < ctx->argc; i++)
		{
			if (ctx->map[i].ansi_ptr)
				free(ctx->map[i].ansi_ptr);
		}
		free(ctx->argv_ansi);
		free(ctx->map);
	}

#else
	/* --------- POSIX / 非 Unicode 分支 (極致零拷貝) --------- */

	void optparse_tchar_init(optparse_tchar_t *ctx, int argc, TCHAR **argv)
	{
		(void)argc; // 原生 optparse 依賴 NULL 結尾，不需要 argc

		// 零分配：直接透傳原始的 argv 給 optparse
		optparse_init(&ctx->opt, (char **)argv);
		ctx->optarg_tchar = NULL;
		ctx->optopt = 0;
		ctx->errmsg[0] = '\0';
	}

	int optparse_tchar_long(optparse_tchar_t *ctx, const struct optparse_long *longopts, int *longindex)
	{
		int ret = optparse_long(&ctx->opt, longopts, longindex);

		ctx->optopt = ctx->opt.optopt;
		strcpy(ctx->errmsg, ctx->opt.errmsg);

		// 零查表：直接將 optarg 指針強轉回 TCHAR* (本質上都是 char*)
		ctx->optarg_tchar = (TCHAR *)ctx->opt.optarg;
		return ret;
	}

	TCHAR *optparse_tchar_arg(optparse_tchar_t *ctx)
	{
		// 零查表：直接返回 optparse 推進後的參數指針
		return (TCHAR *)optparse_arg(&ctx->opt);
	}

	void optparse_tchar_free(optparse_tchar_t *ctx)
	{
		// 零釋放：什麼都不用做
		(void)ctx;
	}

#endif // OPTPARSE_USE_WCHAR
#endif // OPTPARSE_TCHAR_WRAPPER_IMPLEMENTATION
#ifdef __cplusplus
};
#endif
#endif // OPTPARSE_TCHAR_WRAPPER_H
