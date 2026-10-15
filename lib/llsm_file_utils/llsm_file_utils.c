#include "llsm_file_utils.h"
#include <string.h>
#include <limits.h>
#include <math.h>

// 判断一帧是否可压缩
static bool llsm_is_frame_compressible(llsm_container *frame)
{
	if (!frame)
		return false;
	FP_TYPE *f0 = (FP_TYPE *)llsm_container_get(frame, LLSM_FRAME_F0);
	llsm_nmframe *nm = (llsm_nmframe *)llsm_container_get(frame, LLSM_FRAME_NM);
	if (!f0 || !nm || !nm->psd)
		return false;
	if (f0[0] > 0)
	{
		FP_TYPE *rd = (FP_TYPE *)llsm_container_get(frame, LLSM_FRAME_RD);
		FP_TYPE *vtmagn = (FP_TYPE *)llsm_container_get(frame, LLSM_FRAME_VTMAGN);
		if (!rd || !vtmagn)
			return false;
	}
	return true;
}

// 根据构造器获取 3bit 成员数据类型标识
static int get_member_type(llsm_fcopy copyctor)
{
	if (copyctor == (llsm_fcopy)llsm_copy_fparray)
		return LLSM_TYPE_FPARRAY;
	if (copyctor == (llsm_fcopy)llsm_copy_fp)
		return LLSM_TYPE_FP;
	if (copyctor == (llsm_fcopy)llsm_copy_hmframe)
		return LLSM_TYPE_HMFRAME;
	if (copyctor == (llsm_fcopy)llsm_copy_int)
		return LLSM_TYPE_INT;
	if (copyctor == (llsm_fcopy)llsm_copy_nmframe)
		return LLSM_TYPE_NMFRAME;
	return LLSM_TYPE_NULL;
}

static llsm_fcopy get_copyctor_by_type(int type)
{
	switch (type)
	{
	case LLSM_TYPE_FPARRAY:
		return (llsm_fcopy)llsm_copy_fparray;
	case LLSM_TYPE_FP:
		return (llsm_fcopy)llsm_copy_fp;
	case LLSM_TYPE_HMFRAME:
		return (llsm_fcopy)llsm_copy_hmframe;
	case LLSM_TYPE_INT:
		return (llsm_fcopy)llsm_copy_int;
	case LLSM_TYPE_NMFRAME:
		return (llsm_fcopy)llsm_copy_nmframe;
	}
	return NULL;
}

static llsm_fdestructor get_dtor_by_type(int type)
{
	switch (type)
	{
	case LLSM_TYPE_FPARRAY:
		return (llsm_fdestructor)llsm_delete_fparray;
	case LLSM_TYPE_FP:
		return (llsm_fdestructor)llsm_delete_fp;
	case LLSM_TYPE_HMFRAME:
		return (llsm_fdestructor)llsm_delete_hmframe;
	case LLSM_TYPE_INT:
		return (llsm_fdestructor)llsm_delete_int;
	case LLSM_TYPE_NMFRAME:
		return (llsm_fdestructor)llsm_delete_nmframe;
	}
	return NULL;
}

// 计算容器的 48bit 存在性位图
static uint64_t get_container_bitmap(llsm_container *c)
{
	uint64_t bmp = 0;
	if (!c)
		return 0;
	for (int i = 0; i < c->nmember && i < 16; ++i)
	{
		int type = get_member_type(c->copyctors[i]);
		bmp |= ((uint64_t)type & 0x7) << (i * 3);
	}
	return bmp;
}

static bool write_fparray_to_file(llsm_file_context *ctx, FP_TYPE *data, int len)
{
	if (len > 0)
	{
		if (fwrite(data, sizeof(FP_TYPE), len, ctx->fp) != (size_t)len)
			return false;
	}
	return true;
}

static FP_TYPE *read_fparray_from_file(llsm_file_context *ctx, int len)
{
	if (len <= 0 || len > INT_MAX / (int)sizeof(FP_TYPE))
		return NULL;

	FP_TYPE *data = (FP_TYPE *)malloc(len * sizeof(FP_TYPE));
	if (!data)
		return NULL;

	if (fread(data, sizeof(FP_TYPE), len, ctx->fp) != (size_t)len)
	{
		free(data);
		return NULL;
	}
	return data;
}

// === 成员级的读写 ===

