# For `pio run -t upload`, insert `write_flash --erase-all` so the upload clears
# flash and rewrites the bootloader, partition tables, TinyUF2, and firmware.
# Only hook the `upload` target: erase-all plus a single FS image would break the board.

Import("env")


def inject_erase_all(target, source, env):
    flags = env.get("UPLOADERFLAGS")
    if not flags:
        return
    flags = list(flags)
    if "write_flash" not in flags:
        return
    if "--erase-all" in flags:
        return
    i = flags.index("write_flash")
    flags.insert(i + 1, "--erase-all")
    env.Replace(UPLOADERFLAGS=flags)
    print("NotifyMatrix: full flash erase enabled (write_flash --erase-all + all images).")


env.AddPreAction("upload", inject_erase_all)
