/*
 * Copyright (c) Yann Collet, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef ZSTD_CCOMMON_H_MODULE
#define ZSTD_CCOMMON_H_MODULE

/* this module contains definitions which must be identical
 * across compression, decompression and dictBuilder.
 * It also contains a few functions useful to at least 2 of them
 * and which benefit from being inlined */

/*-*************************************
*  Dependencies
***************************************/
#include "compiler.h"
#include "mem.h"
#include "debug.h"                 /* assert, DEBUGLOG, RAWLOG, g_debuglevel */
#include "error_private.h"
#define ZSTD_STATIC_LINKING_ONLY
#include "include/zstd.h"
#define FSE_STATIC_LINKING_ONLY
#include "fse.h"
#define HUF_STATIC_LINKING_ONLY
#include "huf.h"
#include "include/xxhash.h"                /* XXH_reset, update, digest */
#define ZSTD_TRACE 0


/* ---- static assert (debug) --- */
#define ZSTD_STATIC_ASSERT(c) DEBUG_STATIC_ASSERT(c)
#define ZSTD_isError ERR_isError   /* for inlining */
#define FSE_isError  ERR_isError
#define HUF_isError  ERR_isError


/*-*************************************
*  shared macros
***************************************/
#undef MIN
#undef MAX
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))

/*
 * Ignore: this is an internal helper.
 *
 * This is a helper function to help force C99-correctness during compilation.
 * Under strict compilation modes, variadic macro arguments can't be empty.
 * However, variadic function arguments can be. Using a function therefore lets
 * us statically check that at least one (string) argument was passed,
 * independent of the compilation flags.
 */
static INLINE_KEYWORD UNUSED_ATTR
void _force_has_format_string(const char *format, ...) {
  (void)format;
}

/*
 * Ignore: this is an internal helper.
 *
 * We want to force this function invocation to be syntactically correct, but
 * we don't want to force runtime evaluation of its arguments.
 */
#define _FORCE_HAS_FORMAT_STRING(...) \
  if (0) { \
    _force_has_format_string(__VA_ARGS__); \
  }

/*
 * Return the specified error if the condition evaluates to true.
 *
 * In debug modes, prints additional information.
 * In order to do that (particularly, printing the conditional that failed),
 * this can't just wrap RETURN_ERROR().
 */
#define RETURN_ERROR_IF(cond, err, ...) \
  if (cond) { \
    RAWLOG(3, "%s:%d: ERROR!: check %s failed, returning %s", \
           __FILE__, __LINE__, ZSTD_QUOTE(cond), ZSTD_QUOTE(ERROR(err))); \
    _FORCE_HAS_FORMAT_STRING(__VA_ARGS__); \
    RAWLOG(3, ": " __VA_ARGS__); \
    RAWLOG(3, "\n"); \
    return ERROR(err); \
  }

/*
 * Unconditionally return the specified error.
 *
 * In debug modes, prints additional information.
 */
#define RETURN_ERROR(err, ...) \
  do { \
    RAWLOG(3, "%s:%d: ERROR!: unconditional check failed, returning %s", \
           __FILE__, __LINE__, ZSTD_QUOTE(ERROR(err))); \
    _FORCE_HAS_FORMAT_STRING(__VA_ARGS__); \
    RAWLOG(3, ": " __VA_ARGS__); \
    RAWLOG(3, "\n"); \
    return ERROR(err); \
  } while(0);

/*
 * If the provided expression evaluates to an error code, returns that error code.
 *
 * In debug modes, prints additional information.
 */
#define FORWARD_IF_ERROR(err, ...) \
  do { \
    size_t const err_code = (err); \
    if (ERR_isError(err_code)) { \
      RAWLOG(3, "%s:%d: ERROR!: forwarding error in %s: %s", \
             __FILE__, __LINE__, ZSTD_QUOTE(err), ERR_getErrorName(err_code)); \
      _FORCE_HAS_FORMAT_STRING(__VA_ARGS__); \
      RAWLOG(3, ": " __VA_ARGS__); \
      RAWLOG(3, "\n"); \
      return err_code; \
    } \
  } while(0);


/*-*************************************
*  Common constants
***************************************/
#define ZSTD_OPT_NUM    (1<<12)

