#!/system/bin/sh
# pdtest_hal —— vendor HAL 面测试（picodroid 无 system_server：直连 servicemanager 注册面）。
# 覆盖：IHealth/IPowerStats/IThermal/IVibrator/IMemtrack/IKeyMintDevice(+secureclock)/
# system_suspend 系/INfc；GNSS 可能无，不在本表。
#
# 设备实测事实基线（改判据前先核对本注释）：
#   1. idlcli（frameworks/native/cmds/idlcli）只实现了 vibrator 一个 IDL：
#      `idlcli` 无参输出 Usage: "idlcli <idl> ... <idl>  vibrator"。
#      长名形式 `idlcli android.hardware.health.IHealth/default getHealthInfo` 恒报 Invalid Command。
#      vibrator 直调：`idlcli vibrator on 100` → "Status: No error"。
#   2. `dumpsys <service>`（调服务的 dump()）对 HAL 普遍挂起，禁止使用；
#      `dumpsys --pid <service>` 走 getDebugPid binder 事务，binder 线程池健康时立即返回数字
#      PID —— 作为“服务真实响应 binder 事务”的事务级证据。
#   3. `service list` 会对每个已注册服务调 getInterfaceDescriptor，picodroid 上卡在第 2 个
#      服务上整体挂死，禁止使用；逐个 `service check` 代替（只查 servicemanager 注册表）。
#   4. suspend 控制服务的注册名是裸名 `suspend_control` / `suspend_control_internal`（dumpsys -l
#      实测），不是 android.system.suspend.control.* 全名；另有 ISystemSuspend/default。
# 5. health HAL 主线程启动后死等 bpf.progs_loaded 属性（picodroid 无 bpfloader 产出，属性
#      恒空，唯一线程卡 futex，不响应任何 binder 事务）。解锁由平台侧完成：hal app 的
#      start.sh（picodroid 应用契约脚本）root 下 setprop bpf.progs_loaded 1 —— 旧进程退出、
#      init 拉起新进程进入 epoll 主循环并正常应答。
# 判据分级：
#   直调（vibrator）：idlcli 实际 API 调用成功（on 后复位 off）；
#   事务级（health/powerstats/thermal/memtrack/keymint/nfc）：service check 注册断言 +
#      dumpsys --pid 返回数字 PID（binder 事务被服务应答）；
#   suspend：注册（suspend_control + ISystemSuspend）+ 事务级 --pid + init.svc=running。
# 约定：外呼一律 timeout 包裹；失败不中断后续；每项单行 PASS/FAIL 证据；任一 FAIL 退出 1。
export PATH=/system/bin:/vendor/bin

TMO=6
PASS_CNT=0
FAIL_CNT=0
FAILED_LIST=""

emit_pass() { PASS_CNT=$((PASS_CNT + 1)); echo "PASS: $1 — $2"; }
emit_fail() { FAIL_CNT=$((FAIL_CNT + 1)); FAILED_LIST="$FAILED_LIST $1"; echo "FAIL: $1 — $2"; }

# 取首个非空行截断为单行证据
evidence() {
    _e="$(printf '%s\n' "$1" | grep -m1 -v '^[[:space:]]*$' | cut -c1-90)"
    [ -n "$_e" ] || _e="(no output)"
    printf '%s' "$_e"
}

# 注册断言：service check 输出 "Service <name>: found"（not found 不含 ": found"）
svc_found() {
    timeout "$TMO" service check "$1" 2>&1 | grep -q ': found'
}

# 事务级证据：dumpsys --pid 走 getDebugPid binder 事务；成功回显数字 PID，超时/挂起返回空
svc_pid() {
    _p="$(timeout "$TMO" dumpsys --pid "$1" 2>&1 | grep -oE '[0-9]+' | head -1)"
    [ -n "$_p" ] || return 1
    printf '%s' "$_p"
}

init_running() { [ "$(timeout 5 getprop "init.svc.$1" 2>/dev/null)" = "running" ]; }

# check_pid <tag> <svc_name> [init_svc] —— 注册 + 事务级双证据
check_pid() {
    _tag="$1"; _svc="$2"; _init="$3"
    if ! svc_found "$_svc"; then
        emit_fail "$_tag" "not registered: service check '$_svc' says not found"
        return 1
    fi
    _pid="$(svc_pid "$_svc")" || {
        emit_fail "$_tag" "registered but no binder answer: dumpsys --pid '$_svc' no PID (timeout/hang)"
        return 1
    }
    [ -z "$_init" ] || init_running "$_init" || {
        emit_fail "$_tag" "binder ok (pid=$_pid) but init.svc.$_init != running"
        return 1
    }
    emit_pass "$_tag" "registered + binder transaction answered, pid=$_pid${_init:+, init.svc.$_init=running}"
}

