# M5 Tools

## adk-bindgen

`adk-bindgen` is a Rust tool that auto-generates the FFI glue code required for the engine to communicate with an app.

* The tool exports only symbols that are annotated in the C code.
* Some symbols can be automatically exported in the desired way by the tool. For those, some (or all) of the glue code still has to be written manually.

### Usage

To update bindings, run the following command for the relevant `ffi.toml`:

```shell
cargo run -p adk-bindgen update --config <path-to-crate>/ffi.toml
```

To generate the M5 bindings:

```shell
cargo run -p adk-bindgen update --config ./ffi.toml
```

To generate the NVE bindings:

```shell
cargo run -p adk-bindgen update --config ./source/adk/nve/ffi.toml
```

### Prerequisites

See [the global README](/README.md#getting-started).

## historian

The changelog is generated using the `historian` tool, which reviews the Git history of the ncp-m5 repository and sorts the commits into a presentable view.

The `historian` tool can be used as follows:

```shell
cargo run -p historian -- release/0.19.3+nve.1.2.1..release/1.0.0-rc.1 --overrides docs/internal/changelog/overrides.toml --external
```

* `historian` uses the local Git repository's `origin` references. This means that the local clone must be up-to-date in order to achieve the best results.
* The range provided to historian operates on remote/origin ref's - commit hashes and other identifiers are not supported at this time.
* An `overrides` file is used to override the metadata for a particular commit in the case of amending it.
* The `--external` flag is used to configure the `historian` renderer for generating a format meant for a 'partner' audience.
* For more information, see the `--help` dialog.

## shader compiler

In order to generate compiled shaders for all of the platforms of M5, the shader compiler can be used e.g.:

```shell
cargo run -p shader_compiler
```

## font-sample-renderer

The font sample renderer tool generates screenshots of text rendered via M5 with the provided font files and text.

In order to generate screenshots from multiple fonts, the `directory` command can be used:

```shell
cargo run -p font_sample_renderer -- directory --directory etc/fonts/Tazugane --text "こんにちは"
```

For finer granularity, the `csv` command can be used to generate screenshot from the fonts in the provided directory, using CSV for the text input:

```shell
cargo run -p font_sample_renderer -- csv --directory etc/fonts/Tazugane --csv etc/font-rendering-validation.csv
```

## m5_log_parser

The log parser tool parses log files generated while running [run-all.sh](/scripts/run-all.sh) and generates a csv file containing the information needed for the release metrics spreadsheet.

The log parser excepts 3 optional arguments:

1. (`--skip_frames`) The number of frames to skip when parsing logs for FPS (default = 3)
2. (`--directory`) The directory containing the log files (default = build/m5_logs)
3. (`--output`) The output files (default = build/log.csv)

To run using the default arguments, the following command can be used:

```shell
cargo run -p m5_log_parsing
```

## Notes

* For tools that are a part of the `cargo` workspace, running the tool is expected to occur from the root directory of the repository.
