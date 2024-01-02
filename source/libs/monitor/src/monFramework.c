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
#include "monitor.h"
#include "monInt.h"

#include "thash.h"
#include "taos_monitor.h"
#include "thttp.h"
#include "ttime.h"
#include "tglobal.h"

extern SMonitor tsMonitor;
extern char* tsMonUri;
extern char* tsMonFwUri;

#define LEVEL_LEN 11

#define CLUSTER_TABLE "taosd_cluster_info"

#define MASTER_UPTIME  CLUSTER_TABLE":master_uptime"
#define DBS_TOTAL CLUSTER_TABLE":dbs_total"
#define TBS_TOTAL CLUSTER_TABLE":tbs_total"
#define STBS_TOTAL CLUSTER_TABLE":stbs_total"
#define VGROUPS_TOTAL CLUSTER_TABLE":vgroups_total"
#define VGROUPS_ALIVE CLUSTER_TABLE":vgroups_alive"
#define VNODES_TOTAL CLUSTER_TABLE":vnodes_total"
#define VNODES_ALIVE CLUSTER_TABLE":vnodes_alive"
#define DNODES_TOTAL CLUSTER_TABLE":dnodes_total"
#define DNODES_ALIVE CLUSTER_TABLE":dnodes_alive"
#define CONNECTIONS_TOTAL CLUSTER_TABLE":connections_total"
#define TOPICS_TOTAL CLUSTER_TABLE":topics_total"
#define STREAMS_TOTAL CLUSTER_TABLE":streams_total"
#define EXPIRE_TIME  CLUSTER_TABLE":expire_time"
#define TIMESERIES_USED CLUSTER_TABLE":timeseries_used"
#define TIMESERIES_TOTAL CLUSTER_TABLE":timeseries_total"

#define VGROUP_TABLE "taosd_cluster_vgroups_info"

#define TABLES_NUM VGROUP_TABLE":tables_num"
#define STATUS VGROUP_TABLE":status"

#define DNODE_TABLE "taosd_dnodes_info"

#define UPTIME DNODE_TABLE":uptime"
#define CPU_ENGINE DNODE_TABLE":cpu_engine"
#define CPU_SYSTEM DNODE_TABLE":cpu_system"
#define MEM_ENGINE DNODE_TABLE":mem_engine"
#define MEM_SYSTEM DNODE_TABLE":mem_system"
#define DISK_ENGINE DNODE_TABLE":disk_engine"
#define DISK_USED DNODE_TABLE":disk_used"
#define NET_IN DNODE_TABLE":net_in"
#define NET_OUT DNODE_TABLE":net_out"
#define IO_READ DNODE_TABLE":io_read"
#define IO_WRITE DNODE_TABLE":io_write"
#define IO_READ_DISK DNODE_TABLE":io_read_disk"
#define IO_WRITE_DISK DNODE_TABLE":io_write_disk"
#define ERRORS DNODE_TABLE":errors"
#define VNODES_NUM DNODE_TABLE":vnodes_num"
#define MASTERS DNODE_TABLE":masters"
#define HAS_MNODE DNODE_TABLE":has_mnode"
#define HAS_QNODE DNODE_TABLE":has_qnode"
#define HAS_SNODE DNODE_TABLE":has_snode"
#define DNODE_LOG_ERROR DNODE_TABLE":ERROR"
#define DNODE_LOG_INFO DNODE_TABLE":INFO"
#define DNODE_LOG_DEBUG DNODE_TABLE":DEBUG"
#define DNODE_LOG_TRACE DNODE_TABLE":TRACE"

#define DNODE_STATUS "taosd_dnodes_status:status"

#define DATADIR_TABLE "taosd_dnodes_data_dirs"

#define DNODE_DATA_AVAIL DATADIR_TABLE":avail"
#define DNODE_DATA_USED DATADIR_TABLE":used"
#define DNODE_DATA_TOTAL DATADIR_TABLE":total"

#define LOGDIR_TABLE "taosd_dnodes_log_dirs"

#define DNODE_LOG_AVAIL LOGDIR_TABLE":avail"
#define DNODE_LOG_USED LOGDIR_TABLE":used"
#define DNODE_LOG_TOTAL LOGDIR_TABLE":total"