#define ZSTD_REP_NUM      3                 /* number of repcodes */
#define ZSTD_REP_MOVE     (ZSTD_REP_NUM-1)
static UNUSED_ATTR const U32 repStartValue[ZSTD_REP_NUM] = { 1, 4, 8 };

#define KB *(1 <<10)
#define MB *(1 <<20)
#define GB *(1U<<30)

#define BIT7 128
#define BIT6  64
#define BIT5  32
#define BIT4  16
#define BIT1   2
#define BIT0   1

#define ZSTD_WINDOWLOG_ABSOLUTEMIN 10
static UNUSED_ATTR const size_t ZSTD_fcs_fieldSize[4] = { 0, 2, 4, 8 };
static UNUSED_ATTR const size_t ZSTD_did_fieldSize[4] = { 0, 1, 2, 4 };

#define ZSTD_FRAMEIDSIZE 4   /* magic number size */

#define ZSTD_BLOCKHEADERSIZE 3   /* C standard doesn't allow `static const` variable to be init using another `static const` variable */
static UNUSED_ATTR const size_t ZSTD_blockHeaderSize = ZSTD_BLOCKHEADERSIZE;
typedef enum { bt_raw, bt_rle, bt_compressed, bt_reserved } blockType_e;

#define ZSTD_FRAMECHECKSUMSIZE 4

#define MIN_SEQUENCES_SIZE 1 /* nbSeq==0 */
#define MIN_CBLOCK_SIZE (1 /*litCSize*/ + 1 /* RLE or RAW */ + MIN_SEQUENCES_SIZE /* nbSeq==0 */)   /* for a non-null block */

#define HufLog 12
typedef enum { set_basic, set_rle, set_compressed, set_repeat } symbolEncodingType_e;

#define LONGNBSEQ 0x7F00

#define MINMATCH 3

#define Litbits  8
#define MaxLit ((1<<Litbits) - 1)
#define MaxML   52
#define MaxLL   35
#define DefaultMaxOff 28
#define MaxOff  31
#define MaxSeq MAX(MaxLL, MaxML)   /* Assumption : MaxOff < MaxLL,MaxML */
#define MLFSELog    9
#define LLFSELog    9
#define OffFSELog   8
#define MaxFSELog  MAX(MAX(MLFSELog, LLFSELog), OffFSELog)

#define ZSTD_MAX_HUF_HEADER_SIZE 128 /* header + <= 127 byte tree description */
/* Each table cannot take more than #symbols * FSELog bits */
#define ZSTD_MAX_FSE_HEADERS_SIZE (((MaxML + 1) * MLFSELog + (MaxLL + 1) * LLFSELog + (MaxOff + 1) * OffFSELog + 7) / 8)

static UNUSED_ATTR const U32 LL_bits[MaxLL+1] = {
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     1, 1, 1, 1, 2, 2, 3, 3,
     4, 6, 7, 8, 9,10,11,12,
    13,14,15,16
};
static UNUSED_ATTR const S16 LL_defaultNorm[MaxLL+1] = {
     4, 3, 2, 2, 2, 2, 2, 2,
     2, 2, 2, 2, 2, 1, 1, 1,
     2, 2, 2, 2, 2, 2, 2, 2,
     2, 3, 2, 1, 1, 1, 1, 1,
    -1,-1,-1,-1
};
#define LL_DEFAULTNORMLOG 6  /* for static allocation */
static UNUSED_ATTR const U32 LL_defaultNormLog = LL_DEFAULTNORMLOG;

static UNUSED_ATTR const U32 ML_bits[MaxML+1] = {
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     1, 1, 1, 1, 2, 2, 3, 3,
     4, 4, 5, 7, 8, 9,10,11,
    12,13,14,15,16
};
static UNUSED_ATTR const S16 ML_defaultNorm[MaxML+1] = {
     1, 4, 3, 2, 2, 2, 2, 2,
     2, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1,-1,-1,
    -1,-1,-1,-1,-1
};
#define ML_DEFAULTNORMLOG 6  /* for static allocation */
static UNUSED_ATTR const U32 ML_defaultNormLog = ML_DEFAULTNORMLOG;

static UNUSED_ATTR const S16 OF_defaultNorm[DefaultMaxOff+1] = {
     1, 1, 1, 1, 1, 1, 2, 2,
     2, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
    -1,-1,-1,-1,-1
};
#define OF_DEFAULTNORMLOG 5  /* for static allocation */
static UNUSED_ATTR const U32 OF_defaultNormLog = OF_DEFAULTNORMLOG;


