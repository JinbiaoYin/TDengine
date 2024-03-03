/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tsdbUtil2.h"

// SDelBlock ----------
int32_t tTombBlockInit(STombBlock *tombBlock) {
  for (int32_t i = 0; i < TOMB_RECORD_ELEM_NUM; ++i) {
    TARRAY2_INIT(&tombBlock->dataArr[i]);
  }
  return 0;
}

int32_t tTombBlockDestroy(STombBlock *tombBlock) {
  for (int32_t i = 0; i < TOMB_RECORD_ELEM_NUM; ++i) {
    TARRAY2_DESTROY(&tombBlock->dataArr[i], NULL);
  }
  return 0;
}

int32_t tTombBlockClear(STombBlock *tombBlock) {
  for (int32_t i = 0; i < TOMB_RECORD_ELEM_NUM; ++i) {
    TARRAY2_CLEAR(&tombBlock->dataArr[i], NULL);
  }
  return 0;
}

int32_t tTombBlockPut(STombBlock *tombBlock, const STombRecord *record) {
  int32_t code;
  for (int32_t i = 0; i < TOMB_RECORD_ELEM_NUM; ++i) {
    code = TARRAY2_APPEND(&tombBlock->dataArr[i], record->dataArr[i]);
    if (code) return code;
  }
  return 0;
}

int32_t tTombBlockGet(STombBlock *tombBlock, int32_t idx, STombRecord *record) {
  if (idx >= TOMB_BLOCK_SIZE(tombBlock)) return TSDB_CODE_OUT_OF_RANGE;
  for (int32_t i = 0; i < TOMB_RECORD_ELEM_NUM; ++i) {
    record->dataArr[i] = TARRAY2_GET(&tombBlock->dataArr[i], idx);
  }
  return 0;
}

int32_t tTombRecordCompare(const STombRecord *r1, const STombRecord *r2) {
  if (r1->suid < r2->suid) return -1;
  if (r1->suid > r2->suid) return 1;
  if (r1->uid < r2->uid) return -1;
  if (r1->uid > r2->uid) return 1;
  if (r1->version < r2->version) return -1;
  if (r1->version > r2->version) return 1;
  return 0;
}

// STbStatisBlock ----------
int32_t tStatisBlockInit(STbStatisBlock *statisBlock) {
  statisBlock->numOfPKs = 0;
  statisBlock->numOfRecords = 0;
  for (int32_t i = 0; i < ARRAY_SIZE(statisBlock->buffers); ++i) {
    tBufferInit(&statisBlock->buffers[i]);
  }
  for (int32_t i = 0; i < TD_MAX_PK_COLS; ++i) {
    tValueColumnInit(&statisBlock->firstKeyPKs[i]);
    tValueColumnInit(&statisBlock->lastKeyPKs[i]);
  }
  return 0;
}

int32_t tStatisBlockDestroy(STbStatisBlock *statisBlock) {
  statisBlock->numOfPKs = 0;
  statisBlock->numOfRecords = 0;
  for (int32_t i = 0; i < ARRAY_SIZE(statisBlock->buffers); ++i) {
    tBufferDestroy(&statisBlock->buffers[i]);
  }
  for (int32_t i = 0; i < TD_MAX_PK_COLS; ++i) {
    tValueColumnDestroy(&statisBlock->firstKeyPKs[i]);
    tValueColumnDestroy(&statisBlock->lastKeyPKs[i]);
  }
  return 0;
}

int32_t tStatisBlockClear(STbStatisBlock *statisBlock) {
  statisBlock->numOfPKs = 0;
  statisBlock->numOfRecords = 0;
  for (int32_t i = 0; i < ARRAY_SIZE(statisBlock->buffers); ++i) {
    tBufferClear(&statisBlock->buffers[i]);
  }
  for (int32_t i = 0; i < TD_MAX_PK_COLS; ++i) {
    tValueColumnClear(&statisBlock->firstKeyPKs[i]);
    tValueColumnClear(&statisBlock->lastKeyPKs[i]);
  }
  return 0;
}

