/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TestAgent.hpp"

#include <sstream>
#include <cmath>
#include <iomanip>
#include <utility>
#include <iostream>

#include "contrib/json11/json11.hpp"

#include "geopm.h"
#include "geopm_hash.h"

#include "TestRegion.hpp"
#include "FrequencyGovernor.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

/*perf_event_oepn test*/
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
#include <fstream>
#include <string>
#include <stdlib.h>

/*perf_event_oepn test*/

using json11::Json;

namespace geopm
{
    TestAgent::TestAgent()
        : TestAgent(platform_io(), platform_topo(),
                               FrequencyGovernor::make_shared(),
                               std::map<uint64_t, std::shared_ptr<TestRegion> >())
    {

    }

    TestAgent::TestAgent(PlatformIO &plat_io, const PlatformTopo &topo,
                                               std::shared_ptr<FrequencyGovernor> gov,
                                               std::map<uint64_t, std::shared_ptr<TestRegion> > region_map)
        : M_PRECISION(16)
        , M_WAIT_SEC(0.005)
        , M_MIN_LEARNING_RUNTIME(M_WAIT_SEC * 10)
        , M_NETWORK_NUM_SAMPLE_DELAY(2)
        , M_UNMARKED_NUM_SAMPLE_DELAY(2)
        , M_POLICY_PERF_MARGIN_DEFAULT(0.10)  // max 10% performance degradation
        , m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_freq_governor(gov)
        , m_freq_ctl_domain_type(m_freq_governor->frequency_domain_type())
        , m_num_freq_ctl_domain(m_platform_topo.num_domain(m_freq_ctl_domain_type))
        , m_region_map(m_num_freq_ctl_domain, region_map)
        , m_samples_since_boundary(m_num_freq_ctl_domain)
        , m_last_wait(GEOPM_TIME_REF)
        , m_level(-1)
        , m_num_children(0)
        , m_do_send_policy(false)
        , m_perf_margin(M_POLICY_PERF_MARGIN_DEFAULT)
    {
    
    /*perf_event_open test*/
        memset(&pea, 0, sizeof(struct perf_event_attr));
        //pea.type = PERF_TYPE_HARDWARE;
        //pea.type = PERF_TYPE_SOFTWARE;
        pea.type = PERF_TYPE_HW_CACHE;
        pea.size = sizeof(struct perf_event_attr);
        //pea.config = PERF_COUNT_HW_CACHE_MISSES;
        //pea.config = PERF_COUNT_SW_PAGE_FAULTS;
        pea.config = (PERF_COUNT_HW_CACHE_LL)|(PERF_COUNT_HW_CACHE_OP_WRITE << 8)|(PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
        pea.disabled = 1;
        pea.exclude_kernel = 1;
        pea.exclude_hv = 1;
        
        /*multi file descriptor*/
        for(int i=0; i<12; i++){
            fd[i] = syscall(__NR_perf_event_open, &pea, -1, i, -1, 0);
            ioctl(fd[i], PERF_EVENT_IOC_RESET, 0);
            ioctl(fd[i], PERF_EVENT_IOC_ENABLE, 0);
        }
        /*single file descriptor*/
       /*
        fd[0] = syscall(__NR_perf_event_open, &pea, 0, -1, -1, 0);
        ioctl(fd[0], PERF_EVENT_IOC_RESET, 0);
        ioctl(fd[0], PERF_EVENT_IOC_ENABLE, 0);
        */
        outFile.open("/root/test.dat");
        inFile.open("/etc/hostname");
        
        getline(inFile,model_name);
    /*perf_event_open test*/
        
    }

    std::string TestAgent::plugin_name(void)
    {
        return "test_agent";
    }

    std::unique_ptr<Agent> TestAgent::make_plugin(void)
    {
        return geopm::make_unique<TestAgent>();
    }

    void TestAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_level = level;
        if (m_level == 0) {
            m_num_children = 0;
            init_platform_io();
        }
        else {
            m_num_children = fan_in[level - 1];
        }
    }

