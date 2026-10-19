# DDR3 (512MB) の動かし方と知見

## 結論

| 構成 | ツール | DDR3クロック | 容量 | 実機結果 |
|---|---|---|---|---|
| **LiteX + LiteDRAM (`s7ddrphy`)** | **Vivado** | 280MHz (sys 70MHz) | **512MB** | `mem_test 0x40000000 0x20000000`(全域)で bus 0 / addr 0 / data 0 → [applesorce/k7-160t-dp-litex](https://github.com/applesorce/k7-160t-dp-litex) |
| UberDDR3 (単体BIST) | Vivado | 400MHz | 512MB | 全域BISTでエラー0 → [applesorce/K7-160t-dp-UberDDR3](https://github.com/applesorce/K7-160t-dp-UberDDR3)(GPLv3、別リポジトリ) |
| UberDDR3 (LiteX SoCに組み込み) | Vivado | 285.71MHz | 512MB | 全域 `mem_test` でエラー0。400MHzでは読み出しマージンが足りずエラーが出た |
| LiteX + LiteDRAM | openXC7 (yosys + nextpnr-xilinx) | 320MHz (sys 80MHz) | 256MB (16bit) | Memtest OK(`--ddram-16bit`、nextpnrのseedは実機で選ぶ必要あり) |
| LiteX + LiteDRAM 32bit | openXC7 | 280MHz | — | 上位16bit (dq[31:16]) が常に `0xFFFF` → 使えない |
| MIG 7 Series | Vivado | 400MHz | — | `init_calib_complete` が立たない → [../mig/](../mig/) |

## この基板固有の設定(必須)

出荷時のDDRテスト用ビットストリームを prjxray の `bit2fasm.py` でFASMに逆変換し、DDR3の70ピンすべての設定を機械的に照合して確定した。

- **DQ / DQS / DM: `SSTL15` / `DIFF_SSTL15`、DCIなし**(`DCITERMDISABLE` が設定されている)。ベンダー資料にある `SSTL15_T_DCI` は誤り。
- **reset_n: `SSTL15`**(資料の `LVCMOS15` は誤り)。
- 全ピン `SLEW FAST`。
- 出荷時ビットストリームにはINTERNAL_VREFの設定が無い。ただし、LiteDRAM構成は `INTERNAL_VREF 0.750` を設定した状態で動作している。

IOSTANDARDをDCI付きのままにすると、LiteDRAMのキャリブレーションはwrite leveling/read levelingとも窓をまったく検出できず、全滅する。DCIなしに直しただけで、bus/addr errorsが0になった。

> 手法メモ: 動作実績のある既存の `.bit` があれば、`bit2fasm.py`(prjxrayに含まれる)でIOSTANDARD・DCI・VREFの設定を実データとして読み出せる。資料の転記ミスを検証するのに有効。

## 周波数

- **データシートの値をそのまま信用せず、実機でスイープして決めること。** LiteDRAM+Vivadoでは70MHz(DDR3 280MHz)が最も安定した。80MHz(320MHz、DLL-onの規格内)と100MHzはどちらも悪化した(100MHzではdata errorsが約77%)。
- 280MHzはDLL-onモードのJEDEC下限(約303MHz)を下回るが、この基板・この構成では最良だった。
- Vivadoの配置は毎回同じ結果になるわけではないので、ビルドによっては疑似乱数パターンで1%未満のdata errorsが残ることがある。bus/addr errorsは毎回0だった。
- LiteX BIOSの起動時memtestは先頭2MiBしか検査しない。**判定は必ず全域で行う**: `mem_test 0x40000000 0x20000000`

## 切り分けのコツ

- **統計値だけで判断しない。** 「data errors 23%」の正体は、上位16bitが常に `0xFFFF` になる系統的な故障だった。
  - L2キャッシュ(8KiB)に当たって正常に見えることがあるので、**32KiB以上書いてから先頭を読み戻す**。
  - **MPRモード**(`sdram_mr_write 3 0x4`)では、DRAMアレイを経由せず固定パターン 0,1,0,1 が返る。これで上位/下位16bitを別々に検査できる。
- **固定パターンとLFSRパターンで比べる。** 同じ領域を書き直さずに何度も読み、固定パターンでは安定するのにLFSRパターンでは毎回CRCが変わるなら、読み出し側のタイミングマージン不足と判断できる(論理的なバグではない)。

## openXC7(OSSツールチェーン)の制約

- nextpnr-xilinxは外部I/Oのタイミングを解析しない。fmaxの合否と実機での動作が一致しないことがある(fmaxがPASSしたseedで実機がハングした例あり)。**seedは実機のmemtestで選ぶこと。**
- 32bit構成では上位16bit(バンク34のHCLK行より上のバイトレーン)の読み出しが壊れる。同じRTLをVivadoで配置配線すると症状は出ないので、prjxray-dbにデータが欠けている(IOI_CLKのマルチプレクサなど)可能性が高い。原因は確定していない。
- `INTERNAL_VREF` / `CFGBVS` / `CONFIG_VOLTAGE` は無視される。
- DQS→BUFIOでキャプチャする方式(someone755/ddr3-controller型)は使えない。IDELAY→BUFIOの経路はMRCC/SRCCピンにしか存在せず、Bank34でBUFIOに入れられるのは AC4 / AA4 / AA3 / AB2 だけ。この基板ではこれらがすべてDQに使われており、DQSピンはMRCC/SRCCではない。PHASER経由の経路も、prjxray-dbにビットデータが無いため使えない。IDELAY/ODELAYのタップ校正方式(LiteDRAM、UberDDR3)であればこの制約は受けない。

## 参考: DLL-offモード

LiteDRAMにDLL-offモード(CL/CWL = 6/6、DDR3クロック ≤125MHz)を実装して試したが、キャリブレーションで有効なタップが一つも見つからず、addr/data errorsとも100%だった。この基板では採用しない。
