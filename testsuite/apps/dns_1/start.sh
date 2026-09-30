#!/system/bin/sh
# picodroid app: dns v1 —— 原生 DNS 状态测试（见 testsuite/）
export PATH=/system/bin:/vendor/bin
export PD_OUT=/userdata/boot/tests/dns
export PD_LOGDIR=/userdata/boot/logs
mkdir -p "$PD_OUT" 2>/dev/null || true
# 平台契约（非测试逻辑）：picodroid 无 netd/DHCP 客户端，eth1（宿主 cvd-ebr 桥，
# dnsmasq 网关 192.168.98.1，NAT 出外网）不会自动取到地址。在此幂等拉起：
# 已有 IPv4 则不动；否则静态配置同网段地址 + 默认路由。测试断言的是
# netd/dnsresolver 缺席下的 DNS/直连行为，不是地址获取方式。
if ! ip addr show eth1 2>/dev/null | grep -q "inet "; then
    ip link set eth1 up
    ip addr add 192.168.98.50/24 dev eth1 2>/dev/null
    ip route add default via 192.168.98.1 dev eth1 2>/dev/null
fi
# 测试二进制随契约携带（bin/<uname -m>/），不进镜像（用户裁定 2026-09-30）
exec "$(dirname "$0")/bin/$(uname -m)/pdtest_dns"