#define MNODE_ROLE "taosd_mnodes_info:role"
#define VNODE_ROLE "taosd_vnodes_info:role"

void monInitMonitorFW(){
  taos_collector_registry_default_init();

  tsMonitor.metrics = taosHashInit(16, taosGetDefaultHashFunction(TSDB_DATA_TYPE_BINARY), true, HASH_ENTRY_LOCK);
  taos_gauge_t *gauge = NULL;

  int32_t label_count =1;
  const char *sample_labels[] = {"cluster_id"};
  char *metric[] = {MASTER_UPTIME, DBS_TOTAL, TBS_TOTAL, STBS_TOTAL, VGROUPS_TOTAL,
                VGROUPS_ALIVE, VNODES_TOTAL, VNODES_ALIVE, CONNECTIONS_TOTAL, TOPICS_TOTAL, STREAMS_TOTAL,
                    DNODES_TOTAL, DNODES_ALIVE, EXPIRE_TIME, TIMESERIES_USED,
                    TIMESERIES_TOTAL};
  for(int32_t i = 0; i < 16; i++){
    gauge= taos_gauge_new(metric[i], "",  label_count, sample_labels);
    if(taos_collector_registry_register_metric(gauge) == 1){
      taos_counter_destroy(gauge);
    }
    taosHashPut(tsMonitor.metrics, metric[i], strlen(metric[i]), &gauge, sizeof(taos_gauge_t *));
  } 

  int32_t vgroup_label_count = 3;
  const char *vgroup_sample_labels[] = {"cluster_id", "vgroup_id", "database_name"};
  char *vgroup_metrics[] = {TABLES_NUM, STATUS};
  for(int32_t i = 0; i < 2; i++){
    gauge= taos_gauge_new(vgroup_metrics[i], "",  vgroup_label_count, vgroup_sample_labels);
    if(taos_collector_registry_register_metric(gauge) == 1){
      taos_counter_destroy(gauge);
    }
    taosHashPut(tsMonitor.metrics, vgroup_metrics[i], strlen(vgroup_metrics[i]), &gauge, sizeof(taos_gauge_t *));
  }

  int32_t dnodes_label_count = 3;
  const char *dnodes_sample_labels[] = {"cluster_id", "dnode_id", "dnode_ep"};
  char *dnodes_gauges[] = {UPTIME, CPU_ENGINE, CPU_SYSTEM, MEM_ENGINE, MEM_SYSTEM, DISK_ENGINE, DISK_USED, NET_IN,
                            NET_OUT, IO_READ, IO_WRITE, IO_READ_DISK, IO_WRITE_DISK, ERRORS,
                             VNODES_NUM, MASTERS, HAS_MNODE, HAS_QNODE, HAS_SNODE, DNODE_STATUS,
                             DNODE_LOG_ERROR, DNODE_LOG_INFO, DNODE_LOG_DEBUG, DNODE_LOG_TRACE};
  for(int32_t i = 0; i < 24; i++){
    gauge= taos_gauge_new(dnodes_gauges[i], "",  dnodes_label_count, dnodes_sample_labels);
    if(taos_collector_registry_register_metric(gauge) == 1){
      taos_counter_destroy(gauge);
    }
    taosHashPut(tsMonitor.metrics, dnodes_gauges[i], strlen(dnodes_gauges[i]), &gauge, sizeof(taos_gauge_t *));
  }

  int32_t dnodes_data_label_count = 5;
  const char *dnodes_data_sample_labels[] = {"cluster_id", "dnode_id", "dnode_ep", "data_dir_name", "data_dir_level"};
  char *dnodes_data_gauges[] = {DNODE_DATA_AVAIL, DNODE_DATA_USED, DNODE_DATA_TOTAL};
  for(int32_t i = 0; i < 3; i++){
    gauge= taos_gauge_new(dnodes_data_gauges[i], "",  dnodes_data_label_count, dnodes_data_sample_labels);
    if(taos_collector_registry_register_metric(gauge) == 1){
      taos_counter_destroy(gauge);
    }
    taosHashPut(tsMonitor.metrics, dnodes_data_gauges[i], strlen(dnodes_data_gauges[i]), &gauge, sizeof(taos_gauge_t *));
  }

  int32_t dnodes_log_label_count = 4;
  const char *dnodes_log_sample_labels[] = {"cluster_id", "dnode_id", "dnode_ep", "data_dir_name"};
  char *dnodes_log_gauges[] = {DNODE_LOG_AVAIL, DNODE_LOG_USED, DNODE_LOG_TOTAL};
  for(int32_t i = 0; i < 3; i++){
    gauge= taos_gauge_new(dnodes_log_gauges[i], "",  dnodes_log_label_count, dnodes_log_sample_labels);
    if(taos_collector_registry_register_metric(gauge) == 1){
      taos_counter_destroy(gauge);
    }
    taosHashPut(tsMonitor.metrics, dnodes_log_gauges[i], strlen(dnodes_log_gauges[i]), &gauge, sizeof(taos_gauge_t *));
  }

  int32_t mnodes_role_label_count = 3;
  const char *mnodes_role_sample_labels[] = {"cluster_id", "mnode_id", "mnode_ep"};
  char *mnodes_role_gauges[] = {MNODE_ROLE};
  for(int32_t i = 0; i < 1; i++){
    gauge= taos_gauge_new(mnodes_role_gauges[i], "",  mnodes_role_label_count, mnodes_role_sample_labels);
    if(taos_collector_registry_register_metric(gauge) == 1){
      taos_counter_destroy(gauge);
    }
    taosHashPut(tsMonitor.metrics, mnodes_role_gauges[i], strlen(mnodes_role_gauges[i]), &gauge, sizeof(taos_gauge_t *));
  }

  int32_t vnodes_role_label_count = 4;
  const char *vnodes_role_sample_labels[] = {"cluster_id", "vgroup_id", "database_name", "dnode_id"};
  char *vnodes_role_gauges[] = {VNODE_ROLE};
  for(int32_t i = 0; i < 1; i++){
    gauge= taos_gauge_new(vnodes_role_gauges[i], "",  vnodes_role_label_count, vnodes_role_sample_labels);
    if(taos_collector_registry_register_metric(gauge) == 1){
      taos_counter_destroy(gauge);
    }
    taosHashPut(tsMonitor.metrics, vnodes_role_gauges[i], strlen(vnodes_role_gauges[i]), &gauge, sizeof(taos_gauge_t *));
  }
}