static int llsm_write_member_to_file(llsm_file_context *ctx, void *member, int type, bool skip_nm_psd)
{
	int tmp_int, nhar_e;
	int err = LLSM_ERR_OK;

	switch (type)
	{
	case LLSM_TYPE_FPARRAY:
		tmp_int = llsm_fparray_length((FP_TYPE *)member);
		if (fwrite(&tmp_int, sizeof(int), 1, ctx->fp) != 1)
			return LLSM_ERR_FILE_WRITE;
		if (!write_fparray_to_file(ctx, (FP_TYPE *)member, tmp_int))
			return LLSM_ERR_FILE_WRITE;
		break;
	case LLSM_TYPE_FP:
		if (fwrite(member, sizeof(FP_TYPE), 1, ctx->fp) != 1)
			return LLSM_ERR_FILE_WRITE;
		break;
	case LLSM_TYPE_HMFRAME:
	{
		llsm_hmframe *hm = (llsm_hmframe *)member;
		if (fwrite(&hm->nhar, sizeof(int), 1, ctx->fp) != 1)
			return LLSM_ERR_FILE_WRITE;
		if (hm->nhar > 0)
		{
			if (!write_fparray_to_file(ctx, hm->ampl, hm->nhar))
				return LLSM_ERR_FILE_WRITE;
			if (!write_fparray_to_file(ctx, hm->phse, hm->nhar))
				return LLSM_ERR_FILE_WRITE;
		}
		break;
	}
	case LLSM_TYPE_INT:
		if (fwrite(member, sizeof(int), 1, ctx->fp) != 1)
			return LLSM_ERR_FILE_WRITE;
		break;
	case LLSM_TYPE_NMFRAME:
	{
		llsm_nmframe *nm = (llsm_nmframe *)member;
		nhar_e = (ctx->nchannel > 0 && nm->eenv[0] != NULL) ? nm->eenv[0]->nhar : 0;
		if (fwrite(&nhar_e, sizeof(int), 1, ctx->fp) != 1)
			return LLSM_ERR_FILE_WRITE;

		for (int i = 0; i < ctx->nchannel; i++)
		{
			err = llsm_write_member_to_file(ctx, nm->eenv[i], LLSM_TYPE_HMFRAME, skip_nm_psd);
			if (err != LLSM_ERR_OK)
				return err;
		}
		if (ctx->nchannel > 0 && !write_fparray_to_file(ctx, nm->edc, ctx->nchannel))
			return LLSM_ERR_FILE_WRITE;
		if (ctx->npsd > 0 && !skip_nm_psd && !write_fparray_to_file(ctx, nm->psd, ctx->npsd))
			return LLSM_ERR_FILE_WRITE;
		break;
	}
	default:
		break;
	}
	return LLSM_ERR_OK;
}

