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

#define ALLOW_FORBID_FUNC

#include <stdio.h>
#include "taos_metric_formatter_i.h"
#include "taos_metric_sample_t.h"
#include "tjson.h"
#include "taos_monitor_util_i.h"
#include "taos_assert.h"

int taos_metric_formatter_load_sample_new(taos_metric_formatter_t *self, taos_metric_sample_t *sample, 
                                      char *ts, char *format, char *metricName, SJson *arrayTable) {
  TAOS_ASSERT(self != NULL);
  if (self == NULL) return 1;

  int r = 0;

  char* start = strstr(sample->l_value, "{");
  char* end = strstr(sample->l_value, "}");

  int32_t len = end -start;

  char* keyvalues = taosMemoryMalloc(len);
  memset(keyvalues, 0, len);
  memcpy(keyvalues, start + 1, len - 1);

  int32_t count = countOccurrences(keyvalues, ",");

  char** keyvalue = taosMemoryMalloc(sizeof(char*) * (count + 1));
  memset(keyvalue, 0, sizeof(char*) * (count + 1));
  taos_monitor_split_str(keyvalue, keyvalues, ",");

  char** arr = taosMemoryMalloc(sizeof(char*) * (count + 1) * 2);
  memset(arr, 0, sizeof(char*) * (count + 1) * 2);

  bool isfound = true;
  for(int32_t i = 0; i < count + 1; i++){
    char* str = *(keyvalue + i);

    char** pair = arr + i * 2;
    taos_monitor_split_str(pair, str, "=");

    strip(pair[1]);
  }

  SJson* item = tjsonGetArrayItemByPair(arrayTable, arr, count + 1);

  SJson* metrics = NULL;
  if(item == NULL) {
    item = tjsonCreateObject();

    SJson* arrayTag = tjsonCreateArray();
    for(int32_t i = 0; i < count + 1; i++){
      char** pair = arr + i * 2;

      char* key = *pair;
      char* value = *(pair + 1);

      SJson* tag = tjsonCreateObject();
      tjsonAddStringToObject(tag, "name", key);
      tjsonAddStringToObject(tag, "value", value);

      tjsonAddItemToArray(arrayTag, tag);
    }
    tjsonAddItemToObject(item, "tags", arrayTag);

    metrics = tjsonCreateArray();
    tjsonAddItemToObject(item, "metrics", metrics);

    tjsonAddItemToArray(arrayTable, item);
  }
  else{
    metrics = tjsonGetObjectItem(item, "metrics");
  }

  taosMemoryFreeClear(arr);
  taosMemoryFreeClear(keyvalue);
  taosMemoryFreeClear(keyvalues);

  SJson* metric = tjsonCreateObject();
  tjsonAddStringToObject(metric, "name", metricName);
  tjsonAddDoubleToObject(metric, "value", sample->r_value);

  tjsonAddItemToArray(metrics, metric);

  taos_metric_sample_set(sample, 0);

  return 0;
}

int taos_metric_formatter_load_metric_new(taos_metric_formatter_t *self, taos_metric_t *metric, char *ts, char *format, 
                                          SJson* pJson) {
  TAOS_ASSERT(self != NULL);
  if (self == NULL) return 1;

  int r = 0;

  int32_t size = strlen(metric->name);
  char* name = taosMemoryMalloc(size + 1);
  memset(name, 0, size + 1);
  memcpy(name, metric->name, size);
  char* arr[2] = {0};
  taos_monitor_split_str((char**)&arr, name, ":");

  //SJson* item = tjsonGetArrayItemByName(pJson, arr[0]);
  SJson* arrayTable = tjsonGetObjectItem(pJson, arr[0]);
  if(arrayTable == NULL){
    arrayTable = tjsonCreateArray();
    //tjsonAddItemToArray(pJson, item);
    tjsonAddItemToObject(pJson, arr[0], arrayTable);
  }


  for (taos_linked_list_node_t *current_node = metric->samples->keys->head; current_node != NULL;
       current_node = current_node->next) {
    const char *key = (const char *)current_node->item;
    if (metric->type == TAOS_HISTOGRAM) {

    } else {
      taos_metric_sample_t *sample = (taos_metric_sample_t *)taos_map_get(metric->samples, key);
      if (sample == NULL) return 1;
      r = taos_metric_formatter_load_sample_new(self, sample, ts, format, arr[1], arrayTable);
      if (r) return r;
    }
  }
  taosMemoryFreeClear(name);
  return r;
}