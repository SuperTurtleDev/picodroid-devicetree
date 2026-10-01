#!/system/bin/sh
# picodroid-usb —— 真机 USB adb（2026-10-01：真机无 USB gadget 配置则 adbd 不可达，
# 已实证"还是没有adb"）。标准 configfs gadget 流程（adbd 原生支持 f_adb function）：
# 挂 configfs → 建 gadget + adb function → 绑 UDC。幂等。
C=/config/usb_gadget/g1

mountpoint -q /config 2>/dev/null || mount -t configfs none /config 2>/dev/null
[ -d /config/usb_gadget ] || { echo "picodroid-usb: no configfs usb_gadget" > /dev/kmsg; exit 1; }

if [ ! -d "$C" ]; then
    mkdir -p "$C" || exit 1
    echo 0x18d1 > "$C/idVendor"      # Google（GSI 惯例）
    echo 0x4ee9 > "$C/idProduct"
    echo 0x0100 > "$C/bcdDevice"
    mkdir -p "$C/strings/0x409" "$C/configs/b.1/strings/0x409"
    echo picodroid > "$C/strings/0x409/manufacturer"
    echo picodroid > "$C/strings/0x409/product"
    echo adb > "$C/configs/b.1/strings/0x409/configuration"
    mkdir -p "$C/functions/adb"
    ln -sf "$C/functions/adb" "$C/configs/b.1/f1"
fi

# 已绑定的 UDC 不重复绑（重入保护）
CUR=$(cat "$C/UDC" 2>/dev/null)
[ -n "$CUR" ] && { echo "picodroid-usb: already on $CUR" > /dev/kmsg; exit 0; }
UDC=$(ls /sys/class/udc/ 2>/dev/null | head -1)
[ -n "$UDC" ] || { echo "picodroid-usb: no UDC" > /dev/kmsg; exit 1; }
echo "$UDC" > "$C/UDC" || { echo "picodroid-usb: bind $UDC failed" > /dev/kmsg; exit 1; }
echo "picodroid-usb: gadget up on $UDC" > /dev/kmsg
