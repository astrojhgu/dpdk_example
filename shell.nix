with import <nixpkgs> {
  overlays = [

  ];
};
stdenv.mkDerivation {
  name = "dpdk"; # Probably put a more meaningful name here
  buildInputs = with pkgs; [
    pkg-config
    dpdk
    numactl.all
    cmake
    gdb
    libpcap
    pktgen
    yaml-cpp.all
    (python3.withPackages (ps: with ps; [ pyelftools meson ninja ]))
  ];
  LD_LIBRARY_PATH="${dpdk.out}/lib";
}