static int llsm_read_member_from_file(llsm_file_context *ctx, void **out_member, int type, bool skip_nm_psd)
{
	int tmp_int, nhar_e;
	FP_TYPE tmp_fp;
	*out_member = NULL;

	switch (type)
	{
	case LLSM_TYPE_FPARRAY:
	{
		int len = 0;
		if (fread(&len, sizeof(int), 1, ctx->fp) != 1 || len < 0 || len > LLSM_FILE_READ_LEN_MAX)
			return LLSM_ERR_FILE_READ;

		FP_TYPE *raw_data = read_fparray_from_file(ctx, len);
		if (len > 0 && !raw_data)
			return LLSM_ERR_FILE_READ;

		FP_TYPE *arr = llsm_create_fparray(len);
		if (!arr)
		{
			free(raw_data);
			return LLSM_ERR_MEM_ALLOC;
		}
		if (len > 0)
		{
			memcpy(arr, raw_data, len * sizeof(FP_TYPE));
			free(raw_data);
		}
		*out_member = arr;
		break;
	}
	case LLSM_TYPE_FP:
	{
		if (fread(&tmp_fp, sizeof(FP_TYPE), 1, ctx->fp) != 1)
			return LLSM_ERR_FILE_READ;
		*out_member = llsm_create_fp(tmp_fp);
		if (!*out_member)
			return LLSM_ERR_MEM_ALLOC;
		break;
	}
	case LLSM_TYPE_HMFRAME:
	{
		if (fread(&tmp_int, sizeof(int), 1, ctx->fp) != 1 || tmp_int < 0 || tmp_int > LLSM_FILE_READ_LEN_MAX)
			return LLSM_ERR_FILE_READ;
		llsm_hmframe *hm = llsm_create_hmframe(tmp_int);
		if (!hm)
			return LLSM_ERR_MEM_ALLOC;

		if (tmp_int > 0)
		{
			FP_TYPE *ampl = read_fparray_from_file(ctx, tmp_int);
			FP_TYPE *phse = read_fparray_from_file(ctx, tmp_int);
			if (!ampl || !phse)
			{
				free(ampl);
				free(phse);
				llsm_delete_hmframe(hm);
				return LLSM_ERR_FILE_READ;
			}
			memcpy(hm->ampl, ampl, tmp_int * sizeof(FP_TYPE));
			memcpy(hm->phse, phse, tmp_int * sizeof(FP_TYPE));
			free(ampl);
			free(phse);
		}
		*out_member = hm;
		break;
	}
	case LLSM_TYPE_INT:
	{
		if (fread(&tmp_int, sizeof(int), 1, ctx->fp) != 1)
			return LLSM_ERR_FILE_READ;
		*out_member = llsm_create_int(tmp_int);
		if (!*out_member)
			return LLSM_ERR_MEM_ALLOC;
		break;
	}
	case LLSM_TYPE_NMFRAME:
	{
		if (fread(&nhar_e, sizeof(int), 1, ctx->fp) != 1 || nhar_e < 0 || nhar_e > LLSM_FILE_READ_LEN_MAX)
			return LLSM_ERR_FILE_READ;

		// 根据透传的上下文创建 nmframe
		llsm_nmframe *nm = llsm_create_nmframe(ctx->nchannel, nhar_e, ctx->npsd);
		if (!nm)
			return LLSM_ERR_MEM_ALLOC;

		// 递归读取每个 channel 的谐波包络 (HMFRAME)
		for (int i = 0; i < ctx->nchannel; i++)
		{
			void *hm_member = NULL;
			// 释放 create_nmframe 默认分配的空 hmframe，避免内存泄漏
			llsm_delete_hmframe(nm->eenv[i]);
            nm->eenv[i] = NULL;
			int err = llsm_read_member_from_file(ctx, &hm_member, LLSM_TYPE_HMFRAME, skip_nm_psd);
			if (err != LLSM_ERR_OK)
			{
				llsm_delete_nmframe(nm);
				return err;
			}
			nm->eenv[i] = (llsm_hmframe *)hm_member;
		}

		// 读取 EDC 数组 (直接使用 ctx->nchannel 作为长度)
		if (ctx->nchannel > 0)
		{
			FP_TYPE *edc = read_fparray_from_file(ctx, ctx->nchannel);
			if (!edc)
			{
				llsm_delete_nmframe(nm);
				return LLSM_ERR_FILE_READ;
			}
			memcpy(nm->edc, edc, ctx->nchannel * sizeof(FP_TYPE));
			free(edc);
		}

		// 读取 PSD 数组 (直接使用 ctx->npsd 作为长度)
		if (ctx->npsd > 0 && !skip_nm_psd)
		{
			FP_TYPE *psd = read_fparray_from_file(ctx, ctx->npsd);
			if (!psd)
			{
				llsm_delete_nmframe(nm);
				return LLSM_ERR_FILE_READ;
			}
			memcpy(nm->psd, psd, ctx->npsd * sizeof(FP_TYPE));
			free(psd);
		}

		*out_member = nm;
		break;
	}
	default:
		return LLSM_ERR_INVALID_DATA;
	}
	return LLSM_ERR_OK;
}

// === 容器与 Chunk 的读写 ===

static int llsm_write_container_to_file(llsm_file_context *ctx, llsm_container *src, uint64_t bmp, bool is_comp)
{
	for (int i = 0; i < 16; i++)
	{
		int type = (bmp >> (i * 3)) & 0x7;
        if (type > LLSM_TYPE_NMFRAME) return LLSM_ERR_INVALID_DATA;
		if (type != LLSM_TYPE_NULL)
		{
			bool skip_nm_psd = is_comp && (i == LLSM_FRAME_NM);
			int err = llsm_write_member_to_file(ctx, src->members[i], type, skip_nm_psd);
			if (err != LLSM_ERR_OK)
				return err;
		}
	}
	return LLSM_ERR_OK;
}

