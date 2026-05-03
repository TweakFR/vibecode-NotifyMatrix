Import("env")

import os
import re

PROJECT_DIR = env["PROJECT_DIR"]
ENV_PATH = os.path.join(PROJECT_DIR, ".env")
EXAMPLE_PATH = os.path.join(PROJECT_DIR, ".env.example")
OUT_PATH = os.path.join(PROJECT_DIR, "include", "secrets_from_env.h")


def c_string_escape(s: str) -> str:
    out = []
    for ch in s:
        o = ord(ch)
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        elif 32 <= o <= 126:
            out.append(ch)
        else:
            out.append(f"\\{o:03o}")
    return "".join(out)


def parse_env_file(path: str) -> dict:
    out = {}
    if not os.path.isfile(path):
        return out
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$", line)
            if not m:
                continue
            key, val = m.group(1), m.group(2).strip()
            if len(val) >= 2 and val[0] == val[-1] and val[0] in "\"'":
                val = val[1:-1]
            out[key] = val
    return out


def main():
    data = parse_env_file(ENV_PATH)
    if not data:
        data = parse_env_file(EXAMPLE_PATH)
        if data:
            print("gen_secrets_header: .env absent, utilisation de .env.example")
        else:
            print("WARN gen_secrets_header: ni .env ni .env.example — macros vides")

    def get(key: str, default: str = "") -> str:
        return data.get(key, default)

    wifi_ssid = get("WIFI_SSID", "")
    wifi_pass = get("WIFI_PASSWORD", "")
    mqtt_host = get("MQTT_HOST", "127.0.0.1")
    mqtt_user = get("MQTT_USER", "")
    mqtt_pass = get("MQTT_PASSWORD", "")
    mqtt_topic = get("MQTT_TOPIC_NOTIFY", "notifymatrix/notify")
    idfm_key = get("IDFM_API_KEY", "")
    idfm_ref = get("IDFM_MONITORING_REF", "")
    idfm_line_ref = get("IDFM_LINE_REF", "")
    idfm_line_label = get("IDFM_LINE_LABEL", "2225")
    idfm_line_code = get("IDFM_LINE_CODE", "")

    try:
        mqtt_port = int(get("MQTT_PORT", "1883") or "1883")
    except ValueError:
        mqtt_port = 1883

    lines = [
        "#pragma once",
        "/* Fichier généré par scripts/gen_secrets_header.py — ne pas éditer */",
        "",
        f'#define WIFI_SSID "{c_string_escape(wifi_ssid)}"',
        f'#define WIFI_PASSWORD "{c_string_escape(wifi_pass)}"',
        "",
        f'#define MQTT_HOST "{c_string_escape(mqtt_host)}"',
        f"#define MQTT_PORT {mqtt_port}",
        f'#define MQTT_USER "{c_string_escape(mqtt_user)}"',
        f'#define MQTT_PASSWORD "{c_string_escape(mqtt_pass)}"',
        f'#define MQTT_TOPIC_NOTIFY "{c_string_escape(mqtt_topic)}"',
        "",
        f'#define IDFM_API_KEY "{c_string_escape(idfm_key)}"',
        f'#define IDFM_MONITORING_REF "{c_string_escape(idfm_ref)}"',
        f'#define IDFM_LINE_REF "{c_string_escape(idfm_line_ref)}"',
        f'#define IDFM_LINE_LABEL "{c_string_escape(idfm_line_label)}"',
        f'#define IDFM_LINE_CODE "{c_string_escape(idfm_line_code)}"',
        "",
    ]

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


main()
