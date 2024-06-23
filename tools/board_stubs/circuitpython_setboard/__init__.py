# SPDX-FileCopyrightText: 2024 Tim Cocks
#
# SPDX-License-Identifier: MIT
import argparse
import os
import sys
import shutil
import site
from collections import defaultdict


def get_definitions_or_exit(board: str) -> str:
    """Get the definitions file for a board given its name."""

    path = os.path.join(site.getsitepackages()[0], "board_definitions", board)

    file = os.path.join(path, "__init__.pyi")

    if not os.path.exists(file):
        sys.stderr.write(f"Definitions for: '{board}' were not found\n")
        sys.exit(1)

    return file


def get_doc_or_exit(board: str) -> str:
    """Get the docstring for a board given its name."""

    # with get_definitions_or_exit(board).open("r") as f:
    with open(get_definitions_or_exit(board), "r") as f:
        return f.read().split('"""')[1]


def header(txt: str) -> str:
    """Helper text formatter."""
    return txt + "\n" + "-" * len(txt) + "\n"


def set_board():
    parser = argparse.ArgumentParser(
        prog=__name__,
        usage="Install CircuitPython board-specific stubs",
    )
    parser.add_argument("chosen_board", help="selected board", nargs="?")
    parser.add_argument(
        "-l",
        "--list",
        help=f"show available boards. can filter eg: '{__name__} -l feather'",
        action="store_true",
    )

    args = parser.parse_args()

    if args.list:
        port_boards: defaultdict[str, list[str]] = defaultdict(list)

        # NOTE: "" in some_str == True
        looking_for = "" if args.chosen_board is None else args.chosen_board.lower()

        for board in os.listdir(os.path.join(site.getsitepackages()[0], "board_definitions")):
            # NOTE: For the hand-crafted finding of port in the docstring, its
            #       format is assumed to be:
            #
            # <empty line>
            # Board stub for ...
            #  - port: ...
            #  - board_id: ...
            #  - NVM size: ...
            #  - Included modules: ...
            #  - Frozen libraries: ...
            #

            lines = get_doc_or_exit(board).split("\n")
            port = lines[2].split("-")[1].split(":")[1].strip()

            if looking_for not in board.lower() and looking_for not in port.lower():
                continue

            port_boards[port].append(board)

        if not port_boards:
            sys.stdout.write("Nothing found, check out your filter.\n")
            sys.exit(0)

        sys.stdout.write("Available boards are: \n")
        # sort by port name
        for port, boards in sorted(port_boards.items(), key=lambda kv: kv[0]):
            sys.stdout.write(
                header(port)
                + "  * "
                # sort by board name
                + "\n  * ".join(sorted(boards))
                + "\n\n"
            )

        sys.exit(0)

    if args.chosen_board is None:
        sys.stderr.write("Must select a board\n")
        sys.exit(1)

    board_definitions_file = get_definitions_or_exit(args.chosen_board)

    # board_stubs_file = resources.files("board-stubs").joinpath("__init__.pyi")
    board_stubs_file = os.path.join(site.getsitepackages()[0], "board-stubs", "__init__.pyi")
    shutil.copyfile(board_definitions_file, board_stubs_file)
