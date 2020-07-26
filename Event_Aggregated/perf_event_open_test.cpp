#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

#define CPU_NUM 40
#define MAX_READ_EVENT 30
#define CNT_READ_EVENT 2

struct read_format {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};

void do_something() {
  int i;
  char* ptr;

  ptr = (char*)malloc(1024*1024);
  for (i = 0; i < 1024*1024; i++) {
    ptr[i] = (char) (i & 0xff); // pagefault
  }
  free(ptr);
}

int main(int argc, char* argv[]) {
  struct perf_event_attr pea;
  long long val[CPU_NUM][CNT_READ_EVENT];
  int fd[CPU_NUM][CNT_READ_EVENT];
  uint64_t id[CPU_NUM][CNT_READ_EVENT];
  char buf[4096];
  struct read_format* rf = (struct read_format*) buf;
  int i;
  uint32_t type_arr[MAX_READ_EVENT]={
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE
  };
  uint64_t config_arr[MAX_READ_EVENT]={
    PERF_COUNT_HW_CPU_CYCLES,
    PERF_COUNT_HW_CPU_CYCLES
  };

  for(int i=0; i<4096;i++){
    buf[i] = 0;
  }

  for(i=0; i<CNT_READ_EVENT; i++){
    memset(&pea, 0, sizeof(struct perf_event_attr));
    pea.type = PERF_TYPE_HARDWARE;
    pea.size = sizeof(struct perf_event_attr);
    pea.config = PERF_COUNT_HW_CPU_CYCLES;
    pea.disabled = 1;
    pea.exclude_kernel = 1;
    pea.exclude_hv = 1;
    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

    if(i==0){
      for(int j=0 ; j<CPU_NUM; j++){
        fd[j][i] = syscall(__NR_perf_event_open, &pea, -1, j, -1, 0);
        ioctl(fd[j][0], PERF_EVENT_IOC_ID, &id[j][i]);
      }
    }else{
      for(int j=0; j<CPU_NUM; j++){
        fd[j][i] = syscall(__NR_perf_event_open, &pea, -1, j, fd[j][0], 0);
        ioctl(fd[j][i], PERF_EVENT_IOC_ID, &id[j][i]);
      }
    }
    
  }

  for(int i=0; i<CPU_NUM; i++){
    ioctl(fd[i][0], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(fd[i][0], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  }
  do_something();
  //printf("레후레후\n");

  for(int i=0; i<CPU_NUM; i++){
    ioctl(fd[i][0], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    read(fd[i][0], buf, sizeof(buf));

    for (uint64_t j = 0; j < rf->nr; j++) {
      if (rf->values[j].id == id[i][j]) {
        val[i][j] = rf->values[j].value;
      }
    }

    printf("cpu cycles: %u\n", val[i][0]);
    printf("inst: %u\n\n", val[i][1]);


  }



  

  return 0;
}