static int llsm_read_container_from_file(llsm_file_context *ctx, llsm_container **out_cont, uint64_t bmp, bool is_comp)
{
	int nmember = 0;
	for (int i = 0; i < 16; i++)
	{
		if (((bmp >> (i * 3)) & 0x7) != LLSM_TYPE_NULL)
		{
			nmember = i + 1; // 找到最后一位非 NULL 的元素索引
		}
	}

	llsm_container *ret = llsm_create_container(nmember);
	if (!ret)
		return LLSM_ERR_MEM_ALLOC;

	for (int i = 0; i < 16; i++)
	{
		int type = (bmp >> (i * 3)) & 0x7;
        if (type > LLSM_TYPE_NMFRAME) { llsm_delete_container(ret); return LLSM_ERR_INVALID_DATA; }
		if (type != LLSM_TYPE_NULL)
		{
			void *member_ptr = NULL;
			bool skip_nm_psd = is_comp && (i == LLSM_FRAME_NM);
			int err = llsm_read_member_from_file(ctx, &member_ptr, type, skip_nm_psd);
			if (err != LLSM_ERR_OK)
			{
				llsm_delete_container(ret);
				return err;
			}
			llsm_container_attach_(ret, i, member_ptr, get_dtor_by_type(type), get_copyctor_by_type(type));
		}
	}
	*out_cont = ret;
	return LLSM_ERR_OK;
}

static int llsm_write_chunk_to_file(llsm_file_context *ctx, llsm_chunk *src, int nfrm)
{
	if (fwrite(&nfrm, sizeof(int), 1, ctx->fp) != 1)
		return LLSM_ERR_FILE_WRITE;

	int total_containers = 1 + nfrm;
	uint64_t *bmps = (uint64_t *)malloc(total_containers * sizeof(uint64_t));
	if (!bmps)
		return LLSM_ERR_MEM_ALLOC;

	bmps[0] = get_container_bitmap(src->conf);
	for (int i = 0; i < nfrm; i++)
	{
		uint64_t bmp = get_container_bitmap(src->frames[i]);
		if (ctx->copt && llsm_is_frame_compressible(src->frames[i]))
		{
			bmp |= (1ULL << LLSM_RLE_COMPRESSED_BIT); // 打上已压缩标记
		}
		bmps[1 + i] = bmp;
	}

	uint64_t *rle_out = (uint64_t *)malloc(total_containers * sizeof(uint64_t));
	if (!rle_out)
	{
		free(bmps);
		return LLSM_ERR_MEM_ALLOC;
	}

	int rle_count = 0, current_run = 1;
	uint64_t last_bmp = bmps[0];

	// RLE 行程计算 (改用宏)
	for (int i = 1; i < total_containers; i++)
	{
		if (bmps[i] == last_bmp && current_run < LLSM_RLE_MAX_RUN_15BIT)
		{
			++current_run;
		}
		else
		{
			rle_out[rle_count++] = last_bmp | ((uint64_t)current_run << LLSM_RLE_RUN_SHIFT);
			last_bmp = bmps[i];
			current_run = 1;
		}
	}
	rle_out[rle_count++] = last_bmp | ((uint64_t)current_run << LLSM_RLE_RUN_SHIFT);

	if (fwrite(rle_out, sizeof(uint64_t), rle_count, ctx->fp) != (size_t)rle_count)
	{
		free(bmps);
		free(rle_out);
		return LLSM_ERR_FILE_WRITE;
	}
	free(rle_out);

	// 首先写入配置容器
	int err = llsm_write_container_to_file(ctx, src->conf, bmps[0], false);
	if (err != LLSM_ERR_OK)
	{
		free(bmps);
		return err;
	}

	for (int i = 0; i < nfrm; i++)
	{
		bool comp = (bmps[1 + i] & (1ULL << LLSM_RLE_COMPRESSED_BIT)) != 0;

		// 1. 如果该帧支持压缩，先调用 Coder 进行压缩与写入
		if (comp)
		{
			llsm_coder *coder = llsm_create_coder(src->conf, ctx->copt->order_spec, ctx->copt->order_bap);
			FP_TYPE *enc = llsm_coder_encode(coder, src->frames[i]);
			int enc_size = ctx->copt->order_spec + ctx->copt->order_bap + 3;
			if (!write_fparray_to_file(ctx, enc, enc_size))
			{
				free(enc);
				llsm_delete_coder(coder);
				free(bmps);
				return LLSM_ERR_FILE_WRITE;
			}
			free(enc);
			llsm_delete_coder(coder);
		}

		// 2. 掩除那些已经被压缩的内容 (以免写入)，再正常写入
		uint64_t write_bmp = bmps[1 + i] & LLSM_RLE_BMP_MASK;
		if (comp)
		{
			write_bmp &= ~((7ULL << (LLSM_FRAME_F0 * 3)) |
						   (7ULL << (LLSM_FRAME_RD * 3)) |
						   (7ULL << (LLSM_FRAME_VTMAGN * 3)));
		}
		err = llsm_write_container_to_file(ctx, src->frames[i], write_bmp, comp);
		if (err != LLSM_ERR_OK)
		{
			free(bmps);
			return err;
		}
	}

	free(bmps);
	return LLSM_ERR_OK;
}

