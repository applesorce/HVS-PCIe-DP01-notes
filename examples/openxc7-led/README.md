# LED点滅(openXC7)

最小構成の動作確認用。50MHzのクロック(D24)を数えて、約1秒ごとにLED D5(C21)を反転させる。

## ビルドと書き込み

1. [openXC7/demo-projects](https://github.com/openXC7/demo-projects) から `openXC7.mk` をこのディレクトリにコピーする。
2. 以下を実行する。

```sh
make            # yosys → nextpnr-xilinx → fasm2frames → xc7frames2bit
make program    # openFPGALoader で SRAM へ書き込み
```

初回は chipdb の生成に時間がかかる(出力先の `../chipdb/` は事前に `mkdir -p` が必要)。

## 注意

- IOSTANDARDは `LVCMOS33`。Bank14は3.3Vを実測している。ベンダーのサンプル(LVCMOS18)のままでは電圧が合わない。
- Vivadoでビルドする場合、D24はSRCCペアのN側なので `CLOCK_DEDICATED_ROUTE FALSE` が必要([docs/pinout.md](../../docs/pinout.md))。