int32_t tStatisBlockPut(STbStatisBlock *statisBlock, const STbStatisRecord *record) {
  int32_t code;

  if (statisBlock->numOfRecords == 0) {
    statisBlock->numOfPKs = record->firstKey.numOfPKs;
  }

  ASSERT(statisBlock->numOfPKs == record->firstKey.numOfPKs);
  ASSERT(statisBlock->numOfPKs == record->lastKey.numOfPKs);

  code = tBufferPutI64(&statisBlock->suids, record->suid);
  if (code) return code;

  code = tBufferPutI64(&statisBlock->uids, record->uid);
  if (code) return code;

  code = tBufferPutI64(&statisBlock->firstKeyTimestamps, record->firstKey.ts);
  if (code) return code;

  code = tBufferPutI64(&statisBlock->lastKeyTimestamps, record->lastKey.ts);
  if (code) return code;

  code = tBufferPutI64(&statisBlock->counts, record->count);
  if (code) return code;

  for (int32_t i = 0; i < statisBlock->numOfPKs; ++i) {
    code = tValueColumnAppend(&statisBlock->firstKeyPKs[i], &record->firstKey.pks[i]);
    if (code) return code;
    code = tValueColumnAppend(&statisBlock->lastKeyPKs[i], &record->lastKey.pks[i]);
    if (code) return code;
  }

  statisBlock->numOfRecords++;
  return 0;
}

int32_t tStatisBlockGet(STbStatisBlock *statisBlock, int32_t idx, STbStatisRecord *record) {
  int32_t       code;
  SBufferReader reader;

  if (idx < 0 || idx >= statisBlock->numOfRecords) {
    return TSDB_CODE_OUT_OF_RANGE;
  }

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(record->suid), &statisBlock->suids);
  code = tBufferGetI64(&reader, &record->suid);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(record->uid), &statisBlock->uids);
  code = tBufferGetI64(&reader, &record->uid);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(record->firstKey.ts), &statisBlock->firstKeyTimestamps);
  code = tBufferGetI64(&reader, &record->firstKey.ts);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(record->lastKey.ts), &statisBlock->lastKeyTimestamps);
  code = tBufferGetI64(&reader, &record->lastKey.ts);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(record->count), &statisBlock->counts);
  code = tBufferGetI64(&reader, &record->count);
  if (code) return code;

  record->firstKey.numOfPKs = statisBlock->numOfPKs;
  record->lastKey.numOfPKs = statisBlock->numOfPKs;
  for (int32_t i = 0; i < statisBlock->numOfPKs; ++i) {
    code = tValueColumnGet(&statisBlock->firstKeyPKs[i], idx, &record->firstKey.pks[i]);
    if (code) return code;
    code = tValueColumnGet(&statisBlock->lastKeyPKs[i], idx, &record->lastKey.pks[i]);
    if (code) return code;
  }

  return 0;
}

// SBrinRecord ----------
int32_t tBrinBlockInit(SBrinBlock *brinBlock) {
  brinBlock->numOfPKs = 0;
  brinBlock->numOfRecords = 0;
  for (int32_t i = 0; i < ARRAY_SIZE(brinBlock->buffers); ++i) {
    tBufferInit(&brinBlock->buffers[i]);
  }
  for (int32_t i = 0; i < TD_MAX_PK_COLS; ++i) {
    tValueColumnInit(&brinBlock->firstKeyPKs[i]);
    tValueColumnInit(&brinBlock->lastKeyPKs[i]);
  }
  return 0;
}

int32_t tBrinBlockDestroy(SBrinBlock *brinBlock) {
  brinBlock->numOfPKs = 0;
  brinBlock->numOfRecords = 0;
  for (int32_t i = 0; i < ARRAY_SIZE(brinBlock->buffers); ++i) {
    tBufferDestroy(&brinBlock->buffers[i]);
  }
  for (int32_t i = 0; i < TD_MAX_PK_COLS; ++i) {
    tValueColumnDestroy(&brinBlock->firstKeyPKs[i]);
    tValueColumnDestroy(&brinBlock->lastKeyPKs[i]);
  }
  return 0;
}