static void delete_partial_chunk(llsm_chunk *chunk, int nfrm)
{
    if (!chunk) return;
    if (chunk->frames) {
        for (int i=0; i<nfrm; ++i) llsm_delete_container(chunk->frames[i]);
        free(chunk->frames);
    }
    llsm_delete_container(chunk->conf);
    free(chunk);
}

static bool config_types_valid(uint64_t bmp)
{
    const int ints[] = {LLSM_CONF_NFRM, LLSM_CONF_MAXNHAR, LLSM_CONF_MAXNHAR_E,
        LLSM_CONF_NPSD, LLSM_CONF_NCHANNEL, LLSM_CONF_NSPEC};
    const int fps[] = {LLSM_CONF_THOP, LLSM_CONF_FNYQ, LLSM_CONF_LIPRADIUS};
    for (unsigned i=0;i<sizeof(ints)/sizeof(ints[0]);++i)
        if (((bmp >> (ints[i]*3)) & 7) != LLSM_TYPE_INT) return false;
    for (unsigned i=0;i<sizeof(fps)/sizeof(fps[0]);++i)
        if (((bmp >> (fps[i]*3)) & 7) != LLSM_TYPE_FP) return false;
    return ((bmp >> (LLSM_CONF_CHANFREQ*3)) & 7) == LLSM_TYPE_FPARRAY;
}

