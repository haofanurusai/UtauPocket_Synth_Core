#include "llsm_database.h"
#include <cmath>
#include <climits>
#include <memory>
#include <string>

namespace
{
    struct FileClose
    {
        void operator()(FILE *f) const
        {
            if (f)
                fclose(f);
        }
    };
    template <class T>
    T *member(llsm_container *c, int index, llsm_fcopy type)
    {
        return c && index < c->nmember && c->copyctors[index] == type
                   ? static_cast<T *>(c->members[index])
                   : nullptr;
    }
    FP_TYPE *fp(llsm_container *c, int i) { return member<FP_TYPE>(c, i, (llsm_fcopy)llsm_copy_fp); }
    int *ip(llsm_container *c, int i) { return member<int>(c, i, (llsm_fcopy)llsm_copy_int); }
    bool compatible(llsm_container *a, llsm_container *b)
    {
        for (int i : {LLSM_CONF_NPSD, LLSM_CONF_NCHANNEL, LLSM_CONF_NSPEC,
                      LLSM_CONF_MAXNHAR, LLSM_CONF_MAXNHAR_E})
        {
            auto x = ip(a, i), y = ip(b, i);
            if (!x || !y || *x != *y)
                return false;
        }
        for (int i : {LLSM_CONF_THOP, LLSM_CONF_FNYQ, LLSM_CONF_LIPRADIUS})
        {
            auto x = fp(a, i), y = fp(b, i);
            if (!x || !y || *x != *y)
                return false;
        }
        auto x = member<FP_TYPE>(a, LLSM_CONF_CHANFREQ, (llsm_fcopy)llsm_copy_fparray);
        auto y = member<FP_TYPE>(b, LLSM_CONF_CHANFREQ, (llsm_fcopy)llsm_copy_fparray);
        int n = *ip(a, LLSM_CONF_NCHANNEL) - 1;
        if (!x || !y || llsm_fparray_length(x) < n || llsm_fparray_length(y) < n)
            return false;
        for (int i = 0; i < n; ++i)
            if (x[i] != y[i])
                return false;
        return true;
    }
    bool same_options(const llsm_soptions &a, const llsm_soptions &b)
    {
        return a.fs == b.fs && a.use_iczt == b.use_iczt && a.use_l1 == b.use_l1 &&
               a.iczt_param_a == b.iczt_param_a && a.iczt_param_b == b.iczt_param_b;
    }
    int validate(llsm_chunk *c, int n, const llsm_soptions *s)
    {
        auto nf = ip(c->conf, LLSM_CONF_NFRM), nc = ip(c->conf, LLSM_CONF_NCHANNEL);
        auto np = ip(c->conf, LLSM_CONF_NPSD), ns = ip(c->conf, LLSM_CONF_NSPEC);
        auto hop = fp(c->conf, LLSM_CONF_THOP), nyq = fp(c->conf, LLSM_CONF_FNYQ);
        auto lip = fp(c->conf, LLSM_CONF_LIPRADIUS);
        auto cf = member<FP_TYPE>(c->conf, LLSM_CONF_CHANFREQ, (llsm_fcopy)llsm_copy_fparray);
        if (!nf || *nf != n || !nc || *nc < 1 || !np || *np < 2 || !ns || *ns < 2 ||
            !hop || !std::isfinite(*hop) || *hop <= 0 || !nyq || !std::isfinite(*nyq) || *nyq <= 0 ||
            !lip || !std::isfinite(*lip) || *lip <= 0 || !cf || llsm_fparray_length(cf) < *nc - 1)
            return LLSM_ERR_INVALID_DATA;
        double fs = s->fs, h = double(*hop) * fs;
        if (!std::isfinite(fs) || fs <= 0 || fs > INT_MAX || std::floor(fs) != fs ||
            !std::isfinite(h) || h < 1 || h > INT_MAX / 8 ||
            !std::isfinite(s->iczt_param_a) || !std::isfinite(s->iczt_param_b))
            return LLSM_ERR_INVALID_DATA;
        // libllsm2 uses 0.2 s internal rings, a one-second noise template,
        // a 32-sample template crossfade and a power-of-two filter window.
        double nfft = std::pow(2.0, std::ceil(std::log2(h * 2.2 + 32)));
        if (fs < 64 || nfft > fs * 0.2 || fs > INT_MAX / 8)
            return LLSM_ERR_UNSUPPORTED;
        FP_TYPE last = 0;
        for (int j = 0; j < *nc - 1; ++j)
        {
            if (!std::isfinite(cf[j]) || cf[j] <= last || cf[j] >= s->fs / 2)
                return LLSM_ERR_INVALID_DATA;
            last = cf[j];
        }
        for (int j = 0; j < n; ++j)
        {
            auto f = c->frames[j];
            auto f0 = fp(f, LLSM_FRAME_F0);
            auto rd = fp(f, LLSM_FRAME_RD);
            auto nm = member<llsm_nmframe>(f, LLSM_FRAME_NM, (llsm_fcopy)llsm_copy_nmframe);
            if (!f0 || !std::isfinite(*f0) || *f0 < 0 || !nm || nm->nchannel != *nc || nm->npsd != *np)
                return LLSM_ERR_INVALID_DATA;
            for (int k = 0; k < *np; ++k)
                if (!std::isfinite(nm->psd[k]))
                    return LLSM_ERR_INVALID_DATA;
            for (int k = 0; k < *nc; ++k)
                if (!std::isfinite(nm->edc[k]) || nm->edc[k] < 0)
                    return LLSM_ERR_INVALID_DATA;
            if (*f0 > 0)
            {
                auto vt = member<FP_TYPE>(f, LLSM_FRAME_VTMAGN, (llsm_fcopy)llsm_copy_fparray);
                auto ph = member<FP_TYPE>(f, LLSM_FRAME_VSPHSE, (llsm_fcopy)llsm_copy_fparray);
                if (!rd || !std::isfinite(*rd) || *rd <= 0 || !vt || llsm_fparray_length(vt) < *ns ||
                    !ph || llsm_fparray_length(ph) < 1)
                    return LLSM_ERR_INVALID_DATA;
            }
            auto original = llsm_create_fp(*f0), shifted = llsm_create_fp(0);
            auto original_rd = rd ? llsm_create_fp(*rd) : nullptr;
            if (!original || !shifted || (rd && !original_rd))
            {
                llsm_delete_fp(original);
                llsm_delete_fp(shifted);
                llsm_delete_fp(original_rd);
                return LLSM_ERR_MEM_ALLOC;
            }
            llsm_container_attach(f, UTAU_FRAME_F0_ORIGINAL, original, llsm_delete_fp, llsm_copy_fp);
            llsm_container_attach(f, UTAU_FRAME_PHASE_SHIFTED, shifted, llsm_delete_fp, llsm_copy_fp);
            if (original_rd)
                llsm_container_attach(f, UTAU_FRAME_RD_ORIGINAL, original_rd, llsm_delete_fp, llsm_copy_fp);
        }
        return 0;
    }
}

