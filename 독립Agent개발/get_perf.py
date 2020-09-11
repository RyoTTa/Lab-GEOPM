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

    perf_commnad = f'perf stat -a -M {event_1} -e {event_2} -I 300 ./{path} >a.out'
    print(perf_commnad)
    proc = subprocess.Popen(perf_commnad, shell=True ,universal_newlines=True,text=True ,stderr=subprocess.PIPE ,env={'OMP_NUM_THREADS': f'{thread}'})
    
    f = open("test.txt","w")
    '''
    for line in iter(proc.stderr.readline, ''):
        if line[0] == '#' :
            line_ran = 0
        else :
            text_temp.append(line.strip())
            line_ran = 1
        for t in range(len(metric_list+predefined_list) +2 - line_ran):
            text_temp.append(str(proc.stderr.readline().strip()))
        for t in text_temp :
            print(t)
        del text_temp[0:len(text_temp)]
    

    for line in iter(proc.stderr.readline, ''):
        if line[0] == '#' :
            line_ran = 0
        else :
            translate(line.strip().split())
            line_ran = 1
        for t in range(len(metric_list+predefined_list) +2 - line_ran):
            translate(proc.stderr.readline().strip().split())
        print(dic_result)
        dic_result = {}
    '''

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
        
        if float(cur_l3load)/float(past_l3load) > 1.1 :
            if ratio_index != 0 :
                ratio_index = 0
                os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                print(f"U {uncore_ratio[ratio_index]} cur_l3load {cur_l3load} past_l3load {past_l3load}")

        elif float(cur_l3load)/float(past_l3load) < 0.9 : 
            if ratio_index < len(uncore_ratio) -1 : 
                ratio_index += 1
                os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                os.system(f"sudo wrmsr -p0 0x620H 0x000000000000{uncore_ratio[ratio_index]}")
                print(f"D {uncore_ratio[ratio_index]} cur_l3load {cur_l3load} past_l3load {past_l3load}")
            

        
        power += float(dic_result['power/energy-pkg/'])
        past_l3load = cur_l3load
        dic_result = {}

    print(power)

    result = proc.communicate()[0]
    
    ##print(result)
    
    os.system(f"sudo wrmsr -p0 0x620H 0x0000000000000C18")
    os.system(f"sudo wrmsr -p20 0x620H 0x0000000000000C18")


# open perf output file, and if word in line have 'metric_list' element, that line save into list
    '''
    with open("perf_result.txt", "rt") as fp:
        for line in fp:
            if any(i in line for i in metric_list + predefined_list):
               list_str.append(line.strip())


# Spliting 'list_str' element base on a blank space
    for i in list_str:
        list_splited.append([v for v in i.split(" ") if v])

    time = list_splited[0][0]
    total_list = metric_list + predefined_list
# Base on time, get metric value and save dictionary
    for i in list_splited:
        if time == i[0]:
            for metric in (metric_list + predefined_list):
                if any(metric in s for s in i):
                    if i[i.index(metric)-1].isalpha():
                        dic_temp[metric] = i[i.index(metric)-2]
                    else:
                        dic_temp[metric] = i[i.index(metric)-1]
        else:
            dic_result[time] = dic_temp
            dic_temp = {}
            time = i[0]
            if "Instructions" in i:
                dic_temp["Instructions"] = i[i.index("Instructions")-1]

# Save as json file.
    with open("./result/"+path+"_max_"+count+".json", 'w', encoding='utf-8') as fp:
        json.dump(dic_result, fp, indent='\t')
    '''
except Exception as e:
    print(f"Error :: ", e)
