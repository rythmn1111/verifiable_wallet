"""
PlatformIO pre script: ensure build uses project sdkconfig.defaults (TLS + cert bundle).
If build sdkconfig exists but lacks our options, remove it so CMake recreates from
project sdkconfig.defaults. Do once: pio run -t fullclean && pio run
"""
import os

Import("env")

build_dir = env.subst("$BUILD_DIR")
sdkconfig_path = os.path.join(build_dir, "sdkconfig")
if os.path.isfile(sdkconfig_path):
    with open(sdkconfig_path, "r") as f:
        content = f.read()
    # If our TLS/cert options are not applied, remove sdkconfig so it is recreated from sdkconfig.defaults
    if "CONFIG_ESP_TLS_INSECURE=y" not in content or "CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y" not in content:
        try:
            os.remove(sdkconfig_path)
            print("[apply_sdkconfig_defaults] Removed build sdkconfig (missing TLS/cert options); will be recreated from sdkconfig.defaults")
        except OSError as e:
            print("[apply_sdkconfig_defaults] Warning: could not remove sdkconfig:", e)