void monCleanupMonitorFW(){
  taosHashCleanup(tsMonitor.metrics);
  taos_collector_registry_destroy(TAOS_COLLECTOR_REGISTRY_DEFAULT);
  TAOS_COLLECTOR_REGISTRY_DEFAULT = NULL;
}

void monGenClusterInfoTable(SMonInfo *pMonitor){
  SMonClusterInfo *pInfo = &pMonitor->mmInfo.cluster;
  SMonBasicInfo *pBasicInfo = &pMonitor->dmInfo.basic;
  SMonGrantInfo *pGrantInfo = &pMonitor->mmInfo.grant;

  if(pBasicInfo->cluster_id == 0) {
    uError("failed to generate dnode info table since cluster_id is 0");
    return;
  }
  if (pMonitor->mmInfo.cluster.first_ep_dnode_id == 0) return;

  //cluster info
  char buf[TSDB_CLUSTER_ID_LEN] = {0};
  snprintf(buf, TSDB_CLUSTER_ID_LEN, "%"PRId64, pBasicInfo->cluster_id);
  const char *sample_labels[] = {buf};

  taos_gauge_t **metric = NULL;
  
  metric = taosHashGet(tsMonitor.metrics, MASTER_UPTIME, strlen(MASTER_UPTIME));
  taos_gauge_set(*metric, pInfo->master_uptime, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, DBS_TOTAL, strlen(DBS_TOTAL));
  taos_gauge_set(*metric, pInfo->dbs_total, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, TBS_TOTAL, strlen(TBS_TOTAL));
  taos_gauge_set(*metric, pInfo->tbs_total, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, STBS_TOTAL, strlen(STBS_TOTAL));
  taos_gauge_set(*metric, pInfo->stbs_total, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, VGROUPS_TOTAL, strlen(VGROUPS_TOTAL));
  taos_gauge_set(*metric, pInfo->vgroups_total, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, VGROUPS_ALIVE, strlen(VGROUPS_ALIVE));
  taos_gauge_set(*metric, pInfo->vgroups_alive, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, VNODES_TOTAL, strlen(VNODES_TOTAL));
  taos_gauge_set(*metric, pInfo->vnodes_total, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, VNODES_ALIVE, strlen(VNODES_ALIVE));
  taos_gauge_set(*metric, pInfo->vnodes_alive, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, CONNECTIONS_TOTAL, strlen(CONNECTIONS_TOTAL));
  taos_gauge_set(*metric, pInfo->vnodes_alive, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, TOPICS_TOTAL, strlen(TOPICS_TOTAL));
  taos_gauge_set(*metric, pInfo->topics_toal, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, STREAMS_TOTAL, strlen(STREAMS_TOTAL));
  taos_gauge_set(*metric, pInfo->streams_total, sample_labels);

  //dnodes number
  int32_t dnode_total = taosArrayGetSize(pInfo->dnodes);
  int32_t dnode_alive = 0;

  for (int32_t i = 0; i < taosArrayGetSize(pInfo->dnodes); ++i) {
    SMonDnodeDesc *pDnodeDesc = taosArrayGet(pInfo->dnodes, i);

    if(strcmp(pDnodeDesc->status, "ready") == 0){
        dnode_alive++;
    }
  }
    
  metric = taosHashGet(tsMonitor.metrics, DNODES_TOTAL, strlen(DNODES_TOTAL));
  taos_gauge_set(*metric, dnode_total, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, DNODES_ALIVE, strlen(DNODES_ALIVE));
  taos_gauge_set(*metric, dnode_alive, sample_labels);

  //grant info
  metric = taosHashGet(tsMonitor.metrics, EXPIRE_TIME, strlen(EXPIRE_TIME));
  taos_gauge_set(*metric, pGrantInfo->expire_time, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, TIMESERIES_USED, strlen(TIMESERIES_USED));
  taos_gauge_set(*metric, pGrantInfo->timeseries_used, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, TIMESERIES_TOTAL, strlen(TIMESERIES_TOTAL));
  taos_gauge_set(*metric, pGrantInfo->timeseries_total, sample_labels);
}

