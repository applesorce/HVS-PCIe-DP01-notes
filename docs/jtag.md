# JTAGの接続

## 基板側のJTAGコネクタ

基板上の6ピンJTAGコネクタ。**ディップスイッチ側から順番に**次のとおり。

| 順番(ディップスイッチ側から) | 信号 |
|---|---|
| 1 | TMS |
| 2 | TDI |
| 3 | TDO |
| 4 | TCK |
| 5 | GND |
| 6 | VREF |

- **VREF-GND間の実測は約1.9V。** JTAGはFPGAのBank 0 (VCCO_0) で動くので、**1.8V系**として扱う。
- VREFは基板側から出てくる電圧(書き込み器が信号レベルを判定する基準)。外部から電圧をかけないこと。
- **3.3Vの書き込み器を直結しないこと。** レベル変換が必要。

## 秋月の部品だけで作るJTAG書き込み器

```
PC ──USB── AE-FT232HL ──3.3V── AE-LCNV4-MOSFET ──1.8V── 基板のJTAGコネクタ
           (MPSSE)            HV側        LV側
```

| 部品 | 秋月の型番 | 通販コード |
|---|---|---|
| FT232HL ハイスピードUSBシリアル変換モジュール | AE-FT232HL | [106503](https://akizukidenshi.com/catalog/g/g106503/) |
| 4ビット双方向ロジックレベル変換モジュール(BSS138使用) | AE-LCNV4-MOSFET | [113837](https://akizukidenshi.com/catalog/g/g113837/) |

### AE-FT232HL の設定

- ジャンパ **JP3** と **JP4** はどちらもショート(出荷時の状態)にする。
  - JP3: USBバスから5V0端子に5Vを供給する。
  - JP4: 3.3V出力をVCCIOに供給し、I/Oを3.3Vにする。
- 電源はUSBバスパワーだけでよい。
- FT232HのMPSSEでは、JTAGのピンが次のように固定されている: AD0 = TCK、AD1 = TDI、AD2 = TDO、AD3 = TMS。

### 配線表

| 基板JTAG(ディップスイッチ側から) | 信号 | レベル変換 LV側 | レベル変換 HV側 | AE-FT232HL |
|---|---|---|---|---|
| 1 | TMS | LV4(7番ピン) | HV4(6番ピン) | J2-10 **AD3** |
| 2 | TDI | LV2(11番ピン) | HV2(2番ピン) | J2-8 **AD1** |
| 3 | TDO | LV3(8番ピン) | HV3(5番ピン) | J2-9 **AD2** |
| 4 | TCK | LV1(12番ピン) | HV1(1番ピン) | J2-7 **AD0** |
| 5 | GND | GND(10番ピン) | GND(3番ピン) | J2-6 **GND** |
| 6 | VREF | **LV**(9番ピン) | **HV**(4番ピン) | J2-3 **3V3** |

- レベル変換モジュールの電源ピンは、LVを基板のVREF(約1.9V)に、HVをFT232HLの3V3につなぐ。
- 4つのチャネル(1〜4)はどれも同じ回路なので、どの信号をどのチャネルに割り当ててもよい(上の表は一例)。大事なのは、FT232HL側の **AD0〜AD3と信号の対応** と、**HVとLVの向き**。
- FT232HLの3V3を基板のVREFにつながないこと。

### 注意: レベル変換モジュールの電圧範囲

AE-LCNV4-MOSFETの説明書では、**LV電圧の最小値は2.5V**(絶対最大定格の表による)。この基板のJTAG電圧(約1.9V)はその範囲を下回るので、**仕様範囲外の使い方になる**。

この基板では、上の配線でJTAGの認識とSRAMへの書き込みが問題なくできた。それでも、動作が不安定な場合はTCKの周波数を下げてみるか、1.8Vに対応したレベル変換ICを使うこと。

## 付属のJTAG変換基板を使う場合

付属の変換基板の6極コネクタは、ケーブルが写真の左側に出る向きで上から VREF、GND、TCK、TDO、TDI、TMS(並びは基板側コネクタの逆順)。付属ケーブルの線色:

| 上から | 線色 | 信号 |
|---|---|---|
| 1 | 赤 | VREF |
| 2 | 黒 | GND |
| 3 | 黄 | TCK |
| 4 | 緑 | TDO |
| 5 | 青 | TDI |
| 6 | 白 | TMS |

線色は目視で対応を取ったもので、導通は確認していない。上の配線表の「基板JTAG」の列を、この変換基板の同じ信号に置き換えてつなげばよい。

付属のXilinx互換ケーブル(USB ID `03fd:000f`)は、ファームウェアに互換性がなく、openFPGALoaderでもxc3sprogでも通信できなかった。

## ソフトウェア

openFPGALoader(Ubuntuでは `sudo apt install openfpgaloader`)を使う。導入は [ubuntu_setup.md](ubuntu_setup.md) を参照。

認識の確認:

```sh
openFPGALoader -c ft232 --detect
```

正しくつながっていれば、idcode `0x364c093`、manufacturer xilinx、family kintex7、model xc7k160t、irlength 6 と認識される。

SRAMへの書き込み(電源を切ると消える):

```sh
openFPGALoader -c ft232 --fpga-part xc7k160tffg676 -m design.bit
```

Flashのバックアップ(spiOverJtagブリッジが自動で選ばれる。8MB、N25Q64):

```sh
openFPGALoader -c ft232 --fpga-part xc7k160tffg676 --dump-flash --file-size 0x800000 backup.bin
```

openFPGALoaderにはこのボードのプロファイルが無いので、`--fpga-part xc7k160tffg676` を明示する。
SPI Flashへの書き込み(`-f`)は、まだ試していない。

UARTを同時に使う場合(FTDIを2台使う場合)は [uart.md](uart.md) を参照。
