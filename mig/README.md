# MIG 7 Series (Vivado純正DDR3 IP) 構成一式

> **状態: 実機未動作。** MicroBlaze+MIG、LiteX+MIGのどちらの構成でも、実機で `init_calib_complete` が最後まで立たなかった。
> このボードで512MBのDDR3を動かすなら、動作確認済みの [LiteDRAM構成](https://github.com/applesorce/k7-160t-dp-litex)を使うこと。
> ここにあるのは、MIGで再挑戦する人向けの設定ファイルと再現手順、および判明している問題点。

## 内容

| パス | 内容 |
|---|---|
| `config/mig_a_x16_gui_verified.prj` | **MIGのメモリ設定ファイル(最終版)**。最終プロジェクトのMIGセルが参照していたもの |
| `config/mig_a_nodci.prj` | 上記の比較用。DCIを無効化し、ベース部品をx8品(MT41J128M8XX-125)にした版 |
| `config/mig_pinout.xdc` | MIGウィザードの「Read XDC/UCF」で読み込むピン定義。DDR3の70信号すべてを収録(DCIなしのSSTL15) |
| `vivado/top_bd.tcl` | 最終状態のBlock Designを `write_bd_tcl` で書き出したもの。`mig_a_x16_gui_verified.prj` の中身を埋め込み済み |
| `vivado/create_project.tcl` | `top_bd.tcl` からプロジェクトを新規作成する |
| `vivado/build.tcl` | 合成・配置配線・ビットストリーム生成と、`.mmi` の書き出し |
| `vivado/constraints.xdc` | DDR3以外のトップレベル制約(クロック、UART、LED) |
| `fw/` | 外部BSPを使わないMicroBlaze用テストファーム。キャリブレーションの完了を待ってから16KBのメモリテストを行う |

## 再現手順

Vivado 2025.2(ML Standard、無償)で確認した。

```sh
cd mig/vivado
vivado -mode batch -source create_project.tcl   # ./build にプロジェクトを作成
vivado -mode batch -source build.tcl            # bit と mmi を生成

cd ../fw
make          # test.elf
make merge    # build/.../impl_1/top_wrapper_fw.bit にファームを埋め込む
```

この手順で、合成・配置配線・ビットストリーム生成・ファームの埋め込みまで最後まで通ることを確認した。タイミングは WNS +0.942ns / WHS +0.042ns で、すべての制約を満たしている。
ビルド中に出る次の2種類のCRITICAL WARNINGは想定どおりで、問題ない。

- `BD::TCL 2040`: Vivado 2025.2以外で実行したときのバージョン警告。
- `Netlist 29-160`(6件): MIGが生成するXDCが `sys_clk_i` / `clk_ref_i` を外部ピン(AA10/AB11)に割り当てようとするが、この設計ではClocking Wizardから内部で供給しているので、その制約が適用されない。元のプロジェクトでも同じ6件が出ていた。

`top_wrapper_fw.bit` をJTAGでSRAMに書き込み、UART(TP3/TP4、115200bps)で出力を見る。

```sh
openFPGALoader -c ft232 --fpga-part xc7k160tffg676 -m top_wrapper_fw.bit
```

## 設計の構成

```
D24 50MHz ─▶ Clocking Wizard ─┬─ 400MHz ─▶ MIG sys_clk_i (Single-Ended)
                              ├─ 200MHz ─▶ MIG clk_ref_i (Single-Ended, IDELAYCTRL)
                              └─ locked ─▶ MIG sys_rst (ACTIVE LOW なので直結)
MicroBlaze ─ AXI ─┬─ AXI UART Lite   0x40600000  (TP3=V23 TX / TP4=U22 RX)
                  ├─ AXI GPIO calib  0x40000000  (init_calib_complete 入力)
                  ├─ AXI GPIO led    0x40010000  (C21 = LED D5、ハートビート)
                  └─ MIG S_AXI       0x80000000  (512MB)
```

## MIG設定値

| 項目 | 値 |
|---|---|
| FPGA | xc7k160tffg676-1 |
| メモリ | DDR3、Samsung K4B2G1646F-BCK0 ×2 (x16 ×2 = 32bit)。Custom Part `K4B2G1646Fx16`、ベースは MT41J128M16XX-125 |
| メモリクロック | 400MHz (2500ps)、PHY比 4:1 |
| データ幅 | 32bit、ECCなし、Data Maskあり |
| アドレス | Row 14bit / Column 10bit / Bank 3bit |
| CL / CWL | 6 / 5 |
| tRCD / tRP / tRAS | 15ns / 15ns / 37.5ns |
| tRFC / tFAW / tRRD / tREFI | 104ns / 50ns / 10ns / 7.8125µs |
| System Clock / Reference Clock | どちらも Single-Ended |
| System Reset Polarity | ACTIVE LOW |
| AXI | 有効 |

ピン割り当ては、ウィザードに自動選択させずに既存の配線どおり入力する(`config/mig_pinout.xdc`、または [docs/pinout.md](../docs/pinout.md#ddr3) を参照)。

## このボード固有の注意点

出荷時のDDRテスト用ビットストリームを `bit2fasm` で逆解析した結果、ベンダー資料とは異なる次の設定が実機の正解と分かっている。詳細は [docs/ddr3.md](../docs/ddr3.md)。

- DQ/DQSは `SSTL15` / `DIFF_SSTL15`(**DCIなし**)。MIGの既定値 `SSTL15_T_DCI` ではない。
- `reset_n` は `SSTL15`(MIGの既定値 `LVCMOS15` ではない)。
- 出荷時ビットストリームにはINTERNAL_VREFの設定が無い(外部VREFと推定)。ただし、動作確認済みのLiteDRAM構成は `INTERNAL_VREF 0.750` を設定したまま512MBが動いている。MIGの生成XDCも同じく0.750を出力する(`.prj` の `InternalVref=1`)。

## 判明している問題点(未解決)

1. **`init_calib_complete` が立たない。** リセット極性のバグを2つ(MicroBlazeの `ext_reset_in`、MIGの `sys_rst`)修正した後も解決しなかった。最終構成(このフォルダの内容)で実機を試した結果の記録は残っていない。
2. **x16の設定がRTLに反映されない。** `.prj` ではベース部品を x16品(MT41J128M16XX-125)にしているのに、生成されるRTLの `DRAM_WIDTH` は `8` のまま。既存セルを再カスタマイズしても反映されないため、セルを削除して作り直しているが、それでも同じだった。
3. **DCIの指定が `.prj` と生成XDCで食い違う。** 最終版の `.prj` はピンごとに `SSTL15_T_DCI`、`dci_inouts_inputs=1` と記録しているが、生成XDCのIOSTANDARDは `SSTL15`(DCIなし)になっている。GUIの表示、`.prj`、生成物の三者が一致しないことがあるので、**生成されたRTLとXDCを必ずファイルで確認すること**。
4. `.prj` の `TDQS enable` が `Enabled` になっている(x16品にTDQSは無い)。