void monGenVgroupInfoTable(SMonInfo *pMonitor){
  if(pMonitor->dmInfo.basic.cluster_id == 0) return;
  if (pMonitor->mmInfo.cluster.first_ep_dnode_id == 0) return;

  SMonVgroupInfo *pInfo = &pMonitor->mmInfo.vgroup;
  if (pMonitor->mmInfo.cluster.first_ep_dnode_id == 0) return;

  char cluster_id[TSDB_CLUSTER_ID_LEN] = {0};
  snprintf(cluster_id, TSDB_CLUSTER_ID_LEN, "%"PRId64, pMonitor->dmInfo.basic.cluster_id);

  for (int32_t i = 0; i < taosArrayGetSize(pInfo->vgroups); ++i) {
    SMonVgroupDesc *pVgroupDesc = taosArrayGet(pInfo->vgroups, i);

    char vgroup_id[TSDB_NODE_ID_LEN] = {0};
    snprintf(vgroup_id, TSDB_NODE_ID_LEN, "%"PRId32, pVgroupDesc->vgroup_id);

    const char *sample_labels[] = {cluster_id, vgroup_id, pVgroupDesc->database_name};

    taos_gauge_t **metric = NULL;
  
    metric = taosHashGet(tsMonitor.metrics, TABLES_NUM, strlen(TABLES_NUM));
    taos_gauge_set(*metric, pVgroupDesc->tables_num, sample_labels);

    metric = taosHashGet(tsMonitor.metrics, STATUS, strlen(STATUS));
    int32_t status = 0;
    if(strcmp(pVgroupDesc->status, "ready") == 0){
      status = 1;
    }
    taos_gauge_set(*metric, status, sample_labels);
 }
}