int32_t tBrinBlockClear(SBrinBlock *brinBlock) {
  brinBlock->numOfPKs = 0;
  brinBlock->numOfRecords = 0;
  for (int32_t i = 0; i < ARRAY_SIZE(brinBlock->buffers); ++i) {
    tBufferClear(&brinBlock->buffers[i]);
  }
  for (int32_t i = 0; i < TD_MAX_PK_COLS; ++i) {
    tValueColumnClear(&brinBlock->firstKeyPKs[i]);
    tValueColumnClear(&brinBlock->lastKeyPKs[i]);
  }
  return 0;
}

int32_t tBrinBlockPut(SBrinBlock *brinBlock, const SBrinRecord *record) {
  int32_t code;

  ASSERT(record->firstKey.key.numOfPKs == record->lastKey.key.numOfPKs);

  if (brinBlock->numOfRecords == 0) {
    brinBlock->numOfPKs = record->firstKey.key.numOfPKs;
  }

  ASSERT(brinBlock->numOfPKs == record->firstKey.key.numOfPKs);

  code = tBufferPutI64(&brinBlock->suids, record->suid);
  if (code) return code;

  code = tBufferPutI64(&brinBlock->uids, record->uid);
  if (code) return code;

  code = tBufferPutI64(&brinBlock->firstKeyTimestamps, record->firstKey.key.ts);
  if (code) return code;

  code = tBufferPutI64(&brinBlock->firstKeyVersions, record->firstKey.version);
  if (code) return code;

  for (int32_t i = 0; i < record->firstKey.key.numOfPKs; ++i) {
    code = tValueColumnAppend(&brinBlock->firstKeyPKs[i], &record->firstKey.key.pks[i]);
    if (code) return code;
  }

  code = tBufferPutI64(&brinBlock->lastKeyTimestamps, record->lastKey.key.ts);
  if (code) return code;

  code = tBufferPutI64(&brinBlock->lastKeyVersions, record->lastKey.version);
  if (code) return code;

  for (int32_t i = 0; i < record->lastKey.key.numOfPKs; ++i) {
    code = tValueColumnAppend(&brinBlock->lastKeyPKs[i], &record->lastKey.key.pks[i]);
    if (code) return code;
  }

  code = tBufferPutI64(&brinBlock->minVers, record->minVer);
  if (code) return code;

  code = tBufferPutI64(&brinBlock->maxVers, record->maxVer);
  if (code) return code;

  code = tBufferPutI64(&brinBlock->blockOffsets, record->blockOffset);
  if (code) return code;

  code = tBufferPutI64(&brinBlock->smaOffsets, record->smaOffset);
  if (code) return code;

  code = tBufferPutI32(&brinBlock->blockSizes, record->blockSize);
  if (code) return code;

  code = tBufferPutI32(&brinBlock->blockKeySizes, record->blockKeySize);
  if (code) return code;

  code = tBufferPutI32(&brinBlock->smaSizes, record->smaSize);
  if (code) return code;

  code = tBufferPutI32(&brinBlock->numRows, record->numRow);
  if (code) return code;

  code = tBufferPutI32(&brinBlock->counts, record->count);
  if (code) return code;

  brinBlock->numOfRecords++;

  return 0;
}

