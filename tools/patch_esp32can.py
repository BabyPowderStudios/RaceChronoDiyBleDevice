from pathlib import Path

Import("env")


def apply_patch_if_needed() -> None:
    lib_path = (
        Path(env["PROJECT_DIR"])
        / ".pio"
        / "libdeps"
        / env["PIOENV"]
        / "ESP32CAN"
        / "src"
        / "CAN.c"
    )

    if not lib_path.exists():
        print("patch_esp32can: ESP32CAN source not found, skipping")
        return

    original = lib_path.read_text(encoding="utf-8")
    updated = original

    replacements = [
        (
            '#include "esp_intr.h"\n#include "soc/dport_reg.h"\n#include <math.h>\n\n#include "driver/gpio.h"\n',
            '#include "esp_intr_alloc.h"\n#include <math.h>\n\n#include "driver/gpio.h"\n#include "driver/periph_ctrl.h"\n#include "esp_rom_gpio.h"\n#include "soc/gpio_sig_map.h"\n',
        ),
        (
            '\tDPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_CAN_CLK_EN);\n\tDPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_CAN_RST);\n',
            '\tperiph_module_enable(PERIPH_CAN_MODULE);\n\tperiph_module_reset(PERIPH_CAN_MODULE);\n',
        ),
        (
            '\tgpio_matrix_out(CAN_cfg.tx_pin_id, CAN_TX_IDX, 0, 0);\n\tgpio_pad_select_gpio(CAN_cfg.tx_pin_id);\n',
            '\tesp_rom_gpio_connect_out_signal(CAN_cfg.tx_pin_id, CAN_TX_IDX, false, false);\n\tesp_rom_gpio_pad_select_gpio(CAN_cfg.tx_pin_id);\n',
        ),
        (
            '\tgpio_matrix_in(CAN_cfg.rx_pin_id, CAN_RX_IDX, 0);\n\tgpio_pad_select_gpio(CAN_cfg.rx_pin_id);\n',
            '\tesp_rom_gpio_connect_in_signal(CAN_cfg.rx_pin_id, CAN_RX_IDX, false);\n\tesp_rom_gpio_pad_select_gpio(CAN_cfg.rx_pin_id);\n',
        ),
    ]

    for old, new in replacements:
        if old in updated:
            updated = updated.replace(old, new)

    if updated != original:
        lib_path.write_text(updated, encoding="utf-8")
        print("patch_esp32can: patched ESP32CAN CAN.c")
    else:
        print("patch_esp32can: ESP32CAN CAN.c already patched")


apply_patch_if_needed()