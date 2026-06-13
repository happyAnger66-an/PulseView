std_msgs/Header header # for timestamp

string hostname
string vin

float32 cpu_used_percent
float32 mem_free_size
float32 mem_total_size
float32 mem_used_percent

CpuStat[] cpu_stats
CpuIrqStat[] cpu_irq_stats
SoftIrqStat[] softirq_stats
FileSystemStat[] filesystem_stats
NodePubStat[] node_pub_stats
NodeSubStat[] node_sub_stats
ProcStat[] proc_stats
NetStat[] net_stats
GpuStat[] gpu_stats
SnmpTcpStat snmp_tcp_stat
SnmpUdpStat snmp_udp_stat
SnmpIpStat snmp_ip_stat
MemDetailStat mem_detail_stat