# 0) health 前置等待：bpf.progs_loaded 由 hal app 的 start.sh（picodroid 平台契约脚本）
#    置位解锁 health 主线程（见头部注释 5）；置位后服务会重启一次，这里只等注册恢复。
health_bootstrap() {
    _i=0
    while [ "$_i" -lt 8 ]; do
        svc_found android.hardware.health.IHealth/default && return 0
        sleep 1
        _i=$((_i + 1))
    done
    return 1
}

# 1) vibrator：idlcli 唯一实装 IDL，真调 on(100ms) 期待 "Status: No error"，成功后 off 复位
t_vibrator() {
    if ! command -v idlcli >/dev/null 2>&1; then
        emit_fail vibrator "idlcli not available (/system/bin/idlcli missing)"
        return 1
    fi
    _out="$(timeout "$TMO" idlcli vibrator on 100 2>&1)"
    _rc=$?
    # idlcli 成功回显固定为 "Status: No error"（失败为异常串/usage），故正向匹配成功标记
    if [ "$_rc" -eq 0 ] && printf '%s' "$_out" | grep -q 'Status: No error'; then
        timeout "$TMO" idlcli vibrator off >/dev/null 2>&1
        emit_pass vibrator "idlcli 'vibrator on 100': $(evidence "$_out")"
    else
        emit_fail vibrator "idlcli 'vibrator on 100' rc=$_rc: $(evidence "$_out")"
    fi
}

# 2) health：自举后注册 + 事务级证据
t_health() {
    health_bootstrap || emit_fail health "bootstrap failed: bpf.progs_loaded set but IHealth/default not re-registered"
    check_pid health android.hardware.health.IHealth/default vendor.health-cuttlefish
}

# 3) powerstats
t_powerstats() {
    check_pid powerstats android.hardware.power.stats.IPowerStats/default vendor.power.stats-default
}

# 4) thermal：注册 + 事务级（/sys/class/thermal 仅作附注，不参与判定）
t_thermal() {
    check_pid thermal android.hardware.thermal.IThermal/default vendor.thermal-example
}

# 5) memtrack
t_memtrack() {
    check_pid memtrack android.hardware.memtrack.IMemtrack/default vendor.memtrack-default
}

# 6) keymint：主实例 + 同进程兄弟接口（secureclock）双注册佐证
t_keymint() {
    check_pid keymint android.hardware.security.keymint.IKeyMintDevice/default vendor.keymint-default
    _sib="$(timeout "$TMO" service check android.hardware.security.secureclock.ISecureClock/default 2>&1)"
    if printf '%s' "$_sib" | grep -q ': found'; then
        echo "     keymint sibling ISecureClock/default also registered (same provider)"
    fi
}

# 7) suspend：裸名注册（见头部注释 4）+ ISystemSuspend + init.svc
t_suspend() {
    _pid="$(svc_pid suspend_control)"
    if [ -z "$_pid" ]; then
        emit_fail suspend "suspend_control not answering getDebugPid"
        return 1
    fi
    if ! svc_found android.system.suspend.ISystemSuspend/default; then
        emit_fail suspend "suspend_control pid=$_pid but ISystemSuspend/default not registered"
        return 1
    fi
    if ! init_running system_suspend; then
        emit_fail suspend "services up (pid=$_pid) but init.svc.system_suspend != running"
        return 1
    fi
    emit_pass suspend "suspend_control(pid=$_pid) + ISystemSuspend/default registered + wakelock API host running"
}

# 8) nfc：注册 + 事务级 + init.svc（INfc 方法多要 callback 入参，idlcli/无参工具不可用，不做直调）
t_nfc() {
    check_pid nfc android.hardware.nfc.INfc/default nfc_hal_service
}

echo "==== pdtest_hal: vendor HAL 面测试（直调 + servicemanager 注册/事务证据） ===="
echo "tools: idlcli=$(command -v idlcli || echo MISSING) dumpsys=$(command -v dumpsys || echo MISSING) service=$(command -v service || echo MISSING) timeout=$(command -v timeout || echo MISSING)"

t_health
t_powerstats
t_thermal
t_vibrator
t_memtrack
t_keymint
t_suspend
t_nfc

echo "==== SUMMARY: pass=$PASS_CNT fail=$FAIL_CNT ===="
[ -n "$FAILED_LIST" ] && echo "failed:$FAILED_LIST"
[ "$FAIL_CNT" -eq 0 ]
