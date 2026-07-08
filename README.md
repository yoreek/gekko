# ESP32 WiFi Manager

ESP32 firmware (PlatformIO, C++) + web portal (Vue SPA in `portal-spa/`), served by the firmware itself from LittleFS.

## One-time setup after cloning

```sh
git config core.hooksPath .githooks
```

This enables the local pre-commit hook (`.githooks/pre-commit`): before every commit it runs the tests, builds the SPA (`data/`) and the firmware (`webflash/*.bin`), and adds the result to the same commit. If tests or the build fail, the commit is aborted. Without this command the hook file exists in the repository, but git never runs it.

GitHub Actions CI duplicates the test run on the server for every push/PR — a safety net in case the hook isn't activated or a commit was made with `--no-verify`.

## Development commands

See `CLAUDE.md` for firmware build, test, lint, and `portal-spa/` commands.
