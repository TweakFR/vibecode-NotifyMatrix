# Chaque `pio run -t upload` : insère `write_flash --erase-all` pour effacer toute la
# flash puis réécrire bootloader, tables de partitions, TinyUF2 et firmware d’un coup.
# (Uniquement la cible `upload`, pas `uploadfs` : erase-all + une seule image FS casserait la carte.)

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
    print("NotifyMatrix: flash complète (write_flash --erase-all + toutes les images).")


env.AddPreAction("upload", inject_erase_all)