static int llsm_read_chunk_from_file(llsm_file_context *ctx, llsm_chunk **out_chunk, int *out_nfrm)
{
	int nfrm;
	if (fread(&nfrm, sizeof(int), 1, ctx->fp) != 1 || nfrm <= 0 || nfrm > LLSM_FILE_READ_LEN_MAX)
		return LLSM_ERR_FILE_READ;

	int total_containers = 1 + nfrm;
	uint64_t *bmps = (uint64_t *)malloc(total_containers * sizeof(uint64_t));
	if (!bmps)
		return LLSM_ERR_MEM_ALLOC;

	// 解析 RLE
	int decoded = 0;
	while (decoded < total_containers)
	{
		uint64_t rle_val;
		if (fread(&rle_val, sizeof(uint64_t), 1, ctx->fp) != 1)
		{
			free(bmps);
			return LLSM_ERR_FILE_READ;
		}

		// 屏蔽行程位，保留存在的 48 bit 和可能存在的第 48 位(压缩标记)
		uint64_t bmp = rle_val & (LLSM_RLE_BMP_MASK | (1ULL << LLSM_RLE_COMPRESSED_BIT));
		int count = (rle_val >> LLSM_RLE_RUN_SHIFT) & LLSM_RLE_RUN_MASK;
		if (count <= 0 || count > total_containers-decoded) { free(bmps); return LLSM_ERR_INVALID_DATA; }
		for (int i = 0; i < count; i++)
		{
			bmps[decoded++] = bmp;
		}
	}

	if ((bmps[0] & (1ULL << LLSM_RLE_COMPRESSED_BIT)) || !config_types_valid(bmps[0])) { free(bmps); return LLSM_ERR_INVALID_DATA; }

	llsm_chunk *ret = (llsm_chunk *)calloc(1, sizeof(llsm_chunk));
	if (!ret)
	{
		free(bmps);
		return LLSM_ERR_MEM_ALLOC;
	}

	int err = llsm_read_container_from_file(ctx, &ret->conf, bmps[0] & LLSM_RLE_BMP_MASK, false);
	if (err != LLSM_ERR_OK)
	{
		free(bmps);
		delete_partial_chunk(ret, nfrm);
		return err;
	}

    int *conf_n = llsm_container_get(ret->conf, LLSM_CONF_NFRM);
    int *conf_nc = llsm_container_get(ret->conf, LLSM_CONF_NCHANNEL);
    int *conf_np = llsm_container_get(ret->conf, LLSM_CONF_NPSD);
    int *conf_ns = llsm_container_get(ret->conf, LLSM_CONF_NSPEC);
    FP_TYPE *thop = llsm_container_get(ret->conf, LLSM_CONF_THOP);
    FP_TYPE *fnyq = llsm_container_get(ret->conf, LLSM_CONF_FNYQ);
    FP_TYPE *lip = llsm_container_get(ret->conf, LLSM_CONF_LIPRADIUS);
    FP_TYPE *chanfreq = llsm_container_get(ret->conf, LLSM_CONF_CHANFREQ);
    if (!conf_n || *conf_n!=nfrm || !conf_nc || *conf_nc!=ctx->nchannel ||
        !conf_np || *conf_np!=ctx->npsd || !conf_ns || *conf_ns<2 || *conf_ns>LLSM_FILE_READ_LEN_MAX ||
        !thop || !isfinite(*thop) || *thop<=0 || !fnyq || !isfinite(*fnyq) || *fnyq<=0 ||
        !lip || !isfinite(*lip) || *lip<=0 || !chanfreq || llsm_fparray_length(chanfreq)<*conf_nc-1) {
        free(bmps);delete_partial_chunk(ret,nfrm);return LLSM_ERR_INVALID_DATA;
    }

	if (nfrm > 0)
	{
		ret->frames = (llsm_container **)calloc(nfrm, sizeof(llsm_container *));
		if (!ret->frames)
		{
			free(bmps);
			delete_partial_chunk(ret, nfrm);
			return LLSM_ERR_MEM_ALLOC;
		}

		for (int i = 0; i < nfrm; i++)
		{
            uint64_t frame_bmp=bmps[1+i];
            const int indices[]={LLSM_FRAME_F0,LLSM_FRAME_HM,LLSM_FRAME_NM,LLSM_FRAME_PSDRES,
                LLSM_FRAME_RD,LLSM_FRAME_VTMAGN,LLSM_FRAME_VSPHSE};
            const int types[]={LLSM_TYPE_FP,LLSM_TYPE_HMFRAME,LLSM_TYPE_NMFRAME,LLSM_TYPE_FPARRAY,
                LLSM_TYPE_FP,LLSM_TYPE_FPARRAY,LLSM_TYPE_FPARRAY};
            bool valid=true;
            for (unsigned k=0;k<sizeof(indices)/sizeof(indices[0]);++k) {
                int t=(frame_bmp>>(indices[k]*3))&7;
                if (t && t!=types[k]) valid=false;
            }
            if(!valid) { err=LLSM_ERR_INVALID_DATA;break; }

			bool comp = (bmps[1 + i] & (1ULL << LLSM_RLE_COMPRESSED_BIT)) != 0;
			llsm_container *decoded_frame = NULL;

			// 1. 如果该帧标记为被压缩，先读出定长块并通过 Coder 映射到 Layer 1
			if (comp)
			{
				int enc_size = ctx->copt->order_spec + ctx->copt->order_bap + 3;
				FP_TYPE *enc = read_fparray_from_file(ctx, enc_size);
				if (!enc)
				{
					err = LLSM_ERR_FILE_READ;
					break;
				}
				llsm_coder *coder = llsm_create_coder(ret->conf, ctx->copt->order_spec, ctx->copt->order_bap);
				decoded_frame = llsm_coder_decode_layer1(coder, enc); // Layer 1 还原可获得 VTMAGN/RD
				free(enc);
				llsm_delete_coder(coder);
			}

			uint64_t read_bmp = bmps[1 + i] & LLSM_RLE_BMP_MASK;
			if (comp)
			{
				read_bmp &= ~((7ULL << (LLSM_FRAME_F0 * 3)) |
							  (7ULL << (LLSM_FRAME_RD * 3)) |
							  (7ULL << (LLSM_FRAME_VTMAGN * 3)));
			}

			// 2. 读取保留下原本存在的非压缩成员
			llsm_container *frame = NULL;
			err = llsm_read_container_from_file(ctx, &frame, read_bmp, comp);
			if (err != LLSM_ERR_OK)
			{
				if (decoded_frame)
					llsm_delete_container(decoded_frame);
				break;
			}

			// 3. 透明合并（如果被压缩过）
			if (comp)
			{
				llsm_nmframe *dec_nm = (llsm_nmframe *)llsm_container_get(decoded_frame, LLSM_FRAME_NM);
				llsm_nmframe *real_nm = (llsm_nmframe *)llsm_container_get(frame, LLSM_FRAME_NM);
				// PSD 已在 read_container 被故意跳过填充(全是0)，在此从解码帧接管
				if (dec_nm && real_nm && ctx->npsd > 0)
				{
					memcpy(real_nm->psd, dec_nm->psd, ctx->npsd * sizeof(FP_TYPE));
				}
				// 利用 Attach 进行安全的深拷贝回粘，保证对顶层代码无感
				void *f0 = llsm_container_get(decoded_frame, LLSM_FRAME_F0);
				if (f0)
					llsm_container_attach_(frame, LLSM_FRAME_F0, llsm_copy_fp((FP_TYPE *)f0), (llsm_fdestructor)llsm_delete_fp, (llsm_fcopy)llsm_copy_fp);
				void *rd = llsm_container_get(decoded_frame, LLSM_FRAME_RD);
				if (rd)
					llsm_container_attach_(frame, LLSM_FRAME_RD, llsm_copy_fp((FP_TYPE *)rd), (llsm_fdestructor)llsm_delete_fp, (llsm_fcopy)llsm_copy_fp);
				void *vtmagn = llsm_container_get(decoded_frame, LLSM_FRAME_VTMAGN);
				if (vtmagn)
					llsm_container_attach_(frame, LLSM_FRAME_VTMAGN, llsm_copy_fparray((FP_TYPE *)vtmagn), (llsm_fdestructor)llsm_delete_fparray, (llsm_fcopy)llsm_copy_fparray);
				void *vsphse = llsm_container_get(decoded_frame, LLSM_FRAME_VSPHSE);
				if (vsphse)
					llsm_container_attach_(frame, LLSM_FRAME_VSPHSE, llsm_copy_fparray((FP_TYPE *)vsphse), (llsm_fdestructor)llsm_delete_fparray, (llsm_fcopy)llsm_copy_fparray);

				llsm_delete_container(decoded_frame);
			}
			ret->frames[i] = frame;
		}
		if (err != LLSM_ERR_OK)
		{
			free(bmps);
			delete_partial_chunk(ret, nfrm);
			return err;
		}
	}

	free(bmps);
	*out_chunk = ret;
	if (out_nfrm)
		*out_nfrm = nfrm;
	return LLSM_ERR_OK;
}