void monGenDnodeInfoTable(SMonInfo *pMonitor) {
  if(pMonitor->dmInfo.basic.cluster_id == 0) {
    uError("failed to generate dnode info table since cluster_id is 0");
    return;
  }

  char cluster_id[TSDB_CLUSTER_ID_LEN] = {0};
  snprintf(cluster_id, TSDB_CLUSTER_ID_LEN, "%"PRId64, pMonitor->dmInfo.basic.cluster_id);

  char dnode_id[TSDB_NODE_ID_LEN] = {0};
  snprintf(dnode_id, TSDB_NODE_ID_LEN, "%"PRId32, pMonitor->dmInfo.basic.dnode_id);

  const char *sample_labels[] = {cluster_id, dnode_id, pMonitor->dmInfo.basic.dnode_ep};

  taos_gauge_t **metric = NULL;

  //dnode info
  SMonDnodeInfo *pInfo = &pMonitor->dmInfo.dnode;
  SMonSysInfo   *pSys = &pMonitor->dmInfo.sys;
  SVnodesStat   *pStat = &pMonitor->vmInfo.vstat;
  SMonClusterInfo *pClusterInfo = &pMonitor->mmInfo.cluster;

  double interval = (pMonitor->curTime - pMonitor->lastTime) / 1000.0;
  if (pMonitor->curTime - pMonitor->lastTime == 0) {
    interval = 1;
  }

  double cpu_engine = 0;
  double mem_engine = 0;
  double net_in = 0;
  double net_out = 0;
  double io_read = 0;
  double io_write = 0;
  double io_read_disk = 0;
  double io_write_disk = 0;

  SMonSysInfo *sysArrays[6];
  sysArrays[0] = &pMonitor->dmInfo.sys;
  sysArrays[1] = &pMonitor->mmInfo.sys;
  sysArrays[2] = &pMonitor->vmInfo.sys;
  sysArrays[3] = &pMonitor->qmInfo.sys;
  sysArrays[4] = &pMonitor->smInfo.sys;
  sysArrays[5] = &pMonitor->bmInfo.sys;
  for (int32_t i = 0; i < 6; ++i) {
    cpu_engine += sysArrays[i]->cpu_engine;
    mem_engine += sysArrays[i]->mem_engine;
    net_in += sysArrays[i]->net_in;
    net_out += sysArrays[i]->net_out;
    io_read += sysArrays[i]->io_read;
    io_write += sysArrays[i]->io_write;
    io_read_disk += sysArrays[i]->io_read_disk;
    io_write_disk += sysArrays[i]->io_write_disk;
  }

  double req_select_rate = pStat->numOfSelectReqs / interval;
  double req_insert_rate = pStat->numOfInsertReqs / interval;
  double req_insert_batch_rate = pStat->numOfBatchInsertReqs / interval;
  double net_in_rate = net_in / interval;
  double net_out_rate = net_out / interval;
  double io_read_rate = io_read / interval;
  double io_write_rate = io_write / interval;
  double io_read_disk_rate = io_read_disk / interval;
  double io_write_disk_rate = io_write_disk / interval;

  metric = taosHashGet(tsMonitor.metrics, UPTIME, strlen(UPTIME));
  taos_gauge_set(*metric, pInfo->uptime, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, CPU_ENGINE, strlen(CPU_ENGINE));
  taos_gauge_set(*metric, cpu_engine, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, CPU_SYSTEM, strlen(CPU_SYSTEM));
  taos_gauge_set(*metric, pSys->cpu_system, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, MEM_ENGINE, strlen(MEM_ENGINE));
  taos_gauge_set(*metric, mem_engine, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, MEM_SYSTEM, strlen(MEM_SYSTEM));
  taos_gauge_set(*metric, pSys->mem_system, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, DISK_ENGINE, strlen(DISK_ENGINE));
  taos_gauge_set(*metric, pSys->disk_engine, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, DISK_USED, strlen(DISK_USED));
  taos_gauge_set(*metric, pSys->disk_used, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, NET_IN, strlen(NET_IN));
  taos_gauge_set(*metric, net_in_rate, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, NET_OUT, strlen(NET_OUT));
  taos_gauge_set(*metric, net_out_rate, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, IO_READ, strlen(IO_READ));
  taos_gauge_set(*metric, io_read_rate, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, IO_WRITE, strlen(IO_WRITE));
  taos_gauge_set(*metric, io_write_rate, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, IO_READ_DISK, strlen(IO_READ_DISK));
  taos_gauge_set(*metric, io_read_disk_rate, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, IO_WRITE_DISK, strlen(IO_WRITE_DISK));
  taos_gauge_set(*metric, io_write_disk_rate, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, ERRORS, strlen(ERRORS));
  taos_gauge_set(*metric, io_read_disk_rate, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, VNODES_NUM, strlen(VNODES_NUM));
  taos_gauge_set(*metric, pStat->errors, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, MASTERS, strlen(MASTERS));
  taos_gauge_set(*metric, pStat->masterNum, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, HAS_MNODE, strlen(HAS_MNODE));
  taos_gauge_set(*metric, pInfo->has_mnode, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, HAS_QNODE, strlen(HAS_QNODE));
  taos_gauge_set(*metric, pInfo->has_qnode, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, HAS_SNODE, strlen(HAS_SNODE));
  taos_gauge_set(*metric, pInfo->has_snode, sample_labels);

  //log number
  SMonLogs *logs[6];
  logs[0] = &pMonitor->log;
  logs[1] = &pMonitor->mmInfo.log;
  logs[2] = &pMonitor->vmInfo.log;
  logs[3] = &pMonitor->smInfo.log;
  logs[4] = &pMonitor->qmInfo.log;
  logs[5] = &pMonitor->bmInfo.log;

  int32_t numOfErrorLogs = 0;
  int32_t numOfInfoLogs = 0;
  int32_t numOfDebugLogs = 0;
  int32_t numOfTraceLogs = 0;

  for (int32_t j = 0; j < 6; j++) {
    SMonLogs *pLog = logs[j];
    numOfErrorLogs += pLog->numOfErrorLogs;
    numOfInfoLogs += pLog->numOfInfoLogs;
    numOfDebugLogs += pLog->numOfDebugLogs;
    numOfTraceLogs += pLog->numOfTraceLogs;
  }

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_ERROR, strlen(DNODE_LOG_ERROR));
  taos_gauge_set(*metric, numOfErrorLogs, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_INFO, strlen(DNODE_LOG_INFO));
  taos_gauge_set(*metric, numOfInfoLogs, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_DEBUG, strlen(DNODE_LOG_DEBUG));
  taos_gauge_set(*metric, numOfDebugLogs, sample_labels);

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_TRACE, strlen(DNODE_LOG_TRACE));
  taos_gauge_set(*metric, numOfTraceLogs, sample_labels);
}