/*-*******************************************
*  Shared functions to include for inlining
*********************************************/
static void ZSTD_copy8(void* dst, const void* src) {
    ZSTD_memcpy(dst, src, 8);
}

#define COPY8(d,s) { ZSTD_copy8(d,s); d+=8; s+=8; }
static void ZSTD_copy16(void* dst, const void* src) {
    ZSTD_memcpy(dst, src, 16);
}
#define COPY16(d,s) { ZSTD_copy16(d,s); d+=16; s+=16; }

#define WILDCOPY_OVERLENGTH 32
#define WILDCOPY_VECLEN 16

typedef enum {
    ZSTD_no_overlap,
    ZSTD_overlap_src_before_dst
    /*  ZSTD_overlap_dst_before_src, */
} ZSTD_overlap_e;

/*! ZSTD_wildcopy() :
 *  Custom version of ZSTD_memcpy(), can over read/write up to WILDCOPY_OVERLENGTH bytes (if length==0)
 *  @param ovtype controls the overlap detection
 *         - ZSTD_no_overlap: The source and destination are guaranteed to be at least WILDCOPY_VECLEN bytes apart.
 *         - ZSTD_overlap_src_before_dst: The src and dst may overlap, but they MUST be at least 8 bytes apart.
 *           The src buffer must be before the dst buffer.
 */
MEM_STATIC FORCE_INLINE_ATTR
void ZSTD_wildcopy(void* dst, const void* src, ptrdiff_t length, ZSTD_overlap_e const ovtype)
{
    ptrdiff_t diff = (BYTE*)dst - (const BYTE*)src;
    const BYTE* ip = (const BYTE*)src;
    BYTE* op = (BYTE*)dst;
    BYTE* const oend = op + length;

    assert(diff >= 8 || (ovtype == ZSTD_no_overlap && diff <= -WILDCOPY_VECLEN));

    if (ovtype == ZSTD_overlap_src_before_dst && diff < WILDCOPY_VECLEN) {
        /* Handle short offset copies. */
        do {
            COPY8(op, ip)
        } while (op < oend);
    } else {
        assert(diff >= WILDCOPY_VECLEN || diff <= -WILDCOPY_VECLEN);
        /* Separate out the first COPY16() call because the copy length is
         * almost certain to be short, so the branches have different
         * probabilities. Since it is almost certain to be short, only do
         * one COPY16() in the first call. Then, do two calls per loop since
         * at that point it is more likely to have a high trip count.
         */
#ifdef __aarch64__
        do {
            COPY16(op, ip);
        }
        while (op < oend);
#else
        ZSTD_copy16(op, ip);
        if (16 >= length) return;
        op += 16;
        ip += 16;
        do {
            COPY16(op, ip);
            COPY16(op, ip);
        }
        while (op < oend);
#endif
    }
}

MEM_STATIC size_t ZSTD_limitCopy(void* dst, size_t dstCapacity, const void* src, size_t srcSize)
{
    size_t const length = MIN(dstCapacity, srcSize);
    if (length > 0) {
        ZSTD_memcpy(dst, src, length);
    }
    return length;
}

/* define "workspace is too large" as this number of times larger than needed */
#define ZSTD_WORKSPACETOOLARGE_FACTOR 3

/* when workspace is continuously too large
 * during at least this number of times,
 * context's memory usage is considered wasteful,
 * because it's sized to handle a worst case scenario which rarely happens.
 * In which case, resize it down to free some memory */
#define ZSTD_WORKSPACETOOLARGE_MAXDURATION 128

/* Controls whether the input/output buffer is buffered or stable. */
typedef enum {
    ZSTD_bm_buffered = 0,  /* Buffer the input/output */
    ZSTD_bm_stable = 1     /* ZSTD_inBuffer/ZSTD_outBuffer is stable */
} ZSTD_bufferMode_e;


/*-*******************************************
*  Private declarations
*********************************************/
typedef struct seqDef_s {
    U32 offset;         /* Offset code of the sequence */
    U16 litLength;
    U16 matchLength;
} seqDef;