// === 对外公开 API ===

void llsm_delete_coptions(llsm_coptions *opt_c)
{
	if (opt_c)
		free(opt_c);
}

int llsm_write_cache_file(const TCHAR *file_path, llsm_aoptions *opt_a, llsm_soptions *opt_s, llsm_coptions *opt_c, llsm_chunk *chunk, int nfrm)
{
	if (!file_path || !opt_a || !opt_s || !opt_c || !chunk)
		return LLSM_ERR_NULL_PTR;

	FILE *fp = _tfopen(file_path, _T("wb"));
	if (!fp)
	{
		return LLSM_ERR_FILE_OPEN;
	}

	llsm_file_context ctx = {fp, opt_a->nchannel, opt_a->npsd, opt_c};
	int err = LLSM_ERR_OK;

	do
	{
		uint32_t header_magic = (sizeof(FP_TYPE) == 8) ? LLSM_MAGIC_DOUBLE : LLSM_MAGIC_FLOAT;
		if (fwrite(&header_magic, sizeof(uint32_t), 1, fp) != 1)
		{
			err = LLSM_ERR_FILE_WRITE;
			break;
		}

		if (fwrite(opt_a, sizeof(llsm_aoptions), 1, fp) != 1)
		{
			err = LLSM_ERR_FILE_WRITE;
			break;
		}
		if (opt_a->nchannel > 1)
		{
			if (fwrite(opt_a->chanfreq, sizeof(FP_TYPE), opt_a->nchannel - 1, fp) != (size_t)(opt_a->nchannel - 1))
			{
				err = LLSM_ERR_FILE_WRITE;
				break;
			}
		}

		if (fwrite(opt_s, sizeof(llsm_soptions), 1, fp) != 1)
		{
			err = LLSM_ERR_FILE_WRITE;
			break;
		}

		if (fwrite(opt_c, sizeof(llsm_coptions), 1, fp) != 1)
		{
			err = LLSM_ERR_FILE_WRITE;
			break;
		}

		err = llsm_write_chunk_to_file(&ctx, chunk, nfrm);
	} while (0);

	if (fclose(fp) != 0 && err == LLSM_ERR_OK) err = LLSM_ERR_FILE_WRITE;

	if (err != LLSM_ERR_OK)
	{
		_tremove(file_path);
	}
	return err;
}