int32_t tBrinBlockGet(SBrinBlock *brinBlock, int32_t idx, SBrinRecord *record) {
  int32_t       code;
  SBufferReader reader;

  if (idx < 0 || idx >= brinBlock->numOfRecords) {
    return TSDB_CODE_OUT_OF_RANGE;
  }

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->suids);
  code = tBufferGetI64(&reader, &record->suid);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->uids);
  code = tBufferGetI64(&reader, &record->uid);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->firstKeyTimestamps);
  code = tBufferGetI64(&reader, &record->firstKey.key.ts);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->firstKeyVersions);
  code = tBufferGetI64(&reader, &record->firstKey.version);
  if (code) return code;

  for (record->firstKey.key.numOfPKs = 0; record->firstKey.key.numOfPKs < brinBlock->numOfPKs;
       record->firstKey.key.numOfPKs++) {
    code = tValueColumnGet(&brinBlock->firstKeyPKs[record->firstKey.key.numOfPKs], idx,
                           &record->firstKey.key.pks[record->firstKey.key.numOfPKs]);
    if (code) return code;
  }

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->lastKeyTimestamps);
  code = tBufferGetI64(&reader, &record->lastKey.key.ts);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->lastKeyVersions);
  code = tBufferGetI64(&reader, &record->lastKey.version);
  if (code) return code;

  for (record->lastKey.key.numOfPKs = 0; record->lastKey.key.numOfPKs < brinBlock->numOfPKs;
       record->lastKey.key.numOfPKs++) {
    code = tValueColumnGet(&brinBlock->lastKeyPKs[record->lastKey.key.numOfPKs], idx,
                           &record->lastKey.key.pks[record->lastKey.key.numOfPKs]);
    if (code) return code;
  }

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->minVers);
  code = tBufferGetI64(&reader, &record->minVer);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->maxVers);
  code = tBufferGetI64(&reader, &record->maxVer);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->blockOffsets);
  code = tBufferGetI64(&reader, &record->blockOffset);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int64_t), &brinBlock->smaOffsets);
  code = tBufferGetI64(&reader, &record->smaOffset);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int32_t), &brinBlock->blockSizes);
  code = tBufferGetI32(&reader, &record->blockSize);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int32_t), &brinBlock->blockKeySizes);
  code = tBufferGetI32(&reader, &record->blockKeySize);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int32_t), &brinBlock->smaSizes);
  code = tBufferGetI32(&reader, &record->smaSize);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int32_t), &brinBlock->numRows);
  code = tBufferGetI32(&reader, &record->numRow);
  if (code) return code;

  reader = BUFFER_READER_INITIALIZER(idx * sizeof(int32_t), &brinBlock->counts);
  code = tBufferGetI32(&reader, &record->count);
  if (code) return code;

  return 0;
}

// int32_t tBrinBlockEncode(SBrinBlock *brinBlock, SBrinBlk *brinBlk, SBuffer *buffer) {
//   int32_t  code;
//   SBuffer *helperBuffer = NULL;  // TODO

//   brinBlk->dp[0].size = 0;
//   brinBlk->numRec = brinBlock->numOfRecords;
//   brinBlk->numOfPKs = brinBlock->numOfPKs;

//   // minTbid
//   code = tBufferGet(&brinBlock->suids, 0, sizeof(brinBlk->minTbid.suid), &brinBlk->minTbid.suid);
//   if (code) return code;
//   code = tBufferGet(&brinBlock->uids, 0, sizeof(brinBlk->minTbid.uid), &brinBlk->minTbid.uid);
//   if (code) return code;
//   // maxTbid
//   code =
//       tBufferGet(&brinBlock->suids, brinBlock->numOfRecords - 1, sizeof(brinBlk->maxTbid.suid),
//       &brinBlk->maxTbid.suid);
//   if (code) return code;
//   code = tBufferGet(&brinBlock->uids, brinBlock->numOfRecords - 1, sizeof(brinBlk->maxTbid.uid),
//   &brinBlk->maxTbid.uid); if (code) return code;
//   // minVer and maxVer
//   const int64_t *minVers = (int64_t *)tBufferGetData(&brinBlock->minVers);
//   const int64_t *maxVers = (int64_t *)tBufferGetData(&brinBlock->maxVers);
//   brinBlk->minVer = minVers[0];
//   brinBlk->maxVer = maxVers[0];
//   for (int32_t i = 1; i < brinBlock->numOfRecords; ++i) {
//     if (minVers[i] < brinBlk->minVer) brinBlk->minVer = minVers[i];
//     if (maxVers[i] > brinBlk->maxVer) brinBlk->maxVer = maxVers[i];
//   }