void monGenDnodeStatusInfoTable(SMonInfo *pMonitor){
  if(pMonitor->dmInfo.basic.cluster_id == 0) {
    uError("failed to generate dnode info table since cluster_id is 0");
    return;
  }
  if (pMonitor->mmInfo.cluster.first_ep_dnode_id == 0) return;

  char cluster_id[TSDB_CLUSTER_ID_LEN];
  snprintf(cluster_id, TSDB_CLUSTER_ID_LEN, "%"PRId64, pMonitor->dmInfo.basic.cluster_id);

  taos_gauge_t **metric = NULL;  
  //dnodes status

  SMonClusterInfo *pClusterInfo = &pMonitor->mmInfo.cluster;

  for (int32_t i = 0; i < taosArrayGetSize(pClusterInfo->dnodes); ++i) {
    SMonDnodeDesc *pDnodeDesc = taosArrayGet(pClusterInfo->dnodes, i);

    char dnode_id[TSDB_NODE_ID_LEN] = {0};
    snprintf(dnode_id, TSDB_NODE_ID_LEN, "%"PRId32, pDnodeDesc->dnode_id);

    const char *sample_labels[] = {cluster_id, dnode_id, pDnodeDesc->dnode_ep};

    metric = taosHashGet(tsMonitor.metrics, DNODE_STATUS, strlen(DNODE_STATUS));

    int32_t status = 0;
    if(strcmp(pDnodeDesc->status, "ready") == 0){
      status = 1;
    }
    taos_gauge_set(*metric, status, sample_labels);
  }
}

