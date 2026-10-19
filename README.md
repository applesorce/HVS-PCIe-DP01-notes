# HVS PCIe-DP01 (K7-160T-DP) board notes

HYVISION SYSTEM製のFPGAボード **HVS PCIe-DP01**(Kintex-7 **XC7K160T-FFG676-1** 搭載、基板リビジョン REV.C)の非公式資料集。
このリポジトリと関連リポジトリでは、このボードを「K7-160T-DP」と呼んでいる。
公式のSDKやドキュメントがほぼ無い基板について、実機で確認したピン配置・電圧・DDR3の動かし方をまとめている。

## 動作確認済みの内容

| 項目 | 状態 |
|---|---|
| JTAG認識・SRAM書き込み | ✅ openFPGALoader + FT232H |
| 50MHzクロック (D24)、LED D5 (C21) | ✅ |
| UART (TP3/TP4) | ✅ 115200bps / 1Mbps |
| **DDR3 512MB (32bit、2チップ)** | ✅ LiteX + LiteDRAM + Vivado、全域memtestでエラー0 |
| DDR3 (MIG 7 Series) | ❌ キャリブレーションが完了しない(設定ファイルと再現手順のみ同梱) |
| PCIe / DisplayPort | 未確認 |

## ディレクトリ構成

| パス | 内容 |
|---|---|
| [docs/board.md](docs/board.md) | ボード概要: 部品、電源、JTAG、LED、Flash、I/Oバンク |
| [docs/pinout.md](docs/pinout.md) | ピン配置: GPIO用TP 57点、DDR3 70信号、PCIe/GT |
| [docs/ddr3.md](docs/ddr3.md) | DDR3の動かし方、この基板固有の設定、切り分けの知見 |
| [docs/jtag.md](docs/jtag.md) | JTAGコネクタのピン順、秋月 AE-FT232HL + AE-LCNV4-MOSFET で書き込み器を作る配線表 |
| [docs/uart.md](docs/uart.md) | UARTの配線(TP3/TP4)、JTAGとUARTを同時に使う方法 |
| [docs/ubuntu_setup.md](docs/ubuntu_setup.md) | Ubuntu 24.04での環境構築(Vivado 2025.2、openFPGALoader、LiteX、openXC7)と検証したバージョン |
| [mig/](mig/) | Vivado MIG(DDR3)の設定ファイル(`.prj`)、Block Design、MicroBlazeのテストファーム |
| [examples/openxc7-led/](examples/openxc7-led/) | LED点滅の最小例(OSSツールチェーン) |

## 最初に知っておくべきこと

- **電源**: J4に12V(左が12V、右がGND)。
- **JTAGのVREFは1.8V系**(実測約1.9V)。3.3V専用の書き込み器を直結しないこと。コネクタはディップスイッチ側から TMS, TDI, TDO, TCK, GND, VREF の順([docs/jtag.md](docs/jtag.md))。
- **DDR3はDCIなしの `SSTL15`。** ベンダー資料にある `SSTL15_T_DCI` は誤りで、そのまま使うとキャリブレーションが全滅する。
- **Bank 32 (TP55–58) はHPバンク**で、最大1.8V。3.3Vの信号をつながないこと。
- **TP22 (D23)** はクロックピンD24の差動ペアの相方。汎用I/Oとしては使わないこと。

## GPIOとバンク電圧(調査結果の要約)

GPIO用のテストポイントは57点。TP番号とFPGAピンの対応表、基板上の位置図は [docs/pinout.md](docs/pinout.md) にある。

| バンク | 種別 | 電圧 | 該当TP |
|---|---|---|---|
| 12 | HR | VCCOは未測定。TP3/TP4をLVCMOS33のUARTとして使って動作している | TP3–8, TP38, TP41, TP42, TP45, TP46, TP49–54, TP60–66 |
| 13 | HR | **3.3V 実測**(TP10, TP13, TP33) | TP1, TP2, TP9–14, TP31–37, TP39, TP40, TP43, TP44 |
| 14 | HR | **3.3V 実測**(TP23, TP25, TP29) | TP21–30(TP22は使用禁止) |
| 32 | **HP** | 未測定。**最大1.8V** | TP55–58 |
| 33 / 34 | HP | 1.5V (SSTL15) | DDR3専用 |
| 0 | — | **1.8V系**(JTAG VREF 約1.9V 実測) | JTAG |

| 用途 | ピン | 状態 |
|---|---|---|
| クロック 50MHz | D24 (LVCMOS33) | 実機確認済み |
| LED D5 | C21 (LVCMOS33) | 実機確認済み。他の11個のLEDはGPIO用TPとは別の回路 |
| UART TX / RX | TP3 (V23) / TP4 (U22) | 実機確認済み |

ベンダーのLEDサンプルはクロックとLEDを `LVCMOS18` に設定しているが、Bank14の実測は3.3V。自分で作るデザインでは `LVCMOS33` を使う。

## ベンダー資料について

ボードに付属していたベンダー資料(ピン配置表、サンプルプロジェクト、ビットストリーム)は、考慮のすえ公開を差し控えた。
このリポジトリにあるのは、実機での測定・動作確認と、資料の照合で得た結果だけ。資料と実機で食い違った点(DDR3のIOSTANDARD、Bank14の電圧など)は、各ドキュメントに記載している。

## 関連リポジトリ

- [applesorce/k7-160t-dp-litex](https://github.com/applesorce/k7-160t-dp-litex): 512MB DDR3が動くLiteXのplatform/target(LiteDRAM + Vivado、BSD-2-Clause)
- [applesorce/K7-160t-dp-UberDDR3](https://github.com/applesorce/K7-160t-dp-UberDDR3): UberDDR3を単体で動かした例。512MB全域のBISTでエラー0(GPLv3のため別リポジトリ)

## ライセンス

[BSD-2-Clause](LICENSE)