//   // compress data
//   for (int32_t i = 0; i < ARRAY_SIZE(brinBlock->buffers); ++i) {
//     SBuffer      *bf = &brinBlock->buffers[i];
//     SCompressInfo info = {
//         .cmprAlg = brinBlk->cmprAlg,
//     };

//     if (tBufferGetSize(bf) == 8 * brinBlock->numOfRecords) {
//       info.dataType = TSDB_DATA_TYPE_BIGINT;
//     } else if (tBufferGetSize(bf) == 4 * brinBlock->numOfRecords) {
//       info.dataType = TSDB_DATA_TYPE_INT;
//     } else {
//       ASSERT(0);
//     }

//     code = tCompressDataToBuffer(tBufferGetData(bf), tBufferGetSize(bf), &info, buffer, helperBuffer);
//     if (code) return code;
//     brinBlk->size[i] = info.compressedSize;
//     brinBlk->dp[0].size += info.compressedSize;
//   }

//   // encode primary keys
//   SValueColumnCompressInfo firstKeyPKsInfos[TD_MAX_PK_COLS];
//   SValueColumnCompressInfo lastKeyPKsInfos[TD_MAX_PK_COLS];

//   for (int32_t i = 0; i < brinBlk->numOfPKs; ++i) {
//     SValueColumn *vc = &brinBlock->firstKeyPKs[i];
//     firstKeyPKsInfos[i].cmprAlg = brinBlk->cmprAlg;
//     code = tValueColumnCompress(vc, &firstKeyPKsInfos[i], buffer, helperBuffer);
//     if (code) return code;
//   }

//   for (int32_t i = 0; i < brinBlk->numOfPKs; ++i) {
//     SValueColumn *vc = &brinBlock->lastKeyPKs[i];
//     lastKeyPKsInfos[i].cmprAlg = brinBlk->cmprAlg;
//     code = tValueColumnCompress(vc, &lastKeyPKsInfos[i], buffer, helperBuffer);
//     if (code) return code;
//   }

//   return 0;
// }

// int32_t tBrinBlockDecode(const SBuffer *buffer, SBrinBlk *brinBlk, SBrinBlock *brinBlock) {
// if (brinBlk->fmtVersion == 0) {
//   return tBrinBlockDecodeVersion0(buffer, brinBlk, brinBlock);
// } else if (brinBlk->fmtVersion == 1) {
//   return tBrinBlockDecodeVersion1(buffer, brinBlk, brinBlock);
// } else {
//   ASSERT(0);
// }
//   return 0;
// }

// other apis ----------
int32_t tsdbUpdateSkmTb(STsdb *pTsdb, const TABLEID *tbid, SSkmInfo *pSkmTb) {
  if (tbid->suid) {
    if (pSkmTb->suid == tbid->suid) {
      pSkmTb->uid = tbid->uid;
      return 0;
    }
  } else if (pSkmTb->uid == tbid->uid) {
    return 0;
  }

  pSkmTb->suid = tbid->suid;
  pSkmTb->uid = tbid->uid;
  tDestroyTSchema(pSkmTb->pTSchema);
  return metaGetTbTSchemaEx(pTsdb->pVnode->pMeta, tbid->suid, tbid->uid, -1, &pSkmTb->pTSchema);
}

int32_t tsdbUpdateSkmRow(STsdb *pTsdb, const TABLEID *tbid, int32_t sver, SSkmInfo *pSkmRow) {
  if (pSkmRow->pTSchema && pSkmRow->suid == tbid->suid) {
    if (pSkmRow->suid) {
      if (sver == pSkmRow->pTSchema->version) return 0;
    } else if (pSkmRow->uid == tbid->uid && pSkmRow->pTSchema->version == sver) {
      return 0;
    }
  }

  pSkmRow->suid = tbid->suid;
  pSkmRow->uid = tbid->uid;
  tDestroyTSchema(pSkmRow->pTSchema);
  return metaGetTbTSchemaEx(pTsdb->pVnode->pMeta, tbid->suid, tbid->uid, sver, &pSkmRow->pTSchema);
}