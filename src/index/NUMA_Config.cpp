#include "index/NUMA_Config.h"

#include <unordered_map>

pmem::obj::pool_base pop_numa[nap::kMaxNumaCnt];
int numa_map[nap::kMaxThreadCnt];

thread_local int my_thread_id = 40;

const std::string PM_PREFIX = "/mnt/pmem";
const int kNum = 1;
const int kStartNumaNo = 1;
const int kThreadsPerNuma = 10;
const int kStartThreadNo = 40;
// numa# => /mnt/pmem#
const std::unordered_map<int, int> Numa2PmSuffixNo = {
  {1, 0},  // numa 1 => /mnt/pmem0
};

const uint64_t KPmemObjPoolSize = 50UL << 30; // 50 GB


// void bindCore(uint16_t core) {
//   printf("bind %d\n", core);
//   cpu_set_t cpuset;
//   CPU_ZERO(&cpuset);
//   CPU_SET(core, &cpuset);
//   int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
//   if (rc != 0) {
//     printf("can't bind core %d!", core);
//     exit(-1);
//   }
// }


void init_numa_pool()
{
  // core id => numa id
  int numa_id = kStartNumaNo;
  int core_id = kStartThreadNo;
  for(int k = 0; k < kNum; ++k) {
    for (int i = 0; i < kThreadsPerNuma; ++i) {
      printf("core id [%d] => numa id [%d]\n", core_id, numa_id);
      numa_map[core_id] = numa_id;
      ++core_id;
    }
    ++numa_id;
  }

  numa_id = kStartNumaNo;
  for(int k = 0; k < kNum; ++k) {
    auto it = Numa2PmSuffixNo.find(numa_id);
    assert(it != Numa2PmSuffixNo.end());
    int pm_id = it->second;

    std::string pool_name = PM_PREFIX + std::to_string(pm_id) + "/numa";
    printf("numa %d pool: %s\n", numa_id, pool_name.c_str());

    remove(pool_name.c_str());
    pop_numa[numa_id] = pmem::obj::pool<int>::create(
        pool_name, "WQ", KPmemObjPoolSize, S_IWUSR | S_IRUSR);
    ++numa_id;
  }
  // for (int i = 0; i < nap::Topology::kNumaCnt; ++i)
  // {
  //   std::string pool_name =
  //       std::string("/mnt/pm") + std::to_string(i) + "/numa";
  //   printf("numa %d pool: %s\n", i, pool_name.c_str());

  //   remove(pool_name.c_str());
  //   pop_numa[i] = pmem::obj::pool<int>::create(
  //       pool_name, "WQ", PMEMOBJ_MIN_POOL * 1024 * 8, S_IWUSR | S_IRUSR);
  // }
}

void *index_pmem_alloc(size_t size)
{
  PMEMoid oid;
  if (pmemobj_alloc(pop_numa[numa_map[my_thread_id]].handle(), &oid, size, 0,
                    nullptr, nullptr))
  {
    fprintf(stderr, "fail to alloc nvm\n");
    exit(-1);
  }

  return (void *)pmemobj_direct(oid);
}

void index_pmem_free(void *ptr)
{
  auto f_oid = pmemobj_oid(ptr);
  pmemobj_free(&f_oid);
}