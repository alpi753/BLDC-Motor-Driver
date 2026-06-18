import os
import re

TARGET_PATH = "/usr/share/openocd/scripts/target/"


def load_targets():
    """Return available stm32 target configs without .cfg"""
    targets = []
    for f in os.listdir(TARGET_PATH):
        if f.startswith("stm32") and f.endswith(".cfg"):
            targets.append(os.path.splitext(f)[0])
    return set(targets)


def family_from_board(board: str):
    """
    Map STM32 part number → OpenOCD family config.
    Examples:
        stm32f407 -> stm32f4x
        stm32g431 -> stm32g4x
        stm32h743 -> stm32h7x
    """
    m = re.match(r"stm32([a-z])(\d)", board)
    if not m:
        return None

    letter = m.group(1)
    digit = m.group(2)

    # most families follow this pattern
    return f"stm32{letter}{digit}x"


def main(board_name=None):
    if not board_name:
        print("Error: BOARD not defined.")
        return None

    board = board_name.lower()
    targets = load_targets()

    # 1. direct match (rare but possible)
    if board in targets:
        return board

    # 2. try shrinking prefix (handles partial or extra suffixes)
    for i in range(len(board), 6, -1):
        prefix = board[:i]
        if prefix in targets:
            return prefix

    # 3. family inference (stm32f407 -> stm32f4x etc.)
    family = family_from_board(board)
    if family in targets:
        return family

    # 4. last resort: fuzzy match by prefix similarity
    # (e.g., stm32f7 matches stm32f7x)
    for t in targets:
        if board.startswith(t[:7]):  # stm32f?
            return t

    return None


if __name__ == "__main__":
    import sys
    board_name = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("BOARD")
    result = main(board_name)
    print(result if result else -1)