# UARTの接続

基板にはUART用のコネクタもシルク表記も無いので、空いているTPを使う。

| 基板 | USBシリアル(3.3V) |
|---|---|
| TP3 (V23) = TX | RXD |
| TP4 (U22) = RX | TXD |
| GND | GND |

TP3/TP4はBank 12にあり、LVCMOS33で動作した。速度はデザインによる(LiteX BIOSは既定で115200bps)。

## JTAGとUARTを同時に使う(FTDIを2台使う場合)

JTAGのFT232HはopenFPGALoaderがlibusb経由で使い、UARTのアダプタはカーネルの `ftdi_sio` ドライバで `/dev/ttyUSB*` として使う。

FT232Hを `ftdi_sio` から外すため、`ftdi_sio` を全体でblacklistしている環境(`/etc/modprobe.d/ftdi_sio.conf` に `blacklist ftdi_sio`)では、次の手順で両方を使えるようにする。

```sh
sudo modprobe ftdi_sio                         # UARTアダプタ用にドライバを読み込む
ls /sys/bus/usb/drivers/ftdi_sio/              # FT232H側のインターフェース名(例: 3-2:1.0)を確認
echo 3-2:1.0 | sudo tee /sys/bus/usb/drivers/ftdi_sio/unbind   # FT232HだけをJTAG用に切り離す
openFPGALoader -c ft232 --detect               # JTAGが生きていることを確認
```

**どちらが `/dev/ttyUSB0` / `ttyUSB1` になるかは、接続し直すたびに変わることがある。** テストの前に毎回確認すること。

```sh
for d in /dev/ttyUSB*; do echo "$d: $(udevadm info -q property -n $d | grep ID_MODEL_FROM_DATABASE)"; done
```

UARTが無音に見えるときは、まずポートを取り違えていないかを疑う(JTAG側のttyを見ていた、という失敗例がある)。
