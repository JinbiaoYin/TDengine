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

#include "tsdb.h"

struct STsdbSnapshotReader {
  // TODO
};

int32_t tsdbSnapshotReaderOpen(STsdb* pTsdb, STsdbSnapshotReader** ppReader, int64_t sver, int64_t ever) {
  // TODO
  return 0;
}

int32_t tsdbSnapshotReaderClose(STsdbSnapshotReader* pReader) {
  // TODO
  return 0;
}

int32_t tsdbSnapshotRead(STsdbSnapshotReader* pReader, void** ppData, uint32_t* nData) {
  // TODO
  return 0;
}