int llsm_read_cache_file(const TCHAR *file_path, llsm_aoptions **opt_a, llsm_soptions **opt_s, llsm_coptions **opt_c, llsm_chunk **chunk, int *nfrm)
{
	if (!file_path || !opt_a || !opt_s || !opt_c || !chunk)
		return LLSM_ERR_NULL_PTR;

    *opt_a=NULL;*opt_s=NULL;*opt_c=NULL;*chunk=NULL;
    if(nfrm) *nfrm=0;

	FILE *fp = _tfopen(file_path, _T("rb"));
	if (!fp)
	{
		return LLSM_ERR_FILE_OPEN;
	}

	int err = LLSM_ERR_OK;
	llsm_aoptions *local_opt_a = NULL;
	llsm_soptions *local_opt_s = NULL;
	llsm_coptions *local_opt_c = NULL;
	llsm_chunk *local_chunk = NULL;

	do
	{
		uint32_t expected_magic = (sizeof(FP_TYPE) == 8) ? LLSM_MAGIC_DOUBLE : LLSM_MAGIC_FLOAT;
		uint32_t header = 0;
		if (fread(&header, sizeof(uint32_t), 1, fp) != 1 || header != expected_magic)
		{
			err = LLSM_ERR_INVALID_DATA;
			break;
		}

		local_opt_a = (llsm_aoptions *)calloc(1, sizeof(llsm_aoptions));
		if (!local_opt_a)
		{
			err = LLSM_ERR_MEM_ALLOC;
			break;
		}

		if (fread(local_opt_a, sizeof(llsm_aoptions), 1, fp) != 1)
		{
			err = LLSM_ERR_FILE_READ;
			break;
		}

        if(local_opt_a->nchannel<1 || local_opt_a->nchannel>64 || local_opt_a->npsd<2 || local_opt_a->npsd>65536) {
            err=LLSM_ERR_INVALID_DATA;break;
        }

		if (local_opt_a->nchannel > 0)
		{
			llsm_aoptions *temp_a = (llsm_aoptions *)realloc(local_opt_a, sizeof(llsm_aoptions) + (local_opt_a->nchannel - 1) * sizeof(FP_TYPE));
			if (!temp_a)
			{
				err = LLSM_ERR_MEM_ALLOC;
				break;
			}
			local_opt_a = temp_a;

			if (local_opt_a->nchannel > 1)
			{
				if (fread(local_opt_a->chanfreq, sizeof(FP_TYPE), local_opt_a->nchannel - 1, fp) != (size_t)(local_opt_a->nchannel - 1))
				{
					err = LLSM_ERR_FILE_READ;
					break;
				}
			}
		}

		local_opt_s = (llsm_soptions *)calloc(1, sizeof(llsm_soptions));
		if (!local_opt_s)
		{
			err = LLSM_ERR_MEM_ALLOC;
			break;
		}

		if (fread(local_opt_s, sizeof(llsm_soptions), 1, fp) != 1)
		{
			err = LLSM_ERR_FILE_READ;
			break;
		}

		local_opt_c = (llsm_coptions *)calloc(1, sizeof(llsm_coptions));
		if (!local_opt_c)
		{
			err = LLSM_ERR_MEM_ALLOC;
			break;
		}

		if (fread(local_opt_c, sizeof(llsm_coptions), 1, fp) != 1)
		{
			err = LLSM_ERR_FILE_READ;
			break;
		}

        if(local_opt_c->order_spec>65536 || local_opt_c->order_bap>65536 ||
           !isfinite(local_opt_s->fs) || local_opt_s->fs<=0) {
            err=LLSM_ERR_INVALID_DATA;break;
        }

		llsm_file_context ctx = {fp, local_opt_a->nchannel, local_opt_a->npsd, local_opt_c};
		err = llsm_read_chunk_from_file(&ctx, &local_chunk, nfrm);

	} while (0);

	fclose(fp);

	if (err == LLSM_ERR_OK)
	{
		*opt_a = local_opt_a;
		*opt_s = local_opt_s;
		*opt_c = local_opt_c;
		*chunk = local_chunk;
	}
	else
	{
		llsm_delete_coptions(local_opt_c);
		llsm_delete_aoptions(local_opt_a);
		llsm_delete_soptions(local_opt_s);
		if (local_chunk)
			llsm_delete_chunk(local_chunk);
		*opt_c = NULL;
		*opt_a = NULL;
		*opt_s = NULL;
		*chunk = NULL;
	}
	return err;
}
