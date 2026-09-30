#!/system/bin/sh
# pdtest_dns —— 原生 DNS 状态验证（netd 已删：dnsresolver 服务预期不在，域名解析预期缺席）。
# 判据：
#   1) dnsresolver 服务计数：service list | grep -c dnsresolver，预期 0（仅记录，非判分项）
#   2) 域名解析：getent hosts android.com（getent 不可用则 ping 域名兜底）；
#      成功 PASS；失败输出 DEGRADED: dns via netd absent (expected) —— 不算 FAIL
#   3) IP 直连：ping -c1 -W3 8.8.8.8（回退 1.1.1.1）；不通才 FAIL
# 退出码：仅 IP 直连失败为 1；域名 DEGRADED（及全部通过）恒为 0。
export PATH=/system/bin:/vendor/bin

TMO=10

echo "==== pdtest_dns: DNS 状态验证（netd/dnsresolver absent） ===="

# 1) dnsresolver 服务计数（预期 0）
if command -v service >/dev/null 2>&1; then
    _n="$(timeout "$TMO" service list 2>/dev/null | grep -c dnsresolver)"
    echo "INFO: dnsresolver services: $_n (expected 0)"
    [ "$_n" = "0" ] || echo "WARN: dnsresolver present (count=$_n) — netd 删除假设被打破？"
else
    echo "INFO: 'service' not available, dnsresolver count unknown"
fi

# 2) 域名解析（如实记录：成功 PASS / 失败 DEGRADED，不判 FAIL）
_res=""
if command -v getent >/dev/null 2>&1; then
    _g="$(timeout "$TMO" getent hosts android.com 2>&1)"; _rc=$?
    if [ "$_rc" -eq 0 ] && [ -n "$(printf '%s' "$_g" | tr -d '[:space:]')" ]; then
        _res="$(printf '%s\n' "$_g" | head -1 | cut -c1-90)"
    fi
fi
if [ -z "$_res" ]; then
    # getent 不可用/失败 → ping 域名兜底（域名能解析才可能通）
    _p="$(timeout 15 ping -c1 -W3 android.com 2>&1)"
    if printf '%s' "$_p" | grep -q ' time='; then
        _res="ping android.com ok"
    fi
fi
if [ -n "$_res" ]; then
    echo "PASS: dns_resolve — android.com: $_res"
else
    echo "DEGRADED: dns via netd absent (expected)"
fi

# 3) IP 直连（判分项）
_ip=""
for _h in 8.8.8.8 1.1.1.1; do
    _p="$(timeout 15 ping -c1 -W3 "$_h" 2>&1)"; _rc=$?
    if [ "$_rc" -eq 0 ] && printf '%s' "$_p" | grep -q ' time='; then
        _ip="$(printf '%s\n' "$_p" | grep -m1 ' time=' | cut -c1-90)"
        break
    fi
done
_rc=0
if [ -n "$_ip" ]; then
    echo "PASS: ip_direct — $_ip"
else
    echo "FAIL: ip_direct — no reply from 8.8.8.8/1.1.1.1 (ping -c1 -W3)"
    _rc=1
fi

if [ -n "$_res" ]; then _d="resolve-ok"; else _d="degraded"; fi
if [ -n "$_ip" ]; then _i="ok"; else _i="fail"; fi
echo "==== SUMMARY: dns=$_d ip=$_i ===="
exit "$_rc"
