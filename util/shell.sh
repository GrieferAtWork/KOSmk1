#!/bin/bash
bash --init-file "$(dirname $(readlink -f "$0"))/.shell_env.sh"
