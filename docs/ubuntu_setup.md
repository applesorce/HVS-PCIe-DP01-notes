# Ubuntuでの開発環境構築

## 検証した環境

| 項目 | バージョン | 入手方法 |
|---|---|---|
| OS | Ubuntu 24.04.4 LTS | — |
| Vivado | **2025.2**(ML Standard、無償) | AMDのWebインストーラ |
| MIG / MicroBlaze / Clocking Wizard | 4.2 / 11.0 / 6.0(Vivado 2025.2に同梱) | — |
| MicroBlazeのgcc・updatemem | Vivado 2025.2に同梱 | `/tools/Xilinx/2025.2/gnu/microblaze/lin/bin/mb-gcc` |
| openFPGALoader | 0.12.0 | apt |
| RISC-V gcc(LiteX用) | 13.2.0 | apt `gcc-riscv64-unknown-elf` |
| Python(LiteX用) | 3.12.3(venv) | — |
| openXC7(任意) | yosys 0.68 + nextpnr-xilinx | ソースからビルド |

### Vivadoのバージョンについて

- ベンダーのサンプルは古いVivadoで作られている。LED点滅のプロジェクトとビットストリームが **2023.2**、同梱のI/Oレポートが **2018.3**、DDRテスト用ビットストリームが **2017.4**。
- このリポジトリの `mig/` はすべて **2025.2** で作成・確認した。
  - `mig/vivado/top_bd.tcl` は2025.2で書き出したもの。別のバージョンで実行するとCRITICAL WARNINGが出て、IPのバージョン差で設定が変わる可能性がある。
  - `mig/config/*.prj` はMIG 4.2形式。
- XC7K160Tは **ML Standard(無償)** の対象デバイスなので、Enterprise版や有償ライセンスは要らない。

## 1. 基本パッケージ

```sh
sudo apt update
sudo apt install -y \
  build-essential gcc g++ make bash csh tcsh \
  tar gzip unzip xz-utils perl locales \
  libtinfo6 libncurses6 libncurses-dev libglib2.0-0t64 \
  libx11-6 libxext6 libxrender1 libxtst6 libxi6 libsm6 libice6 \
  libfontconfig1 libfreetype6 libgtk-3-0t64 zlib1g libtool xterm \
  libusb-1.0-0 usbutils fxload libftdi1-2
sudo locale-gen en_US.UTF-8
```

Ubuntu 24.04には `libtinfo5` が無いが、`libtinfo6` だけで問題は起きなかった。インストーラが途中で flex / bison / libfl2 / libfl-dev を追加で要求するので、それに従う。

## 2. Vivado 2025.2

インストール先を事前にユーザー所有で作っておくと、sudoなしでインストールできる。

```sh
sudo mkdir -p /tools/Xilinx && sudo chown -R $USER:$USER /tools/Xilinx
chmod +x FPGAs_AdaptiveSoCs_Unified_SDI_2025.2_1114_2157_Lin64.bin
./FPGAs_AdaptiveSoCs_Unified_SDI_2025.2_1114_2157_Lin64.bin    # GUIインストーラ(AMDアカウントが必要)
```

インストーラで選んだ項目:

| 項目 | 選択 |
|---|---|
| Edition | **Vivado ML Standard** |
| Design Tools | Vivado ✓、Vitis HLS ✓(任意)、Vitis Model Composer ✗、DocNav ✗ |
| Devices | **7 Series のみ** |
| Cable Drivers | インストールしない(書き込みにはopenFPGALoaderを使う) |
| Destination | `/tools/Xilinx` |

ダウンロード量は約17GB、ディスク使用量は約66GB。

```sh
source /tools/Xilinx/2025.2/Vivado/settings64.sh
vivado -version     # ライセンスエラーが出なければOK
```

## 3. openFPGALoader

```sh
sudo apt install -y openfpgaloader
sudo usermod -aG plugdev,dialout $USER   # 入れ直した後は再ログインが必要
```

aptパッケージにはudevルール(`/lib/udev/rules.d/99-openfpgaloader.rules`)が含まれていて、FTDIデバイスにplugdevグループからアクセスできるようになる。`dialout` は `/dev/ttyUSB*`(UART)を使うためのグループ。

接続と動作確認は [jtag.md](jtag.md)、UARTは [uart.md](uart.md) を参照。

## 4. LiteX(512MB DDR3構成を使う場合)

```sh
sudo apt install -y gcc-riscv64-unknown-elf
python3 -m venv ~/litex-venv && source ~/litex-venv/bin/activate
```

その後は [LiteXの公式手順](https://github.com/enjoy-digital/litex#quick-start-guide) に従って `litex_setup.py` でインストールし、[applesorce/k7-160t-dp-litex](https://github.com/applesorce/k7-160t-dp-litex) を使う。Ubuntu 24.04ではPEP668のため、システムのPythonに直接pipで入れられない。venvを使うこと。

## 5. openXC7(任意、OSSツールチェーン)

yosys + nextpnr-xilinx + prjxray。LED点滅と16bit DDR3までは動作した。32bit DDR3には使えない([ddr3.md](ddr3.md) を参照)。
[openXC7/toolchain-installer](https://github.com/openXC7/toolchain-installer) の `toolchain-sources-builder.sh` でビルドするときの注意点:

1. yosys 0.68はMakefileを廃止してCMakeに移行している。`build_yosys()` を `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc) && cmake --install build` に書き換える。
2. prjxrayの `pip3 install --user -r requirements.txt` がPEP668で拒否される。`--break-system-packages` を付ける。
3. prjxray-dbは、nextpnr-xilinxがサブモジュールとして参照しているコミット(`xilinx/external/prjxray-db`)に合わせる。インストーラ指定の古いタグのままだと、`OBUF_HP_BANK_GLUE not found` で失敗する。
4. 依存パッケージ: libboost-dev、libboost-{thread,program-options,iostreams,python,filesystem}-dev、libeigen3-dev、libantlr4-runtime-dev、default-jre-headless、flex、bison、cython3、python3-{intervaltree,openpyxl,ordered-set,parse,progressbar2,json5,pytest,pytest-runner,simplejson,yapf}。
5. fasmの高速パーサ(ANTLR)はランタイムのバージョンが合わずビルドに失敗するが、低速パーサ(textX)に自動で切り替わるので実用上は問題ない。
6. `pypy3` が無い場合は、Makefileで `PYPY3 = python3` を指定する。`openXC7.mk` はchipdbの出力先ディレクトリを自動では作らないので、事前に `mkdir -p` しておく。
7. `SSTL15_T_DCI` と `SLEW=FAST` の組み合わせはデータベースに無い(この基板はDCIを使わないので、SSTL15を指定すれば問題ない)。

LED点滅の最小例: [../examples/openxc7-led/](../examples/openxc7-led/)