void monGenDataDiskTable(SMonInfo *pMonitor){
  if(pMonitor->dmInfo.basic.cluster_id == 0) return;

  SMonDiskInfo *pInfo = &pMonitor->vmInfo.tfs;

  char cluster_id[TSDB_CLUSTER_ID_LEN] = {0};
  snprintf(cluster_id, TSDB_CLUSTER_ID_LEN, "%" PRId64, pMonitor->dmInfo.basic.cluster_id);

  char dnode_id[TSDB_NODE_ID_LEN] = {0};
  snprintf(dnode_id, TSDB_NODE_ID_LEN, "%"PRId32, pMonitor->dmInfo.basic.dnode_id);


  taos_gauge_t **metric = NULL;

  for (int32_t i = 0; i < taosArrayGetSize(pInfo->datadirs); ++i) {
    SMonDiskDesc *pDatadirDesc = taosArrayGet(pInfo->datadirs, i);

    char level[LEVEL_LEN] = {0};
    snprintf(dnode_id, LEVEL_LEN, "%"PRId32, pDatadirDesc->level);

    const char *sample_labels[] = {cluster_id, dnode_id, pMonitor->dmInfo.basic.dnode_ep, pDatadirDesc->name, level};

    metric = taosHashGet(tsMonitor.metrics, DNODE_DATA_AVAIL, strlen(DNODE_DATA_AVAIL));
    taos_gauge_set(*metric, pDatadirDesc->size.avail, sample_labels); 

    metric = taosHashGet(tsMonitor.metrics, DNODE_DATA_USED, strlen(DNODE_DATA_USED));
    taos_gauge_set(*metric, pDatadirDesc->size.used, sample_labels); 

    metric = taosHashGet(tsMonitor.metrics, DNODE_DATA_TOTAL, strlen(DNODE_DATA_TOTAL));
    taos_gauge_set(*metric, pDatadirDesc->size.total, sample_labels); 
  }
}

void monGenLogDiskTable(SMonInfo *pMonitor){
  if(pMonitor->dmInfo.basic.cluster_id == 0) return;

  SMonDiskDesc *pLogDesc = &pMonitor->dmInfo.dnode.logdir;
  SMonDiskDesc *pTempDesc = &pMonitor->dmInfo.dnode.tempdir;

  char cluster_id[TSDB_CLUSTER_ID_LEN] = {0};
  snprintf(cluster_id, TSDB_CLUSTER_ID_LEN, "%" PRId64, pMonitor->dmInfo.basic.cluster_id);

  char dnode_id[TSDB_NODE_ID_LEN] = {0};
  snprintf(dnode_id, TSDB_NODE_ID_LEN, "%"PRId32, pMonitor->dmInfo.basic.dnode_id);

  taos_gauge_t **metric = NULL;

  const char *sample_log_labels[] = {cluster_id, dnode_id, pMonitor->dmInfo.basic.dnode_ep, pLogDesc->name};

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_AVAIL, strlen(DNODE_LOG_AVAIL));
  taos_gauge_set(*metric, pLogDesc->size.avail, sample_log_labels); 

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_USED, strlen(DNODE_LOG_USED));
  taos_gauge_set(*metric, pLogDesc->size.used, sample_log_labels); 

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_TOTAL, strlen(DNODE_LOG_TOTAL));
  taos_gauge_set(*metric, pLogDesc->size.total, sample_log_labels); 

  const char *sample_temp_labels[] = {cluster_id, dnode_id, pMonitor->dmInfo.basic.dnode_ep, pTempDesc->name};

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_AVAIL, strlen(DNODE_LOG_AVAIL));
  taos_gauge_set(*metric, pTempDesc->size.avail, sample_temp_labels); 

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_USED, strlen(DNODE_LOG_USED));
  taos_gauge_set(*metric, pTempDesc->size.used, sample_temp_labels); 

  metric = taosHashGet(tsMonitor.metrics, DNODE_LOG_TOTAL, strlen(DNODE_LOG_TOTAL));
  taos_gauge_set(*metric, pTempDesc->size.total, sample_temp_labels); 
}