int LLSMDatabase::load(const TCHAR *path)
{
    std::unique_ptr<FILE, FileClose> file(_tfopen(path, _T("rb")));
    if (!file)
        return LLSM_ERR_FILE_OPEN;
    std::basic_string<TCHAR> base(path);
    auto pos = base.find_last_of(_T("/\\"));
    base = pos == base.npos ? std::basic_string<TCHAR>() : base.substr(0, pos + 1);
    int loaded = 0;
    for (;;)
    {
        LLSMDiskRecord disk{};
        size_t count = fread(&disk, 1, sizeof(disk), file.get());
        if (count == 0 && !ferror(file.get()))
            break;
        if (count != sizeof(disk))
            return LLSM_ERR_FILE_READ;
        unsigned bank = disk.midi_symbol & 255, prog = disk.midi_symbol >> 8;
        if (bank == 0 || bank > 127 || prog == 0 || prog > 127 || bank == '/' || bank == '\\' ||
            prog == '/' || prog == '\\' || bank == ':' || prog == ':' || records[disk.midi_symbol].chunk)
            return LLSM_ERR_INVALID_DATA;
        std::basic_string<TCHAR> name = base;
        name.push_back(static_cast<TCHAR>(bank));
        name.push_back(static_cast<TCHAR>(prog));
        name += _T(".LS2");
        llsm_aoptions *a = nullptr;
        llsm_soptions *s = nullptr;
        llsm_coptions *co = nullptr;
        auto &record = records[disk.midi_symbol];
        int n = 0;
        int err = llsm_read_cache_file(name.c_str(), &a, &s, &co, &record.chunk, &n);
        std::unique_ptr<llsm_aoptions, decltype(&llsm_delete_aoptions)> ag(a, llsm_delete_aoptions);
        std::unique_ptr<llsm_soptions, decltype(&llsm_delete_soptions)> sg(s, llsm_delete_soptions);
        std::unique_ptr<llsm_coptions, decltype(&llsm_delete_coptions)> cg(co, llsm_delete_coptions);
        if (err)
            return err;
        if (n <= 0 || n > UINT16_MAX || disk.nfrm != n || disk.vowel_lfrm >= disk.vowel_rfrm ||
            disk.vowel_rfrm >= n || disk.con > disk.vowel_lfrm || disk.pre > n || disk.ovl > n ||
            (disk.flag & ~uint16_t(1)))
            return LLSM_ERR_INVALID_DATA;
        err = validate(record.chunk, n, s);
        if (err)
            return err;
        if (opt_s && (!same_options(*opt_s, *s) || !compatible(conf, record.chunk->conf)))
            return LLSM_ERR_INVALID_DATA;
        if (!opt_s)
        {
            opt_s = sg.release();
            conf = record.chunk->conf;
        }
        static_cast<LLSMDiskRecord &>(record) = disk;
        record.inv_overlap = FP_TYPE(1) / (disk.ovl ? disk.ovl : 1);
        ++loaded;
    }
    if (fclose(file.release()) != 0)
        return LLSM_ERR_FILE_READ;
    return loaded ? 0 : LLSM_ERR_INVALID_DATA;
}
