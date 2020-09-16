import os
import subprocess
import json
import sys
import time

# Parameter
# python3 get_perf_max.py bench_name count thread



try:
    list_splited = []
    list_str = []
    dic_result = {}
    dic_temp = {}
    text_temp = []
    uncore_ratio = ['1919', '1515', '1212', '0F0F', '0C0C']
    # 2.5GHz
    ratio_index = 0

    def translate(line): 
        if len(line) == 3 :
            dic_result[line[2]] = line[1]
        elif len(line) == 4 :
            dic_result[line[3]] = line[1]
        elif len(line) == 6 :
            dic_result[line[5]] = line[4]
        
    
# if you want another perf 'Metric groups' event, add it the list below.
    metric_list = ["Instructions","CPU_Utilization","CPI"]
    event_1 = ' -M '.join(metric_list)

    #predefined_list = ["llc_misses.mem_read","llc_misses.mem_write","unc_m_act_count.wr","unc_m_cas_count.all","unc_m_power_channel_ppd","unc_m_pre_count.rd","unc_m_pre_count.wr","unc_m_rpq_inserts","unc_m_wpq_inserts","power/energy-pkg/","power/energy-ram/"]
    predefined_list = ["LLC-store-misses","LLC-loads","power/energy-pkg/","power/energy-ram/"]

    event_2 = ' -e '.join(predefined_list)

    #temp = input("<Path>/<Program> >> ")
    #path = temp.split()[0]
    #count = temp.split()[1]
    path = sys.argv[1]
    count = sys.argv[2]
    thread = sys.argv[3]
    
    if os.path.isfile(path) == False:
        raise Exception('Invaild path')

# make shell command and run it. 'result' is program result
# perf save result file in currunt directory, nadmed 'perf_result.txt'
    
    os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[0]}")
    os.system(f"sudo wrmsr -p20 0x620H 0x000000000000{uncore_ratio[0]}")

    perf_commnad = f'perf stat -a -M {event_1} -e {event_2} -I 200 ./{path} >a.out'
    print(perf_commnad)
    proc = subprocess.Popen(perf_commnad, shell=True ,universal_newlines=True,text=True ,stderr=subprocess.PIPE ,env={'OMP_NUM_THREADS': f'{thread}'})
    
    f = open("test.txt","w")

    line = proc.stderr.readline()
    state = True
    power = 0.0

    if line[0] == '#' :
            line_ran = 0
    else :
            translate(line.strip().split())
            line_ran = 1
    for t in range(len(metric_list+predefined_list) +2 - line_ran):
        translate(proc.stderr.readline().strip().split())

    past_l3load = float(dic_result['LLC-loads'])
    power += float(dic_result['power/energy-pkg/'])
    #/Instructions 추가 
    #--per-node 추가
    #이벤트별 uncore freq 로그 그래프 추가
    dic_result = {}
    for line in iter(proc.stderr.readline, ''):
        if line[0] == '#' :
            line_ran = 0
        else :
            translate(line.strip().split())
            line_ran = 1
        for t in range(len(metric_list+predefined_list) +2 - line_ran):
            translate(proc.stderr.readline().strip().split())

        cur_l3load = float(dic_result['LLC-loads'])
        cur_CPI = float(dic_result['CPI'])
        
        if float(cur_l3load)/float(past_l3load) > 1.1 :
            if cur_CPI < 0.8 :
                if ratio_index != len(uncore_ratio) -1 :
                    ratio_index = len(uncore_ratio) -1
                    os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                    os.system(f"sudo wrmsr -p20 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                    print(f"DD {uncore_ratio[ratio_index]} cur_l3load {cur_l3load} past_l3load {past_l3load}")
            else :
                if ratio_index != 0 :
                    ratio_index = 0
                    os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                    os.system(f"sudo wrmsr -p20 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                    print(f"U {uncore_ratio[ratio_index]} cur_l3load {cur_l3load} past_l3load {past_l3load}")

        elif float(cur_l3load)/float(past_l3load) < 0.9 : 
            if cur_CPI > 2 : 
                if ratio_index != 0 :
                    ratio_index = 0
                    os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                    os.system(f"sudo wrmsr -p20 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                    print(f"UU {uncore_ratio[ratio_index]} cur_l3load {cur_l3load} past_l3load {past_l3load}")
            else:
                if ratio_index < len(uncore_ratio) -1 : 
                    ratio_index += 1
                    os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                    os.system(f"sudo wrmsr -p20 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                    print(f"D {uncore_ratio[ratio_index]} cur_l3load {cur_l3load} past_l3load {past_l3load}")
            
        power += float(dic_result['power/energy-pkg/'])
        past_l3load = cur_l3load
        dic_result = {}

    print(power)

    result = proc.communicate()[0]
    
    ##print(result)
    
    os.system(f"sudo wrmsr -p0 0x620H 0x0000000000000C18")
    os.system(f"sudo wrmsr -p20 0x620H 0x0000000000000C18")

except Exception as e:
    print(f"Error :: ", e)