typedef struct {
    seqDef* sequencesStart;
    seqDef* sequences;      /* ptr to end of sequences */
    BYTE* litStart;
    BYTE* lit;              /* ptr to end of literals */
    BYTE* llCode;
    BYTE* mlCode;
    BYTE* ofCode;
    size_t maxNbSeq;
    size_t maxNbLit;

    /* longLengthPos and longLengthID to allow us to represent either a single litLength or matchLength
     * in the seqStore that has a value larger than U16 (if it exists). To do so, we increment
     * the existing value of the litLength or matchLength by 0x10000.
     */
    U32   longLengthID;   /* 0 == no longLength; 1 == Represent the long literal; 2 == Represent the long match; */
    U32   longLengthPos;  /* Index of the sequence to apply long length modification to */
} seqStore_t;

typedef struct {
    U32 litLength;
    U32 matchLength;
} ZSTD_sequenceLength;

/*
 * Returns the ZSTD_sequenceLength for the given sequences. It handles the decoding of long sequences
 * indicated by longLengthPos and longLengthID, and adds MINMATCH back to matchLength.
 */
MEM_STATIC ZSTD_sequenceLength ZSTD_getSequenceLength(seqStore_t const* seqStore, seqDef const* seq)
{
    ZSTD_sequenceLength seqLen;
    seqLen.litLength = seq->litLength;
    seqLen.matchLength = seq->matchLength + MINMATCH;
    if (seqStore->longLengthPos == (U32)(seq - seqStore->sequencesStart)) {
        if (seqStore->longLengthID == 1) {
            seqLen.litLength += 0xFFFF;
        }
        if (seqStore->longLengthID == 2) {
            seqLen.matchLength += 0xFFFF;
        }
    }
    return seqLen;
}

/*
 * Contains the compressed frame size and an upper-bound for the decompressed frame size.
 * Note: before using `compressedSize`, check for errors using ZSTD_isError().
 *       similarly, before using `decompressedBound`, check for errors using:
 *          `decompressedBound != ZSTD_CONTENTSIZE_ERROR`
 */
typedef struct {
    size_t compressedSize;
    unsigned long long decompressedBound;
} ZSTD_frameSizeInfo;   /* decompress & legacy */

const seqStore_t* ZSTD_getSeqStore(const ZSTD_CCtx* ctx);   /* compress & dictBuilder */
void ZSTD_seqToCodes(const seqStore_t* seqStorePtr);   /* compress, dictBuilder, decodeCorpus (shouldn't get its definition from here) */

/* custom memory allocation functions */
void* ZSTD_customMalloc(size_t size, ZSTD_customMem customMem);
void* ZSTD_customCalloc(size_t size, ZSTD_customMem customMem);
void ZSTD_customFree(void* ptr, ZSTD_customMem customMem);


MEM_STATIC U32 ZSTD_highbit32(U32 val)   /* compress, dictBuilder, decodeCorpus */
{
    assert(val != 0);
    {
#   if (__GNUC__ >= 3)   /* GCC Intrinsic */
        return __builtin_clz (val) ^ 31;
#   else   /* Software version */
        static const U32 DeBruijnClz[32] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 };
        U32 v = val;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return DeBruijnClz[(v * 0x07C4ACDDU) >> 27];
#   endif
    }
}


/* ZSTD_invalidateRepCodes() :
 * ensures next compression will not use repcodes from previous block.
 * Note : only works with regular variant;
 *        do not use with extDict variant ! */
void ZSTD_invalidateRepCodes(ZSTD_CCtx* cctx);   /* zstdmt, adaptive_compression (shouldn't get this definition from here) */


typedef struct {
    blockType_e blockType;
    U32 lastBlock;
    U32 origSize;
} blockProperties_t;   /* declared here for decompress and fullbench */

/*! ZSTD_getcBlockSize() :
 *  Provides the size of compressed block from block header `src` */
/* Used by: decompress, fullbench (does not get its definition from here) */
size_t ZSTD_getcBlockSize(const void* src, size_t srcSize,
                          blockProperties_t* bpPtr);

/*! ZSTD_decodeSeqHeaders() :
 *  decode sequence header from src */
/* Used by: decompress, fullbench (does not get its definition from here) */
size_t ZSTD_decodeSeqHeaders(ZSTD_DCtx* dctx, int* nbSeqPtr,
                       const void* src, size_t srcSize);



#endif   /* ZSTD_CCOMMON_H_MODULE */