void monGenMnodeRoleTable(SMonInfo *pMonitor){
  SMonClusterInfo *pInfo = &pMonitor->mmInfo.cluster;
  if (pMonitor->mmInfo.cluster.first_ep_dnode_id == 0) return;
  SMonBasicInfo *pBasicInfo = &pMonitor->dmInfo.basic;
  if(pBasicInfo->cluster_id == 0) return;

  char buf[TSDB_CLUSTER_ID_LEN] = {0};
  snprintf(buf, TSDB_CLUSTER_ID_LEN, "%" PRId64, pBasicInfo->cluster_id);

  taos_gauge_t **metric = NULL;
  
  for (int32_t i = 0; i < taosArrayGetSize(pInfo->mnodes); ++i) {

    SMonMnodeDesc *pMnodeDesc = taosArrayGet(pInfo->mnodes, i);

    char mnode_id[TSDB_NODE_ID_LEN] = {0};
    snprintf(mnode_id, TSDB_NODE_ID_LEN, "%"PRId32, pMnodeDesc->mnode_id);

    const char *sample_labels[] = {buf, mnode_id, pMnodeDesc->mnode_ep};

    metric = taosHashGet(tsMonitor.metrics, MNODE_ROLE, strlen(MNODE_ROLE));
    taos_gauge_set(*metric, pMnodeDesc->syncState, sample_labels);
  }
}

void monGenVnodeRoleTable(SMonInfo *pMonitor){
  SMonVgroupInfo *pInfo = &pMonitor->mmInfo.vgroup;
  if (pMonitor->mmInfo.cluster.first_ep_dnode_id == 0) return;

  SMonBasicInfo *pBasicInfo = &pMonitor->dmInfo.basic;
  if(pBasicInfo->cluster_id == 0) return;

  char buf[TSDB_CLUSTER_ID_LEN] = {0};
  snprintf(buf, TSDB_CLUSTER_ID_LEN, "%" PRId64, pBasicInfo->cluster_id);

  taos_gauge_t **metric = NULL;

  for (int32_t i = 0; i < taosArrayGetSize(pInfo->vgroups); ++i) {
    SMonVgroupDesc *pVgroupDesc = taosArrayGet(pInfo->vgroups, i);

    char vgroup_id[TSDB_VGROUP_ID_LEN] = {0};
    snprintf(vgroup_id, TSDB_VGROUP_ID_LEN, "%"PRId32, pVgroupDesc->vgroup_id);

    for (int32_t j = 0; j < TSDB_MAX_REPLICA; ++j) {
      SMonVnodeDesc *pVnodeDesc = &pVgroupDesc->vnodes[j];
      if (pVnodeDesc->dnode_id <= 0) continue;

      char dnode_id[TSDB_NODE_ID_LEN] = {0};
      snprintf(dnode_id, TSDB_NODE_ID_LEN, "%"PRId32, pVnodeDesc->dnode_id);

      const char *sample_labels[] = {buf, vgroup_id, pVgroupDesc->database_name, dnode_id};

      metric = taosHashGet(tsMonitor.metrics, VNODE_ROLE, strlen(VNODE_ROLE));
      taos_gauge_set(*metric, pVnodeDesc->syncState, sample_labels);
    }
  }
}

void monSendPromReport() {
  char ts[50] = {0};
  sprintf(ts, "%" PRId64, taosGetTimestamp(TSDB_TIME_PRECISION_MILLI));

  char* promStr = NULL;
  char* pCont = (char *)taos_collector_registry_bridge_new(TAOS_COLLECTOR_REGISTRY_DEFAULT, ts, "%" PRId64, &promStr);
  if(tsMonitorLogProtocol){
    uInfoL("report cont:\n%s\n", pCont);
    uDebugL("report cont prom:\n%s\n", promStr);
  }
  if (pCont != NULL) {
    EHttpCompFlag flag = tsMonitor.cfg.comp ? HTTP_GZIP : HTTP_FLAT;
    if (taosSendHttpReport(tsMonitor.cfg.server, tsMonFwUri, tsMonitor.cfg.port, pCont, strlen(pCont), flag) != 0) {
      uError("failed to send monitor msg");
    }else{
      taos_collector_registry_clear_batch(TAOS_COLLECTOR_REGISTRY_DEFAULT);
    }
    taosMemoryFreeClear(pCont);
  }
  if(promStr != NULL){
    taosMemoryFreeClear(promStr);
  }
}