    bool TestAgent::update_policy(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("TestAgent::" + std::string(__func__) +
                            "(): in_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_perf_margin = in_policy[M_POLICY_PERF_MARGIN];
        // @todo: to support dynamic policies, policy values need to be passed to regions
        return m_freq_governor->set_frequency_bounds(in_policy[M_POLICY_FREQ_MIN],
                                                     in_policy[M_POLICY_FREQ_MAX]);
    }

    void TestAgent::validate_policy(std::vector<double> &policy) const
    {
#ifdef GEOPM_DEBUG
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("TestAgent::" + std::string(__func__) +
                            "(): policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (std::isnan(policy[M_POLICY_PERF_MARGIN])) {
            policy[M_POLICY_PERF_MARGIN] = M_POLICY_PERF_MARGIN_DEFAULT;
        }
        else if (policy[M_POLICY_PERF_MARGIN] < 0.0 || policy[M_POLICY_PERF_MARGIN] > 1.0) {
            throw Exception("TestAgent::" + std::string(__func__) + "(): performance margin must be between 0.0 and 1.0.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_freq_governor->validate_policy(policy[M_POLICY_FREQ_MIN],
                                         policy[M_POLICY_FREQ_MAX]);

        if (std::isnan(policy[M_POLICY_FREQ_FIXED])) {
            policy[M_POLICY_FREQ_FIXED] = m_platform_io.read_signal("FREQUENCY_MAX",
                                                                    GEOPM_DOMAIN_BOARD, 0);
        }
    }

        void TestAgent::split_policy(const std::vector<double> &in_policy,
                                            std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t) m_num_children) {
            throw Exception("TestAgent::" + std::string(__func__) +
                            "(): out_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        for (auto &child_policy : out_policy) {
            if (child_policy.size() != M_NUM_POLICY) {
                throw Exception("TestAgent::" + std::string(__func__) +
                                "(): child_policy vector not correctly sized.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
#endif
        m_do_send_policy = update_policy(in_policy);

        if (m_do_send_policy) {
            std::fill(out_policy.begin(), out_policy.end(), in_policy);
        }
    }

    bool TestAgent::do_send_policy(void) const
    {
        return m_do_send_policy;
    }

    void TestAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                                std::vector<double> &out_sample)
    {

    }

    bool TestAgent::do_write_batch(void) const
    {
        return m_freq_governor->do_write_batch();
    }

    void TestAgent::adjust_platform(const std::vector<double> &in_policy)//�÷��� ���� hash, hint ���� �����
    {
        update_policy(in_policy);
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            uint64_t hash = m_last_region_info[ctl_idx].hash;
            uint64_t hint = m_last_region_info[ctl_idx].hint;
            int samples = m_samples_since_boundary[ctl_idx];
            if (GEOPM_REGION_HASH_UNMARKED == hash) {
                if (M_UNMARKED_NUM_SAMPLE_DELAY < samples) {
                    m_target_freq[ctl_idx] = m_freq_governor->get_frequency_max();
                }
            }
            else if (GEOPM_REGION_HINT_NETWORK == hint) {
                if (M_NETWORK_NUM_SAMPLE_DELAY < samples) {
                    m_target_freq[ctl_idx] = m_freq_governor->get_frequency_min();
                }
            }
            else {
                auto region_it = m_region_map[ctl_idx].find(hash);
                if (region_it != m_region_map[ctl_idx].end()) {
                    m_target_freq[ctl_idx] = region_it->second->freq();
                }
                else {
                    throw Exception("EnergyEfficientAgent::" + std::string(__func__) +
                                    "(): unknown target frequency hash = " + std::to_string(hash),
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
            }
            //std::cout << ctl_idx << ' ' << m_last_region_info[ctl_idx].hostnames << std::endl;
	        //std::cout << ctl_idx << ' ' << m_last_region_info[ctl_idx].cycles << std::endl;
        }
        m_freq_governor->adjust_platform(m_target_freq);
    }

    void TestAgent::sample_platform(std::vector<double> &out_sample)//���ø��Ͽ� region�� ����?
    {
        double freq_min = m_freq_governor->get_frequency_min();
        double freq_max = m_freq_governor->get_frequency_max();
        double freq_step = m_freq_governor->get_frequency_step();

        outFile << model_name << "\n";
        //outFile << "hostname : " <<' ' << getenv("lscpu") << "\n";
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            
            /*multi file descriptor*/
            ioctl(fd[ctl_idx], PERF_EVENT_IOC_DISABLE, 0);
            read(fd[ctl_idx], &count, sizeof(long long));
            /*single file descriptor*/
            /*
            ioctl(fd[0], PERF_EVENT_IOC_DISABLE, 0);
            read(fd[0], &count, sizeof(long long));
            */
            struct m_region_info_s current_region_info {
                .hash = (uint64_t)m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HASH][ctl_idx]),
                .hint = (uint64_t)m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HINT][ctl_idx]),
                .runtime = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_RUNTIME][ctl_idx]),
                .count = (uint64_t)m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_COUNT][ctl_idx]),
		        .freq = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_FREQ][ctl_idx]),
		        .temp = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_TEMP][ctl_idx]),
                .cycles = count,
                .hostnames = model_name
                };
            outFile << ctl_idx <<' ' << current_region_info.cycles << "\n";
            // If region hash has changed, or region count changed for the same region
            // update current region (entry)
            if (m_last_region_info[ctl_idx].hash != current_region_info.hash ||
                m_last_region_info[ctl_idx].count != current_region_info.count) {
                m_samples_since_boundary[ctl_idx] = 0;
                if (current_region_info.hash != GEOPM_REGION_HASH_UNMARKED &&
                    current_region_info.hint != GEOPM_REGION_HINT_NETWORK) {
                    /// set the freq for the current region (entry)
                    auto current_region_it = m_region_map[ctl_idx].find(current_region_info.hash);
                    if (current_region_it == m_region_map[ctl_idx].end()) {
                        auto tmp = m_region_map[ctl_idx].emplace(current_region_info.hash,
                                                                 std::make_shared<TestRegionImp>
                                                                 (freq_min, freq_max, freq_step, m_perf_margin));
                        current_region_it = tmp.first;
                    }
                }
                /// update previous region (exit)
                struct m_region_info_s last_region_info = m_last_region_info[ctl_idx];
                if (last_region_info.hash != GEOPM_REGION_HASH_UNMARKED &&
                    last_region_info.hint != GEOPM_REGION_HINT_NETWORK) {
                    auto last_region_it = m_region_map[ctl_idx].find(last_region_info.hash);
                    if (last_region_it == m_region_map[ctl_idx].end()) {
                        throw Exception("TestAgent::" + std::string(__func__) +
                                        "(): region exit before entry detected.",
                                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    }
                    if (last_region_info.runtime != 0.0 &&
                        last_region_info.runtime < M_MIN_LEARNING_RUNTIME) {
                        last_region_it->second->disable();
                    }
                    // Higher is better for performance, so negate
                    last_region_it->second->update_exit(-1.0 * last_region_info.runtime);
                }
                m_last_region_info[ctl_idx] = current_region_info;
            }
            else {
                ++m_samples_since_boundary[ctl_idx];
            }
            //multi file descriptor
            ioctl(fd[ctl_idx], PERF_EVENT_IOC_RESET, 0);
            ioctl(fd[ctl_idx], PERF_EVENT_IOC_ENABLE, 0);
            count=0;
        }
        //single file descriptor
        /*
        ioctl(fd[0], PERF_EVENT_IOC_RESET, 0);
        ioctl(fd[0], PERF_EVENT_IOC_ENABLE, 0);
        count=0;
        */
    }

    bool TestAgent::do_send_sample(void) const
    {
        return false;
    }

    void TestAgent::wait(void)
    {
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> TestAgent::policy_names(void)
    {
        return {"FREQ_MIN", "FREQ_MAX", "PERF_MARGIN", "FREQ_FIXED"};
    }

    std::vector<std::string> TestAgent::sample_names(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > TestAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > TestAgent::report_host(void) const
    {
        auto region_freq_map = report_region();
        std::vector<std::pair<std::string, std::string> > result;
        std::ostringstream oss;
        oss << std::setprecision(TestAgent::M_PRECISION) << std::scientific;
        for (const auto &region : region_freq_map) {
            oss << "\n    0x" << std::hex << std::setfill('0') << std::setw(16) << std::fixed;
            oss << region.first;
            oss << std::setfill('\0') << std::setw(0) << std::scientific;
            oss << ": " << region.second[0].second;  // Only item in the vector is requested frequency
        }
        oss << "\n";
        result.push_back({"Final online freq map", oss.str()});

        return result;
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > TestAgent::report_region(void) const
    {
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > result;
        std::map<uint64_t, std::pair<double, double> > region_frequency_count_map;
        for (const auto &region_map_it : m_region_map) {
            // If region is in this map, online learning was used to set frequency
            for (const auto &region : region_map_it) {
                if (!region.second->is_learning()) {
                    auto it = region_frequency_count_map.find(region.first);
                    if (it == region_frequency_count_map.end()) {
                        region_frequency_count_map[region.first] = std::make_pair(region.second->freq(), 1.0);
                    }
                    else {
                        it->second.first += region.second->freq();
                        it->second.second += 1.0;
                    }
                }
            }
        }
        for (const auto &it : region_frequency_count_map) {
            /// @todo re-implement with m_region_map and m_hash_freq_map keys as pair (hash + hint)
            // Average frequencies for all domains that completed learning
            double requested_freq = it.second.first / it.second.second;
            result[it.first].push_back(std::make_pair("requested-online-frequency",
                                                      std::to_string(requested_freq)));
        }
        return result;
    }

    std::vector<std::string> TestAgent::trace_names(void) const
    {
        return {};
    }

    std::vector<std::function<std::string(double)> > TestAgent::trace_formats(void) const
    {
        return {};
    }

    void TestAgent::trace_values(std::vector<double> &values)
    {

    }

    void TestAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("TestAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_platform_io.write_control("FREQUENCY", GEOPM_DOMAIN_BOARD, 0, policy[M_POLICY_FREQ_FIXED]);
    }

    void TestAgent::init_platform_io(void)   //�÷��� �ʱ�ȭ
    {
        m_freq_governor->init_platform_io();
        const struct m_region_info_s DEFAULT_REGION { .hash = GEOPM_REGION_HASH_UNMARKED,
                                                      .hint = GEOPM_REGION_HINT_UNKNOWN,
                                                      .runtime = 0.0,
                                                      .count = 0,
						                                .freq = 0.0,
						                                .temp = 0.0};
        m_last_region_info = std::vector<struct m_region_info_s>(m_num_freq_ctl_domain, DEFAULT_REGION);
        m_target_freq.resize(m_num_freq_ctl_domain, m_freq_governor->get_frequency_max());
        std::vector<std::string> signal_names = {"REGION_HASH", "REGION_HINT", "REGION_RUNTIME", "REGION_COUNT", "FREQUENCY", "TEMPERATURE_CORE"};

        for (size_t sig_idx = 0; sig_idx < signal_names.size(); ++sig_idx) {
            m_signal_idx.push_back(std::vector<int>());
            for (int ctl_idx = 0; ctl_idx < m_num_freq_ctl_domain; ++ctl_idx) {
                m_signal_idx[sig_idx].push_back(m_platform_io.push_signal(signal_names[sig_idx],
                            m_freq_ctl_domain_type, ctl_idx));
            }
        }
    }
}
