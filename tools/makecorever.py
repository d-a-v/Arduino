#!/usr/bin/env python3

# Generate the core_version.h header per-build
#
# Copyright (C) 2019 - Earle F. Philhower, III
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import argparse
import os
import subprocess
import re


def generate(path, platform_path, git_ver="ffffffff", platform_version="unspecified"):
    def git(*args):
        cmd = ["git", "-C", platform_path]
        cmd.extend(args)
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True, stderr=subprocess.DEVNULL)
        return proc.stdout.readlines()[0].strip()

    git_desc = platform_version;
    try:
        git_ver = git("rev-parse", "--short=8", "HEAD")
        git_desc = git("describe", "--tags")
    except Exception:
        pass

    text = "#define ARDUINO_ESP8266_GIT_VER   0x{}\n".format(git_ver)
    text += "#define ARDUINO_ESP8266_GIT_DESC  {}\n".format(git_desc)
    text += "\n"

    version = re.split("\.", platform_version)
    # major: if present, skip "unix-" in "unix-3"
    text += "#define ARDUINO_ESP8266_MAJOR     {}\n".format(re.split("-", version[0])[-1])
    text += "#define ARDUINO_ESP8266_MINOR     {}\n".format(version[1])
    # revision can be ".n" or ".n-dev"
    revision = re.split("-", version[2])
    text += "#define ARDUINO_ESP8266_REVISION  {}\n".format(revision[0])
    text += "\n"
    if len(revision) > 1:
        text += "#define ARDUINO_ESP8266_DEV       1 // developpment version\n"
    else:
        text += "#define ARDUINO_ESP8266_RELEASE   \"{}\"\n".format(git_desc)
        text += "#define ARDUINO_ESP8266_RELEASE_{}\n".format(re.sub("[-\.]", "_", git_desc))

    try:
        with open(path, "r") as inp:
            old_text = inp.read()
        if old_text == text:
            return
    except Exception:
        pass

    with open(path, "w") as out:
        out.write(text)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate core_version.h")

    parser.add_argument(
        "-b", "--build_path", action="store", required=True, help="build.path variable"
    )
    parser.add_argument(
        "-p",
        "--platform_path",
        action="store",
        required=True,
        help="platform.path variable",
    )
    parser.add_argument(
        "-v", "--version", action="store", required=True, help="version variable"
    )
    parser.add_argument("-i", "--include_dir", default="core")

    args = parser.parse_args()

    include_dir = os.path.join(args.build_path, args.include_dir)
    try:
        os.makedirs(include_dir)
    except Exception:
        pass

    generate(
        os.path.join(include_dir, "core_version.h"),
        args.platform_path,
        platform_version=args.version,
    )
