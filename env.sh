SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
export PIN_ROOT=$SCRIPT_DIR/pin
export TRACER=$SCRIPT_DIR/ChampSim/tracer/pin/obj-intel64/champsim_tracer.so